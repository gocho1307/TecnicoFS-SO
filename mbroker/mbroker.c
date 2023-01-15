/*
 *      File: mbroker.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that contains the actual server.
 */

#include "mbroker.h"
#include "../fs/operations.h"
#include "../fs/state.h"
#include "../producer-consumer/producer-consumer.h"
#include "../utils/better-locks.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Requests queue
static pc_queue_t *requests_queue;

// Boxes table
static int n_boxes;
static box_t *boxes_table;
static allocation_state_t *free_boxes;
static pthread_rwlock_t free_boxes_lock;

// Workers
static pthread_t *workers;

static void print_usage() {
    fprintf(stderr, "Usage: mbroker <register_pipe_name> <max_sessions>\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }
    char *p;
    errno = 0;
    long max_sessions = strtol(argv[2], &p, 10);
    if (errno != 0 || *p != '\0' || max_sessions <= 0) {
        fprintf(stdout, "Number of max sessions is invalid.\n");
        return EXIT_FAILURE;
    }

    char *register_pipename = argv[1];

    signal(SIGPIPE, SIG_IGN);

    if (mbroker_init(register_pipename, (size_t)max_sessions) != 0) {
        return EXIT_FAILURE;
    }

    int register_pipe_out = open(register_pipename, O_RDONLY);
    if (register_pipe_out < 0) {
        WARN("Failed to open register pipe");
        unlink(register_pipename);
        return EXIT_FAILURE;
    }

    while (true) {
        ssize_t bytes_read;
        uint8_t code;

        // Main listener loop
        bytes_read = pipe_read(register_pipe_out, &code, sizeof(uint8_t));
        while (bytes_read > 0) {
            if (code != PROTOCOL_CODE_PUB_REGISTER &&
                code != PROTOCOL_CODE_SUB_REGISTER &&
                code != PROTOCOL_CODE_CREATE_REQUEST &&
                code != PROTOCOL_CODE_REMOVE_REQUEST &&
                code != PROTOCOL_CODE_LIST_REQUEST) {
                continue;
            }
            if (mbroker_receive_connection(code, register_pipe_out) != 0) {
                close(register_pipe_out);
                unlink(register_pipename);
                return EXIT_FAILURE;
            }
            bytes_read = pipe_read(register_pipe_out, &code, sizeof(uint8_t));
        }

        if (bytes_read < 0) {
            WARN("Failed to read pipe");
            close(register_pipe_out);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }

        // Opens and closes a dummy pipe to avoid having active wait for another
        // process to open the pipe. The 'open' function blocks until the pipe
        // is opened on the other side.
        int tmp_pipe = open(register_pipename, O_RDONLY);
        if (tmp_pipe < 0) {
            WARN("Failed to open register pipe");
            close(register_pipe_out);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }
        if (close(tmp_pipe) < 0) {
            WARN("Failed to close register pipe");
            close(register_pipe_out);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }
    }

    return 0;
}

int mbroker_init(char *register_pipename, size_t max_sessions) {
    // We initialize the tfs with the number of files the same as max sessions,
    // so we can have <max_sessions> boxes all at once, and the block size we
    // set to 1MiB to have room for 1024 messages.
    tfs_params params = tfs_default_params();
    params.max_open_files_count = max_sessions;
    params.block_size = 1024 * 1024;
    if (tfs_init(&params) != 0) {
        WARN("Failed to initialize the tfs file system");
        return -1;
    }

    // Allocates a pointer for the global requests queue
    requests_queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));
    if (requests_queue == NULL) {
        tfs_destroy();
        return -1;
    }
    if (pcq_create(requests_queue, max_sessions) != 0) {
        tfs_destroy();
        free(requests_queue);
        return -1;
    }

    // Initializes everything related to the workers
    workers = malloc(max_sessions * sizeof(pthread_t));
    if (workers == NULL) {
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        return -1; // allocation failed
    }
    if (workers_init(max_sessions) != 0) {
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        free(workers);
        return -1;
    }

    // Prepares all the boxes on the server
    boxes_table = malloc(inode_table_size() * sizeof(box_t));
    free_boxes = malloc(inode_table_size() * sizeof(allocation_state_t));
    if (boxes_table == NULL) {
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        free(workers);
        return -1; // allocation failed
    }
    for (size_t i = 0; i < inode_table_size(); i++) {
        free_boxes[i] = FREE;
        mutex_init(&boxes_table[i].mutex);
        cond_init(&boxes_table[i].cond);
    }
    rwlock_init(&free_boxes_lock);

    // Actually boots up the server
    fprintf(stdout, "Starting mbroker server with pipe called: %s\n",
            register_pipename);
    if ((unlink(register_pipename) != 0 && errno != ENOENT) ||
        mkfifo(register_pipename, 0777) < 0) {
        WARN("Failed to create register pipe");
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        free(workers);
        free(boxes_table);
        free(free_boxes);
        return -1;
    }

    return 0;
}

