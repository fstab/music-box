#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "error_codes.h"
#include "controller.h"

static int exec_load(int argc, char **argv);
static int exec_unload(int argc, char **argv);
static int exec_play(int argc, char **argv);
static int exec_pause(int argc, char **argv);
static int exec_wait(int argc, char **argv);
static int exec_resume(int argc, char **argv);
static int exec_sleep(int argc, char **argv);
static int exec_quit(int argc, char **argv);
static int exec_help(int argc, char **argv);

static void initialize_readline();
static char *next_non_whitespace(char *line);
static void split(char *line, int *argc_p, char **argv_p[]);
static int execute_cmdline(char *line);
static struct command *find_command(char *name);
static char **command_completion(const char *text, int start, int end);
static char *command_generator(const char *text, int state);
static char *quote_filename(char *s, int rtype, char *qcp);
static char *dequote_filename(char *s, int quote_char);
static int char_is_quoted(char *text, int index);
static void realloc_argv(int, char ***, size_t *);
static void strip_backslashes_in_place(char *s);
static void print_indented(const char *s, int indent);
/* for comments on __attribute__ see "log.h" */
static void usr_msg(const char *, ...) __attribute__ ((format (printf, 1, 2)));

#define WARNING_MSG \
"This is a test program. It doesn't support unexpected things, like strange "\
"characters in filenames.\n"

#define GREETING_MSG \
"Type \"help\" for help.\n"

/* Struct for defining a command for this shell */
struct command {
    char *name;                          /* command name, like "play" */
    int (*func)(int argc, char **argv);  /* function implementing the command */
    const char *usage;                   /* usage message, can be NULL */
    const char *helpmsg;                 /* help message, can be NULL */
};

/* Array containing all commands supported by this shell.
 * This array is terminated with a NULL command */
static struct command commands[] = {
    { "load", exec_load, "load <file.mp3> as <var>\n",
      "Load an mp3 file and store it in variable <var>\n"
      "For filenames with spaces, type the filename in \"double quotes\".\n"},
    { "play", exec_play, "play <var>\n",
      "Start playing the file loaded as <var>\n" },
    { "pause", exec_pause, "pause <var>\n",
       "Pause the file loaded as <var>\n" },
    { "resume", exec_resume, "resume <var>\n",
      "If <var> is paused, resume playing <var>\n" },
    { "wait", exec_wait, "wait for <var>\n",
      "wait until file <var> is done playing\n" },
    { "unload", exec_unload, "unload <var>\n",
      "Unload the mp3 file and free the variable <var>\n" },
    { "sleep", exec_sleep, "sleep <seconds>\n",
      "sleep for <seconds> seconds\n" },
    { "quit", exec_quit, "quit\n",
      "quit this application\n" },
    { "exit", exec_quit, NULL, NULL },
    { "bye", exec_quit, NULL, NULL },
    { "help", exec_help, NULL, NULL },
    { "?", exec_help, NULL, NULL },
    { NULL, NULL, NULL, NULL }
};

static int quiet = 0; /* when true: do not print to stdout */
static int done = 0;  /* when true: exit the main loop */
static controller *ctrl = NULL;

void shell_set_quiet(int q) {
    quiet = q;
}

/* This is the main loop. It is terminated when the user executes the
 * "quit" command */
void shell_run() {
    char *line, *cmd;
    error_code r;
    initialize_readline();
    usr_msg("%s\n%s", WARNING_MSG, GREETING_MSG);
    if ((r = new_controller(&ctrl))) {
        usr_msg("Failed to create audio controller: %s\n",
            error_code_to_string(r));
        exit(-1);
    }
    while ( ! is_controller_ready(ctrl) ) {
        usleep(10*1000);
    }
    while ( ! done ) {
        line = readline(quiet ? NULL : "> ");
        if ( line == NULL ) {
            usr_msg("\n");
            exec_quit(0, NULL);
            break;
        }
        cmd = next_non_whitespace(line);
        if ( *cmd == '\0' ) {
            free(line);
            continue;
        }
        add_history(cmd);
        execute_cmdline(cmd);
        free(line);
    }
    return;
}

/* Execute a command line. */
static int execute_cmdline(char *line) {
    int result = 0;
    int argc = 0;
    char **argv = NULL;
    struct command *command;

    assert ( line != NULL );
    split(line, &argc, &argv);
    assert ( argc >= 1 ); /* argv[0] is the command name */
    command = find_command(argv[0]);
    if (!command) {
        usr_msg("%s: No such command. Type \"help\" for help.\n", argv[0]);
        result = -1;
    }
    else {
        result = (*(command->func))(argc, argv);
    }
    free(argv);
    return result;
}

static char *next_non_whitespace(char *line) {
    assert ( line != NULL );
    while ( isspace(*line) ) {
        line++;
    }
    return line;
}

