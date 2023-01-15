#ifndef __MBROKER_H__
#define __MBROKER_H__

#include "../protocol/protocol.h"
#include <sys/types.h>

/**
 * Server request
 */
typedef struct {
    uint8_t code;
    char client_named_pipe_path[CLIENT_NAMED_PIPE_MAX_LEN];
    union {
        char box_name[BOX_NAME_MAX_LEN];
        struct {
        } empty;
    };
} request_t;

/**
 * Initialises the mbroker parameters, threads and pipes, and the tfs
 *
 * Input:
 *	- pipename: Register fifo name
 *	- max_sessions: Maximum number of sessions mbroker will support
 *
 * Returns 0 if successful, -1 otherwise.
 */
int mbroker_init(char *pipename, size_t max_sessions);

/**
 * Finishes reading a request coming through the register pipe, where the
 * request code has already been read and sent as a parameter. It then 
 * creates the according request_t structure and enqueues it in the pcq.
 *
 * Input:
 *	- code: Request code, acording to the mbroker protocol
 *	- register_pipe_in: File descriptor of the already open register fifo,
 *	from where we read the remaining part of the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int mbroker_receive_connection(uint8_t code, int register_pipe_in);

/**
 * Creates num_threads worker threads to process incoming requests.
 *
 * Input:
 *	- num_threads: number of threads to create
 *
 *	Returns 0 if successful, -1 otherwise:
 */
int workers_init(size_t num_threads);

/**
 * Function run by the threads sent in workers_init. They remain in 
 * idle wait until it the pcq dequeues a request_t, for which they call
 * its respective handling function.
 *
 * Input:
 *	- arg: exists to comply with pthreads' function format
 *
 * Returns NULL.
 */
void *workers_reception(void *arg);

/**
 * Handles the publisher regist request.
 *
 * Input:
 *	- request: the data necessary for granting the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int workers_handle_pub_register(request_t *request);

/**
 * Handles the subscriber regist request.
 *
 * Input:
 *	- request: the data necessary for granting the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int workers_handle_sub_register(request_t *request);

/**
 * Handles the publisher regist request.
 *
 * Input:
 *	- request: the data necessary for granting the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int workers_handle_manager_create(request_t *request);

/**
 * Handles the box removal request.
 *
 * Input:
 *	- request: the data necessary for granting the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int workers_handle_manager_remove(request_t *request);

/**
 * Handles the listing request.
 *
 * Input:
 *	- request: the data necessary for granting the request
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int workers_handle_manager_listing(request_t *request);

/**
 * Creates a box named box_name.
 *
 * Input:
 *	- box_name: name of the box to be created
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int box_create(char *box_name);

/**
 * Deletes the box with box_name name.
 *
 * Input: 
 *	- box_name: name of the box to be deleted.
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int box_delete(char *box_name);

/**
 * Finds the position of the box in the boxes_table
 *
 * Input:
 *	- box_name: name of the box to find
 *
 *	Return position of the box in the array is successful, -1 otherwise.
 */
int box_find(char *box_name);

#endif // __MBROKER_H__
