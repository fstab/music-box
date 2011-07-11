#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "shell.h"

static FILE *open_logfile(const char *path);

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "l:q")) != -1) {
        switch (opt) {
            case 'q':
                shell_set_quiet(1);
                break;
            case 'l':
                set_logfile(open_logfile(optarg));
                break;
            default:
                fprintf(stderr, "Usage: %s [-l logfile] [-q]\n", argv[0]);
                fprintf(stderr, "   -l <logfile>: Log to file instead of "
                            "stderr\n");
                fprintf(stderr, "   -q: Quiet mode. Do not show the prompt. "
                            "Usefull for scripting.\n");
                exit(EXIT_FAILURE);
        }
    }
    set_log_level_debug(); /* this is a test program, we always want debug */
    shell_run();
    return 0;
}

FILE *open_logfile(const char *path) {
    FILE *file;
    if ( (file = fopen(path, "a+")) == NULL ) {
        fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return file;
}
