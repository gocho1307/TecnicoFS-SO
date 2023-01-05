#include "../../fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_FORMAT "/f%d"
#define THREAD_COUNT 10
#define MAX_PATH_SIZE 32
#define CICLES 500

void _format_path(char *path, size_t max_size, int thread_i);
int _hard_link(char *target, int thread_i);
int _unlink(int thread_i);
void *run_test(void *arg);

/**
 * This test is similiar to threads_multiple_links.c except we just create many
 * links while opening and closing the main file that hard link points to. In
 * the end it makes sure the file was deleted, this is done in order to make
 * sure all these operations are thread-safe.
 */
int main() {
    assert(tfs_init(NULL) != -1);

    int fd = tfs_open("/test", TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);

    pthread_t tid[THREAD_COUNT];
    int table[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_create(&tid[i], NULL, run_test, &table[i]) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(tid[i], NULL) == 0);
    }

    assert(tfs_open("/test", 0) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *run_test(void *arg) {
    int thread_i = *((int *)arg);
    for (int i = 0; i < CICLES; i++) {
        if (thread_i == 0) {
            int fd = tfs_open("/test", 0);
            tfs_close(fd);
        } else {
            _hard_link("/test", thread_i);
            _unlink(thread_i);
        }
        if (i >= CICLES / 2) {
            tfs_unlink("/test");
        }
    }

    return 0;
}

void _format_path(char *path, size_t max_size, int thread_i) {
    int ret = snprintf(path, max_size, PATH_FORMAT, thread_i);
    assert(ret > 0);
    assert(ret <= max_size);
}

int _hard_link(char *target, int thread_i) {
    char name[MAX_PATH_SIZE];
    _format_path(name, MAX_PATH_SIZE, thread_i);

    return tfs_link(target, name);
}

int _unlink(int thread_i) {
    char path[MAX_PATH_SIZE];
    _format_path(path, MAX_PATH_SIZE, thread_i);
    return tfs_unlink(path);
}
