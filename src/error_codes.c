#include "error_codes.h"

const char *error_code_to_string(enum error_code err) {
    switch ( err ) {
        case SUCCESS:
            return "success";
        case CONSUMER_OUT_OF_MEMORY:
            return "out of memory";
        case CONSUMER_PA_ERROR:
            return "error communicating with pulseaudio";
        case CONSUMER_REACHED_MAX_NUMBER_OF_PRODUCERS:
            return "reached maximum number of simultaneous sound files";
        case CONSUMER_DOES_NOT_KNOW_PRODUCER:
            return "sound file not loaded";
        case PRODUCER_OUT_OF_MEMORY:
            return "out of memory";
        case PRODUCER_FAILED_TO_START_BACKGROUND_THREAD:
            return "failed to start background thread for decoding mp3";
        case PRODUCER_FAILED_TO_READ_MP3_FILE:
            return "failed to read mp3 file";
        case PRODUCER_NOT_READY:
            return "sound file is not ready";
        case PRODUCER_NOT_PLAYING:
            return "sound file is not playing";
        case CONTROLLER_REACHED_MAX_NUMBER_OF_PRODUCERS:
            return "reached maximum number of simultaneous sound files";
        case CONTROLLER_VARNAME_IS_ALREADY_TAKEN:
            return "var name is already taken";
        case CONTROLLER_VAR_NOT_FOUND:
            return "var not found";
        case CONTROLLER_ILLEGAL_CONSUMER_STATE:
            return "illegal state of audio player";
        case CONTROLLER_LOAD_MP3_FAILED:
            return "failed to load mp3";
        case CONTROLLER_OUT_OF_MEMORY:
            return "out of memory";
        default:
            return "unknown error";
    }
}
