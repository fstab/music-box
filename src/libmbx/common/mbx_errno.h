#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/**
 * Enumeration of error codes.
 *
 * Rule for users of the music box library: When a function returns an error
 * code, there is no need to write a log message. Logging takes place in the
 * function that generates the error code. It is unnecessary to have an
 * additional log message after the error code was returned.
 */
typedef enum mbx_error_code {

    /**
     * No error.
     */
    MBX_SUCCESS = 0,

    /**
     * Out of memory.
     */
    MBX_OUT_OF_MEMORY,

    /**
     * Failed to open the configuration file.
     */
    MBX_CONFIG_FILE_OPEN_FAILED,

    /**
     * Syntax error in config file.
     */
    MBX_CONFIG_FILE_SYNTAX_ERROR,

    /**
     * There was an error communicating with the PulseAudio server.
     */
    MBX_PULSEAUDIO_ERROR,

    /**
     * Invalid output device name.
     */
    MBX_DEVICE_DOES_NOT_EXIST,

    /**
     * Failed to load an MP3 file. This happens either if the file cannot be
     * read, or if the MP3 data cannot be decoded.
     */
    MBX_FAILED_TO_LOAD_MP3

} mbx_error_code;

/**
 * Return a string describing the error code.
 *
 * @param  error_code
 *         An error code returned by a libmbx function.
 * @return A string describing the error code.
 */
extern const char *mbx_error_code_to_string(mbx_error_code error_code);

#endif
