#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "controller.h"
#include "libmbx/common/log.h"
#include "libmbx/out/audio_output.h"
#include "libmbx/common/mbx_errno.h"
#include "libmbx/common/xmalloc.h"

/* If buffer size exceeds 8 seconds, something is wrong... */
#define MAX_SAMPLES_IN_BUFFER (MBX_SAMPLE_RATE * 2 * 8)

/*
 * The controller has two decks: deck A and deck B.
 * A deck is like a turntable.
 * You can load an MP3 file into a deck, and you can use the crossfacer
 * to mix the decks together.
 */
struct deck {
    _mbx_track track;
    double vol;
};

/*
 * The controller has two outputs: One for the speakers, one for the
 * headphones. The struct out represents one output.
 */
struct out {
    _mbx_out out;
    sample_t buf_left[MAX_SAMPLES_IN_BUFFER];   // left stereo channel
    sample_t buf_right[MAX_SAMPLES_IN_BUFFER];  // right stereo channel
    sample_t *read_pos_left;
    sample_t *read_pos_right;
    sample_t *write_pos_left;
    sample_t *write_pos_right;
};

/*
 * This is the data structure for the controller.
 */
struct _mbx_ctrl {
    struct deck deck_a;
    struct deck deck_b;
    _mbx_track samples[MAX_SAMPLE_FILES];
    struct out speakers;
    struct out headphones;
    struct deck *headphones_input;
    double crossfader;
};

/* Helper function for the initialization of a new controller */
static void init_deck(struct deck *deck);
static void init_out(struct out *out);
static mbx_error_code load(_mbx_track *track_p, const char *path);

/* The output callbacks are called by the audio_output when audio data must
 * be written to the output device. */
static void output_cb_speakers(sample_t *left, sample_t *right,
    size_t n_samples, void *userdata);
static void output_cb_headphones(sample_t *left, sample_t *right,
    size_t n_samples, void *userdata);

mbx_error_code mbx_ctrl_new(mbx_ctrl *ctrl_p, mbx_config cfg) {
    mbx_error_code r;
    int i;
    const char *speakers_dev, *headphones_dev;
    mbx_ctrl ctrl = _mbx_xmalloc(sizeof(struct _mbx_ctrl));
    init_deck(&ctrl->deck_a);
    init_deck(&ctrl->deck_b);
    for ( i=0; i<MAX_SAMPLE_FILES; i++ ) {
        ctrl->samples[i] = NULL;
    }
    init_out(&ctrl->speakers);
    init_out(&ctrl->headphones);
    speakers_dev = mbx_config_get(cfg, MBX_CFG_SPEAKERS_DEVICE);
    if ( (r = _mbx_out_new(&ctrl->speakers.out, "speakers",
            speakers_dev, output_cb_speakers, ctrl)) != MBX_SUCCESS ) {
        mbx_ctrl_shutdown_and_free(ctrl);
        return r;
    }
    headphones_dev = mbx_config_get(cfg, MBX_CFG_HEADPHONES_DEVICE);
    if ( (r = _mbx_out_new(&ctrl->headphones.out, "headphones",
            headphones_dev, output_cb_headphones, ctrl)) != MBX_SUCCESS ) {
        mbx_ctrl_shutdown_and_free(ctrl);
        return r;
    }
    *ctrl_p = ctrl;
    return MBX_SUCCESS;
}

static void init_deck(struct deck *deck) {
    deck->track = NULL;
    deck->vol = 1;
}

static void init_out(struct out *out) {
    bzero(out->buf_left, MAX_SAMPLES_IN_BUFFER * sizeof(sample_t));
    bzero(out->buf_right, MAX_SAMPLES_IN_BUFFER * sizeof(sample_t));
    out->read_pos_left = out->buf_left;
    out->read_pos_right = out->buf_right;
    out->write_pos_left = out->buf_left;
    out->write_pos_right = out->buf_right;
    out->out = NULL;
}

mbx_error_code mbx_ctrl_deck_a_load(mbx_ctrl ctrl, const char *path) {
    return load(&ctrl->deck_a.track, path);
}

