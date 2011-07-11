#include <string.h>
#include <errno.h>
#include <pulse/pulseaudio.h>
#include "mad_decoder.h"
#include "audio_consumer.h"
#include "log.h"

/* pulseaudio callbacks */
static void stream_write_cb(pa_stream *, size_t, void *);
static void context_state_cb(pa_context *, void *);
static void stream_underflow_cb(pa_stream *, void *);
static void stream_drain_complete_cb(pa_stream*, int, void *);
static void context_drain_complete_cb(pa_context *, void *);
/* helper functions */
static const char *pa_msg(audio_consumer *);
static pa_buffer_attr make_bufattr();
static void context_ready(audio_consumer *out);
static error_code malloc_and_init_audio_consumer(audio_consumer**,enum channel);
static error_code connect_to_pa_server(audio_consumer *out);
static error_code set_pa_context_state_callback(audio_consumer *out);
static error_code set_pa_proplist(audio_consumer *out);
static error_code set_pa_mainloop(audio_consumer *out);
static error_code set_pa_connection_context(audio_consumer *out);
static error_code set_pa_stream(audio_consumer *out);
static error_code connect_pa_stream(audio_consumer *out);
static error_code start_pa_mainloop_in_new_thread(audio_consumer *out);
static void unset_all_callbacks(audio_consumer *out);
static void fetch_data_from_producers(audio_consumer *, signed short *, size_t);
static void do_free_audio_consumer(audio_consumer *out);
static void do_shutdown(audio_consumer *out);

/******************************************************************************
 * new_audio_consumer() and its helper functions
 *****************************************************************************/

error_code new_audio_consumer(audio_consumer **out_p, enum channel channel) {
    error_code r;
    if ( ( r = malloc_and_init_audio_consumer(out_p, channel) ) ) {
        return r;
    }
    if ( ( r = set_pa_mainloop(*out_p) ) ) {
        do_free_audio_consumer(*out_p);
        return r;
    }
    if ( ( r = set_pa_proplist(*out_p) ) ) {
        do_free_audio_consumer(*out_p);
        return r;
    }
    if ( ( r = set_pa_connection_context(*out_p) ) ) {
        do_free_audio_consumer(*out_p);
        return r;
    }
    if ( ( r = set_pa_context_state_callback(*out_p) ) ) {
        do_free_audio_consumer(*out_p);
        return r;
    }
    if ( ( r = connect_to_pa_server(*out_p) ) ) {
        pa_context_set_state_callback((*out_p)->pa_ctx, NULL, NULL);
        do_free_audio_consumer(*out_p);
        return r;
    }
    if ( ( r = start_pa_mainloop_in_new_thread(*out_p) ) ) {
        unset_all_callbacks(*out_p);
        do_free_audio_consumer(*out_p);
        return r;
    }
    return SUCCESS;
}

/* Initialize a new audio_consumer. This is a helper function for
 * new_audio_consumer() */
static error_code malloc_and_init_audio_consumer(audio_consumer **out_p,
        enum channel channel)
{
    int i;
    if ((*out_p = malloc(sizeof(audio_consumer))) == NULL ) {
        log_error(CONSUMER, "Failed to allocate memory for audio_consumer: %s",
            strerror(errno));
        return CONSUMER_OUT_OF_MEMORY;
    }
    (*out_p)->channel = channel;
    (*out_p)->trigger_shutdown = 0;
    (*out_p)->state = CONSUMER_NOT_INITIALIZED;
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        (*out_p)->producers[i] = NULL;
    }
    (*out_p)->underflows = 0;    /* number of underflow events */
    (*out_p)->sample_spec.rate = 44100;
    (*out_p)->sample_spec.channels = 2; /* stereo */
    (*out_p)->sample_spec.format = PA_SAMPLE_S16LE;
    /* an invalid sample spec would be a programming error */
    assert(pa_sample_spec_valid(&(*out_p)->sample_spec));
    (*out_p)->pa_ml = NULL;
    (*out_p)->pa_ctx = NULL;
    (*out_p)->stream = NULL;
    (*out_p)->pa_props = NULL;
    return SUCCESS;
}