/* Find a command in the commands[] array */
static struct command *find_command(char *name) {
    int i=0;
    for (i = 0; commands[i].name != NULL; i++) {
        if ( strcmp(name, commands[i].name) == 0 ) {
            return &commands[i];
        }
    }
    return NULL;
}

/* Split the line into tokens.
 *
 * Tokens are separated by spaces, but there is an exception:
 * If a token is put in "double quotes", spaces within these double quotes
 * are not counted as token boundaries.
 *
 * There are two special characters: double quote and backslash. These
 * characters must be escaped with a backslash, like \" and \\.
 *
 * Parameters:
 *  - line:   Command line, as typed by the user. This line will be altered!
 *            split() will strip backslashes from line and put a terminating
 *            '\0' at the end of each token.
 *  - argc_p: The number of tokens will be put here.
 *  - argv_p: split() will put a pointer to the start of each token here.
 *            *argv_p will be created with malloc(). The caller must free() it.
 *            The pointers in *argv_p will point to the start characters
 *            in line.
 */
static void split(char *line, int *argc_p, char **argv_p[]) {
    int argc = 0;
    char **argv = NULL;
    size_t argv_size = 0;
    char *next_token = line; /* pointer to start of next token in line */
    int in_quotes = 0;       /* If in_quotes is true, the current position is
                              * within "..." double quotes -> we do not count
                              * spaces as token boundaries */
    int in_token = 0;        /* true, when the current pos is within a token */
    int i=0;
    for ( i=0; line[i]; i++ ) {
        if ( line[i] == '\"' && ! char_is_quoted(line, i) ) {
            line[i] = '\0';
            in_quotes = !in_quotes;
            continue;
        }
        if ( isspace(line[i]) && in_quotes ) {
            continue;
        }
        if ( isspace(line[i]) && in_token ) {
            line[i] = '\0';
            realloc_argv(argc+1, &argv, &argv_size);
            strip_backslashes_in_place(next_token);
            argv[argc++] = next_token;
            in_token = 0;
            continue;
        }
        if ( ! isspace(line[i]) && ! in_token ) {
            in_token = 1;
            next_token = line+i;
            continue;
        }
    }
    if ( in_token ) {
        realloc_argv(argc+1, &argv, &argv_size);
        argv[argc++] = next_token;
    }
    argv[argc] = NULL;
    *argv_p = argv;
    *argc_p = argc;
}

/* replace \" with " and \\ with \ */
static void strip_backslashes_in_place(char *s) {
    int i=0, j=0;
    while ( s[i] ) {
        if ( s[i] == '\\' ) {
            s[j++] = s[++i];
            if ( s[i++] == '\0' ) {
                return; /* s ended with \\\0 */
            }
        }
        s[j++] = s[i++];
    }
    s[j] = '\0';
}

/* realloc_argv() also works at the first call, when argv_p is NULL and
 * argv_size is 0 */
static void realloc_argv(int argc, char **argv_p[], size_t *argv_size_p) {
    if ( argc + 1 >= *argv_size_p ) { /* +1 for terminating NULL */
        *argv_size_p = *argv_size_p == 0 ? 8 : *argv_size_p * 2;
        *argv_p = realloc(*argv_p, *argv_size_p * sizeof(char *));
        if ( *argv_p == NULL ) {
            fprintf(stderr, "Out of memory.\n");
            exit(-1);
        }
    }
}

