/*
 *      File: manager.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that manages the boxes in the server.
 */

#include "manager.h"
#include "../protocol/protocol.h"
#include "../utils/insertion-sort.h"
#include "../utils/logging.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr,
            "Usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) {
        print_usage();
        return EXIT_FAILURE;
    }
    if (strcmp(argv[3], "create") && strcmp(argv[3], "remove") &&
        strcmp(argv[3], "list")) {
        print_usage();
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    // Session pipe name is truncated to fit the request message
    char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN] = {0};
    strcpy(session_pipename, argv[2]);

    // Creates the session pipename for the manager
    if ((unlink(session_pipename) != 0 && errno != ENOENT) ||
        mkfifo(session_pipename, 0777) < 0) {
        WARN("Failed to create session pipe");
        return EXIT_FAILURE;
    }

    // Handles the correct manager operation
    char *operation = argv[3];
    int result;
    switch (operation[0]) {
    case MANAGER_CREATE:
        result = manager_handle_box_management(
            argv[1], PROTOCOL_CODE_CREATE_REQUEST, session_pipename, argv[4]);
        break;
    case MANAGER_REMOVE:
        result = manager_handle_box_management(
            argv[1], PROTOCOL_CODE_REMOVE_REQUEST, session_pipename, argv[4]);
        break;
    case MANAGER_LISTING:
        result = manager_handle_box_listing(argv[1], session_pipename);
        break;
    default:
        unlink(session_pipename);
        return EXIT_FAILURE;
    }
    if (result != 0) {
        unlink(session_pipename);
        return EXIT_FAILURE;
    }

    unlink(session_pipename);
    return 0;
}

int manager_handle_box_management(char *register_pipe_name, uint8_t code,
                                  char *session_pipename, char *box_name) {
    if (client_request_connection(register_pipe_name, code, session_pipename,
                                  box_name) != 0) {
        return -1;
    }

    int session_pipe_out = open(session_pipename, O_RDONLY);
    if (session_pipe_out < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    // We read and validate the code
    if (pipe_read(session_pipe_out, &code, sizeof(uint8_t)) <= 0) {
        close(session_pipe_out);
        return -1;
    }
    if (code != PROTOCOL_CODE_CREATE_ANSWER &&
        code != PROTOCOL_CODE_REMOVE_ANSWER) {
        close(session_pipe_out);
        return -1;
    }

    int32_t return_code;
    if (pipe_read(session_pipe_out, &return_code, sizeof(int32_t)) <= 0) {
        close(session_pipe_out);
        return -1;
    }
    // We print the message output for the operation granted by the server
    if (return_code == 0) {
        fprintf(stdout, "OK\n");
    } else {
        char error_message[MSG_MAX_LEN];
        if (pipe_read(session_pipe_out, error_message,
                      sizeof(char) * MSG_MAX_LEN) <= 0) {
            close(session_pipe_out);
            return -1;
        }
        fprintf(stdout, "ERROR %s\n", error_message);
    }

    close(session_pipe_out);
    return 0;
}

int manager_handle_box_listing(char *register_pipe_name,
                               char *session_pipename) {
    if (client_request_connection(register_pipe_name,
                                  PROTOCOL_CODE_LIST_REQUEST, session_pipename,
                                  NULL) != 0) {
        return -1;
    }

    int session_pipe_out = open(session_pipename, O_RDONLY);
    if (session_pipe_out < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    box_t boxes[MAX_BOX_AMOUNT];
    int size = -1;
    uint8_t last = 0, code = 0;
    while (last == 0) {
        // We read the information for each box in order
        // code | last | box_name | box_size | n_publishers | n_subscribers
        if (pipe_read(session_pipe_out, &code, sizeof(uint8_t)) <= 0 ||
            code != PROTOCOL_CODE_LIST_ANSWER) {
            close(session_pipe_out);
            return -1;
        }

        box_t new_box;
        if (pipe_read(session_pipe_out, &last, sizeof(uint8_t)) <= 0 ||
            pipe_read(session_pipe_out, &new_box.name,
                      sizeof(char) * BOX_NAME_MAX_LEN) <= 0 ||
            pipe_read(session_pipe_out, &new_box.size, sizeof(uint64_t)) <= 0 ||
            pipe_read(session_pipe_out, &new_box.n_publishers,
                      sizeof(uint64_t)) <= 0 ||
            pipe_read(session_pipe_out, &new_box.n_subscribers,
                      sizeof(uint64_t)) <= 0) {
            close(session_pipe_out);
            return -1;
        }
        if (last == 1 && new_box.name[0] == '\0') {
            break;
        }
        sorted_insert(boxes, size++, &new_box);
    }
    close(session_pipe_out);

    // We iterate over all the boxes and print the information one by one
    for (int i = 0; i <= size; i++) {
        fprintf(stdout, "%s %zu %zu %zu\n", boxes[i].name, boxes[i].size,
                boxes[i].n_publishers, boxes[i].n_subscribers);
    }
    // When no boxes are found the size is -1
    if (size == -1) {
        fprintf(stdout, "NO BOXES FOUND\n");
    }

    return 0;
}
