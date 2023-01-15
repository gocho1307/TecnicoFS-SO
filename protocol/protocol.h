#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <limits.h>
#include <stdint.h>
#include <sys/types.h>

#define CLIENT_NAMED_PIPE_MAX_LEN (256)
#define BOX_NAME_MAX_LEN (32)
#define MSG_MAX_LEN (1024)
#define PIPE_BUFFER_MAX_LEN (PIPE_BUF)

#define packet_ensure_len_limit(len)                                           \
    if (len > PIPE_BUFFER_MAX_LEN) {                                           \
        return -1;                                                             \
    }

/**
 * Protocol codes (for interactions between the server and the clients)
 */
enum {
    PROTOCOL_CODE_PUB_REGISTER = 1,
    PROTOCOL_CODE_SUB_REGISTER = 2,
    PROTOCOL_CODE_CREATE_REQUEST = 3,
    PROTOCOL_CODE_CREATE_ANSWER = 4,
    PROTOCOL_CODE_REMOVE_REQUEST = 5,
    PROTOCOL_CODE_REMOVE_ANSWER = 6,
    PROTOCOL_CODE_LIST_REQUEST = 7,
    PROTOCOL_CODE_LIST_ANSWER = 8,
    PROTOCOL_CODE_MESSAGE_RECEIVE = 9,
    PROTOCOL_CODE_MESSAGE_SEND = 10,
};

/**
 * Protocol requests (for interactions between the server and the clients)
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
 * Copies de data to the specified packet.
 *
 * Input:
 * - packet: The packet to be filled
 * - packet_offset: Offset to beggin writing the data
 * - data: The data to be written to the packet
 * - data_len: The length of the data
 */
void packet_write(void *packet, size_t *packet_offset, const void *data,
                  size_t data_len);

/**
 * Sends a request to the register pipe.
 *
 * Input:
 *	- register_pipename: Name of the register fifo, from which mbroker
 *receives requests
 *	- code: Request code number
 *	- session_pipename: Name of the client pipe
 *	- box_name: Except for the listing request, contains the name of the
 *desired box.
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int client_request_connection(char *register_pipename, int code,
                              char *session_pipename, char *box_name);

#endif // __PROTOCOL_H__
