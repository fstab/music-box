#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "libmbx/api.h"
#include "libmbx/out/audio_output.h"
#include "libmbx/common/mbx_errno.h"
#include "libmbx/mp3lib/track.h"
#include "libmbx/config/config.h"

#define MAX_SAMPLE_FILES 16

typedef struct _mbx_ctrl *mbx_ctrl;

/**
 * Create and initialize a new music box controller.
 *
 * The music box controller is allocated and initialized with the values in
 * <tt>cfg</tt>. The initialization will also start background threads that
 * are used for playing audio. You must shut down and free the controller
 * using mbx_ctrl_shutdown_and_free().
 *
 * @param  ctrl_p
 *         A pointer to the newly initialized controller will be put here.
 * @param  cfg
 *         The configuration that is used for initializing the controller.
 * @return #MBX_SUCCESS, #MBX_DEVICE_DOES_NOT_EXIST, #MBX_PULSEAUDIO_ERROR
 */
extern mbx_error_code mbx_ctrl_new(mbx_ctrl *ctrl_p, mbx_config cfg);

/**
 * Load an MP3 file on deck A.
 * <p>
 * If deck A is already loaded, the old file will be freed and replaced with
 * the new file.
 *
 * @param  ctrl
 *         The controller
 * @param  path
 *         The path to the MP3 file to be loaded on deck A.
 * @return #MBX_SUCCESS, #MBX_FAILED_TO_LOAD_MP3
 */
extern mbx_error_code mbx_ctrl_deck_a_load(mbx_ctrl ctrl, const char *path);

/**
 * Load an MP3 file on deck B.
 * <p>
 * If deck B is already loaded, the old file will be freed and replaced with
 * the new file.
 *
 * @param  ctrl
 *         The controller
 * @param  path
 *         The path to the MP3 file to be loaded on deck B.
 * @return #MBX_SUCCESS, #MBX_FAILED_TO_LOAD_MP3
 */
extern mbx_error_code mbx_ctrl_deck_b_load(mbx_ctrl ctrl, const char *path);

/**
 * Load an MP3 file into a sample slot.
 *
 * The controller provides two decks and #MAX_SAMPLE_FILES sample slots.
 * An MP3 file in a sample slot can be played, but there are no controls like
 * volume, crossfader, rewind, etc.
 * <p>
 * If the slot is already loaded, the old file will be freed and replaced with
 * the new file.
 *
 * @param  ctrl
 *         The controller
 * @param  path
 *         The path to the MP3 file to be loaded on deck B.
 * @param  slot
 *         The slot number, where the file should be loaded.<br>
 *         <tt>0 <= slot < #MAX_SAMPLE_FILES</tt>
 * @return #MBX_SUCCESS, #MBX_FAILED_TO_LOAD_MP3
 */
extern mbx_error_code mbx_ctrl_sample_load(mbx_ctrl ctrl, const char *path,
        int slot);

/**
 * Play/Resume the file currently loaded on deck A.
 *
 * If there is no file loaded on deck A, nothing happens.
 *
 * @param  ctrl
 *         The controller
 */
extern void mbx_ctrl_deck_a_play(mbx_ctrl ctrl);

/**
 * Play/Resume the file currently loaded on deck B.
 *
 * If there is no file loaded on deck B, nothing happens.
 *
 * @param  ctrl
 *         The controller
 */
extern void mbx_ctrl_deck_b_play(mbx_ctrl ctrl);

/**
 * Play the file currently loaded on a sample slot.
 *
 * If there is no file loaded on that sample slot, nothing happens.
 *
 * @param  ctrl
 *         The controller
 * @param  slot
 *         The slot to be played.
 */
extern void mbx_ctrl_sample_play(mbx_ctrl ctrl, int slot);

/**
 * Pause the file currently loaded on deck A.
 *
 * If there is no file loaded on deck A, nothing happens.
 *
 * @param  ctrl
 *         The controller
 */
extern void mbx_ctrl_deck_a_pause(mbx_ctrl ctrl);

/**
 * Pause the file currently loaded on deck B.
 *
 * If there is no file loaded on deck B, nothing happens.
 *
 * @param  ctrl
 *         The controller
 */
extern void mbx_ctrl_deck_b_pause(mbx_ctrl ctrl);

/**
 * Disconnect from the audio output, and free all resources.
 *
 * @param  ctrl
 *         The controller
 */
extern void mbx_ctrl_shutdown_and_free(mbx_ctrl ctrl);

#endif
