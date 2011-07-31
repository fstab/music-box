#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pulse/pulseaudio.h>
#include "audio_output.h"
#include "libmbx/common/log.h"
#include "libmbx/common/xmalloc.h"

/*
 * Terminology:
 *
 * In music box's terminology, we use the term "output device" for a device
 * that can play audio. Music box usually uses two output devices: One for
 * the speakers, and one for the headphones.
 *
 * In PulseAudio's terminology, an output device is called a "sink".
 * That's why we implement a sink list callback in order to get the list of
 * output devices from PulseAudio.
 */

/* The output device names are stored in a list_of_strings. */
struct list_of_strings {
    char **strings;
    size_t n_strings;
    size_t size;
};

/* Push a new string to the list_of_strings. */
static void push_a_copy(const char *s, struct list_of_strings *list);

/* Context state callback function for PulseAudio. */
static void pa_context_state_cb(pa_context *c, void *userdata);

/* Sinklist callback function for PulseAudio. */
static void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol,
    void *userdata);

/* The implementation of create_output_device_name_list() is based on the
 * example code in
 * http://www.ypass.net/blog/2009/10/
 *    pulseaudio-an-async-example-to-get-device-lists/
 */
mbx_error_code mbx_create_output_device_name_list(char ***dev_names, size_t *n_devs)
{
    pa_mainloop *pa_ml = pa_mainloop_new();
    pa_mainloop_api *pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_context *pa_ctx = pa_context_new(pa_mlapi, "music box (listing output "
        "devices)");
    pa_operation *pa_op;
    pa_context_state_t pa_context_state = PA_CONTEXT_UNCONNECTED;
    int do_iterate = 1;
    int error = 0;

    pa_context_connect(pa_ctx, NULL, 0, NULL);
    /* The state callback will update the state when we are connected to the
     * PulseAudio server, or if an error occurs. */
    pa_context_set_state_callback(pa_ctx, pa_context_state_cb,
        &pa_context_state);

    while ( do_iterate ) {
        switch ( pa_context_state ) {
            case PA_CONTEXT_UNCONNECTED:
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                pa_mainloop_iterate(pa_ml, 1, NULL); // we must wait.
                break;
            case PA_CONTEXT_READY:
                do_iterate = 0;
                break;
            case PA_CONTEXT_FAILED:
                mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Connection to PulseAudio server failed: "
                    "%s", pa_strerror(pa_context_errno(pa_ctx)));
                error = 1;
                break;
            case PA_CONTEXT_TERMINATED:
                mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "Connection to PulseAudio server "
                    "terminated unexpectedly.");
                error = 1;
                break;
            default:
                mbx_log_error(MBX_LOG_AUDIO_OUTPUT, "The PulseAudio context has an unexpected "
                    "state: %d", pa_context_state);
                error = 1;
                break;
        }
        if ( error ) {
            do_iterate = 0;
        }
    }
    if ( ! error ) {
        struct list_of_strings result = { NULL, 0, 0 };
        pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, &result);
        while ( pa_operation_get_state(pa_op) == PA_OPERATION_RUNNING ) {
            pa_mainloop_iterate(pa_ml, 1, NULL); // wait.
        }
        pa_operation_unref(pa_op);
        *dev_names = result.strings;
        *n_devs = result.n_strings;
    }
    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);
    return error ? MBX_PULSEAUDIO_ERROR : MBX_SUCCESS;
}


/* This callback gets called when the pulseaudio context changes state. */
static void pa_context_state_cb(pa_context *c, void *userdata) {
    *((pa_context_state_t *) userdata) = pa_context_get_state(c);
}

/* pa_mainloop calls this function when it's ready to tell us about a sink.
 * Since we're not threading, there's no need for mutexes on the
 * list_of_strings.
 */
static void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol,
        void *userdata) {
    struct list_of_strings *list = (struct list_of_strings *) userdata;
    assert ( userdata != NULL );
    if ( l != NULL ) {
        push_a_copy(l->name, list);
    }
}

void mbx_free_output_device_name_list(char **dev_names) {
    if ( dev_names != NULL ) {
        int i=0;
        while ( dev_names[i] != NULL ) {
            _mbx_xfree(dev_names[i++]);
        }
        _mbx_xfree(dev_names);
    }
}

mbx_error_code mbx_output_device_exists(const char *name, int *result) {
    char **dev_names;
    size_t n_devs;
    size_t i;
    mbx_error_code r;
    if ( name == NULL ) {
        *result = 0;
        return MBX_SUCCESS;
    }
    r = mbx_create_output_device_name_list(&dev_names, &n_devs);
    if ( r != MBX_SUCCESS ) {
        return MBX_PULSEAUDIO_ERROR;
    }
    *result = 0;
    for ( i=0; i<n_devs; i++ ) {
        if ( ! strcmp(dev_names[i], name) ) {
            *result = 1;
        }
    }
    mbx_free_output_device_name_list(dev_names);
    return MBX_SUCCESS;
}

static void push_a_copy(const char *s, struct list_of_strings *list) {
    if ( list->size < list->n_strings + 2 ) { // including terminating NULL
        list->size = list->size == 0 ? 2 : list->size * 2;
        list->strings = _mbx_xrealloc(list->strings, list->size*sizeof(char *));
    }
    list->strings[list->n_strings++] = _mbx_xstrdup(s);
    list->strings[list->n_strings] = NULL;
}
