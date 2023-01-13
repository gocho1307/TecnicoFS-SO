/*
 *      File: mbroker.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that contains the actual server.
 */

#include "mbroker.h"
#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../utils/better-locks.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t shutdown_signaler = 0;

// Requests queue
static pc_queue_t *requests_queue;

// Boxes table
static box_t *boxes_table;
static allocation_state_t *free_boxes;
static pthread_mutex_t free_boxes_mutex;

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
        printf("Number of max sessions is invalid.\n");
        return EXIT_FAILURE;
    }

    char *register_pipename = argv[1];

    signal(SIGINT, mbroker_shutdown);
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

    while (shutdown_signaler == 0) {
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

        ssize_t bytes_read;
        uint8_t code;

        // Main listener loop
        bytes_read = pipe_read(register_pipe_out, &code, sizeof(uint8_t));
        while (bytes_read > 0) {
            if (code != SERVER_CODE_PUB_REGISTER &&
                code != SERVER_CODE_SUB_REGISTER &&
                code != SERVER_CODE_CREATE_REQUEST &&
                code != SERVER_CODE_REMOVE_REQUEST &&
                code != SERVER_CODE_LIST_REQUEST) {
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
    }

    printf("Closing mbroker server with pipe called: %s\n", register_pipename);
    close(register_pipe_out);
    unlink(register_pipename);
    mbroker_destroy((size_t)max_sessions);
    return 0;
}

int mbroker_init(char *register_pipename, size_t max_sessions) {
    // We initialize the tfs with the number of files the same as max sessions,
    // so it's possible to have <max_sessions> boxes all at once
    tfs_params params = tfs_default_params();
    params.max_open_files_count = max_sessions;
    if (tfs_init(&params) != 0) {
        WARN("Failed to initialize the tfs file system");
        return -1;
    }

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

    printf("Starting mbroker server with pipe called: %s\n", register_pipename);
    if (mkfifo(register_pipename, 0777) < 0) {
        WARN("Failed to create register pipe");
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        free(workers);
        return -1;
    }

    boxes_table = malloc(inode_table_size() * sizeof(box_t));
    if (boxes_table == NULL) {
        tfs_destroy();
        pcq_destroy(requests_queue);
        free(requests_queue);
        free(workers);
        unlink(register_pipename);
        return -1; // allocation failed
    }
    for (size_t i = 0; i < inode_table_size(); i++) {
        mutex_init(&boxes_table[i].mutex);
        cond_init(&boxes_table[i].cond);
    }
    mutex_init(&free_boxes_mutex);

    return 0;
}

void mbroker_shutdown(int signum) { shutdown_signaler = signum; }

void mbroker_destroy(size_t num_threads) {
    if (tfs_destroy() != 0) {
        WARN("Failed to destroy tfs");
    }

    if (pcq_destroy(requests_queue) != 0) {
        WARN("Failed to destroy requests queue");
    }
    free(requests_queue);

    if (workers_destroy(num_threads) != 0) {
        WARN("Failed to destroy workers");
    }
	free(workers);

    for (size_t i = 0; i < inode_table_size(); i++) {
        mutex_destroy(&boxes_table[i].mutex);
        cond_destroy(&boxes_table[i].cond);
    }
    mutex_destroy(&free_boxes_mutex);

    free(boxes_table);
    boxes_table = NULL;
}

int mbroker_receive_connection(int code, int register_pipe_in) {
    request_t *request = (request_t *)malloc(sizeof(request_t));
    request->code = code;

    char client_named_pipe_path[CLIENT_NAMED_PIPE_MAX_LEN];
    if (pipe_read(register_pipe_in, client_named_pipe_path,
                  sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN) == -1) {
        WARN("Failed to estabilish connection to mbroker");
        return -1;
    }
    strcpy(request->client_named_pipe_path, client_named_pipe_path);

    if (request->code != SERVER_CODE_LIST_REQUEST) {
        char box_name[BOX_NAME_MAX_LEN];
        if (pipe_read(register_pipe_in, box_name,
                      BOX_NAME_MAX_LEN * sizeof(char)) == -1) {
            WARN("Failed to read pipe");
            return -1;
        }
        strcpy(request->box_name, box_name);
    }

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

int workers_destroy(size_t num_threads) {
    for (size_t i = 0; i < num_threads; i++) {
		if (pthread_join(workers[i], NULL) != 0) {
			WARN("Failed to join worker thread");
            return -1;
		}
	}

    return 0;
}

void *workers_reception(void *arg) {
    request_t *request;
    while (true) {
        request = (request_t *)pcq_dequeue(requests_queue);
        switch (request->code) {
        case SERVER_CODE_PUB_REGISTER:
            workers_handle_pub_register(request);
            break;
        case SERVER_CODE_SUB_REGISTER:
            workers_handle_sub_register(request);
            break;
        case SERVER_CODE_CREATE_REQUEST:
            workers_handle_manager_create(request);
            break;
        case SERVER_CODE_REMOVE_REQUEST:
            workers_handle_manager_remove(request);
            break;
        case SERVER_CODE_LIST_REQUEST:
            workers_handle_manager_listing(request);
            break;
        default:
            break;
        }
        free(request);
    }
}

int workers_handle_pub_register(request_t *request) { return 0; }

int workers_handle_sub_register(request_t *request) { return 0; }

int workers_handle_manager_create(request_t *request) {
    size_t packet_len =
        sizeof(uint8_t) + sizeof(int32_t) + sizeof(char) * MSG_MAX_LEN;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    uint8_t code = SERVER_CODE_CREATE_ANSWER;
    int32_t return_code = 0;
    char error_message[MSG_MAX_LEN] = {0};
    size_t packet_offset = 0;
    memset(packet, 0, packet_len);
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));

    int f = -1;
    if ((f = tfs_open(request->box_name, TFS_O_CREAT)) != 0) {
        return_code = -1;
        strncpy(error_message, "Couldn't create box", MSG_MAX_LEN - 1);
        WARN("MANAGER: %s", error_message);
    }
    tfs_close(f);
    packet_write(packet, &packet_offset, &return_code, sizeof(int32_t));
    packet_write(packet, &packet_offset, error_message,
                 sizeof(char) * strlen(error_message));

    int session_pipe_in = open(request->client_named_pipe_path, O_WRONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }
    if (pipe_write(session_pipe_in, packet, packet_len) != 0) {
        close(session_pipe_in);
        return -1;
    }

    close(session_pipe_in);
    return 0;
}

int workers_handle_manager_remove(request_t *request) {
    size_t packet_len =
        sizeof(uint8_t) + sizeof(int32_t) + sizeof(char) * MSG_MAX_LEN;
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    uint8_t code = SERVER_CODE_REMOVE_ANSWER;
    int32_t return_code = 0;
    char error_message[MSG_MAX_LEN] = {0};
    size_t packet_offset = 0;
    memset(packet, 0, packet_len);
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));

    if (tfs_unlink(request->box_name) != 0) {
        return_code = -1;
        strncpy(error_message, "Couldn't remove box", MSG_MAX_LEN - 1);
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
    if (pipe_write(session_pipe_in, packet, packet_len) != 0) {
        close(session_pipe_in);
        return -1;
    }

    close(session_pipe_in);
    return 0;
}

int workers_handle_manager_listing(request_t *request) { return 0; }
