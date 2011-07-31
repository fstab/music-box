#ifndef TRACK_H
#define TRACK_H

#include <stdio.h>
#include <stdlib.h>
#include "libmbx/common/mbx_errno.h"
#include "libmbx/out/audio_output.h" /* defines sample_t */

/**
 * A #_mbx_track represents an MP3 file.
 */
typedef struct _mbx_track *_mbx_track;

/**
 * Allocate a new #_mbx_track, and initialize it with the audio data from an
 * MP3 file. The #_mbx_track must be freed with _mbx_track_free().
 *
 * @param  track
 *         A pointer to the newly created #_mbx_track is put here.
 * @param  path
 *         The path to the MP3 file to be loaded.
 * @return #MBX_SUCCESS, #MBX_FAILED_TO_LOAD_MP3
 */
extern mbx_error_code _mbx_track_new(_mbx_track *track, const char *path);

/**
 * Free an #_mbx_track
 *
 * @param  track
 *         The #_mbx_track to be freed.
 */
extern void _mbx_track_free(_mbx_track track);

/**
 * Start playing, or resume playing at the current position.
 *
 * This function is called by the #mbx_ctrl. The function just sets a flag,
 * such that consecutive calles to _mbx_track_get_next_sample() will return
 * audio data.
 *
 * @param  track
 *         The #_mbx_track that should start playing.
 */
extern void _mbx_track_play(_mbx_track track);

/**
 * Pause playing at the current position.
 *
 * This function is called by the #mbx_ctrl. The function just removes a flag,
 * such that consecutive calles to _mbx_track_get_next_sample() will no longer
 * return audio data.
 *
 * @param  track
 *         The #_mbx_track that should pause playing.
 */
extern void _mbx_track_pause(_mbx_track track);

/**
 * Check if the producer is playing.
 *
 * If true, then _mbx_track_get_next_sample() will return audio data. If false,
 * then _mbx_track_get_next_sample() will return zeros.
 *
 * @param  track
 *         The #_mbx_track, that should be checked.
 * @return <tt>1</tt> if the track is playing, <tt>0</tt> otherwise.
 */
extern int _mbx_track_is_playing(_mbx_track track);

/**
 * This function is called by #mbx_ctrl in order to get the next audio samples
 * to be played.
 *
 * If the track is not playing, zeros are returned.
 *
 * @param   track
 *          The track whose audio samples are requested.
 * @param   left
 *          The next audio sample for the left stereo channel will be put here.
 * @param   right
 *          The next audio sample for the right stereo channel will be put here.
 */
extern void _mbx_track_get_next_sample(_mbx_track track, sample_t *left, sample_t *right);

#endif
