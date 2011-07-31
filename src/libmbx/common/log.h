#ifndef MBX_LOG_H
#define MBX_LOG_H

#include <stdio.h>

/*! \file log.h
 *  \brief This is a simple logging library.
 *  
 * For notes on logging error codes, see mbx_errno.h
 */

/**
 * The component writing the log message.
 */
enum _mbx_component {
    /** The MP3 handling code */
    MBX_LOG_MP3LIB,
    /** The PulseAudio output */
    MBX_LOG_AUDIO_OUTPUT,
    /** The xmalloc helper for memory management */
    MBX_LOG_XMALLOC,
    /** The config file reader */
    MBX_LOG_CONFIG,
    /** The controller */
    MBX_LOG_CONTROLLER
};

/**
 * Set the logfile.
 *
 * By default, log messages are written to stderr. If mbx_log_file() is called,
 * log messages are written into a file instead.
 *
 * @param  file
 *         The logfile to be used.
 */
extern void mbx_log_file(FILE *file);

/* GNU's __attribute__ allows us to get printf()-style compiler warnings.
 * If we're not using GNU C, elide __attribute__ */

#ifndef __GNUC__
#  define  __attribute__(x) /*NOTHING*/
#endif

/**
 * Write a debug message.
 *
 * @param  component
 *         The component writing the debug message.
 * @param  fmt
 *         format string and subsequent parameters, as in printf()
 */
extern void mbx_log_debug(enum _mbx_component component, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

/**
 * Write an info message.
 *
 * @param  component
 *         The component writing the info message.
 * @param  fmt
 *         format string and subsequent parameters, as in printf()
 */
extern void mbx_log_info(enum _mbx_component component, const char *fmt,  ...)
    __attribute__ ((format (printf, 2, 3)));

/**
 * Write a warn message.
 *
 * @param  component
 *         The component writing the warn message.
 * @param  fmt
 *         format string and subsequent parameters, as in printf()
 */
extern void mbx_log_warn(enum _mbx_component component, const char *fmt,  ...)
    __attribute__ ((format (printf, 2, 3)));

/**
 * Write an error message.
 *
 * @param  component
 *         The component writing the error message.
 * @param  fmt
 *         format string and subsequent parameters, as in printf()
 */
extern void mbx_log_error(enum _mbx_component component, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

/**
 * Write a fatal message.
 *
 * Fatal errors are unrecoverable. The component detecting the fatal error
 * should call mbx_log_fatal(), and then terminate the application by calling
 * exit().
 *
 * @param  component
 *         The component writing the fatal message.
 * @param  fmt
 *         format string and subsequent parameters, as in printf()
 */
extern void mbx_log_fatal(enum _mbx_component component, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));


/**
 * Set the log level to DEBUG.
 */
extern void mbx_log_level_debug();

/**
 * Set the log level to INFO.
 */
extern void mbx_log_level_info();

/**
 * Set the log level to WARN.
 */
extern void mbx_log_level_warn();

/**
 * Set the log level to ERROR.
 */
extern void mbx_log_level_error();

/**
 * Set the log level to FATAL.
 */
extern void mbx_log_level_fatal();

/**
 * Disable logging.
 */
extern void mbx_log_disable();

#endif
