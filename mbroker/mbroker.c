/*
 *      File: mbroker.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that contains the actual server.
 */

#include "mbroker.h"
#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

volatile sig_atomic_t shutdown_signaler = 0;
static pc_queue_t *requests_queue;

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

	pthread_t* thread_ids;
    if (mbroker_init(register_pipename, (size_t)max_sessions, &thread_ids) != 0) {
        return EXIT_FAILURE;
    }

    int register_pipe_in = open(register_pipename, O_RDONLY);
    if (register_pipe_in < 0) {
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
            close(register_pipe_in);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }
        if (close(tmp_pipe) < 0) {
            WARN("Failed to close register pipe");
            close(register_pipe_in);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }

        ssize_t bytes_read;
        uint8_t code;

        // Main listener loop
        bytes_read = pipe_read(register_pipe_in, &code, sizeof(uint8_t));
        while (bytes_read > 0) {
            if (code != SERVER_CODE_PUB_REGISTER &&
                code != SERVER_CODE_SUB_REGISTER &&
                code != SERVER_CODE_CREATE_REQUEST &&
                code != SERVER_CODE_REMOVE_REQUEST &&
                code != SERVER_CODE_LIST_REQUEST) {
                continue;
            }
            if ( mbroker_receive_connection(code, register_pipe_in) != 0) {
				WARN("Failed to receive connection");
				close(register_pipe_in);
				unlink(register_pipename);
				return EXIT_FAILURE;
			}
            bytes_read = pipe_read(register_pipe_in, &code, sizeof(uint8_t));
        }

        if (bytes_read < 0) {
            WARN("Failed to read pipe");
            close(register_pipe_in);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }
    }

	// TODO: Wait for all threads to finish
    close(register_pipe_in);
    unlink(register_pipename);
    mbroker_destroy();
    return 0;
}

int mbroker_init(char *register_pipename, size_t max_sessions, pthread_t **thread_ids) {
    // We initialize the tfs with the number of files the same as max sessions,
    // so it's possible to have <max_sessions> boxes all at once
    tfs_params params = tfs_default_params();
    params.max_open_files_count = max_sessions; // not sure about this
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

    if (workers_init(max_sessions, thread_ids) != 0) {
        tfs_destroy();
        free(requests_queue);
        return -1;
    }

    printf("Starting mbroker server with pipe called: %s\n", register_pipename);
    if (mkfifo(register_pipename, 0777) < 0) {
        WARN("Failed to create register pipe");
        tfs_destroy();
        free(requests_queue);
        return -1;
    }

    return 0;
}

void mbroker_shutdown(int signum) { shutdown_signaler = signum; }

void mbroker_destroy(void) {
    if (tfs_destroy() != 0) {
        WARN("Failed to destroy tfs");
    }

    if (pcq_destroy(requests_queue) != 0) {
        WARN("Failed to destroy requests queue");
    }
    free(requests_queue);
}

int mbroker_receive_connection(int code, int register_pipe_in) {

	char bar;
	ssize_t bytes_read = pipe_read(register_pipe_in, &bar, sizeof(char));

	char client_named_pipe_path[256];
	bytes_read = pipe_read(register_pipe_in, client_named_pipe_path, 256);

	request_t *request = (request_t *)malloc(sizeof(request_t));
	strcpy(request->client_named_pipe_path, client_named_pipe_path);
	request->code = code;

	char box_name[32];
	if ( code != SERVER_CODE_LIST_REQUEST ) {
		bytes_read = pipe_read(register_pipe_in, &bar, sizeof(char));
        if (bytes_read < 0) {
            WARN("Failed to read pipe");
            return EXIT_FAILURE;
        }
		bytes_read = pipe_read(register_pipe_in, box_name, 32);
        if (bytes_read < 0) {
            WARN("Failed to read pipe");
            return EXIT_FAILURE;
        }
		strcpy(request->box_name, box_name);
	}

	if (pcq_enqueue(requests_queue, (void*)request) != 0) {
		WARN("Failed to enqueue request");
		return EXIT_FAILURE;
	}
	
    // TODO: Create request, parse that request according to the code (code ==
    // SERVER_CODE_LIST_REQUEST makes it not have box name, everything else gets
    // parsed the same way) and enqueue it in the requests_queue.
}

int workers_init(int num_threads, pthread_t **thread_ids) {

	*thread_ids = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
	if (*thread_ids == NULL) {
		return -1;
	}

	for (int i = 0; i < num_threads; i++) {
		if (pthread_create(&(*thread_ids)[i], NULL, workers_reception, NULL) != 0) {
			return -1;
		}
	}

    return 0;
}

void *workers_reception(void* arg) {

	request_t *request = (request_t *)malloc(sizeof(request_t));
	while (shutdown_signaler == 0) {
		request = (request_t *)pcq_dequeue(requests_queue);
		switch(request->code) {
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
		}
	}

    // TODO: Dequeues a request and does a switch case on the code to choose the
    // right handler for that request.
    return NULL;
}

int workers_destroy(int num_threads, pthread_t *thread_ids) {
	return 0;
}

int workers_handle_pub_register(request_t *request) { return 0; }

int workers_handle_sub_register(request_t *request) { return 0; }

int workers_handle_manager_create(request_t *request) { return 0; }

int workers_handle_manager_remove(request_t *request) { return 0; }

int workers_handle_manager_listing(request_t *request) { return 0; }
