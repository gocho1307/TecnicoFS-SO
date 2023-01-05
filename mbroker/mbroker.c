#include "mbroker.h"
#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../server/server.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

    if (mbroker_init(register_pipename, max_sessions) != 0) {
        return EXIT_FAILURE;
    }

    int register_pipe_in = open(register_pipename, O_RDONLY);
    if (register_pipe_in < 0) {
        WARN("Failed to open register pipe");
        unlink(register_pipename);
        return EXIT_FAILURE;
    }

    return 0;
}

int mbroker_init(char *pipename, size_t max_sessions) {
    // We initialize the tfs with the number of files the same as max sessions
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

    // if (workers_init(max_sessions) != 0) { return -1; }

    if (unlink(pipename) != 0 && errno != ENOENT) {
        WARN("Failed to delete register pipe");
        tfs_destroy();
        free(requests_queue);
        return -1;
    }
    printf("Starting mbroker server with pipe called: %s\n", pipename);
    if (mkfifo(pipename, 0777) < 0) {
        WARN("Failed to create register pipe");
        tfs_destroy();
        free(requests_queue);
        return -1;
    }

    return 0;
}
