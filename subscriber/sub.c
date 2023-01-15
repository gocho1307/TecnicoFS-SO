/*
 *      File: sub.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that reads information from the server boxes.
 */

#include "sub.h"
#include "../protocol/protocol.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t shutdown_signaler = 0;

static void print_usage() {
    fprintf(stderr, "Usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        print_usage();
        return EXIT_FAILURE;
    }

    signal(SIGINT, subscriber_shutdown);
    signal(SIGPIPE, SIG_IGN);

    // Session pipe name is truncated to fit the request message
    char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN] = {0};
    strcpy(session_pipename, argv[2]);

    // Creates the session pipename for the subscriber
    if ((unlink(session_pipename) != 0 && errno != ENOENT) ||
        mkfifo(session_pipename, 0777) < 0) {
        WARN("Failed to create session pipe");
        return EXIT_FAILURE;
    }

    // Requests the mbroker for a connection
    if (client_request_connection(argv[1], PROTOCOL_CODE_SUB_REGISTER,
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
    int session_pipe_out = open(session_pipename, O_RDONLY);
    if (session_pipe_out < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    int n_messages = 0;
    uint8_t code;
    char message[MSG_MAX_LEN] = {0};
    while (shutdown_signaler == 0) {
        if (read(session_pipe_out, &code, sizeof(uint8_t)) != sizeof(uint8_t) ||
            code != PROTOCOL_CODE_MESSAGE_SEND ||
            read(session_pipe_out, &message, sizeof(char) * MSG_MAX_LEN) !=
                sizeof(char) * MSG_MAX_LEN) {
            break;
        }
        fprintf(stdout, "%s\n", message);
        n_messages++;
    }
    // We always show the number of messages when the session ends
    fprintf(stdout, "Number of messages read: %d\n", n_messages);
    close(session_pipe_out);

    return 0;
}

void subscriber_shutdown(int signum) { shutdown_signaler = signum; }
