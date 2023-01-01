#include "sub.h"
#include "../server/server.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

static char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN] = {0};
static int session_pipe_in;
static int number_messages = 0;

static void print_usage() {
    fprintf(stderr, "usage: sub <register_pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    signal(SIGINT, shutdown);

    // Gets a unique session pipename for the subscriber based on its pid and
    // initializes the session pipe
    pid_t client_pid = getpid();
    if (client_get_named_pipe(session_pipename, CLIENT_NAMED_PIPE_MAX_LEN,
                                 "subscriber", client_pid) == -1) {
        return EXIT_FAILURE;
    }
    if (client_init(session_pipename) == -1) {
        return EXIT_FAILURE;
    }

    // Requests the mbroker for a connection
    if (client_request_connection(argv[1], SERVER_CODE_SUB_REGISTER,
                                  session_pipename, argv[2]) == -1) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    // Reads messages from the mbroker and prints them into the stdout
    if (read_messages() == -1) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    return 0;
}

int read_messages() {
    session_pipe_in = open(session_pipename, O_RDONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    uint8_t code;
    char message[MESSAGE_MAX_LEN];
    while (pipe_read(session_pipe_in, &code, sizeof(uint8_t)) == 0 &&
           code == SERVER_CODE_MESSAGE_SEND) {
        if (pipe_read(session_pipe_in, &message,
                      sizeof(char) * MESSAGE_MAX_LEN) != 0) {
            close(session_pipe_in);
            return -1;
        }
        fprintf(stdout, "%s\n", message);
        number_messages++;
    }
    close(session_pipe_in);

    return -1;
}

void shutdown(int signum) {
    (void)signum;
    write(STDOUT_FILENO, &number_messages, sizeof(int));
    char new_line = '\n';
    write(STDOUT_FILENO, &new_line, sizeof(char));

    close(session_pipe_in);
    unlink(session_pipename);
}