mbx_error_code mbx_ctrl_deck_b_load(mbx_ctrl ctrl, const char *path) {
    return load(&ctrl->deck_b.track, path);
}

mbx_error_code mbx_ctrl_sample_load(mbx_ctrl ctrl, const char *path, int slot) {
    assert ( slot >= 0 && slot < MAX_SAMPLE_FILES );
    return load(&ctrl->samples[slot], path);
}

static mbx_error_code load(_mbx_track *track_p, const char *path) {
    _mbx_track old_track = *track_p;
    mbx_error_code r;
    if ( ( r = _mbx_track_new(track_p, path) ) != MBX_SUCCESS ) {
        return MBX_FAILED_TO_LOAD_MP3;
    }
    if ( old_track != NULL ) {
        _mbx_track_free(old_track);
    }
    return MBX_SUCCESS;
}

void mbx_ctrl_deck_a_play(mbx_ctrl ctrl) {
    if  ( ctrl->deck_a.track != NULL ) {
        _mbx_track_play(ctrl->deck_a.track);
    }
}

void mbx_ctrl_deck_b_play(mbx_ctrl ctrl) {
    if  ( ctrl->deck_b.track != NULL ) {
        _mbx_track_play(ctrl->deck_b.track);
    }
}

void mbx_ctrl_sample_play(mbx_ctrl ctrl, int slot) {
    assert ( slot >= 0 && slot < MAX_SAMPLE_FILES );
    if ( ctrl->samples[slot] != NULL ) {
        _mbx_track_play(ctrl->samples[slot]);
    }
}

void mbx_ctrl_deck_a_pause(mbx_ctrl ctrl) {
    if  ( ctrl->deck_a.track != NULL ) {
        _mbx_track_pause(ctrl->deck_a.track);
    }
}

void mbx_ctrl_deck_b_pause(mbx_ctrl ctrl) {
    if  ( ctrl->deck_b.track != NULL ) {
        _mbx_track_pause(ctrl->deck_b.track);
    }
}

void mbx_ctrl_shutdown_and_free(mbx_ctrl ctrl) {
    int slot;
    if ( ctrl->speakers.out != NULL ) {
        _mbx_out_shutdown_and_free(ctrl->speakers.out);
        ctrl->speakers.out = NULL;
    }
    if ( ctrl->headphones.out != NULL ) {
        _mbx_out_shutdown_and_free(ctrl->headphones.out);
        ctrl->headphones.out = NULL;
    }
    for ( slot=0; slot<MAX_SAMPLE_FILES; slot++ ) {
        if ( ctrl->samples[slot] != NULL ) {
            _mbx_track_free(ctrl->samples[slot]);
            ctrl->samples[slot] = NULL;
        }
    }
    if ( ctrl->deck_a.track != NULL ) {
        _mbx_track_free(ctrl->deck_a.track);
        ctrl->deck_a.track = NULL;
    }
    if ( ctrl->deck_b.track != NULL ) {
        _mbx_track_free(ctrl->deck_b.track);
        ctrl->deck_b.track = NULL;
    }
    free(ctrl);
}

/*****************************************************************************
 * Implementation of the output callbacks.
 ****************************************************************************/

/* Helper function to make sure that at least n_samples are available in the
 * ctrl's output buffers. */
static void fill_buffer(mbx_ctrl ctrl, size_t n_samples_to_write);

static void output_cb_headphones(sample_t *left, sample_t *right,
        size_t n_samples, void *userdata) {
    bzero(left, sizeof(sample_t) * n_samples);
    bzero(right, sizeof(sample_t) * n_samples);
}

