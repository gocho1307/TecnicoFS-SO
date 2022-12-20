#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_FORMAT "/f%ld_%ld_%d"
#define FILE_COUNT 4
#define THREAD_COUNT 3
#define MAX_PATH_SIZE 32

void _format_path(char *dest, size_t max_size, size_t rep, size_t file_idx,
                  int thread_i);
int _open(size_t rep, size_t file_idx, tfs_file_mode_t mode, int thread_i);
int _symbolic_link(size_t rep_target, size_t file_idx_target, size_t rep_name,
                   size_t file_idx_name, int thread_i);
int _hard_link(size_t rep_target, size_t file_idx_target, size_t rep_name,
               size_t file_idx_name, int thread_i);
int _unlink(size_t rep, size_t file_idx, int thread_i);
void assert_contents_ok(size_t rep, size_t file_idx, int thread_i);
void write_contents(size_t rep, size_t file_idx, int thread_i);
void run_test(size_t rep, int thread_i);
void *thread_link_func(void *arg);

uint8_t const file_contents[] = "AAA";

/**
 * Tests if the link, sym_link and unlink are concurrent with the whole
 * TecnicoFS. It makes many files (FILE_COUNT) each with multiple chained
 * sym_links that eventually point to a hard link and that hard link points to a
 * file. In the end it makes sure the content of said files is ok.
 */
int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t tid[THREAD_COUNT];
    int table[3] = {0, 1, 2};

    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_create(&tid[i], NULL, thread_link_func, &table[i]) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(tid[i], NULL) == 0);
    }

    // confirm that the others see the write
    for (size_t i = 0; i < FILE_COUNT / 2; i++) {
        assert_contents_ok(i + 100, i, 100);
    }

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *thread_link_func(void *arg) {
    int thread_i = *((int *)arg);
    for (size_t rep = 0; rep < FILE_COUNT; rep++) {
        // create original file
        {
            int fd = _open(rep + 100, rep, TFS_O_CREAT, 100);
            assert(fd != -1);
            assert(tfs_close(fd) != -1);
        }

        assert(_hard_link(rep + 100, rep, rep, rep, thread_i) != -1);

        // create links
        for (size_t i = 0; i < FILE_COUNT; i++) {
            if (i == rep) {
                continue;
            }

            // instead of linking to the original always, let's create a link to
            // the previously linked name
            if (i == 0) {
                // no links exist yet, link to the original
                assert(_symbolic_link(rep, rep, rep, i, thread_i) != -1);
            } else {
                // link to the link created in the previous iteration
                assert(_symbolic_link(rep, i - 1, rep, i, thread_i) != -1);
            }
        }

        run_test(rep, thread_i);
    }

    return 0;
}

void run_test(size_t rep, int thread_i) {
    // _open(rep, <number from 0 to FILE_COUNT>, mode) opens one of the
    // alternative names to the same file

    // write in one of the links
    write_contents(rep, 3, thread_i);

    // delete half the links
    for (size_t i = FILE_COUNT / 2; i < FILE_COUNT; i++) {
        if (rep == i) {
            continue;
        }
        assert(_unlink(rep, i, thread_i) != -1);
    }

    // finish removing links
    for (size_t i = 0; i < FILE_COUNT / 2; i++) {
        assert(_unlink(rep, i, thread_i) != -1);
    }
}

void _format_path(char *dest, size_t max_size, size_t rep, size_t file_idx,
                  int thread_i) {
    int ret = snprintf(dest, max_size, PATH_FORMAT, rep, file_idx, thread_i);
    assert(ret > 0);
    assert(ret <= max_size);
}

int _open(size_t rep, size_t file_idx, tfs_file_mode_t mode, int thread_i) {
    char path[MAX_PATH_SIZE];
    _format_path(path, MAX_PATH_SIZE, rep, file_idx, thread_i);
    return tfs_open(path, mode);
}

int _symbolic_link(size_t rep_target, size_t file_idx_target, size_t rep_name,
                   size_t file_idx_name, int thread_i) {
    char target[MAX_PATH_SIZE];
    _format_path(target, MAX_PATH_SIZE, rep_target, file_idx_target, thread_i);

    char name[MAX_PATH_SIZE];
    _format_path(name, MAX_PATH_SIZE, rep_name, file_idx_name, thread_i);

    return tfs_sym_link(target, name);
}

int _hard_link(size_t rep_target, size_t file_idx_target, size_t rep_name,
               size_t file_idx_name, int thread_i) {
    char target[MAX_PATH_SIZE];
    _format_path(target, MAX_PATH_SIZE, rep_target, file_idx_target, 100);

    char name[MAX_PATH_SIZE];
    _format_path(name, MAX_PATH_SIZE, rep_name, file_idx_name, thread_i);

    return tfs_link(target, name);
}

int _unlink(size_t rep, size_t file_idx, int thread_i) {
    char path[MAX_PATH_SIZE];
    _format_path(path, MAX_PATH_SIZE, rep, file_idx, thread_i);
    return tfs_unlink(path);
}

void assert_contents_ok(size_t rep, size_t file_idx, int thread_i) {
    int f = _open(rep, file_idx, 0, thread_i);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    memset(buffer, 0, sizeof(buffer));
    ssize_t r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == THREAD_COUNT);
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(size_t rep, size_t file_idx, int thread_i) {
    int f = _open(rep, file_idx, TFS_O_APPEND, thread_i);
    assert(f != -1);

    assert(tfs_write(f, "A", 1) == 1);

    assert(tfs_close(f) != -1);
}
