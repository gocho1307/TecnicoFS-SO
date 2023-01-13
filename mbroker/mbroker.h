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
 *
 */
int mbroker_init(char *pipename, size_t max_sessions);

/**
 *
 */
void mbroker_shutdown(int signum);

/**
 *
 */
void mbroker_destroy(long max_sessions);

/**
 *
 */
int mbroker_receive_connection(int code, int register_pipe_in);

/**
 *
 */
int workers_init(int num_threads);

/**
 *
 */
void *workers_reception(void *arg);

/**
 *
 */
int workers_handle_pub_register(request_t *request);

/**
 *
 */
int workers_handle_sub_register(request_t *request);

/**
 *
 */
int workers_handle_manager_create(request_t *request);

/**
 *
 */
int workers_handle_manager_remove(request_t *request);

/**
 *
 */
int workers_handle_manager_listing(request_t *request);

#endif // __MBROKER_H__
