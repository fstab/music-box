#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include "audio_producer.h"
#include "mad_decoder.h"
#include "log.h"

static error_code write_next_sample(audio_producer *,short *,size_t,short **);
static void *do_load_mp3(void *);

/******************************************************************************
 * For comments on extern functions, see audio_producer.h
 *****************************************************************************/

error_code new_audio_producer(audio_producer **prod_p) {
    *prod_p = malloc(sizeof(audio_producer));
    if ( *prod_p == NULL ) {
        log_error(PRODUCER, "Failed to allocate memory for new audio_producer:"
            " %s", strerror(errno));
        return PRODUCER_OUT_OF_MEMORY;
    }
    (*prod_p)->sample_data = NULL;
    (*prod_p)->current_pos_speaker = NULL;
    (*prod_p)->current_pos_headphone = NULL;
    (*prod_p)->end_pos = NULL;
    (*prod_p)->filename = NULL;
    (*prod_p)->state = PRODUCER_NOT_INITIALIZED;
    return SUCCESS;
}

error_code start_loading_mp3(audio_producer *prod, const char *filename) {
    int r;
    assert ( prod->state == PRODUCER_NOT_INITIALIZED );
    prod->state = PRODUCER_LOADING_MP3;
    /* test if file can be opened */
    FILE *file;
    if ( (file = fopen(filename, "r")) == NULL ) {
        log_error(PRODUCER, "%s: %s", filename, strerror(errno));
        prod->state = PRODUCER_FAILED_TO_LOAD_MP3;
        return PRODUCER_FAILED_TO_READ_MP3_FILE;
    }
    fclose(file);
    /* test ok -> start the background thread */
    prod->filename = strdup(filename);
    pthread_t thread;
    r = pthread_create(&thread, NULL, do_load_mp3, (void *) prod);
    if ( r != 0 ) {
        log_fatal(PRODUCER, "Failed to start mp3-loading thread: %s",
            strerror(r));
        return PRODUCER_FAILED_TO_START_BACKGROUND_THREAD;
    }
    return SUCCESS;
}

/* do_load_mp3() is called in a background thread. It reads and decodes the
 * mp3 and sets the audio_producer state when it's done.
 * The return value is only to comply with the signature required by
 * pthread_create(), we always return NULL */
static void *do_load_mp3(void *userarg) {
    audio_producer *prod = (audio_producer *) userarg;
    signed short *data;
    size_t length;
    FILE *file;
    error_code r;
    assert ( prod != NULL && prod->filename != NULL );
    if ( ( file = fopen(prod->filename, "r") ) == NULL ) {
        prod->state = PRODUCER_FAILED_TO_LOAD_MP3;
        return NULL;
    }
    if ( ( r = mad_decode(file, &data, &length) ) ) {
        prod->state = PRODUCER_FAILED_TO_LOAD_MP3;
        fclose(file);
        return NULL;
    }
    fclose(file);
    prod->sample_data = data;
    prod->end_pos = data + length;
    prod->current_pos_speaker = data;
    prod->current_pos_headphone = data;
    prod->state = PRODUCER_READY;
    return NULL;
}

int producer_state_is_playing(audio_producer *prod) {
    return prod->state == PRODUCER_PLAYING;
}

int producer_state_is_ready(audio_producer *prod) {
    return prod->state == PRODUCER_READY;
}

int producer_state_is_failed_to_load_mp3(audio_producer *prod) {
    return prod->state == PRODUCER_FAILED_TO_LOAD_MP3;
}

int producer_state_is_loading_mp3(audio_producer *prod) {
    return prod->state == PRODUCER_LOADING_MP3;
}

int producer_state_is_end_of_mp3(audio_producer *prod) {
    return prod->state == PRODUCER_END_OF_MP3;
}

error_code free_audio_producer(audio_producer *prod) {
    while ( prod->state == PRODUCER_LOADING_MP3 ) {
        /* we cannot free the producer while the background thread is still
         * decoding the file */
        usleep(5*1000);
    }
    free((void *) prod->filename);
    free((void *) prod->sample_data);
    free(prod);
    return SUCCESS;
}

error_code start_playing(audio_producer *prod) {
    if ( prod->state != PRODUCER_READY ) {
        log_debug(PRODUCER, "play called, but producer is not READY "
            "(current state is %s).", producer_state_to_string(prod));
        return PRODUCER_NOT_READY;
    }
    prod->state = PRODUCER_PLAYING;
    return SUCCESS;
}

error_code producer_pause(audio_producer *prod) {
    if ( prod->state != PRODUCER_PLAYING ) {
        log_debug(PRODUCER, "pause called, but producer is not PLAYING "
            "(current state is %s).", producer_state_to_string(prod));
        return PRODUCER_NOT_PLAYING;
    }
    prod->state = PRODUCER_READY;
    return SUCCESS;
}

error_code write_next_sample_to_headphones(audio_producer *prod,
        signed short *output, size_t length) {
    return write_next_sample(prod, output, length, &(prod->current_pos_headphone));
}

error_code write_next_sample_to_speakers(audio_producer *prod,
        signed short *output, size_t length) {
    return write_next_sample(prod, output, length, &(prod->current_pos_speaker));
}

/* This is called by the controller to fetch the next audio samples.
 * The current implementation just adds the samples to the output buffer.
 * TODO: This does not take volume into account. Implement a more elaborate
 * mixing algorithm. */
static error_code write_next_sample(audio_producer *prod, signed short *output,
        size_t length, signed short **cur_pos) {
    size_t i;
    if ( prod->state != PRODUCER_PLAYING ) {
        return SUCCESS; // do nothing -> play silence
    }
    for ( i=0; i<length; i++ ) {
        if ( *cur_pos >= prod->end_pos ) {
            log_debug(PRODUCER, "Reached end of file. Stopping.");
            prod->state = PRODUCER_END_OF_MP3;
            return SUCCESS;
        }
        output[i] += **cur_pos;
        *cur_pos = *cur_pos + 1;
    }
    return SUCCESS;
}

const char *producer_state_to_string(audio_producer *prod) {
    switch ( prod->state ) {
        case PRODUCER_NOT_INITIALIZED:
            return "PRODUCER_NOT_INITIALIZED";
        case PRODUCER_FAILED_TO_LOAD_MP3:
            return "PRODUCER_FAILED_TO_LOAD_MP3";
        case PRODUCER_READY:
            return "PRODUCER_READY";
        case PRODUCER_PLAYING:
            return "PRODUCER_PLAYING";
        case PRODUCER_END_OF_MP3:
            return "PRODUCER_END_OF_MP3";
        case PRODUCER_LOADING_MP3:
            return "PRODUCER_LOADING_MP3";
    }
    return "UNKNOWN";
}
