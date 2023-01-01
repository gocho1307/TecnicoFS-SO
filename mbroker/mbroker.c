#include "../server/server.h"
#include "../utils/logging.h"

static void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    return 0;
}
