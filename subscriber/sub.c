/*
 *      File: sub.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that reads information from the server boxes.
 */

#include "sub.h"
#include "../common/common.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t shutdown_signaler = 0;

static void print_usage() {
    fprintf(stderr, "Usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN + 1] = {0};
    strncpy(session_pipename, argv[2], CLIENT_NAMED_PIPE_MAX_LEN);

    signal(SIGINT, subscriber_shutdown);

    // Starts the session pipename for the subscriber
    if (client_init(session_pipename) != 0) {
        return EXIT_FAILURE;
    }

    // Requests the mbroker for a connection
    if (client_request_connection(argv[1], SERVER_CODE_SUB_REGISTER,
                                  session_pipename, argv[3]) != 0) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    // Reads messages from the mbroker and prints them into the stdout
    if (subscriber_read_messages(session_pipename) != 0) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    unlink(session_pipename);
    return 0;
}

int subscriber_read_messages(char *session_pipename) {
    int number_messages = 0;
    int session_pipe_in = open(session_pipename, O_RDONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    uint8_t code;
    char message[MESSAGE_MAX_LEN] = {0};
    while (shutdown_signaler == 0) {
        if (pipe_read(session_pipe_in, &code, sizeof(uint8_t)) ||
            code != SERVER_CODE_MESSAGE_SEND) {
            close(session_pipe_in);
            return -1;
        }
        if (pipe_read(session_pipe_in, &message,
                      sizeof(char) * MESSAGE_MAX_LEN) != 0) {
            close(session_pipe_in);
            return -1;
        }
        fprintf(stdout, "%s\n", message);
        number_messages++;
    }
    printf("Number of messages read: %d\n", number_messages);
    close(session_pipe_in);

    return 0;
}

void subscriber_shutdown(int signum) { shutdown_signaler = signum; }
