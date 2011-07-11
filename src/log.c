#include "log.h"
#include <stdio.h>
#include <stdarg.h>

enum log_level {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5,
    DISABLED = 100
};

static enum log_level log_level = DEFAULT_LOG_LEVEL;
static FILE *logfile = NULL; /* if NULL, log messages will go to stderr */

static const char *messenger_to_string(enum messenger);
static const char *log_level_to_string(enum log_level);
static void do_log(enum log_level, enum messenger, const char *fmt, va_list ap);

void log_debug(enum messenger messenger, const char *fmt, ...) {
    va_list args;
    if ( log_level <= DEBUG ) {
        va_start(args, fmt);
        do_log(DEBUG, messenger, fmt, args);
        va_end(args);
    }
}

void log_info(enum messenger messenger, const char *fmt, ...) {
    va_list args;
    if ( log_level <= INFO ) {
        va_start(args, fmt);
        do_log(INFO, messenger, fmt, args);
        va_end(args);
    }
}

void log_warn(enum messenger messenger, const char *fmt, ...) {
    va_list args;
    if ( log_level <= WARN ) {
        va_start(args, fmt);
        do_log(WARN, messenger, fmt, args);
        va_end(args);
    }
}

void log_error(enum messenger messenger, const char *fmt, ...) {
    va_list args;
    if ( log_level <= ERROR ) {
        va_start(args, fmt);
        do_log(ERROR, messenger, fmt, args);
        va_end(args);
    }
}

void log_fatal(enum messenger messenger, const char *fmt, ...) {
    va_list args;
    if ( log_level <= FATAL ) {
        va_start(args, fmt);
        do_log(FATAL, messenger, fmt, args);
        va_end(args);
    }
}

void set_log_level_debug() {
    log_level = DEBUG;
}

void set_log_level_info() {
    log_level = INFO;
}

void set_log_level_warn() {
    log_level = WARN;
}

void set_log_level_error() {
    log_level = ERROR;
}

void set_log_level_fatal() {
    log_level = FATAL;
}

void disable_logging() {
    log_level = DISABLED;
}

void set_logfile(FILE *file) {
    logfile = file;
}

static void do_log(enum log_level level, enum messenger messenger, const char *format, va_list ap) {
    FILE *out = logfile == NULL ? stderr : logfile;
    fprintf(out, "%s: %s: ", log_level_to_string(level),
        messenger_to_string(messenger));
    vfprintf(out, format, ap);
    fprintf(out, "\n");
    fflush(out);
}

static const char *log_level_to_string(enum log_level level) {
    switch(level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARN:
            return "WARN";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        default:
            return "???";
    }
}

static const char *messenger_to_string(enum messenger messenger) {
    switch ( messenger ) {
        case MP3_DECODER:
            return "MP3 decoder";
        case CONSUMER:
            return "Consumer";
        case PRODUCER:
            return "Producer";
        case CONTROLLER:
            return "Controller";
        default:
            return "???";
    }
}
