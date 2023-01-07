/*
 *      File: pub.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that publishes information into the server boxes.
 */

#include "pub.h"
#include "../common/common.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr, "Usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN + 1] = {0};
    strncpy(session_pipename, argv[2], CLIENT_NAMED_PIPE_MAX_LEN);

    // Starts the session pipename for the publisher
    if (client_init(session_pipename) != 0) {
        return EXIT_FAILURE;
    }

    // Requests the mbroker for a connection
    if (client_request_connection(argv[1], SERVER_CODE_PUB_REGISTER,
                                  session_pipename, argv[3]) != 0) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    // Writes messages from the stdin into the mbroker
    if (publisher_write_messages(session_pipename) != 0) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    unlink(session_pipename);
    return 0;
}

int publisher_write_messages(char *session_pipename) {
    size_t packet_len = sizeof(uint8_t) + sizeof(char) * MESSAGE_MAX_LEN;
    int8_t *packet = packet_create(packet_len);
    if (packet == NULL) {
        return -1;
    }
    size_t packet_offset = 0;

    // [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
    int session_pipe_in = open(session_pipename, O_WRONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }
    uint8_t code = SERVER_CODE_MESSAGE_RECEIVE;
    packet_write(packet, &packet_offset, &code, sizeof(uint8_t));
    char message[MESSAGE_MAX_LEN] = {0};
    while (fgets(message, MESSAGE_MAX_LEN, stdin) != NULL) {
        if (message[strlen(message) - 1] == '\n') {
            message[strlen(message) - 1] = '\0';
        }
        packet_write(packet, &packet_offset, message,
                     sizeof(char) * MESSAGE_MAX_LEN);
        if (pipe_write(session_pipe_in, packet, packet_len) != 0) {
            free(packet);
            close(session_pipe_in);
            return -1;
        }
        packet_offset = sizeof(uint8_t);
        memset(message, 0, MESSAGE_MAX_LEN);
    }
    free(packet);
    close(session_pipe_in);

    return 0;
}
