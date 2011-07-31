#ifndef MBX_DEVICE_NAME_LIST_H
#define MBX_DEVICE_NAME_LIST_H

#include "libmbx/common/mbx_errno.h"

/**
 * Create a list of available output devices.
 *
 * Creates a list of output device names that are valid values for
 * #MBX_CFG_SPEAKERS_DEVICE and #MBX_CFG_HEADPHONES_DEVICE in #mbx_config.
 *
 * @param  dev_names
 *         A pointer to an array of output device names is put here.
 *         The array is NULL terminated.
 *         You must free the array calling mbx_free_output_device_name_list()
 * @param  n_devs
 *         The number of output device names is put here.
 * @return #MBX_SUCCESS, #MBX_PULSEAUDIO_ERROR
 */
extern mbx_error_code mbx_create_output_device_name_list(char ***dev_names, size_t *n_devs);

/**
 * Free the output device name list.
 *
 * Free the output device name list created by
 * mbx_create_output_device_name_list()
 *
 * @param  output_list
 *         List of output device names created by
 *         mbx_create_output_device_name_list()
 */
extern void mbx_free_output_device_name_list(char **output_list);

/**
 * Check if an output device with that name exists.
 *
 * @param  name
 *         The device name to be checked. If name is NULL, result will
 *         be set to 0.
 * @param  result
 *         Return parameter. Will be set to 1 if an output device with
 *         that name exists. 0 otherwise.
 * @return #MBX_SUCCESS, #MBX_PULSEAUDIO_ERROR
 */
extern mbx_error_code mbx_output_device_exists(const char *name, int *result);

#endif
