#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/* TODO: Add documentation for each error_code, and document which error_code
 * can be returned by which function */

/* Logging takes place when a function creates an error code, there is no
 * need for additional log lines when the error code is handled. */

typedef enum error_code {
    SUCCESS = 0,
    CONSUMER_OUT_OF_MEMORY,
    CONSUMER_PA_ERROR,
    CONSUMER_REACHED_MAX_NUMBER_OF_PRODUCERS,
    CONSUMER_DOES_NOT_KNOW_PRODUCER,
    PRODUCER_OUT_OF_MEMORY,
    PRODUCER_FAILED_TO_START_BACKGROUND_THREAD,
    PRODUCER_FAILED_TO_READ_MP3_FILE,
    PRODUCER_NOT_READY,
    PRODUCER_NOT_PLAYING,
    CONTROLLER_OUT_OF_MEMORY,
    CONTROLLER_REACHED_MAX_NUMBER_OF_PRODUCERS,
    CONTROLLER_VARNAME_IS_ALREADY_TAKEN,
    CONTROLLER_VAR_NOT_FOUND,
    CONTROLLER_ILLEGAL_CONSUMER_STATE,
    CONTROLLER_LOAD_MP3_FAILED,
} error_code;

extern const char *error_code_to_string(enum error_code);

#endif
