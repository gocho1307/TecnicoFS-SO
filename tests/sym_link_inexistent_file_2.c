#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char *file_path = "/f1";
    const char *link_path = "/l1";

    tfs_params params = tfs_default_params();
    params.max_inode_count = 3;
    params.max_block_count = 3;
    assert(tfs_init(&params) != -1);

    // Creates file and instantly deletes it making it inexistent
    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != 1);
    assert(tfs_close(fd) != 1);
    assert(tfs_unlink(file_path) != 1);

    // Create symbolic link to inexistent file
    assert(tfs_sym_link(file_path, link_path) != -1);

    // Makes sure file_path is not yet created
    assert(tfs_open(file_path, 0) == -1);

    // Create new file via the symbolic link
    fd = tfs_open(link_path, TFS_O_CREAT);
    assert(fd != -1);

    // Immediately close file
    assert(tfs_close(fd) != -1);

    // Open new file
    fd = tfs_open(file_path, 0);
    assert(fd != -1);

    // Immediately close file
    assert(tfs_close(fd) != -1);

    // Removes all the created files
    assert(tfs_unlink(link_path) != -1);
    assert(tfs_unlink(file_path) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}
