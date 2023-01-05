#include "../../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OPENED_FILES 5

const char *file_path = "/f1";
const char *link_path = "/l1";
const char *to_write = "SSSS!";
int fd[OPENED_FILES];
char buffer[1] = {0}, bigger_buffer[6] = {0};

/**
 *
 */
int main() {
    assert(tfs_init(NULL) != -1);

    // Create file
    fd[0] = tfs_open(file_path, TFS_O_CREAT);
    assert(fd[0] != -1);
    assert(tfs_write(fd[0], to_write, strlen(to_write)) != -1);
    assert(tfs_close(fd[0]) != -1);

    for (int i = 0; i < OPENED_FILES; i++) {
        fd[i] = tfs_open(file_path, 0);
        assert(fd[i] != -1);
    }

    assert(tfs_unlink(file_path) != -1);

    int other_fd = tfs_open(file_path, TFS_O_CREAT);
    assert(other_fd != -1);
    assert(tfs_read(other_fd, bigger_buffer, sizeof(bigger_buffer)) == 0);
    assert(memcmp(bigger_buffer, to_write, strlen(to_write)) != 0);
    assert(tfs_close(other_fd) != -1);

    assert(tfs_unlink(file_path) != -1);

    for (int i = 0; i < OPENED_FILES; i++) {
        assert(tfs_link(file_path, link_path) == -1);
        assert(tfs_open(file_path, 0) == -1);
        assert(tfs_read(fd[i], buffer, 1) == 1);
        assert(tfs_close(fd[i]) != -1);
    }

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}