int mbroker_receive_connection(uint8_t code, int register_pipe_in) {
    request_t *request = (request_t *)malloc(sizeof(request_t));
    request->code = code;

    // Stores the client pipe path associated with the given request
    char client_named_pipe_path[CLIENT_NAMED_PIPE_MAX_LEN];
    if (pipe_read(register_pipe_in, client_named_pipe_path,
                  sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN) <= 0) {
        WARN("Failed to estabilish connection to mbroker");
        return -1;
    }
    strcpy(request->client_named_pipe_path, client_named_pipe_path);

    // Gets the box name if the request is for a specific box operation
    if (request->code != PROTOCOL_CODE_LIST_REQUEST) {
        char box_name[BOX_NAME_MAX_LEN];
        if (pipe_read(register_pipe_in, box_name,
                      BOX_NAME_MAX_LEN * sizeof(char)) <= 0) {
            WARN("Failed to read pipe");
            return -1;
        }
        request->box_name[0] = '/';
        strcpy(request->box_name + 1, box_name);
    }

    // Enqueues the request for the workers to process. When the queue is full,
    // it waits.
    if (pcq_enqueue(requests_queue, (void *)request) != 0) {
        WARN("Failed to enqueue request");
        return -1;
    }

    return 0;
}

int workers_init(size_t num_threads) {
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&workers[i], NULL, workers_reception, NULL) != 0) {
            return -1;
        }
    }

    return 0;
}

void *workers_reception(void *arg) {
    (void)arg;
    request_t *request;
    while (true) {
        // Dequeues a request and processes it. Waits until new ones appear.
        request = (request_t *)pcq_dequeue(requests_queue);
        if (request == NULL) {
            continue;
        }
        switch (request->code) {
        case PROTOCOL_CODE_PUB_REGISTER:
            workers_handle_pub_register(request);
            break;
        case PROTOCOL_CODE_SUB_REGISTER:
            workers_handle_sub_register(request);
            break;
        case PROTOCOL_CODE_CREATE_REQUEST:
            workers_handle_manager_create(request);
            break;
        case PROTOCOL_CODE_REMOVE_REQUEST:
            workers_handle_manager_remove(request);
            break;
        case PROTOCOL_CODE_LIST_REQUEST:
            workers_handle_manager_listing(request);
            break;
        default:
            break;
        }
        free(request);
    }

    return NULL;
}

