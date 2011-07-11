#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "log.h"
#include "controller.h"
#include "audio_producer.h"
#include "audio_consumer.h"

static error_code store_new_var(controller *, const char *, audio_producer *);
static int get_index_of(controller *, const char *);
static error_code get_producer(controller *, const char *, audio_producer **);
static error_code assert_producer_state(const char *, enum producer_state,
    audio_producer *prod);

/******************************************************************************
 * For comments on extern functions, see controller.h
 *****************************************************************************/

error_code new_controller(controller **ctrl_p) {
    error_code r;
    int i;
    controller *ctrl = malloc(sizeof(controller));
    if ( ctrl == NULL ) {
        return CONTROLLER_OUT_OF_MEMORY;
    }
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        var v = { NULL, NULL };
        ctrl->vars[i] = v;
    }
    if ( (r = new_audio_consumer(&ctrl->out, SPEAKERS)) ) {
        free(ctrl);
        ctrl = NULL;
    }
    *ctrl_p = ctrl;
    return r;
}

int is_controller_ready(controller *ctrl) {
    return is_consumer_ready(ctrl->out);
}

error_code controller_shutdown_and_free(controller *ctrl) {
    error_code r;
    int i;
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( ctrl->vars[i].name != NULL ) {
            controller_unload_var(ctrl, ctrl->vars[i].name);
        }
    }
    if ( ( r = consumer_shutdown(ctrl->out) ) ) {
        return r;
    }
    while ( ! is_consumer_shutdown(ctrl->out) ) {
        usleep(10*1000);
    }
    free(ctrl);
    return SUCCESS;
}

/* path is a pointer to the path in the current command line. It will be
 * overwritten when the user types the next command. We need to copy it in
 * order to keep it.
 */
error_code controller_load(controller *ctrl, const char *varname,
        const char *path) {
    audio_producer *prod;
    error_code r;
    if ( ( r = new_audio_producer(&prod) ) ) {
        return r;
    }
    if ( ( r = start_loading_mp3(prod, path) ) ) {
        return r;
    }
    if ( ( r = store_new_var(ctrl, varname, prod) ) ) {
        free_audio_producer(prod);
        return r;
    }
    while ( producer_state_is_loading_mp3(prod) ) {
        usleep(10*1000); // 10ms
    }
    if ( producer_state_is_failed_to_load_mp3(prod) ) {
        /* This happens if the mp3 file cannot be read. In that case, the
         * audio_producer has already logged an error message. */
        controller_unload_var(ctrl, varname);
        return CONTROLLER_LOAD_MP3_FAILED;
    }
    assert ( producer_state_is_ready(prod) );
    if ( ( r = register_input(ctrl->out, prod) ) ) {
        controller_unload_var(ctrl, varname);
        return r;
    }
    return SUCCESS;
}

/* Get the index of the variable called varname in ctrl->vars.
 * If varname doesn't exist, return -1. */
static int get_index_of(controller *ctrl, const char *varname) {
    int i;
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( ctrl->vars[i].name != NULL ) {
            if ( ! strcmp(ctrl->vars[i].name, varname) ) {
                return i;
            }
        }
    }
    return -1;
}

int controller_var_exists(controller *ctrl, const char *varname) {
    return get_index_of(ctrl, varname) != -1;
}

int is_max_vars_reached(controller *ctrl) {
    int i;
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( ctrl->vars[i].name == NULL ) {
            return 0;
        }
    }
    return 1;
}

static error_code store_new_var(controller *ctrl, const char *varname,
        audio_producer *prod) {
    int i;
    if ( controller_var_exists(ctrl, varname) ) {
        log_warn(CONTROLLER, "Failed to store %s: This name is already taken.",
            varname);
        return CONTROLLER_VARNAME_IS_ALREADY_TAKEN;
    }
    for ( i=0; i<MAX_PRODUCERS; i++ ) {
        if ( ctrl->vars[i].name == NULL ) {
            ctrl->vars[i].name = strdup(varname);
            ctrl->vars[i].producer = prod;
            return SUCCESS;
        }
    }
    log_warn(CONTROLLER, "Failed to store %s: This program is limited to "
        "%d vars.", varname, MAX_PRODUCERS);
    return CONTROLLER_REACHED_MAX_NUMBER_OF_PRODUCERS;
}

