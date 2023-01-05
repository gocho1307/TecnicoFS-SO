#include "../../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

char *str_ext_file1 =
    "s5vSTm4EeXMGMh^C#Pj3wa6eZ#K%GSAik%Phx^J$@f&wdffxCNRK^psv7UCBf3YmY^PC&"
    "DCHgo6AdMeK*kaSCZV#BX9QiybRJxgu";
char *str_ext_file2 = "AAA";
char *path_copied_file = "/f1";
char *path_src1 = "tests/fs-tests/file_to_copy_many_different_characters.txt";
char *path_src2 = "tests/fs-tests/file_to_copy_very_small.txt";
char buffer[600] = {0};

/**
 *
 */
int main() {
    assert(tfs_init(NULL) != -1);

    int f, t;
    ssize_t r;

    // Copy normal file into fs
    f = tfs_copy_from_external_fs(path_src2, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file2));
    assert(!memcmp(buffer, str_ext_file2, strlen(str_ext_file2)));

    t = tfs_close(f);
    assert(t != -1);
    memset(buffer, 0, sizeof(buffer) - 1);

    // Copy into file that already has data in the fs
    f = tfs_copy_from_external_fs(path_src1, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file1));
    assert(!memcmp(buffer, str_ext_file1, strlen(str_ext_file1)));

    t = tfs_close(f);
    assert(t != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
