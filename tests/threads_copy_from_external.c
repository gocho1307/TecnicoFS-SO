#include "fs/operations.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_LEN 50

#define COUNT_PER_FILE 8
#define THREAD_COUNT 6
#define FILE_COUNT 4

char *input_files[] = {
    "./tests/test1.txt",
    "./tests/test2.txt",
    "./tests/test3.txt",
    "./tests/test4.txt",
};
char *input_files_content[] = {"AAAA!", "BBB!", "CCCCC!", "DD!"};
char *tfs_files[] = {"/f1", "/f2", "/f3", "/f4"};

void *write_external_file(void *input);
void *copy_external_thread();
void assert_contents_ok(int file_id);

/**
 * Tests if the external FS files files are copied to the TécnicoFS
 * concurrently.
 */
int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t tid[FILE_COUNT];
    int file_id[FILE_COUNT];

    // Creates files concurrently
    for (int i = 0; i < FILE_COUNT; ++i) {
        file_id[i] = i;
        assert(pthread_create(&tid[i], NULL, write_external_file,
                              (void *)(&file_id[i])) == 0);
    }

    for (int i = 0; i < FILE_COUNT; ++i) {
        assert(pthread_join(tid[i], NULL) == 0);
    }

    pthread_t tid2[THREAD_COUNT];

    // copy from external fs concurrently
    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_create(&tid2[i], NULL, copy_external_thread, NULL) == 0);
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(tid2[i], NULL) == 0);
    }

    for (int i = 0; i < FILE_COUNT; ++i) {
        assert_contents_ok(i);
    }

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *write_external_file(void *input) {
    int file_id = *((int *)input);

    // Open external file
    FILE *fd = fopen(input_files[file_id], "w");
    assert(fd != NULL);

    // Write the contents to the external file
    size_t bytes_written = fwrite(input_files_content[file_id], sizeof(char),
                                  strlen(input_files_content[file_id]), fd);
    assert(bytes_written == strlen(input_files_content[file_id]));

    assert(fclose(fd) == 0);

    return NULL;
}

void *copy_external_thread(void) {
    for (int file_id = 0; file_id < FILE_COUNT; ++file_id) {
        for (int i = 0; i < COUNT_PER_FILE; ++i) {
            assert(tfs_copy_from_external_fs(input_files[file_id],
                                             tfs_files[file_id]) != -1);
        }
    }

    return NULL;
}

void assert_contents_ok(int file_id) {
    int fd = tfs_open(tfs_files[file_id], 0);
    assert(fd != -1);

    char buffer[BUFFER_LEN];
    memset(buffer, 0, BUFFER_LEN);
    ssize_t bytes_read = tfs_read(fd, buffer, sizeof(buffer));
    assert(bytes_read == strlen(input_files_content[file_id]));
    assert(!memcmp(buffer, input_files_content[file_id],
                   strlen(input_files_content[file_id])));

    assert(tfs_close(fd) != -1);
}