int workers_handle_pub_register(request_t *request) {
    int session_pipe_out = open(request->client_named_pipe_path, O_RDONLY);
    if (session_pipe_out < 0) {
        return 0;
    }

    int box_i = box_find(request->box_name);
    if (box_i == -1) {
        close(session_pipe_out);
        return 0; // The given box name doesn't exist on the server
    }
    mutex_lock(&boxes_table[box_i].mutex);
    if (boxes_table[box_i].n_publishers > 0) {
        mutex_unlock(&boxes_table[box_i].mutex);
        close(session_pipe_out);
        return 0; // We close the pipe if there already is a publisher
    }
    // Increments the number of publishers on that box to 1
    boxes_table[box_i].n_publishers++;

    int box_f = tfs_open(request->box_name, TFS_O_APPEND);
    if (box_f < 0) {
        boxes_table[box_i].n_publishers--;
        mutex_unlock(&boxes_table[box_i].mutex);
        close(session_pipe_out);
        return 0; // The given box doesn't exist on the server
    }
    mutex_unlock(&boxes_table[box_i].mutex);

    // In a loop it reads from the publisher client the messages, processes them
    // and sends them to the corresponding box
    uint8_t code;
    char message[MSG_MAX_LEN];
    ssize_t bytes_read = 0;
    size_t to_write = sizeof(char) * MSG_MAX_LEN;
    while (true) {
        bytes_read = pipe_read(session_pipe_out, &code, sizeof(uint8_t));
        if (bytes_read <= 0 || code != PROTOCOL_CODE_MESSAGE_RECEIVE) {
            break;
        }
        // Checks if the box was already deleted, ending the session in that
        // case and freeing the worker
        rwlock_rdlock(&free_boxes_lock);
        if (free_boxes[box_i] != TAKEN) {
            rwlock_unlock(&free_boxes_lock);
            break;
        }
        rwlock_unlock(&free_boxes_lock);

        // We write in blocks of 1KiB so the box doesn't get partial messages
        // (If we wrote in blocks the size of the actual message content, the
        // last message might not be fully written)
        bytes_read = pipe_read(session_pipe_out, message, to_write);
        if (bytes_read <= 0 ||
            tfs_write(box_f, message, to_write) != to_write) {
            break;
        }
        // We update the box size and signal the subscribers that a new message
        // appeared
        mutex_lock(&boxes_table[box_i].mutex);
        boxes_table[box_i].size += to_write;
        cond_broadcast(&boxes_table[box_i].cond);
        mutex_unlock(&boxes_table[box_i].mutex);
    }
    mutex_lock(&boxes_table[box_i].mutex);
    boxes_table[box_i].n_publishers--;
    mutex_unlock(&boxes_table[box_i].mutex);
    tfs_close(box_f);
    close(session_pipe_out);

    return 0;
}

int workers_handle_sub_register(request_t *request) {
    int session_pipe_in = open(request->client_named_pipe_path, O_WRONLY);
    if (session_pipe_in < 0) {
        return 0;
    }

    int box_i = box_find(request->box_name);
    if (box_i == -1) {
        close(session_pipe_in);
        return 0; // The given box name doesn't exist on the server
    }
    mutex_lock(&boxes_table[box_i].mutex);
    // Increments the number of subscribers on the given box
    boxes_table[box_i].n_subscribers++;

    int box_f = tfs_open(request->box_name, 0);
    if (box_f < 0) {
        boxes_table[box_i].n_subscribers--;
        close(session_pipe_in);
        return 0; // The box doesn't exist on the server
    }
    mutex_unlock(&boxes_table[box_i].mutex);

    size_t packet_len = sizeof(uint8_t) + sizeof(char) * MSG_MAX_LEN;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];
    size_t packet_offset = 0;

    // In a loop it writes to the subscriber client the messages from the box in
    // blocks of 1KiB
    uint8_t code = PROTOCOL_CODE_MESSAGE_SEND;
    char message[MSG_MAX_LEN];
    size_t to_read = sizeof(char) * MSG_MAX_LEN;
    while (true) {
        // Waits until a new message gets written on the box by a publisher
        mutex_lock(&boxes_table[box_i].mutex);
        while (free_boxes[box_i] == TAKEN &&
               tfs_read(box_f, message, to_read) != to_read) {
            cond_wait(&boxes_table[box_i].cond, &boxes_table[box_i].mutex);
        }
        mutex_unlock(&boxes_table[box_i].mutex);
        // Checks if the box was already deleted, ending the session in that
        // case and freeing the worker
        rwlock_rdlock(&free_boxes_lock);
        if (free_boxes[box_i] != TAKEN) {
            rwlock_unlock(&free_boxes_lock);
            break;
        }
        rwlock_unlock(&free_boxes_lock);
        do {
            packet_offset = 0;
            memset(packet, 0, packet_len);
            packet_write(packet, &packet_offset, &code, sizeof(uint8_t));
            packet_write(packet, &packet_offset, message, to_read);
            if (pipe_write(session_pipe_in, packet, packet_len) <= 0) {
                mutex_lock(&boxes_table[box_i].mutex);
                boxes_table[box_i].n_subscribers--;
                mutex_unlock(&boxes_table[box_i].mutex);
                tfs_close(box_f);
                close(session_pipe_in);
                return 0;
            }
        } while (tfs_read(box_f, message, to_read) == to_read);
    }
    mutex_lock(&boxes_table[box_i].mutex);
    boxes_table[box_i].n_subscribers--;
    mutex_unlock(&boxes_table[box_i].mutex);
    tfs_close(box_f);
    close(session_pipe_in);

    return 0;
}