/* usr_msg() is like printf(), but it can be disabled by setting quiet */
static void usr_msg(const char *fmt, ...) {
    va_list args;
    if ( ! quiet ) {
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

/******************************************************************************
 * The following is configuration of the GNU Readline library.
 * We enable TAB-completion and we add support for filenames with spaces.
 *
 * See section 2.6.4: "A Short Completion Example" in the GNU Readline docu.
 * http://cnswww.cns.cwru.edu/php/chet/readline/readline.html#SEC44
 *****************************************************************************/

static void initialize_readline() {
    rl_readline_name = "music-box";
    rl_completer_quote_characters = "\"";
    rl_filename_quote_characters = "\"\\";
    rl_filename_quoting_function = quote_filename;
    rl_filename_dequoting_function = dequote_filename;
    rl_char_is_quoted_p = char_is_quoted;
    rl_attempted_completion_function = command_completion;
}

/* Find out if the string s contains the character c */
static int contains_char(const char *s, char c) {
    int i;
    for ( i=0; s[i]; i++ ) {
        if ( c == s[i] ) {
            return 1;
        }
    }
    return 0;
}

/* Find out if the char at text[index] is preceeded by a backslash */
static int char_is_quoted(char *text, int index) {
    if ( index == 0 ) {
        return 0;
    }
    return text[index-1] == '\\';
}

/* Escape special characters with backslash */
static char *quote_filename(char *text, int match_type, char *quote_pointer) {
    int i=0, j=0;
    char *quoted_text = malloc(strlen(text) * 2 + 1);
    if ( quoted_text == NULL ) {
        fprintf(stderr, "Out of memory.");
        exit(-1);
    }
    for ( i=0; text[i]; i++ ) {
        if ( contains_char(rl_filename_quote_characters, text[i]) ) {
            quoted_text[j++] = '\\';
        }
        quoted_text[j++] = text[i];
    }
    quoted_text[j++] = '\0';
    return quoted_text;
}

/* Remove escaping backslashes */
static char *dequote_filename(char *text, int quote_char) {
    int i=0, j=0;
    char *dequoted_text = malloc(strlen(text));
    if ( dequoted_text == NULL ) {
        fprintf(stderr, "Out of memory.");
        exit(-1);
    }
    for ( i=0; text[i]; i++ ) {
        if ( text[i] == '\\' ) {
            i++;
        }
        dequoted_text[j++] = text[i];
    }
    dequoted_text[j] = '\0';
    return dequoted_text;
}

/* TAB-completion: Attempt to complete on the contents of text. */
static char **command_completion(const char *text, int start, int end) {
    char **matches = NULL;
    if (start == 0) { /* beginning of line: complete a music-box command */
        matches = rl_completion_matches(text, command_generator);
    }
    return (matches);
}

/* TAB-completion for our shell commands. Will be called when the user
 * hits <tab> when typing the first word of a command line. */
static char *command_generator(const char *text, int state) {
    static int list_index, len;
    char *name;
    if (!state) { /* this is the first call for a new word to complete */
        list_index = 0;
        len = strlen (text);
    }
    while ((name = commands[list_index].name)) {
        list_index++;
        if ( strncmp (name, text, len) == 0 ) {
            return strdup(name); /* GNU Readline will call free() */
        }
    }
    return NULL;
}

/******************************************************************************
 * Implementation of the music box shell commands
 *****************************************************************************/

static int exec_load(int argc, char **argv) {
    error_code r;
    if ( argc != 4 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        usr_msg("If the filename contains spaces, put the filename in "
            "\"double quotes\".\n");
        return -1;
    }
    if ( controller_var_exists(ctrl, argv[3]) ) {
        usr_msg("Error executing load: var %s is already taken\n", argv[3]);
        return -1;
    }
    if ((r = controller_load(ctrl, argv[3], argv[1]))) {
        usr_msg("Error executing load: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_unload(int argc, char **argv) {
    error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( ( r = controller_unload_var(ctrl, argv[1]) ) ) {
        usr_msg("Error executing unload: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_play(int argc, char **argv) {
    error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( ( r = controller_play(ctrl, argv[1]) ) ) {
        usr_msg("Error executing play: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_pause(int argc, char **argv) {
    error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( ( r = controller_pause(ctrl, argv[1]) ) ) {
        usr_msg("Error executing pause: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_wait(int argc, char **argv) {
    error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( ( r = controller_wait(ctrl, argv[1]) ) ) {
        usr_msg("Error executing wait: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_resume(int argc, char **argv) {
    error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( ( r = controller_resume(ctrl, argv[1]) ) ) {
        usr_msg("Error executing resume: %s\n", error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_sleep(int argc, char **argv) {
    int n_seconds;
    if ( argc != 2 ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    n_seconds = atoi(argv[1]);
    if ( n_seconds <= 0 ) {
        usr_msg("Error executing sleep: %s is not a positive number.\n",
            argv[1]);
        return -1;
    }
    sleep(n_seconds);
    return 0;
}

static int exec_quit(int argc, char **argv) {
    usr_msg("shutting down...\n");
    error_code r = controller_shutdown_and_free(ctrl);
    if ( r != SUCCESS ) {
        usr_msg("shutdown failed: %s\n", error_code_to_string(r));
    }
    done = 1;
    return 0;
}

static int exec_help(int argc, char **argv) {
    int i=0;
    usr_msg("This shell supports the following commands:\n");
    for ( i=0; commands[i].name != NULL; i++ ) {
        if ( commands[i].usage != NULL && commands[i].helpmsg != NULL ) {
            print_indented(commands[i].usage, 2);
            print_indented(commands[i].helpmsg, 4);
        }
    }
    return 0;
}

/* print spaces at the beginning of each new line */
static void print_indented(const char *s, int indent) {
    int i=0, j=0;
    int is_new_line = 1;
    for ( i=0; s[i]; i++ ) {
        if ( is_new_line ) {
            for ( j=0; j<indent; j++ ) {
                usr_msg(" ");
            }
            is_new_line = 0;
        }
        usr_msg("%c", s[i]);
        if ( s[i] == '\n' ) {
            is_new_line = 1;
        }
    }
}
