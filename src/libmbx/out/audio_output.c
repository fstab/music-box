#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pulse/pulseaudio.h>
#include "audio_output.h"
#include "audio_output.h"
#include "log_context_state.h"
#include "libmbx/common/log.h"
#include "libmbx/common/xmalloc.h"
#include "libmbx/out/device_name_list.h"

/* pulseaudio callbacks */
static void stream_write_cb(pa_stream *, size_t, void *);
static void context_state_cb(pa_context *, void *);
static void stream_underflow_cb(pa_stream *, void *);
static void stream_drain_complete_cb(pa_stream*, int, void *);
static void context_drain_complete_cb(pa_context *, void *);
/* helper functions */
static const char *pa_msg(_mbx_out);
static pa_buffer_attr make_bufattr();
static void context_ready(_mbx_out out);
static void malloc_and_init(_mbx_out *out_p, const char *, const char *, _mbx_out_cb cb, void *userdata);
static void unset_all_callbacks(_mbx_out out);
static void do_free_audio_output(_mbx_out out);
static void do_shutdown(_mbx_out out);
static mbx_error_code init_pulseaudio(_mbx_out out);

enum state {
    _MBX_OUT_INITIALIZING,     /* Not yet connected to PulseAudio */
    _MBX_OUT_READY,            /* Ready to play audio */
    _MBX_OUT_PULSEAUDIO_ERROR, /* Initialization failed */
    _MBX_OUT_SHUT_DOWN         /* Connection to PulseAudio was terminated */
};

struct _mbx_out {
    const char *name;     /* For debug messages. "headphones" or "speakers" */
    const char *dev_name; /* Name of PulseAudio sink */
    _mbx_out_cb cb;
    void *output_cb_userdata;
    pa_sample_spec sample_spec;
    pa_threaded_mainloop *pa_ml;
    pa_context *pa_ctx;
    pa_stream *stream;
    pa_proplist *pa_props;
    enum state state;
    int underflows;            // Counter for underflow events.
    int trigger_shutdown;      // Becomes true when shutdown() is called.
};

/******************************************************************************
 * _mbx_out_new() and its helper functions
 *****************************************************************************/

mbx_error_code _mbx_out_new(_mbx_out *out_p, const char *name, const char *dev_name, _mbx_out_cb cb, void *output_cb_userdata) {
    int dev_exists = 0;
    if ( mbx_output_device_exists(dev_name, &dev_exists) != MBX_SUCCESS ) {
        return MBX_PULSEAUDIO_ERROR;
    }
    if ( ! dev_exists ) {
        return MBX_DEVICE_DOES_NOT_EXIST;
    }
    malloc_and_init(out_p, name, dev_name, cb, output_cb_userdata);
    if ( init_pulseaudio(*out_p) != MBX_SUCCESS ) {
        unset_all_callbacks(*out_p);
        do_free_audio_output(*out_p);
        return MBX_PULSEAUDIO_ERROR;
    }
    while ( (*out_p)->state == _MBX_OUT_INITIALIZING ) {
        usleep(10*1000); /* 10ms */
    }
    if ( (*out_p)->state == _MBX_OUT_PULSEAUDIO_ERROR ) {
        unset_all_callbacks(*out_p);
        do_free_audio_output(*out_p);
        return MBX_PULSEAUDIO_ERROR;
    }
    return MBX_SUCCESS;
}

/* Initialize a new _mbx_out. This is a helper function for _mbx_out_new() */
static void malloc_and_init(_mbx_out *out_p,
        const char *name, const char *dev_name, _mbx_out_cb cb,
        void *output_cb_userdata)
{
    *out_p = _mbx_xmalloc(sizeof(struct _mbx_out));
    bzero(*out_p, sizeof(struct _mbx_out));
    (*out_p)->state = _MBX_OUT_INITIALIZING;
    (*out_p)->output_cb_userdata = output_cb_userdata;
    (*out_p)->name = _mbx_xstrdup(name);
    (*out_p)->dev_name = _mbx_xstrdup(dev_name);
    /* TODO: The sample spec is hard coded. It should be determined depending
     * on the capabilities of the sound card. */
    (*out_p)->sample_spec.rate = MBX_SAMPLE_RATE;
    (*out_p)->sample_spec.channels = 2; /* stereo */
    (*out_p)->sample_spec.format = PA_SAMPLE_S16LE;
    /* an invalid sample spec would be a programming error */
    assert(pa_sample_spec_valid(&(*out_p)->sample_spec));
    (*out_p)->cb = cb;
}

/******************************************************************************
 * Start the PulseAudio mainloop background thread.
 * This function returns as soon as the PulseAudio thread is started.
 * Initialization of PulseAudio is done in the background thread.
 * In order to learn when initialization is finished, you need to wait until
 * context_state_cb() is called, and check the PulseAudio context state.
 *****************************************************************************/