/* Creates a new pa_threaded_mainloop and configures the audio_consumer
 * with it. This is a helper function for new_audio_consumer() */
static error_code set_pa_mainloop(audio_consumer *out) {
    if ((out->pa_ml = pa_threaded_mainloop_new()) == NULL ) {
        log_error(CONSUMER, "Unable to allocate a pulseaudio threaded main "
            "loop object.");
        return CONSUMER_PA_ERROR;
    }
    return SUCCESS;
}


/* Creates a new proplist and configures the audio_consumer with it
 * This is a helper function for new_audio_consumer() */
static error_code set_pa_proplist(audio_consumer *out) {
    out->pa_props = pa_proplist_new();
    /* TODO: Set properties in proplist */
    return SUCCESS;
}

/* Creates a new pulseaudio connection context and configures the
 * audio_consumer with it. This is a helper function for new_audio_consumer()*/
static error_code set_pa_connection_context(audio_consumer *out) {
    pa_mainloop_api *pa_mlapi = pa_threaded_mainloop_get_api(out->pa_ml);
    pa_proplist *pa_props = out->pa_props;
    pa_context *pa_ctx = pa_context_new_with_proplist(pa_mlapi, NULL, pa_props);
    if ( pa_ctx == NULL ) {
        log_error(CONSUMER, "Unable to instantiate a pulseaudio connection "
            "context.");
        return CONSUMER_PA_ERROR;
    }
    out->pa_ctx = pa_ctx;
    return SUCCESS;
}

/* The state callback will be called when the context is connected to
 * the pulseaudio server. This is a helper function for new_audio_consumer() */
static error_code set_pa_context_state_callback(audio_consumer *out) {
    pa_context_set_state_callback(out->pa_ctx, context_state_cb, out);
    return SUCCESS;
}

/* Connect the pulseaudio context to the pulseaudio server. This is a helper
 * function for new_audio_consumer() */
static error_code connect_to_pa_server(audio_consumer *out) {
    if ( pa_context_connect(out->pa_ctx, NULL, 0, NULL) < 0 ) {
        log_error(CONSUMER, "Unable to connect to the pulseaudio server: %s",
            pa_msg(out));
        return CONSUMER_PA_ERROR;
    }
    return SUCCESS;
}

/* Start pulseaudio threaded mainloop. This is a helper function for
 * new_audio_consumer() */
static error_code start_pa_mainloop_in_new_thread(audio_consumer *out) {
    if ( pa_threaded_mainloop_start(out->pa_ml) ) {
        log_error(CONSUMER, "Failed to start pulseaudio mainloop "
            "in background thread.");
        return CONSUMER_PA_ERROR;
    }
    return SUCCESS;
}

/******************************************************************************
 * context_state_cb()
 * This will be called by pulseaudio when the context changes state.
 * So far, we are only interested in PA_CONTEXT_READY: When the context is
 * ready, we initialize a pulseaudio stream and connect it to the server.
 *****************************************************************************/

static void context_state_cb(pa_context *context, void *userdata) {
    audio_consumer *out = (audio_consumer *) userdata;
    pa_context_state_t state = pa_context_get_state(context);
    assert ( out != NULL && out->pa_ctx == context );
    switch  (state) {
        case PA_CONTEXT_READY:
            log_debug(CONSUMER, "The connection to the pulseaudio server is "
                "established. The pulseaudio context is ready to execute "
                "operations.");
            context_ready(out);
            out->state = CONSUMER_READY;
            break;
        case PA_CONTEXT_UNCONNECTED:
            log_debug(CONSUMER, "The pulseaudio context hasn't been connected "
                "yet.");
            break;
        case PA_CONTEXT_CONNECTING:
            log_debug(CONSUMER, "A pulseaudio connection is being "
                "established.");
            break;
        case PA_CONTEXT_AUTHORIZING:
            log_debug(CONSUMER, "The pulseaudio client is authorizing itself "
                "to the daemon.");
            break;
        case PA_CONTEXT_SETTING_NAME:
            log_debug(CONSUMER, "The pulseaudio client is passing its "
                "application name to the daemon.");
            break;
        case PA_CONTEXT_FAILED:
            log_debug(CONSUMER, "The pulseaudio connection failed or was "
                "disconnected.");
            break;
        case PA_CONTEXT_TERMINATED:
            log_debug(CONSUMER, "The pulseaudio connection was terminated "
                "cleanly.");
            break;
        default:
            log_warn(CONSUMER, "Illegal argument: context_state_cb() received "
                "an unexpected state: %d", state);
    }
}

