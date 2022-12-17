#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>

int main() {
    char *path1 = "/f1";
    char *external_path = "";

    /* Tests different scenarios where tfs_copy_from_external_fs is expected to
     * fail */

    assert(tfs_init(NULL) != -1);

    int f1 = tfs_open(path1, TFS_O_CREAT);
    assert(f1 != -1);
    assert(tfs_close(f1) != -1);

    // Scenario 1: source file does not exist
    assert(tfs_copy_from_external_fs("./unexistent", path1) == -1);

    // Scenario 2: source file is in a directory that doesn't exit
    assert(tfs_copy_from_external_fs("./unexistent-directory/unexistent", path1) == -1);

    // Scenario 3: the destination path name is invalid
    assert(tfs_copy_from_external_fs(external_path, "tmp-test") == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