static mbx_error_code init_pulseaudio(_mbx_out out) {
    if ((out->pa_ml = pa_threaded_mainloop_new()) == NULL ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Unable to allocate a PulseAudio threaded main "
            "loop object.");
        return MBX_PULSEAUDIO_ERROR;
    }
    out->pa_props = pa_proplist_new(); /* TODO: Set properties in proplist */
    pa_mainloop_api *pa_mlapi = pa_threaded_mainloop_get_api(out->pa_ml);
    out->pa_ctx = pa_context_new_with_proplist(pa_mlapi, NULL, out->pa_props);
    if ( out->pa_ctx == NULL ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Unable to instantiate a pulseaudio connection "
            "context.");
        return MBX_PULSEAUDIO_ERROR;
    }
    pa_context_set_state_callback(out->pa_ctx, context_state_cb, out);
    if ( pa_context_connect(out->pa_ctx, NULL, 0, NULL) < 0 ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Unable to connect to the pulseaudio server: %s",
            pa_msg(out));
        return MBX_PULSEAUDIO_ERROR;
    }
    if ( pa_threaded_mainloop_start(out->pa_ml) ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Failed to start pulseaudio mainloop "
            "in background thread.");
        return MBX_PULSEAUDIO_ERROR;
    }
    return MBX_SUCCESS;
}

/******************************************************************************
 * The context_state_cb() will be called by PulseAudio when the PulseAudio
 * context changes state.
 * So far, we are only interested in PA_CONTEXT_READY: When the context is
 * ready, we initialize a PulseAudio stream and connect it to the server.
 *****************************************************************************/
static void context_state_cb(pa_context *context, void *userdata) {
    _mbx_out out = (_mbx_out ) userdata;
    pa_context_state_t state = pa_context_get_state(context);
    assert ( out != NULL && out->pa_ctx == context );
    log_context_state(state);
    switch  (state) {
        case PA_CONTEXT_READY:
            context_ready(out);
            out->state = _MBX_OUT_READY;
            break;
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            /* Do nothing until the state becomes PA_CONTEXT_READY. */
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
        default:
            /* Initialization error. */
            out->state = _MBX_OUT_PULSEAUDIO_ERROR;
    }
}

/******************************************************************************
 * This is called when the PulseAudio context state becomes PA_CONTEXT_READY.
 * We initialize a new PulseAudio stream, configure it with our callbacks,
 * and connect the new stream to the PulseAudio server.
 *****************************************************************************/
static void context_ready(_mbx_out out) {
    out->stream = pa_stream_new(out->pa_ctx, "playback", &out->sample_spec, NULL);
    if ( out->stream == NULL ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Unable to create pulseaudio stream: %s", pa_msg(out));
        out->state = _MBX_OUT_PULSEAUDIO_ERROR;
        return;
    }
    /* will be called when pulseaudio requests audio data */
    pa_stream_set_write_callback(out->stream, stream_write_cb, out);
    /* will be called when an buffer underflow occurs */
    pa_stream_set_underflow_callback(out->stream, stream_underflow_cb, out);
    pa_buffer_attr bufattr = make_bufattr();
    int r = pa_stream_connect_playback(out->stream, out->dev_name, &bufattr,
        PA_STREAM_INTERPOLATE_TIMING |
        PA_STREAM_ADJUST_LATENCY |
        PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
    if (r < 0) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Failed to connect stream to pulseaudio sink: %s",
            pa_msg(out));
        out->state = _MBX_OUT_PULSEAUDIO_ERROR;
        return;
    }
    out->state = _MBX_OUT_READY;
}

/* Helper function for context_ready().
 * Initializes pa_buffer_attr for the given latency.
 * TODO: We don't use any latency settings so far. For more info on the bufattr
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
 * This will be called by PulseAudio when we need to provide audio data
 * for playback.
 *****************************************************************************/
static void stream_write_cb(pa_stream *s, size_t n_requested_bytes,
        void *userdata) {
    _mbx_out out = (_mbx_out ) userdata;
    sample_t *data_to_write = NULL;
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
        assert(n_bytes_to_write % (2*sizeof(sample_t)) == 0);
        if ( r < 0 ) {
            mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Prepare writing data to the pulseaudio server"
                " failed: %s", pa_msg(out));
            pa_stream_cancel_write(s);
            return;
        }
        if ( n_bytes_to_write > 0 ) {
            int i;
            assert ( n_bytes_to_write / sizeof(sample_t) < 44100L*2*8 );
            sample_t left[44100L*2*8];
            sample_t right[44100L*2*8];
            out->cb(left, right, n_bytes_to_write / (2*sizeof(sample_t)), out->output_cb_userdata);
            for ( i=0; i<n_bytes_to_write/(2*sizeof(sample_t)); i++ ) {
                data_to_write[2*i] = left[i];
                data_to_write[2*i+1] = right[i];
            }
            pa_stream_write(s, data_to_write, n_bytes_to_write, NULL, 0,
                PA_SEEK_RELATIVE);
            n_bytes_written += n_bytes_to_write;
        }
    }
}

