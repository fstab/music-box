#ifndef AUDIO_PRODUCER_H
#define AUDIO_PRODUCER_H

#include <stdio.h>
#include <stdlib.h>
#include "error_codes.h"

/******************************************************************************
 * An audio_producer represents an mp3 file.
 * All audio_producers are managed by the controller.
 *****************************************************************************/

typedef enum producer_state {
    PRODUCER_NOT_INITIALIZED,
    PRODUCER_LOADING_MP3,
    PRODUCER_FAILED_TO_LOAD_MP3,
    PRODUCER_READY,
    PRODUCER_PLAYING,
    PRODUCER_END_OF_MP3
} producer_state;

typedef struct audio_producer {
    enum producer_state state;
    signed short *sample_data;
    signed short *current_pos_speaker;
    signed short *current_pos_headphone;
    signed short *end_pos;
    const char *filename;
} audio_producer;

/* Create a new audio_producer. A reference to the newly created producer
 * is put in *prod. The producer must be freed using free_audio_producer()
 * when it is no longer needed. */
extern error_code new_audio_producer(audio_producer **prod);

/* Start reading and decoding an mp3 file. This function returns immediately.
 * The reading and decoding are done asynchronously in a background thread.
 * Use producer_state_is_loading_mp3() to find out when the file is
 * completely loaded */
extern error_code start_loading_mp3(audio_producer *prod, const char *filename);

/* free() the memory used by this audio producer */
extern error_code free_audio_producer(audio_producer *prod);

/* Start playing, or resume playing at the current position.
 * The following conditions must be met before start_playing can be called:
 *  - The audio_producer is registered with an audio_consumer
 *  - The audio_producer is in state READY, see producer_state_is_ready() */
extern error_code start_playing(audio_producer *prod);

/* Pause playing. */
extern error_code producer_pause(audio_producer *prod);

/* This function is called by the audio_consumer. It is used to fetch the next
 * samples to be played on the headphones. */
extern error_code write_next_sample_to_headphones(audio_producer *,
    signed short *, size_t);

/* This function is called by the audio_consumer. It is used to fetch the next
 * samples to be played on the speakers. */
extern error_code write_next_sample_to_speakers(audio_producer *,
    signed short *, size_t);

/* Check if the producer is in state PLAYING */
extern int producer_state_is_playing(audio_producer *prod);

/* Check if the producer is in state LOADING_MP3 */
extern int producer_state_is_loading_mp3(audio_producer *prod);

/* Check if the producer is in state READY, i.e. the mp3 was successfully
 * loaded and the producer is ready to play the mp3. */
extern int producer_state_is_ready(audio_producer *prod);

/* Check if the producer is in state FAILED_TO_LOAD_MP3 */
extern int producer_state_is_failed_to_load_mp3(audio_producer *prod);

/* Check if the producer is in state END_OF_MP3, i.e. the entire mp3 was
 * played. */
extern int producer_state_is_end_of_mp3(audio_producer *prod);

/* Helper method for writing log messages: Returns a string representation of
 * the current producer state */
extern const char *producer_state_to_string(audio_producer *prod);

#endif
