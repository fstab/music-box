#ifndef AUDIO_CONSUMER_H
#define AUDIO_CONSUMER_H

#include <stdlib.h>
#include <pulse/pulseaudio.h>
#include "audio_producer.h"
#include "error_codes.h"

#define MAX_PRODUCERS 16

/******************************************************************************
 * An audio_consumer represents a pulseaudio sink.
 * There will be two audio_consumers: One for the headphones, and one for
 * the speakers. Both audio_consumers are managed by the controller.
 *****************************************************************************/

enum channel {
    HEADPHONES,
    SPEAKERS
};

enum consumer_state {
    CONSUMER_NOT_INITIALIZED,
    CONSUMER_READY,
    CONSUMER_PULSEAUDIO_ERROR,
    CONSUMER_SHUT_DOWN
};

typedef struct audio_consumer {
    enum channel channel;
    audio_producer *producers[MAX_PRODUCERS];
    pa_sample_spec sample_spec;
    pa_threaded_mainloop *pa_ml;
    pa_context *pa_ctx;
    pa_stream *stream;
    pa_proplist *pa_props;
    enum consumer_state state;
    int underflows;            // Counter for underflow events.
    int trigger_shutdown;      // Becomes true when shutdown() is called.
} audio_consumer;

/* Create a new audio_consumer and connect it to pulseaudio.
 * This function returns immediately. The connection to pulseaudio
 * is established in the background. Use is_consumer_ready() to
 * find out when the consumer is initialized. */
extern error_code new_audio_consumer(audio_consumer **, enum channel);

/* Free the memory used by the audio_consumer. The audio_consumer must
 * be shut down before you can call free_audio_consumer() */
extern error_code free_audio_consumer(audio_consumer *);

/* Register an audio_producer as input for an audio_consumer.
 * Up to MAX_PRODUCERS per consumer are supported */
extern error_code register_input(audio_consumer *, audio_producer *);

/* Unregister an audio_producer */
extern error_code unregister_input(audio_consumer *, audio_producer *);

/* Shutdown the connection to pulseaudio. This function returns immediately.
 * The pulseaudio stream is shut down in the background. Use
 * is_consumer_shutdown() to learn when the consumer is terminated. */
extern error_code consumer_shutdown(audio_consumer *);

/* This function should be called after new_audio_consumer() to see when
 * the initialization is finished. */
extern int is_consumer_ready(audio_consumer *);

/* This function should be called after shutdown() to see when the pulseaudio
 * stream is terminated. */
extern int is_consumer_shutdown(audio_consumer *);

#endif
