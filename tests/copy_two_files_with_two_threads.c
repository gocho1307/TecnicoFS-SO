#include "fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void* func (void* file) {

	int f;
    ssize_t r;
	char buffer[600];

	f = tfs_copy_from_external_fs( ((char**)file)[0], ((char**)file)[1] );
	assert(f != -1);

    f = tfs_open(((char**)file)[1], TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen( ((char**)file)[3] ));
    assert(!memcmp(buffer, ((char**)file)[3], strlen(((char**)file)[3] )));

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

	return NULL;
}

int main () {

	char* file_to_copy_1 = "tests/file_to_copy.txt";
	char* file_to_copy_2 = "tests/file_to_copy_many_different_characters.txt";
	char* path_copied_file_1 = "/f1";
	char* path_copied_file_2 = "/f2";
	char* str_ext_file_1 = "BBB!";
	char* str_ext_file_2 = "s5vSTm4EeXMGMh^C#Pj3wa6eZ#K%GSAik%Phx^J$@f&wdffxCNRK^psv7UCBf3YmY^PC&DCHgo6AdMeK*kaSCZV#BX9QiybRJxgu";

	assert(tfs_init(NULL) != -1);

	int result;
	pthread_t pid[2];

	char* file_1[] = {file_to_copy_1, path_copied_file_1, str_ext_file_1};
	result = pthread_create(&pid[0], NULL, func, (void*)file_1);
	assert(!result);

	char* file_2[] = {file_to_copy_2, path_copied_file_2, str_ext_file_2};
	pthread_create(&pid[1], NULL, func, (void*)file_2);
	assert(!result);

	result = pthread_join(pid[0], NULL);
	assert(!result);
	result = pthread_join(pid[1], NULL);
	assert(!result);

    printf("Successful test.\n");

    return 0;
}