/* Helper function for context_state_cb().
 * Initializes the stream when the context becomes ready. */
static void context_ready(audio_consumer *out) {
    error_code r;
    if ( ( r = set_pa_stream(out) ) ) {
        out->state = CONSUMER_PULSEAUDIO_ERROR;
        unset_all_callbacks(out);
        return;
    }
    /* will be called when pulseaudio requests audio data */
    pa_stream_set_write_callback(out->stream, stream_write_cb, out);
    /* will be called when an buffer underflow occurs */
    pa_stream_set_underflow_callback(out->stream, stream_underflow_cb, out);
    if ( ( r = connect_pa_stream(out) ) ) {
        out->state = CONSUMER_PULSEAUDIO_ERROR;
        unset_all_callbacks(out);
        return;
    }
}

/* Creates a new pa_stream and configures the audio_consumer with it.
 * This is a helper function for context_ready() */
static error_code set_pa_stream(audio_consumer *out) {
    pa_stream *s;
    s = pa_stream_new(out->pa_ctx, "playback", &out->sample_spec, NULL);
    if ( s == NULL ) {
        log_error(CONSUMER, "Unable to create pulseaudio stream: %s",
            pa_msg(out));
        return CONSUMER_PA_ERROR;
    }
    out->stream = s;
    return SUCCESS;
}

/* Connects the pulseaudio stream to the pulseaudio server.
 * This is a helper function for context_ready() */
static error_code connect_pa_stream(audio_consumer *out) {
    pa_buffer_attr bufattr = make_bufattr();
    int r = pa_stream_connect_playback(out->stream, NULL, &bufattr,
        PA_STREAM_INTERPOLATE_TIMING |
        PA_STREAM_ADJUST_LATENCY |
        PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
    if (r < 0) {
        log_error(CONSUMER, "Failed to connect stream to pulseaudio sink: %s",
            pa_msg(out));
        return CONSUMER_PA_ERROR;
    }
    return SUCCESS;
}

/* Initialize pa_buffer_attr for the given latency.
 * We don't use any latency settings so far. For more info on the bufattr
 * fields see http://freedesktop.org/software/pulseaudio/doxygen/streams.html
 */
static pa_buffer_attr make_bufattr() {
    pa_buffer_attr bufattr;
    bufattr.fragsize  = (uint32_t)-1;
    bufattr.maxlength = (uint32_t)-1;
    bufattr.minreq    = (uint32_t)-1;
    bufattr.prebuf    = (uint32_t)-1;
    bufattr.tlength   = (uint32_t)-1;
    return bufattr;
}

/******************************************************************************
 * stream_write_cb()
 * This will be called by pulseaudio when we need to provide audio data
 * for playback.
 *****************************************************************************/

static void stream_write_cb(pa_stream *s, size_t n_requested_bytes,
        void *userdata) {
    audio_consumer *out = (audio_consumer *) userdata;
    unsigned char *data_to_write = NULL;
    size_t n_bytes_written = 0;

    assert ( out != NULL && out->stream == s);
    if ( out->trigger_shutdown ) {
        do_shutdown(out);
        return;
    }
    while ( n_bytes_written < n_requested_bytes ) {
        int r;
        size_t n_bytes_to_write = n_requested_bytes - n_bytes_written;
        r = pa_stream_begin_write(s, (void**)&data_to_write, &n_bytes_to_write);
        if ( r < 0 ) {
            log_error(CONSUMER, "Prepare writing data to the pulseaudio server"
                " failed: %s", pa_msg(out));
            pa_stream_cancel_write(s);
            return;
        }
        if ( n_bytes_to_write > 0 ) {
            memset(data_to_write, 0, n_bytes_to_write); // 0 is silence.
            fetch_data_from_producers(out, (signed short *) data_to_write,
                n_bytes_to_write / sizeof(short));
            pa_stream_write(s, data_to_write, n_bytes_to_write, NULL, 0,
                PA_SEEK_RELATIVE);
            n_bytes_written += n_bytes_to_write;
        }
    }
}

/* Helper function for stream_write_cb():
 * Ask the producers for the next samples to play. If a producer is not
 * playing, it will just add silence */
static void fetch_data_from_producers(audio_consumer *out, signed short *data,
        size_t n_samples) {
    int i=0;
    for ( i=0; i<n_samples; i++ ) {
        data[i] = (signed short) 0;
    }
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( out->producers[i] != NULL ) {
            if ( out->channel == HEADPHONES ) {
                write_next_sample_to_headphones(out->producers[i], data,
                    n_samples);
            }
            else {
                write_next_sample_to_speakers(out->producers[i], data,
                    n_samples);
            }
        }
    }
}

