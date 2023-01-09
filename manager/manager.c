/*
 *      File: manager.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that manages the boxes in the server.
 */

#include "manager.h"
#include "../common/common.h"
#include "../fs/state.h"
#include "../utils/insertion-sort.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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

    // Session pipe name is truncated to fit the request message
    char session_pipename[CLIENT_NAMED_PIPE_MAX_LEN] = {0};
    strncpy(session_pipename, argv[2], CLIENT_NAMED_PIPE_MAX_LEN - 1);

    // Creates the session pipename for the manager
    if (mkfifo(session_pipename, 0777) < 0) {
        WARN("Failed to create session pipe");
        return -1;
    }

    // Handles the correct manager operation
    char *operation = argv[3];
    int result;
    switch (operation[0]) {
    case 'c':
        result = manager_handle_box_management(
            argv[1], SERVER_CODE_CREATE_REQUEST, session_pipename, argv[4]);
        break;
    case 'r':
        result = manager_handle_box_management(
            argv[1], SERVER_CODE_REMOVE_REQUEST, session_pipename, argv[4]);
        break;
    case 'l':
        result = manager_handle_box_listing(argv[1], session_pipename);
        break;
    default:
        unlink(session_pipename);
        return EXIT_FAILURE;
        break;
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

    int session_pipe_in = open(session_pipename, O_RDONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    if (pipe_read(session_pipe_in, &code, sizeof(uint8_t)) != 0) {
        close(session_pipe_in);
        return -1;
    }
    if (code != SERVER_CODE_CREATE_ANSWER &&
        code != SERVER_CODE_REMOVE_ANSWER) {
        close(session_pipe_in);
        return -1;
    }

    int32_t return_code;
    if (pipe_read(session_pipe_in, &return_code, sizeof(int32_t)) != 0) {
        close(session_pipe_in);
        return -1;
    }
    if (return_code == 0) {
        fprintf(stdout, "OK\n");
    } else {
        char error_message[MESSAGE_MAX_LEN];
        if (pipe_read(session_pipe_in, error_message,
                      sizeof(char) * MESSAGE_MAX_LEN) != 0) {
            close(session_pipe_in);
            return -1;
        }
        fprintf(stdout, "ERROR %s\n", error_message);
    }

    close(session_pipe_in);
    return 0;
}

int manager_handle_box_listing(char *register_pipe_name,
                               char *session_pipename) {
    if (client_request_connection(register_pipe_name, SERVER_CODE_LIST_REQUEST,
                                  session_pipename, NULL) != 0) {
        return -1;
    }

    int session_pipe_in = open(session_pipename, O_RDONLY);
    if (session_pipe_in < 0) {
        WARN("Failed to open pipe");
        return -1;
    }

    size_t max_box_number = inode_table_size();
    box boxes[max_box_number];
    int size = -1;
    uint8_t last = 0, code;
    while (last == 0) {
        if (pipe_read(session_pipe_in, &code, sizeof(uint8_t)) != 0 ||
            code != SERVER_CODE_LIST_ANSWER) {
            close(session_pipe_in);
            return -1;
        }

        box new_box;
        if (pipe_read(session_pipe_in, &last, sizeof(uint8_t)) ||
            pipe_read(session_pipe_in, &new_box.name,
                      sizeof(char) * BOX_NAME_MAX_LEN) ||
            pipe_read(session_pipe_in, &new_box.size, sizeof(uint64_t)) ||
            pipe_read(session_pipe_in, &new_box.n_publishers,
                      sizeof(uint64_t)) ||
            pipe_read(session_pipe_in, &new_box.n_subscribers,
                      sizeof(uint64_t))) {
            close(session_pipe_in);
            return -1;
        }
        sorted_insert(boxes, size++, new_box);
    }

    for (int i = 0; i <= size; i++) {
        fprintf(stdout, "%s %zu %zu %zu\n", boxes[i].name, boxes[i].size,
                boxes[i].n_publishers, boxes[i].n_subscribers);
    }
    if (size == -1) {
        fprintf(stdout, "NO BOXES FOUND\n");
    }

    close(session_pipe_in);
    return 0;
}