int workers_handle_manager_create(request_t *request) {
    size_t packet_len =
        sizeof(uint8_t) + sizeof(int32_t) + sizeof(char) * MSG_MAX_LEN;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    uint8_t code = PROTOCOL_CODE_CREATE_ANSWER;
    int32_t return_code = 0;
    char error_message[MSG_MAX_LEN] = {0};
    size_t packet_offset = 0;
    memset(packet, 0, packet_len);
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));

    // Sees if the box already exists on the server, giving an error in that
    // case
    int f = tfs_open(request->box_name, 0);
    if (f >= 0 || (f = tfs_open(request->box_name, TFS_O_CREAT)) < 0 ||
        box_create(request->box_name) != 0) {
        return_code = -1;
        strcpy(error_message, "Couldn't create box");
        WARN("MANAGER: %s", error_message);
    }
    tfs_close(f);
    packet_write(packet, &packet_offset, &return_code, sizeof(int32_t));
    packet_write(packet, &packet_offset, error_message,
                 sizeof(char) * strlen(error_message));

    int session_pipe_in = open(request->client_named_pipe_path, O_WRONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return 0;
    }
    pipe_write(session_pipe_in, packet, packet_len);
    close(session_pipe_in);
    return 0;
}

int workers_handle_manager_remove(request_t *request) {
    size_t packet_len =
        sizeof(uint8_t) + sizeof(int32_t) + sizeof(char) * MSG_MAX_LEN;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    uint8_t code = PROTOCOL_CODE_REMOVE_ANSWER;
    int32_t return_code = 0;
    char error_message[MSG_MAX_LEN] = {0};
    size_t packet_offset = 0;
    memset(packet, 0, packet_len);
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));

    // Deletes the box from the server, if it can't it gives an error. When
    // deleting a box the clients need to end their programs on their side.
    if (box_delete(request->box_name) != 0 ||
        tfs_unlink(request->box_name) != 0) {
        return_code = -1;
        strcpy(error_message, "Couldn't remove box");
        WARN("MANAGER: %s", error_message);
    }
    packet_write(packet, &packet_offset, &return_code, sizeof(int32_t));
    packet_write(packet, &packet_offset, error_message,
                 sizeof(char) * strlen(error_message));

    int session_pipe_in = open(request->client_named_pipe_path, O_WRONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }
    pipe_write(session_pipe_in, packet, packet_len);
    close(session_pipe_in);
    return 0;
}