/******************************************************************************
 * stream_underflow_cb()
 * This will be called by PulseAudio when a buffer underflow occurs.
 * We should use this to increase latency.
 *****************************************************************************/
static void stream_underflow_cb(pa_stream *s, void *userdata) {
    mbx_log_info(MBX_LOG_AUDIO_OUTPUT, "Pulseaudio buffer underflow.");
    _mbx_out out = (_mbx_out ) userdata;
    out->underflows++;
    /* TODO: increase latency, as in SimpleAsyncPlayback.c */
}

/******************************************************************************
 * shutdown()
 *****************************************************************************/

/* The extern shutdown() function will just set a flag. The real shutdown
 * starts in stream_write_cb(), when do_shutdown() is called */
void _mbx_out_shutdown_and_free(_mbx_out out) {
    assert ( out != NULL );
    out->trigger_shutdown = 1;
    while ( out->state == _MBX_OUT_READY ) {
        usleep(10*1000);
    }
    do_free_audio_output(out);
}

/* Shutdown the application. This is called from the pulseaudio mainloop
 * thread in stream_write_cb() */
static void do_shutdown(_mbx_out out) {
    mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "Shutting down.");
    unset_all_callbacks(out);
    /* stream_drain_complete_cb will be called when drain is done */
    pa_operation *o = pa_stream_drain(out->stream,stream_drain_complete_cb,out);
    if ( o == NULL ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Failed to start pulseaudio stream drain: %s",
            pa_msg(out));
        /* In case of error, we do our best and continue manually */
        stream_drain_complete_cb(out->stream, 0, out);
    }
    pa_operation_unref(o);
}

/* This callback is called when the stream drain is complete, i.e. the stream
 * can be disconnected and we can start draining the pulseaudio context. */
static void stream_drain_complete_cb(pa_stream*s, int success, void *userdata){
    _mbx_out out = (_mbx_out) userdata;
    assert(out != NULL && out->stream == s);
    mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "Draining pulseaudio stream has completed.");
    if (!success) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Failed to complete pulseaudio stream drain: %s",
            pa_msg(out));
    }
    pa_stream_disconnect(s);
    pa_stream_unref(s);
    pa_operation *o;
    o = pa_context_drain(out->pa_ctx, context_drain_complete_cb, out);
    if ( o == NULL ) {
        mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Failed to start pulseaudio context drain: %s",
            pa_msg(out));
        /* In case of error, we do our best and continue manually */
        context_drain_complete_cb(out->pa_ctx, out);
    }
    pa_operation_unref(o);
}

/* This callback is called when the context drain is complete, i.e. the
 * context is ready to be disconnected. */
static void context_drain_complete_cb(pa_context*c, void *userdata) {
    _mbx_out out = (_mbx_out ) userdata;
    assert ( out != NULL && out->pa_ctx == c);
    mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "Draining pulseaudio context has completed.");
    pa_context_disconnect(c);
    out->state = _MBX_OUT_SHUT_DOWN;
}

/******************************************************************************
 * Common helper functions
 *****************************************************************************/

static void do_free_audio_output(_mbx_out out) {
    if ( out != NULL ) {
        if ( out->pa_props != NULL ) {
            pa_proplist_free(out->pa_props);
            out->pa_props = NULL;
        }
        if ( out->pa_ml != NULL ) {
            pa_threaded_mainloop_free(out->pa_ml);
            out->pa_ml = NULL;
        }
        if ( out->dev_name != NULL ) {
            _mbx_xfree((void *) out->dev_name);
            out->dev_name = NULL;
        }
        if ( out->name != NULL ) {
            _mbx_xfree((void *) out->name);
            out->name = NULL;
        }
        _mbx_xfree(out);
    }
}

/* When we shutdown, or in case of error, we must make sure that pulseaudio
 * quits calling callbacks with the defunct audio_output.
 * This method sets all callbacks NULL.
 * ---
 * TODO: The name of this function is misleading: Actually, we just unset
 * the callbacks needed for playback, but we keep the callbacks needed during
 * shutdown */
static void unset_all_callbacks(_mbx_out out) {
    if ( out->pa_ctx != NULL ) {
        pa_context_set_state_callback(out->pa_ctx, NULL, NULL);
    }
    if ( out->stream != NULL ) {
        pa_stream_set_write_callback(out->stream, NULL, NULL);
        pa_stream_set_underflow_callback(out->stream, NULL, NULL);
    }
}

/* Get pulseaudio's error message */
static const char *pa_msg(_mbx_out out) {
    const char *result = NULL;
    if ( out->pa_ctx != NULL ) {
        result = pa_strerror(pa_context_errno(out->pa_ctx));
    }
    if ( result == NULL ) {
        return "unknown";
    }
    return result;
}
