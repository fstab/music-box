#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "libmbx/common/mbx_errno.h"
#include "libmbx/api.h"

static char *cmd_completion_list(const char *text, int state);
static char *cmd_completion_set(const char *text, int state);

static int exec_config_list_devices(int argc, char **argv);
static int exec_config_set(int argc, char **argv);
static int exec_config_show(int argc, char **argv);
static int exec_config_load_file(int argc, char **argv);
static int exec_run(int argc, char **argv);

static int exec_load(int argc, char **argv);
static int exec_play(int argc, char **argv);
static int exec_pause(int argc, char **argv);
static int exec_sleep(int argc, char **argv);
static int exec_quit(int argc, char **argv);
static int exec_help(int argc, char **argv);

static char get_deck(int argc, char **argv);
static int get_sample_num(int argc, char **argv);

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


/* Struct for defining a command for this shell */
struct command {
    /* command name, like "load", "play", "stop" */
    char *name;
    /* function implementing the command */
    int (*func)(int argc, char **argv);
    /* generator function, in case the command has parameters, and wants
     * to support readline's <TAB> completion. */
    char *(*cmdgen)(const char *text, int state);
    /* usage message, can be NULL */
    const char *usage;
    /* help message, can be NULL */
    const char *helpmsg;
};

static struct command config_commands[] = {
    { "list",
      exec_config_list_devices,
      cmd_completion_list,
      "list devices",
      "List available output devices" },
    { "set",
      exec_config_set,
      cmd_completion_set,
      "set <var> <value>",
      "Set a configuration variable, overwriting current settings.\n"
      "The following variables are available:\n"
      "set headphones <device>\n"
      "set speakers <device>\n"
      "set mp3dir <path>\n"},
    { "show",
      exec_config_show,
      NULL,
      "show",
      "Print current configuration." },
    { "load-config-file",
      exec_config_load_file,
      NULL, "load-config-file <file>",
      "Load a config file." },
    { "run",
      exec_run,
      NULL,
      "run",
      "Exit configuration mode and start the music box." },
    { "quit",
      exec_quit,
      NULL,
      "quit",
      "Quit this application" },
    { "exit", exec_quit, NULL, NULL, NULL },
    { "bye", exec_quit, NULL, NULL, NULL },
    { "help", exec_help, NULL, NULL, NULL },
    { "?", exec_help, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

/* Array containing all commands supported by this shell.
 * This array is terminated with a NULL command */
static struct command run_commands[] = {
    { "load", exec_load, NULL, "load <file.mp3> on deck [a|b]\nload <file.mp3> as sample <n>\n",
      "Load an mp3 file and store it in variable <var>\n"
      "For filenames with spaces, type the filename in \"double quotes\".\n"},
    { "play", exec_play, NULL, "play deck [a|b]\nplay sample <n>\n",
      "Start playing the file loaded as <var>\n" },
    { "pause", exec_pause, NULL, "pause deck [a|b]\n",
       "Pause the file loaded as <var>\n" },
    { "sleep", exec_sleep, NULL, "sleep <seconds>\n",
      "sleep for <seconds> seconds\n" },
    { "quit", exec_quit, NULL, "quit\n",
      "quit this application\n" },
    { "exit", exec_quit, NULL, NULL, NULL },
    { "bye", exec_quit, NULL, NULL, NULL },
    { "help", exec_help, NULL, NULL, NULL },
    { "?", exec_help, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static struct command *commands;
static int quiet = 0; /* when true: do not print to stdout */
static int done = 0;  /* when true: exit the main loop */
static mbx_config cfg;
static mbx_ctrl ctrl;

void shell_set_quiet(int q) {
    quiet = q;
}

static void enter_config_mode() {
    commands = config_commands;
    usr_msg("You are now in config mode. Type \"help\" for a list of "
        "configuration commands.\n");
}

/* This is the main loop. It is terminated when the user executes the
 * "quit" command */
void shell_run() {
    char *line, *cmd;
    mbx_error_code r = mbx_config_new(&cfg);
    if ( r != MBX_SUCCESS ) {
        usr_msg("Failed to initialize music box: %s", mbx_error_code_to_string(r));
        exit(-1);
    }
    initialize_readline();
    enter_config_mode();
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
    size_t i=0;
    // fprintf(stderr, "\nstart: %d, end: %d, text: %s, rl_line_buffer: %s\n", start, end, text, rl_line_buffer);
    if (start == 0) { /* beginning of line: complete a music-box command */
        matches = rl_completion_matches(text, command_generator);
    }
    /* start > 0:
     * The first few characters have already been read to rl_line_buffer.
     * Try to perform command speciffic completion */
    for ( i=0; commands[i].name != NULL; i++ ) {
        const char *cmd = commands[i].name;
        if ( commands[i].cmdgen != NULL ) {
            if ( start >= strlen(cmd) + 1) { /* strlen + space */
                if ( ! strncmp(cmd, rl_line_buffer, strlen(cmd)) ) {
                    matches = rl_completion_matches(text, commands[i].cmdgen);
                }
            }
        }
    }
    return matches;
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

static char *cmd_completion_list(const char *text, int state) {
    if ( ! state ) { /* first call */
        return strdup("devices");
    }
    return NULL;
}

static char *cmd_completion_config_vars(const char *text, int state) {
    static size_t i, len;
    char *vars[] = { "headphones", "speakers", "mp3dir", NULL };
    char *var;
    if ( ! state ) { /* first call */
        i = 0;
        len = strlen(text);
    }
    while ( (var = vars[i++]) != NULL ) {
        if ( strncmp(var, text, len) == 0 ) {
            return strdup(var); /* GNU Readline will call free() */
        }
    }
    return NULL;
}


static char *cmd_completion_output_device(const char *text, int state) {
    static size_t i, len, n_devices;
    static char **device_names;
    char *dev_name;
    mbx_error_code r;
    if ( ! state ) { /* first call */
        i = 0;
        len = strlen(text);
        r = mbx_create_output_device_name_list(&device_names, &n_devices);
        if ( r != MBX_SUCCESS ) {
            return NULL;
        }
    }
    while ( (dev_name = device_names[i++]) != NULL ) {
        if ( strncmp(dev_name, text, len) == 0 ) {
            return strdup(dev_name); /* GNU Readline will call free() */
        }
    }
    /* last call */
    mbx_free_output_device_name_list(device_names);
    return NULL;
}

static char *cmd_completion_set(const char *text, int state) {
    if ( strstr(rl_line_buffer, "mp3dir") ) {
        return rl_filename_completion_function(text, state);
    }
    if ( strstr(rl_line_buffer, "headphones") || strstr(rl_line_buffer, "speakers") ) {
        return cmd_completion_output_device(text, state);
    }
    return cmd_completion_config_vars(text, state);
}

/**
 * config commands
 */

static int exec_config_list_devices(int argc, char **argv) {
    char **device_names;
    size_t n_devices;
    mbx_error_code r;
    int i;
    if ( argc != 2 || strcmp("devices", argv[1]) ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    r = mbx_create_output_device_name_list(&device_names, &n_devices);
    if ( r != MBX_SUCCESS ) {
        usr_msg("Error executing list: %s\n", mbx_error_code_to_string(r));
        return -1;
    }
    usr_msg("Found %zu device%s:\n", n_devices, n_devices != 1 ? "s" : "");
    for ( i=0; i<n_devices; i++ ) {
        usr_msg("%02d: %s\n", i+1, device_names[i]);
    }
    mbx_free_output_device_name_list(device_names);
    return 0;
}

static int exec_config_set(int argc, char **argv) {
    if ( argc != 3 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    if ( ! strcmp("headphones", argv[1]) ) {
        mbx_config_set(cfg, MBX_CFG_HEADPHONES_DEVICE, argv[2]);
    }
    else if ( ! strcmp("speakers", argv[1]) ) {
        mbx_config_set(cfg, MBX_CFG_SPEAKERS_DEVICE, argv[2]);
    }
    else if ( ! strcmp("mp3dir", argv[1]) ) {
        mbx_config_set(cfg, MBX_CFG_MP3DIR, argv[2]);
    }
    else {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    return 0;
}

static int print_config(mbx_config_var var, const char *name) {
    const char *val = mbx_config_get(cfg, var);
    printf("%s: %s\n", name, val == NULL ? "not set" : val);
    return 0;
}

static int exec_config_show(int argc, char **argv) {
    if ( argc != 1 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    print_config(MBX_CFG_HEADPHONES_DEVICE, "headphones");
    print_config(MBX_CFG_SPEAKERS_DEVICE, "speakers");
    print_config(MBX_CFG_MP3DIR, "mp3dir");
    return 0;
}

static int exec_config_load_file(int argc, char **argv) {
    mbx_error_code r;
    if ( argc != 2 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    r = mbx_config_load_file(cfg, argv[1]);
    if ( r != MBX_SUCCESS ) {
        usr_msg("Failed to load  %s: %s\n", argv[1], mbx_error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_run(int argc, char **argv) {
    mbx_error_code r;
    if ( argc != 1 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    if ( (r = mbx_ctrl_new(&ctrl, cfg)) != MBX_SUCCESS ) {
        usr_msg("Failed to initialize music box: %s\n", mbx_error_code_to_string(r));
        return -1;
    }
    commands = run_commands;
    usr_msg("You are now in \"run\" mode. Type \"help\" for a list of "
        "commands.");
    return 0;
}

/******************************************************************************
 * Implementation of the music box shell commands
 *****************************************************************************/

static int exec_load(int argc, char **argv) {
    mbx_error_code r;
    if ( argc != 5 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        usr_msg("If the filename contains spaces, put the filename in "
            "\"double quotes\".\n");
        return -1;
    }
    if ( ! strcmp("deck", argv[3]) ) {
        switch ( get_deck(argc, argv) ) {
            case 'a':
                r = mbx_ctrl_deck_a_load(ctrl, argv[1]);
                break;
            case 'b':
                r = mbx_ctrl_deck_b_load(ctrl, argv[1]);
                break;
            default:
                usr_msg("Usage: %s", find_command(argv[0])->usage);
                return -1;
        }
    }
    else if ( ! strcmp("sample", argv[3]) ) {
        int n = get_sample_num(argc, argv);
        if ( n <= 0 || n > MAX_SAMPLE_FILES ) {
            usr_msg("<n> must be between 1 and %d\n", MAX_SAMPLE_FILES);
            return -1;
        }
        r = mbx_ctrl_sample_load(ctrl, argv[1], n-1);
    }
    else {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    if ( r != MBX_SUCCESS ) {
        usr_msg("Error executing load: %s\n", mbx_error_code_to_string(r));
        return -1;
    }
    return 0;
}

static int exec_play(int argc, char **argv) {
    if ( argc != 3 ) {
        usr_msg("Usage:\n%s\n", find_command(argv[0])->usage);
        return -1;
    }
    if ( ! strcmp("deck", argv[1]) ) {
        switch ( get_deck(argc, argv) ) {
            case 'a':
                mbx_ctrl_deck_a_play(ctrl);
                break;
            case 'b':
                mbx_ctrl_deck_b_play(ctrl);
                break;
            default:
                usr_msg("Usage: %s", find_command(argv[0])->usage);
                return -1;
        }
    }
    else if ( ! strcmp("sample", argv[1]) ) {
        int n = get_sample_num(argc, argv);
        if ( n <= 0 || n > MAX_SAMPLE_FILES ) {
            usr_msg("<n> must be between 1 and %d\n", MAX_SAMPLE_FILES);
            return -1;
        }
        mbx_ctrl_sample_play(ctrl, n-1);
    }
    else {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    return 0;
}

/*
 * Der Befehl endet mit "deck a" oder "deck b"
 * Es wird 'a' oder 'b' oder '\0' zurueckgeliefert.
 */
static char get_deck(int argc, char **argv) {
    if ( argc < 2 ) {
        return '\0';
    }
    if ( strcmp("deck", argv[argc-2]) ) {
        return '\0';
    }
    if ( ! strcmp("a", argv[argc-1]) || ! strcmp("b", argv[argc-1]) ) {
        return *argv[argc-1];
    }
    return '\0';
}

/*
 * Der Befehl endet mit "sample <n>"
 */
static int get_sample_num(int argc, char **argv) {
    char *endp;
    if ( argc < 2 ) {
        return -1;
    }
    if ( strcmp("sample", argv[argc-2]) ) {
        return -1;
    }
    long result = strtol(argv[argc-1], &endp, 10);
    if ( *argv[argc-1] == '\0' || *endp != '\0' ) {
        return -1;
    }
    return (int) result;
}


static int exec_pause(int argc, char **argv) {
    char deck = get_deck(argc, argv);
    if ( argc != 3 || ! deck ) {
        usr_msg("Usage: %s", find_command(argv[0])->usage);
        return -1;
    }
    deck == 'a' ? mbx_ctrl_deck_a_pause(ctrl) : mbx_ctrl_deck_b_pause(ctrl);
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
    mbx_ctrl_shutdown_and_free(ctrl);
    done = 1;
    return 0;
}

static int exec_help(int argc, char **argv) {
    int i=0;
    if ( commands == config_commands ) {
        usr_msg("You are now in config mode. ");
    }
    else {
        usr_msg("You have sucessfully started the music box. ");
    }
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
    if ( ! is_new_line ) {
        usr_msg("\n");
    }
}
