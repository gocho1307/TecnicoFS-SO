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

    int fd1 = tfs_open(file_path, TFS_O_CREAT);
    assert(fd1 != -1);

    // Immediately close file
    assert(tfs_close(fd1) != -1);

    // Create symbolic link
    assert(tfs_sym_link(file_path, link_path) != -1);

    // Remove original file
    assert(tfs_unlink(file_path) != -1);
    // Makes sure the original file was deleted
    assert(tfs_open(file_path, 0) == -1);
    // Makes sure symbolic link is unusable
    assert(tfs_open(link_path, 0) == -1);

    // Create new file with same filename
    fd1 = tfs_open(link_path, TFS_O_CREAT);
    assert(fd1 != -1);
    // Makes sure the file was created with the file_path name
    int fd2 = tfs_open(file_path, 0);
    assert(fd2 != -1);

    // Closes all the files
    assert(tfs_close(fd1) != -1);
    assert(tfs_close(fd2) != -1);

    // Removes all the created files
    assert(tfs_unlink(link_path) != -1);
    assert(tfs_unlink(file_path) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}