/******************************************************************************
 * stream_underflow_cb()
 * This will be called by pulseaudio when a buffer underflow occurs.
 * We should use this to increase latency.
 *****************************************************************************/

static void stream_underflow_cb(pa_stream *s, void *userdata) {
    log_info(CONSUMER, "Pulseaudio buffer underflow.");
    audio_consumer *out = (audio_consumer *) userdata;
    out->underflows++;
    /* TODO: increase latency, as in SimpleAsyncPlayback.c */
}

/******************************************************************************
 * shutdown()
 *****************************************************************************/

/* The extern shutdown() function will just set a flag. The real shutdown
 * starts in stream_write_cb(), when do_shutdown() is called */
error_code consumer_shutdown(audio_consumer *out) {
    out->trigger_shutdown = 1;
    return SUCCESS;
}

/* Shutdown the application. This is called from the pulseaudio mainloop
 * thread in stream_write_cb() */
static void do_shutdown(audio_consumer *out) {
    log_debug(CONSUMER, "Shutting down.");
    unset_all_callbacks(out);
    /* stream_drain_complete_cb will be called when drain is done */
    pa_operation *o = pa_stream_drain(out->stream,stream_drain_complete_cb,out);
    if ( o == NULL ) {
        log_error(CONSUMER, "Failed to start pulseaudio stream drain: %s",
            pa_msg(out));
        /* In case of error, we do our best and continue manually */
        stream_drain_complete_cb(out->stream, 0, out);
    }
    pa_operation_unref(o);
}

/* This callback is called when the stream drain is complete, i.e. the stream
 * can be disconnected and we can start draining the pulseaudio context. */
static void stream_drain_complete_cb(pa_stream*s, int success, void *userdata){
    audio_consumer *out = (audio_consumer *) userdata;
    assert(out != NULL && out->stream == s);
    log_debug(CONSUMER, "Draining pulseaudio stream has completed.");
    if (!success) {
        log_error(CONSUMER, "Failed to complete pulseaudio stream drain: %s",
            pa_msg(out));
    }
    pa_stream_disconnect(s);
    pa_stream_unref(s);
    pa_operation *o;
    o = pa_context_drain(out->pa_ctx, context_drain_complete_cb, out);
    if ( o == NULL ) {
        log_error(CONSUMER, "Failed to start pulseaudio context drain: %s",
            pa_msg(out));
        /* In case of error, we do our best and continue manually */
        context_drain_complete_cb(out->pa_ctx, out);
    }
    pa_operation_unref(o);
}

/* This callback is called when the context drain is complete, i.e. the
 * context is ready to be disconnected. */