static void output_cb_speakers(sample_t *left, sample_t *right,
        size_t n_samples, void *userdata) {
    size_t i = 0;
    mbx_ctrl ctrl = (mbx_ctrl) userdata;
    assert ( ctrl != NULL );
    // Make sure that at least n_samples are available in the ctrl's buffer.
    fill_buffer(ctrl, n_samples);
    // Write n_samples from the ctrl's buffer to the output.
    for ( i = 0; i < n_samples; i++) {
        left[i] = *ctrl->speakers.read_pos_left;
        right[i] = *ctrl->speakers.read_pos_right;
        ctrl->speakers.read_pos_left += 1;
        ctrl->speakers.read_pos_right += 1;
        if ( ctrl->speakers.read_pos_left >= ctrl->speakers.buf_left + MAX_SAMPLES_IN_BUFFER ) {
            assert ( ctrl->speakers.read_pos_right >= ctrl->speakers.buf_right + MAX_SAMPLES_IN_BUFFER );
            // log_debug(CONSUMER, "Read buffer cycle");
            ctrl->speakers.read_pos_left -= MAX_SAMPLES_IN_BUFFER;
            ctrl->speakers.read_pos_right -= MAX_SAMPLES_IN_BUFFER;
        }
        assert(ctrl->speakers.read_pos_right<ctrl->speakers.buf_right + MAX_SAMPLES_IN_BUFFER);
    }
}

static size_t diff(sample_t *a, sample_t *b) {
    return a > b ? a - b : b - a;
}

static void fill_buffer(mbx_ctrl ctrl, size_t n_samples_to_write) {
    int i=0;
    size_t n_samples_in_buffer = 0;
    if ( n_samples_to_write > MAX_SAMPLES_IN_BUFFER ) {
        mbx_log_fatal(MBX_LOG_CONTROLLER, "Output buffer exceeds %zu samples.", n_samples_to_write);
        exit(-1);
    }
    n_samples_in_buffer = diff(ctrl->speakers.read_pos_left, ctrl->speakers.write_pos_left);
    assert ( n_samples_in_buffer == diff(ctrl->speakers.read_pos_right, ctrl->speakers.write_pos_right) );
    // The loop writes one sample value at each time.
    while ( n_samples_in_buffer < n_samples_to_write ) {
        *ctrl->speakers.write_pos_left = 0;
        *ctrl->speakers.write_pos_right = 0;
        // Write audio data from all sample files that are currently playing.
        for ( i=0; i<MAX_SAMPLE_FILES; i++ ) {
            if ( ctrl->samples[i] != NULL ) {
                if ( _mbx_track_is_playing(ctrl->samples[i]) ) {
                    sample_t left;
                    sample_t right;
                    _mbx_track_get_next_sample(ctrl->samples[i], &left, &right);
                    *ctrl->speakers.write_pos_left += left;
                    *ctrl->speakers.write_pos_right += right;
                }
            }
        }
        // Write audio data from deck A
        if ( ctrl->deck_a.track != NULL && _mbx_track_is_playing(ctrl->deck_a.track) ) {
            sample_t left;
            sample_t right;
            _mbx_track_get_next_sample(ctrl->deck_a.track, &left, &right);
            *ctrl->speakers.write_pos_left += left;
            *ctrl->speakers.write_pos_right += right;
        }
        // Write audio data from deck B
        if ( ctrl->deck_b.track != NULL && _mbx_track_is_playing(ctrl->deck_b.track) ) {
            sample_t left;
            sample_t right;
            _mbx_track_get_next_sample(ctrl->deck_b.track, &left, &right);
            *ctrl->speakers.write_pos_left += left;
            *ctrl->speakers.write_pos_right += right;
        }
        // Move write position forward by one.
        ctrl->speakers.write_pos_left += 1;
        ctrl->speakers.write_pos_right += 1;
        if ( ctrl->speakers.write_pos_left >= ctrl->speakers.buf_left + MAX_SAMPLES_IN_BUFFER ) {
            assert ( ctrl->speakers.write_pos_right >= ctrl->speakers.buf_right + MAX_SAMPLES_IN_BUFFER );
            // log_debug(CONSUMER, "Write buffer cycle");
            ctrl->speakers.write_pos_left -= MAX_SAMPLES_IN_BUFFER;
            ctrl->speakers.write_pos_right -= MAX_SAMPLES_IN_BUFFER;
        }
        assert(ctrl->speakers.write_pos_right<ctrl->speakers.buf_right + MAX_SAMPLES_IN_BUFFER);
        n_samples_in_buffer++;
    }
}
