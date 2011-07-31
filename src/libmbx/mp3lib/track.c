#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include "track.h"
#include "mad_decoder.h"
#include "libmbx/common/log.h"
#include "libmbx/common/xmalloc.h"

// static error_code write_next_sample(audio_producer *,short *,size_t,short **);

/******************************************************************************
 * For comments on extern functions, see audio_producer.h
 *****************************************************************************/

enum state {
    TRACK_READY,
    TRACK_PLAYING,
    TRACK_END_OF_MP3
};

struct _mbx_track {
    enum state state;
    sample_t *sample_data;
    sample_t *current_pos_speaker;
    sample_t *end_pos;
    const char *filename;
};

mbx_error_code _mbx_track_new(_mbx_track *track_p, const char *path) {
    sample_t *data;
    size_t length;
    FILE *file;
    mbx_error_code r;
    if ( ( file = fopen(path, "r") ) == NULL ) {
        return MBX_FAILED_TO_LOAD_MP3;
    }
    if ( ( r = mad_decode(file, &data, &length) ) != MBX_SUCCESS ) {
        fclose(file);
        return MBX_FAILED_TO_LOAD_MP3;
    }
    fclose(file);
    *track_p = _mbx_xmalloc(sizeof(struct _mbx_track));
    bzero(*track_p, sizeof(struct _mbx_track));
    (*track_p)->sample_data = data;
    (*track_p)->end_pos = data + length;
    (*track_p)->current_pos_speaker = data;
    (*track_p)->state = TRACK_READY;
    return MBX_SUCCESS;
}

void _mbx_track_free(_mbx_track track) {
    _mbx_xfree((void *) track->filename);
    _mbx_xfree((void *) track->sample_data);
    bzero(track, sizeof(struct _mbx_track));
    _mbx_xfree(track);
}

void _mbx_track_play(_mbx_track track) {
    track->state = TRACK_PLAYING;
}

void _mbx_track_pause(_mbx_track track) {
    track->state = TRACK_READY;
}

int _mbx_track_is_playing(_mbx_track track) {
    return track->state == TRACK_PLAYING;
}

void _mbx_track_get_next_sample(_mbx_track track, sample_t *left, sample_t *right) {
    if ( track->state != TRACK_PLAYING ) {
        *left = *right = 0;
    }
    else if ( track->current_pos_speaker >= track->end_pos ) {
        if ( track->state != TRACK_END_OF_MP3 ) {
            mbx_log_debug(MBX_LOG_MP3LIB, "Reached end of file. Stopping.");
            track->state = TRACK_END_OF_MP3;
        }
    }
    else {
        *left = *track->current_pos_speaker;
        track->current_pos_speaker += 1;
        *right = *track->current_pos_speaker;
        track->current_pos_speaker += 1;
    }
}

