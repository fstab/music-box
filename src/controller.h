#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "audio_consumer.h"
#include "audio_producer.h"
#include "error_codes.h"

/******************************************************************************
 * The controller will become the heart of the music box.
 * It keeps track of the audio_producers and audio_consumers, and provides
 * a function for everything that can be done with the music box.
 *
 * The current implementation supports some basic shell commands. A sample
 * shell session looks like this:
 *
 * load test1.mp3 as x
 * load test2.mp3 as y
 * play x
 * sleep 2
 * play y
 * wait x
 * wait y
 * quit
 *****************************************************************************/

typedef struct var {
    const char *name;
    audio_producer *producer;
} var;

typedef struct controller {
    var vars[MAX_PRODUCERS];
    audio_consumer *out;
} controller;

/* Allocate a new controller and put a reference to it in *ctrl_p.
 * The initialization of the audio_consumers is started in a background
 * process. You can call controller_is_ready() to check if the controller
 * is fully initialized.
 * In order to free() the controller, call controller_shutdown_and_free() */
extern error_code new_controller(controller **ctrl_p);

/* Load the mp3 file in path and remember it as variable varname */
extern error_code controller_load(controller *, const char *varname,
    const char *path);

/* Destroy the variable varname and release all resources associated with it */
extern error_code controller_unload_var(controller *, const char *varname);

/* Play the mp3 assiciated with varname */
extern error_code controller_play(controller *, const char *varname);

/* Wait until the mp3 associated with varname is finished playing */
extern error_code controller_wait(controller *, const char *varname);

/* Pause the mp3 assiciated with varname */
extern error_code controller_pause(controller *, const char *varname);

/* Resume the mp3 assiciated with varname */
extern error_code controller_resume(controller *, const char *varname);

/* Unload all variables, shut down the audio_consumer and free() everything */
extern error_code controller_shutdown_and_free(controller *);

/* Check if varname exists */
extern int controller_var_exists(controller *, const char *varname);

/* Check if the the maximum number of simultaniously used variables is
 * reached */
extern int controller_is_max_vars_reached(controller *);

/* Check if the controller is fully initialized. This must be true before
 * the controller can be used. */
extern int is_controller_ready(controller *);

#endif
