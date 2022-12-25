#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * External mutexes/locks
 */
extern pthread_mutex_t free_open_file_entries_mutex;
extern pthread_rwlock_t *inode_locks;

/**
 * Inode
 */
typedef enum { T_FILE, T_DIRECTORY, T_SYM_LINK } inode_type;

typedef struct {
    inode_type i_node_type;

    size_t i_size;
    int i_data_block;
    size_t i_hard_links;

    // in a more complete FS, more fields should exist here
} inode_t;

/**
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

/**
 * Open file entry (in open file table)
 */
typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

typedef struct {
    int of_inumber;
    size_t of_offset;
} open_file_entry_t;

int state_init(tfs_params);
int state_destroy(void);
size_t state_block_size(void);

int inode_create(inode_type n_type);
void inode_delete(int inumber);
inode_t *inode_get(int inumber);

int add_dir_entry(inode_t *inode, char const *sub_name, int sub_inumber);
int clear_dir_entry(inode_t *inode, char const *sub_name);
int find_in_dir(inode_t const *inode, char const *sub_name);

int data_block_alloc(void);
void data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset);
int remove_from_open_file_table(int fhandle);
ssize_t write_to_open_file(int fhandle, void const *buffer, size_t to_write);
ssize_t read_from_open_file(int fhandle, void *buffer, size_t len);
open_file_entry_t *get_open_file_entry(int fhandle);
bool is_file_open(int inumber);

#endif // STATE_H
