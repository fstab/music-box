#include "mbx_errno.h"

const char *mbx_error_code_to_string(enum mbx_error_code err) {
    switch ( err ) {
        case MBX_SUCCESS:
            return "success";
        case MBX_OUT_OF_MEMORY:
            return "out of memory";
        case MBX_CONFIG_FILE_OPEN_FAILED:
            return "failed to open the configuration file";
        case MBX_CONFIG_FILE_SYNTAX_ERROR:
            return "syntax error in configuration file";
        case MBX_PULSEAUDIO_ERROR:
            return "error communicating with pulseaudio";
        case MBX_DEVICE_DOES_NOT_EXIST:
            return "invalid device name for audio output";
        case MBX_FAILED_TO_LOAD_MP3:
            return "failed to load MP3 file";
        default:
            return "unknown error";
    }
}
