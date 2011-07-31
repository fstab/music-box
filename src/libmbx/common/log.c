#include "log.h"
#include <stdio.h>
#include <stdarg.h>

#define _MBX_DEFAULT_LOGLEVEL _MBX_LOGLEVEL_DEBUG

enum log_level {
    _MBX_LOGLEVEL_DEBUG = 1,
    _MBX_LOGLEVEL_INFO  = 2,
    _MBX_LOGLEVEL_WARN  = 3,
    _MBX_LOGLEVEL_ERROR = 4,
    _MBX_LOGLEVEL_FATAL = 5,
    _MBX_LOGGING_DISABLED = 100
};

static enum log_level log_level = _MBX_DEFAULT_LOGLEVEL;

static FILE *logfile = NULL; /* if NULL, log messages will go to stderr */

static const char *component_to_string(enum _mbx_component);
static const char *log_level_to_string(enum log_level);
static void do_log(enum log_level, enum _mbx_component, const char *fmt,
        va_list ap);

void mbx_log_debug(enum _mbx_component component, const char *fmt, ...) {
    va_list args;
    if ( log_level <= _MBX_LOGLEVEL_DEBUG ) {
        va_start(args, fmt);
        do_log(_MBX_LOGLEVEL_DEBUG, component, fmt, args);
        va_end(args);
    }
}

void mbx_log_info(enum _mbx_component component, const char *fmt, ...) {
    va_list args;
    if ( log_level <= _MBX_LOGLEVEL_INFO ) {
        va_start(args, fmt);
        do_log(_MBX_LOGLEVEL_INFO, component, fmt, args);
        va_end(args);
    }
}

void mbx_log_warn(enum _mbx_component component, const char *fmt, ...) {
    va_list args;
    if ( log_level <= _MBX_LOGLEVEL_WARN ) {
        va_start(args, fmt);
        do_log(_MBX_LOGLEVEL_WARN, component, fmt, args);
        va_end(args);
    }
}

void mbx_log_error(enum _mbx_component component, const char *fmt, ...) {
    va_list args;
    if ( log_level <= _MBX_LOGLEVEL_ERROR ) {
        va_start(args, fmt);
        do_log(_MBX_LOGLEVEL_ERROR, component, fmt, args);
        va_end(args);
    }
}

void mbx_log_fatal(enum _mbx_component component, const char *fmt, ...) {
    va_list args;
    if ( log_level <= _MBX_LOGLEVEL_FATAL ) {
        va_start(args, fmt);
        do_log(_MBX_LOGLEVEL_FATAL, component, fmt, args);
        va_end(args);
    }
}

void mbx_log_level_debug() {
    log_level = _MBX_LOGLEVEL_DEBUG;
}

void mbx_log_level_info() {
    log_level = _MBX_LOGLEVEL_INFO;
}

void mbx_log_level_warn() {
    log_level = _MBX_LOGLEVEL_WARN;
}

void mbx_log_level_error() {
    log_level = _MBX_LOGLEVEL_ERROR;
}

void mbx_log_level_fatal() {
    log_level = _MBX_LOGLEVEL_FATAL;
}

void mbx_log_disable() {
    log_level = _MBX_LOGGING_DISABLED;
}

void mbx_log_file(FILE *file) {
    logfile = file;
}

static void do_log(enum log_level level, enum _mbx_component component,
        const char *format, va_list ap) {
    FILE *out = logfile == NULL ? stderr : logfile;
    fprintf(out, "%s: %s: ", log_level_to_string(level),
        component_to_string(component));
    vfprintf(out, format, ap);
    fprintf(out, "\n");
    fflush(out);
}

static const char *log_level_to_string(enum log_level level) {
    switch(level) {
        case _MBX_LOGLEVEL_DEBUG:
            return "DEBUG";
        case _MBX_LOGLEVEL_INFO:
            return "INFO";
        case _MBX_LOGLEVEL_WARN:
            return "WARN";
        case _MBX_LOGLEVEL_ERROR:
            return "ERROR";
        case _MBX_LOGLEVEL_FATAL:
            return "FATAL";
        default:
            return "???";
    }
}

static const char *component_to_string(enum _mbx_component component) {
    switch ( component ) {
        case MBX_LOG_MP3LIB:
            return "MP3 handler";
        case MBX_LOG_CONTROLLER:
            return "Controller";
        case MBX_LOG_AUDIO_OUTPUT:
            return "Audio output";
        case MBX_LOG_XMALLOC:
            return "Memory management";
        case MBX_LOG_CONFIG:
            return "Config";
        default:
            return "???";
    }
}