error_code controller_unload_var(controller *ctrl, const char *varname) {
    int i;
    error_code r;
    audio_producer *prod = NULL;
    assert ( varname != NULL && *varname != '\0' );
    if ( ( i = get_index_of(ctrl, varname) ) == -1 ) {
        log_error(CONTROLLER, "failed to access %s: var not found.", varname);
        return CONTROLLER_VAR_NOT_FOUND;
    }
    prod = ctrl->vars[i].producer;
    free((void *) ctrl->vars[i].name);
    ctrl->vars[i].name = NULL;
    if ( ( r = unregister_input(ctrl->out, prod) ) ) {
        return r;
    }
   return free_audio_producer(prod);
}

error_code controller_play(controller *ctrl, const char *varname) {
    audio_producer *prod;
    error_code r;
    if ( ( r = get_producer(ctrl, varname, &prod) ) ) {
        return r;
    }
    if ( ( r = assert_producer_state("play", PRODUCER_READY, prod) ) ) {
        return r;
    }
    return start_playing(prod);
}

error_code controller_wait(controller *ctrl, const char *varname) {
    audio_producer *prod;
    error_code r;
    if ( ( r = get_producer(ctrl, varname, &prod) ) ) {
        return r;
    }
    if ( producer_state_is_end_of_mp3(prod) ) {
        return SUCCESS;
    }
    /* TODO: There is a race condition here: The producer might have reached
     * the end of mp3 between the line above and the line below */
    if ( ( r = assert_producer_state("wait", PRODUCER_PLAYING, prod) ) ) {
        return r;
    }
    while ( ! producer_state_is_end_of_mp3(prod) ) {
        usleep(10*1000); /* 10ms */
    }
    return SUCCESS;
}

error_code controller_pause(controller *ctrl, const char *varname) {
    audio_producer *prod;
    error_code r;
    if ( ( r = get_producer(ctrl, varname, &prod) ) ) {
        return r;
    }
    if ( ( r = assert_producer_state("pause", PRODUCER_PLAYING, prod) ) ) {
        return r;
    }
    return producer_pause(prod);
}

error_code controller_resume(controller *ctrl, const char *varname) {
    audio_producer *prod;
    error_code r;
    if ( ( r = get_producer(ctrl, varname, &prod) ) ) {
        return r;
    }
    if ( ( r = assert_producer_state("resume", PRODUCER_READY, prod) ) ) {
        return r;
    }
    return start_playing(prod);
}

/* Get the audio_producer associated with varname from ctrl->vars */
static error_code get_producer(controller *ctrl, const char *varname,
        audio_producer **prod) {
    assert ( varname != NULL && *varname != '\0' );
    int i;
    if ( ( i = get_index_of(ctrl, varname) ) == -1 ) {
        log_error(CONTROLLER, "failed to access %s: var not found.", varname);
        return CONTROLLER_VAR_NOT_FOUND;
    }
    *prod = ctrl->vars[i].producer;
    return SUCCESS;
}

/* This is a helper function to ensure that the audio_producer is in the
 * expected state. In case the check failes, an error is logged, and an
 * error code is returned. */
static error_code assert_producer_state(const char *cmd,
        enum producer_state expected_state, audio_producer *prod) {
    int result = 0;
    assert ( expected_state == PRODUCER_READY ||
             expected_state == PRODUCER_PLAYING );
    switch ( expected_state ) {
        case PRODUCER_READY:
            result = producer_state_is_ready(prod);
            break;
        case PRODUCER_PLAYING:
            result = producer_state_is_playing(prod);
            break;
        default:
            log_fatal(CONTROLLER, "Failed to check for state %s",
                producer_state_to_string(prod));
            return CONTROLLER_ILLEGAL_CONSUMER_STATE;
    }
    if ( ! result ) {
        log_error(CONTROLLER, "%s failed: file is in state %s", cmd,
            producer_state_to_string(prod));
        return CONTROLLER_ILLEGAL_CONSUMER_STATE;
    }
    return SUCCESS;
}
