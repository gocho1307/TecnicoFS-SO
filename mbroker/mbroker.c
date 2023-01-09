/*
 *      File: mbroker.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that contains the actual server.
 */

#include "mbroker.h"
#include "../common/common.h"
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

    if (mbroker_init(register_pipename, (size_t)max_sessions) != 0) {
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
        while ((bytes_read = pipe_read(register_pipe_in, &code, sizeof(char))) >
               0) {
            switch (code) {
            case SERVER_CODE_PUB_REGISTER:
                break;
            case SERVER_CODE_SUB_REGISTER:
                break;
            case SERVER_CODE_CREATE_REQUEST:
                break;
            case SERVER_CODE_REMOVE_REQUEST:
                break;
            case SERVER_CODE_LIST_REQUEST:
                break;
            default:
                close(register_pipe_in);
                unlink(register_pipename);
                return EXIT_FAILURE;
            }
        }

        if (bytes_read < 0) {
            WARN("Failed to read pipe");
            close(register_pipe_in);
            unlink(register_pipename);
            return EXIT_FAILURE;
        }
    }

    close(register_pipe_in);
    unlink(register_pipename);
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

    if (workers_init(max_sessions) != 0) {
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

int workers_init(int num_threads) {
    (void)num_threads;
    return 0;
}

void mbroker_shutdown(int signum) { shutdown_signaler = signum; }