int workers_handle_manager_listing(request_t *request) {
    size_t packet_len = sizeof(uint8_t) * 2 + sizeof(char) * BOX_NAME_MAX_LEN +
                        sizeof(uint64_t) * 3;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    int session_pipe_in = open(request->client_named_pipe_path, O_WRONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    // Reads all the current boxes on the server and sends all the information
    // to the manager client (boxes sent are not sorted because that's up to the
    // client)
    uint8_t code = PROTOCOL_CODE_LIST_ANSWER;
    uint8_t last = 0;
    rwlock_rdlock(&free_boxes_lock);
    for (int i = 0, n_boxes_listed = 0;
         n_boxes_listed <= n_boxes && i < inode_table_size(); i++) {
        // Only looks at taken boxes
        if (n_boxes > 0 && free_boxes[i] == FREE) {
            continue;
        }
        // When we have 0 boxes or we reached the last box we set the last flag
        // to 1
        if (n_boxes == 0 || n_boxes_listed + 1 == n_boxes) {
            last = 1;
        }
        // Writes all the information of a box in a packet and sends it
        size_t packet_offset = 0;
        memset(packet, 0, packet_len);
        packet_write(packet, &packet_offset, &code, sizeof(uint8_t));
        packet_write(packet, &packet_offset, &last, sizeof(uint8_t));
        mutex_lock(&boxes_table[i].mutex);
        if (n_boxes > 0) {
            packet_write(packet, &packet_offset, boxes_table[i].name + 1,
                         sizeof(char) * BOX_NAME_MAX_LEN);
            packet_write(packet, &packet_offset, &boxes_table[i].size,
                         sizeof(uint64_t));
            packet_write(packet, &packet_offset, &boxes_table[i].n_publishers,
                         sizeof(uint64_t));
            packet_write(packet, &packet_offset, &boxes_table[i].n_subscribers,
                         sizeof(uint64_t));
        }
        mutex_unlock(&boxes_table[i].mutex);
        if (pipe_write(session_pipe_in, packet, packet_len) <= 0) {
            rwlock_unlock(&free_boxes_lock);
            close(session_pipe_in);
            return -1;
        }
        // When this counter reaches the number of boxes, we know we are
        // finished
        n_boxes_listed++;
    }
    rwlock_unlock(&free_boxes_lock);

    close(session_pipe_in);
    return 0;
}

int box_create(char *box_name) {
    rwlock_wrlock(&free_boxes_lock);
    for (size_t i = 0; i < inode_table_size(); i++) {
        // Finds first free entry in boxes table
        if (free_boxes[i] == FREE) {
            // Found a free entry, so takes it for the new box
            free_boxes[i] = TAKEN;
            n_boxes++;
            rwlock_unlock(&free_boxes_lock);

            mutex_lock(&boxes_table[i].mutex);
            memset(boxes_table[i].name, 0, sizeof(char) * BOX_NAME_MAX_LEN);
            strcpy(boxes_table[i].name, box_name);
            boxes_table[i].size = 0;
            boxes_table[i].n_subscribers = 0;
            boxes_table[i].n_subscribers = 0;
            mutex_unlock(&boxes_table[i].mutex);

            return 0;
        }
    }
    rwlock_unlock(&free_boxes_lock);

    return -1;
}

int box_delete(char *box_name) {
    rwlock_wrlock(&free_boxes_lock);
    for (size_t i = 0; i < inode_table_size(); i++) {
        // Finds the entry in boxes table
        if (free_boxes[i] == TAKEN &&
            strcmp(boxes_table[i].name, box_name) == 0) {
            // Marks entry as deleted
            mutex_lock(&boxes_table[i].mutex);
            free_boxes[i] = FREE;
            cond_broadcast(&boxes_table[i].cond);
            mutex_unlock(&boxes_table[i].mutex);
            n_boxes--;
            rwlock_unlock(&free_boxes_lock);

            return 0;
        }
    }
    rwlock_unlock(&free_boxes_lock);

    return -1;
}

int box_find(char *box_name) {
    rwlock_rdlock(&free_boxes_lock);
    for (size_t i = 0; i < inode_table_size(); i++) {
        // Finds the entry in boxes table
        if (free_boxes[i] == TAKEN &&
            strcmp(boxes_table[i].name, box_name) == 0) {
            rwlock_unlock(&free_boxes_lock);

            return (int)i;
        }
    }
    rwlock_unlock(&free_boxes_lock);

    return -1;
}
