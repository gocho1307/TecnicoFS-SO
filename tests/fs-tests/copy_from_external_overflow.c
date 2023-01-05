#include "../../fs/operations.h"
#include <assert.h>
#include <stdio.h>

char *path_copied_file = "/f1";
char *path_src = "tests/file_overflow.txt";

/**
 *
 */
int main() {
    assert(tfs_init(NULL) != -1);

    int f;

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
