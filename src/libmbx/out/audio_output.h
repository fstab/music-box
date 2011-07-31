#ifndef AUDIO_CONSUMER_H
#define AUDIO_CONSUMER_H

#include <stdlib.h>
#include <pulse/pulseaudio.h>
#include "libmbx/common/mbx_errno.h"
#include "audio_output.h"

/******************************************************************************
 * An audio_output represents a pulseaudio sink.
 * There will be two audio_outputs: One for the headphones, and one for
 * the speakers. Both audio_outputs are managed by the controller.
 *****************************************************************************/

/**
 * Number of samples per second.
 */
#define MBX_SAMPLE_RATE 44100

/**
 * A sample value is a signed 16 Bit short integer.
 */
typedef signed short sample_t;

/**
 * An #_mbx_out represents an audio output device.
 *
 * The controller uses two audio output devices: One for the speakers, and
 * one for the headphones.
 */
typedef struct _mbx_out *_mbx_out;

/**
 * Callback that is used by the audio output to get sample values from the
 * controller.
 *
 * @param  left
 *         An array of #sample_t. The sample values for the left stereo channel
 *         must be put into .
 * @param  right
 *         An array of #sample_t. The sample values for the right stereo
 *         channel must be put here.
 * @param  n_samples
 *         Number of sample values requested
 * @param  userdata
 *         The #output_cb_userdata will be put here, see new_audio_output()
 */
typedef void (* _mbx_out_cb)
    (sample_t *left, sample_t *right, size_t n_samples, void *userdata);

/* Create a new audio_output and connect it to pulseaudio. */
extern mbx_error_code _mbx_out_new(_mbx_out *, const char *name, const char *dev_name, _mbx_out_cb cb, void *output_cb_userdata);

/* Shutdown the connection to pulseaudio and free all resources associated
 * with the connection. */
extern void _mbx_out_shutdown_and_free(_mbx_out out);

#endif
