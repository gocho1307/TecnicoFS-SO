#include "manager.h"
#include "../server/server.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN] = {0};
static int session_pipe_in;

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        print_usage();
        return EXIT_FAILURE;
    }

    // Gets a unique session pipename for the subscriber based on its pid and
    // initializes the session pipe
    pid_t client_pid = getpid();
    if (client_get_named_pipe(session_pipename, CLIENT_NAMED_PIPE_MAX_LEN,
                                 "manager", client_pid) == -1) {
        return EXIT_FAILURE;
    }
    if (client_init(session_pipename) == -1) {
        return EXIT_FAILURE;
    }

    // Handles the correct manager operation
    char *operation = argv[2];
    int result;
    switch (operation[0]) {
    case 'c':
        result = handle_box_create(argv[1], argv[2]);
        break;
    case 'r':
        result = handle_box_remove(argv[1], argv[2]);
        break;
    case 'l':
        result = handle_box_listing(argv[1]);
        break;
    default:
        return EXIT_FAILURE;
        break;
    }
    if (result == -1) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    close(session_pipe_in);
    unlink(session_pipename);
    return 0;
}

int handle_box_create(char *register_pipe_name, char *box_name) {
    if (client_request_connection(register_pipe_name,
                                  SERVER_CODE_CREATE_REQUEST, session_pipename,
                                  box_name) == -1) {
        return -1;
    }

    return handle_box_management(box_name);
}

int handle_box_remove(char *register_pipe_name, char *box_name) {
    if (client_request_connection(register_pipe_name,
                                  SERVER_CODE_REMOVE_REQUEST, session_pipename,
                                  box_name) == -1) {
        return -1;
    }

    return handle_box_management(box_name);
}

int handle_box_management(char *box_name) {
    session_pipe_in = open(session_pipename, O_RDONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    uint8_t code;
    if (pipe_read(session_pipe_in, &code, sizeof(uint8_t)) == -1) {
        close(session_pipe_in);
        return -1;
    }
    if (code != SERVER_CODE_CREATE_ANSWER &&
        code != SERVER_CODE_REMOVE_ANSWER) {
        close(session_pipe_in);
        return -1;
    }

    int32_t return_code;
    if (pipe_read(session_pipe_in, &return_code, sizeof(int32_t)) == -1) {
        close(session_pipe_in);
        return -1;
    }
    if (return_code == 0 && code == SERVER_CODE_CREATE_ANSWER) {
        printf("Successfully created box with name: %s\n", box_name);
    } else if (return_code == 0 && code == SERVER_CODE_REMOVE_ANSWER) {
        printf("Successfully removed box with name: %s\n", box_name);
    } else {
        char error_message[MESSAGE_MAX_LEN];
        if (pipe_read(session_pipe_in, error_message,
                      sizeof(char) * MESSAGE_MAX_LEN) == -1) {
            close(session_pipe_in);
            return -1;
        }
        printf("ERROR: %s\n", error_message);
    }

    return 0;
}

int handle_box_listing(char *register_pipe_name) {
    if (client_request_connection(register_pipe_name, SERVER_CODE_LIST_REQUEST,
                                  session_pipename, NULL) == -1) {
        return -1;
    }

    return 0;
}
