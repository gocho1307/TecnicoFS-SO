#ifndef __SERVER_H__
#define __SERVER_H__

#include <limits.h>
#include <sys/types.h>

/**
 * Server codes (for interactions between the server and clients)
 */
enum {
    SERVER_CODE_PUB_REGISTER = 1,
    SERVER_CODE_SUB_REGISTER = 2,
    SERVER_CODE_CREATE_REQUEST = 3,
    SERVER_CODE_CREATE_ANSWER = 4,
    SERVER_CODE_REMOVE_REQUEST = 5,
    SERVER_CODE_REMOVE_ANSWER = 6,
    SERVER_CODE_LIST_REQUEST = 7,
    SERVER_CODE_LIST_ANSWER = 8,
    SERVER_CODE_MESSAGE_RECEIVE = 9,
    SERVER_CODE_MESSAGE_SEND = 10,
};

#define CLIENT_NAMED_PIPE_MAX_LEN (256)
#define CLIENT_NAMED_PIPE_FORMAT ("/tmp/%s_%ld.pipe")
#define BOX_NAME_MAX_LEN (32)
#define MESSAGE_MAX_LEN (1024)
#define PIPE_BUFFER_MAX_LEN (PIPE_BUF)

/**
 * Uses POSIX's read function, but handles EINTR and checks if everything was
 * correctly read from the pipe.
 */
ssize_t pipe_read(int pipe_fd, void *buf, size_t buf_len);

/**
 * Uses POSIX's write function, but handles EINTR and checks if everything was
 * correctly written to the pipe.
 */
ssize_t pipe_write(int pipe_fd, const void *buf, size_t buf_len);

/**
 *
 */
int8_t *packet_create(size_t packet_len);

/**
 *
 */
void packet_write(void *packet, size_t *packet_offset, const void *data,
                  size_t data_len);

/**
 *
 */
int client_init(char *session_pipename);

/**
 *
 */
int client_request_connection(char *register_pipename, int code,
                              char *session_pipename, char *box_name);

#endif // __SERVER_H__
