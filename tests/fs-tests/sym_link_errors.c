#include "../../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *file_path = "/f1";
const char *link_path = "/l1";

/**
 *
 */
int main() {
    assert(tfs_init(NULL) != -1);

    // Create file
    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_close(fd) != -1);

    assert(tfs_sym_link(file_path, link_path) != -1);

    assert(tfs_unlink(link_path) != -1);

    assert(tfs_link("/wrong-file", link_path) == -1);

    assert(tfs_link(file_path, "error-file-name") == -1);

    assert(tfs_open(link_path, 0) == -1);

    assert(tfs_sym_link(link_path, link_path) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}