static void context_drain_complete_cb(pa_context*c, void *userdata) {
    audio_consumer *out = (audio_consumer *) userdata;
    assert ( out != NULL && out->pa_ctx == c);
    log_debug(CONSUMER, "Draining pulseaudio context has completed.");
    pa_context_disconnect(c);
    out->state = CONSUMER_SHUT_DOWN;
}

/******************************************************************************
 * functions for querying the audio_consumer's state
 *****************************************************************************/

/* see audio_consumer.h */

int is_consumer_ready(audio_consumer *out) {
    return out->state == CONSUMER_READY;
}

int is_consumer_shutdown(audio_consumer *out) {
    return out->state == CONSUMER_SHUT_DOWN;
}

/******************************************************************************
 * register_input()
 *****************************************************************************/

/* see audio_consumer.h */
error_code register_input(audio_consumer *out, audio_producer *prod) {
    int i=0;
    log_debug(CONSUMER, "Registering new audio producer.");
    assert ( out != NULL && prod != NULL );
    if ( out->state == CONSUMER_PULSEAUDIO_ERROR ) {
        log_error(CONSUMER, "Rejecting new audio producer, because there "
            " was a pulseaudio error.");
        return CONSUMER_PA_ERROR;
    }
    assert ( out->state == CONSUMER_READY );
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( out->producers[i] == NULL ) {
            out->producers[i] = prod;
            log_debug(CONSUMER, "Audio producer registered.");
            return SUCCESS;
        }
    }
    log_warn(CONSUMER, "Maximum number of producers reached: %d",
        MAX_PRODUCERS);
    return CONSUMER_REACHED_MAX_NUMBER_OF_PRODUCERS;
}

/******************************************************************************
 * unregister_input()
 *****************************************************************************/

/* see audio_consumer.h */
error_code unregister_input(audio_consumer *out, audio_producer *prod) {
    int i=0;
    log_debug(CONSUMER, "Unregistering audio producer.");
    assert(out != NULL && prod != NULL);
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( out->producers[i] == prod ) {
            out->producers[i] = NULL;
            log_debug(CONSUMER, "Audio producer unregistered.");
            return SUCCESS;
        }
    }
    log_error(CONSUMER, "The audio_producer was not registered.");
    return CONSUMER_DOES_NOT_KNOW_PRODUCER;
}


/******************************************************************************
 * free_audio_consumer()
 *****************************************************************************/

/* see audio_consumer.h */
error_code free_audio_consumer(audio_consumer *out) {
    assert ( out != NULL && out->state == CONSUMER_SHUT_DOWN );
    do_free_audio_consumer(out);
    return SUCCESS;
}

/******************************************************************************
 * Common helper functions
 *****************************************************************************/

static void do_free_audio_consumer(audio_consumer *out) {
    if ( out != NULL ) {
        if ( out->pa_props != NULL ) {
            pa_proplist_free(out->pa_props);
            out->pa_props = NULL;
        }
        if ( out->pa_ml != NULL ) {
            pa_threaded_mainloop_free(out->pa_ml);
            out->pa_ml = NULL;
        }
        free(out);
    }
}

/* When we shutdown, or in case of error, we must make sure that pulseaudio
 * quits calling callbacks with the defunct audio_consumer.
 * This method sets all callbacks NULL.
 * ---
 * TODO: The name of this function is misleading: Actually, we just unset
 * the callbacks needed for playback, but we keep the callbacks needed during
 * shutdown */
static void unset_all_callbacks(audio_consumer *out) {
    if ( out->pa_ctx != NULL ) {
        pa_context_set_state_callback(out->pa_ctx, NULL, NULL);
    }
    if ( out->stream != NULL ) {
        pa_stream_set_write_callback(out->stream, NULL, NULL);
        pa_stream_set_underflow_callback(out->stream, NULL, NULL);
    }
}

/* Get pulseaudio's error message */
static const char *pa_msg(audio_consumer *out) {
    const char *result = NULL;
    if ( out->pa_ctx != NULL ) {
        result = pa_strerror(pa_context_errno(out->pa_ctx));
    }
    if ( result == NULL ) {
        return "unknown";
    }
    return result;
}
