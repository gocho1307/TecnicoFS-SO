#include "../../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

char *str_ext_file = "BBB!";
char *path_copied_file1 = "/f1";
char *path_copied_file_link1 = "/l1";
char *path_copied_file2 = "/f2";
char *path_copied_file_link2 = "/l2";
char *path_src = "tests/fs-tests/file_to_copy.txt";
char buffer1[40] = {0}, buffer2[40] = {0};

int main() {
    assert(tfs_init(NULL) != -1);

    int f1, f2;
    ssize_t r1, r2;

    f1 = tfs_open(path_copied_file1, TFS_O_CREAT);
    assert(f1 != -1);
    assert(tfs_close(f1) != -1);

    f2 = tfs_open(path_copied_file2, TFS_O_CREAT);
    assert(f2 != -1);
    assert(tfs_close(f2) != -1);

    assert(tfs_link(path_copied_file1, path_copied_file_link1) != -1);
    assert(tfs_sym_link(path_copied_file2, path_copied_file_link2) != -1);

    f1 = tfs_copy_from_external_fs(path_src, path_copied_file_link1);
    assert(f1 != -1);

    f2 = tfs_copy_from_external_fs(path_src, path_copied_file_link2);
    assert(f2 != -1);

    f1 = tfs_open(path_copied_file_link1, TFS_O_CREAT);
    assert(f1 != -1);

    f2 = tfs_open(path_copied_file_link2, TFS_O_CREAT);
    assert(f2 != -1);

    r1 = tfs_read(f1, buffer1, sizeof(buffer1) - 1);
    r2 = tfs_read(f2, buffer2, sizeof(buffer2) - 1);
    assert(r1 == r2);
    assert(r1 == strlen(str_ext_file));
    assert(!memcmp(buffer1, buffer2, strlen(buffer1)));
    assert(!memcmp(buffer1, str_ext_file, strlen(str_ext_file)));

    assert(tfs_close(f1) != -1);
    assert(tfs_close(f2) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
