/*
 *      File: server.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Contains the functions shared between all the server
 *                   programs.
 */

#include "server.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t pipe_read(int pipe_fd, void *buf, size_t buf_len) {
    ssize_t read_bytes;
    do {
        read_bytes = read(pipe_fd, buf, buf_len);
    } while (read_bytes < 0 && errno == EINTR);

    if (read_bytes != buf_len) {
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

int8_t *packet_create(size_t packet_len) {
    if (packet_len > PIPE_BUFFER_MAX_LEN) {
        WARN("Failed to create packet");
        return NULL;
    }
    int8_t *packet = (int8_t *)malloc(packet_len);
    if (packet == NULL) {
        WARN("Failed to create packet");
    }
    return packet;
}

void packet_write(void *packet, size_t *packet_offset, const void *data,
                  size_t data_len) {
    memcpy(packet + *packet_offset, data, data_len);
    *packet_offset += data_len;
}

int client_init(char *session_pipename) {
    if (unlink(session_pipename) != 0 && errno != ENOENT) {
        WARN("Failed to delete session pipe");
        return -1;
    }
    if (mkfifo(session_pipename, 0777) < 0) {
        WARN("Failed to create session pipe");
        return -1;
    }
    return 0;
}

int client_request_connection(char *register_pipename, int code,
                              char *session_pipename, char *name) {
    char box_name[BOX_NAME_MAX_LEN] = {0};
    strncpy(box_name, name, BOX_NAME_MAX_LEN);

    size_t packet_len = sizeof(uint8_t) +
                        sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN +
                        sizeof(char) * BOX_NAME_MAX_LEN;
    int8_t *packet = packet_create(packet_len);
    if (packet == NULL) {
        return -1;
    }
    size_t packet_offset = 0;

    // [ code (uint8_t) ] | [ client_named_pipe_path (char[256]) ] |
    // [ box_name (char[32]) ]
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));
    packet_write(packet, &packet_offset, session_pipename,
                 sizeof(char) * CLIENT_NAMED_PIPE_MAX_LEN);
    packet_write(packet, &packet_offset, box_name,
                 sizeof(char) * BOX_NAME_MAX_LEN);

    int register_pipe_in = open(register_pipename, O_WRONLY);
    if (register_pipe_in < 0) {
        WARN("Failed to open register pipe");
        free(packet);
        return -1;
    }
    if (pipe_write(register_pipe_in, packet, packet_len) == -1) {
        WARN("Failed to estabilish connection to mbroker");
        free(packet);
        close(register_pipe_in);
        return -1;
    }
    free(packet);
    if (close(register_pipe_in) < 0) {
        WARN("Failed to close register pipe");
        return -1;
    }
    return 0;
}
