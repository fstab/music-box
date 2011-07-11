#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/******************************************************************************
 * This is a very simple logging library.
 *****************************************************************************/

#define DEFAULT_LOG_LEVEL DEBUG

enum messenger {
    MP3_DECODER,
    PRODUCER,
    CONSUMER,
    CONTROLLER
} messenger;

/* By default, log messages are written to stderr. set_logfile() can be called
 * to log into a file instead */
extern void set_logfile(FILE *);

/* GNU's __attribute__ allows us to get printf()-style compiler warnings.
 * For example:
 *     log_warn(CONSUMER, "Number one %d and two %d", 1);
 * would yield a compiler warning: "too few arguments for format".
 * For more on this, see http://www.unixwiz.net/techtips/gnu-c-attributes.html
 */

/* If we're not using GNU C, elide __attribute__ */

#ifndef __GNUC__
#  define  __attribute__(x) /*NOTHING*/
#endif

extern void log_debug(enum messenger, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern void log_info(enum messenger, const char *fmt,  ...) __attribute__ ((format (printf, 2, 3)));
extern void log_warn(enum messenger, const char *fmt,  ...) __attribute__ ((format (printf, 2, 3)));
extern void log_error(enum messenger, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern void log_fatal(enum messenger, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

/******************************************************************************
 * functions for setting the log level.
 *****************************************************************************/

extern void set_log_level_debug();
extern void set_log_level_info();
extern void set_log_level_warn();
extern void set_log_level_error();
extern void set_log_level_fatal();
extern void disable_logging();

#endif
