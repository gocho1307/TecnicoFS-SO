/*
 *      File: protocol.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Contains all the functions used to manage the communication
 *                   protocol between the client and the server.
 */

#include "protocol.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

ssize_t pipe_read(int pipe_fd, void *buf, size_t buf_len) {
    ssize_t read_bytes;
    do {
        read_bytes = read(pipe_fd, buf, buf_len);
    } while (read_bytes < 0 && errno == EINTR);

    if (read_bytes != 0 && read_bytes != buf_len) {
        return -1;
    }

    return read_bytes;
}

ssize_t pipe_write(int pipe_fd, const void *buf, size_t buf_len) {
    ssize_t written_bytes;
    do {
        written_bytes = write(pipe_fd, buf, buf_len);
    } while (written_bytes < 0 && errno == EINTR);

    if (written_bytes != buf_len) {
        return -1;
    }

    return written_bytes;
}

void packet_write(void *packet, size_t *packet_offset, const void *data,
                  size_t data_len) {
    memcpy(packet + *packet_offset, data, data_len);
    *packet_offset += data_len;
}

int client_request_connection(char *register_pipename, int code,
                              char *session_pipename, char *box_name) {
    size_t packet_len =
        sizeof(uint8_t) + sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN;
    if (box_name != NULL) {
        packet_len += sizeof(char) * BOX_NAME_MAX_LEN;
    }
    packet_ensure_len_limit(packet_len);
    int8_t packet[packet_len];

    // [ code (uint8_t) ] | [ client_named_pipe_path (char[256]) ] |
    // [ box_name (char[32]) ]
    size_t packet_offset = 0;
    memset(packet, 0, packet_len);
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));
    packet_write(packet, &packet_offset, session_pipename,
                 sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN);
    if (box_name != NULL) {
        packet_write(packet, &packet_offset, box_name,
                     sizeof(char) * strlen(box_name));
    }

    int register_pipe_in = open(register_pipename, O_WRONLY);
    if (register_pipe_in < 0) {
        WARN("Failed to open register pipe");
        return -1;
    }
    if (pipe_write(register_pipe_in, packet, packet_len) <= 0) {
        WARN("Failed to estabilish connection to mbroker");
        close(register_pipe_in);
        return -1;
    }
    if (close(register_pipe_in) < 0) {
        WARN("Failed to close register pipe");
        return -1;
    }

    return 0;
}
