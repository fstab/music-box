#ifndef MBX_CONFIG_H
#define MBX_CONFIG_H

/**
 * Data structure holding the music box configuration.
 */
typedef struct _mbx_config *mbx_config;

/**
 * Enumeration of the configuration variables that are supported by #mbx_config.
 */
typedef enum {
    /**
     * The name of the pulseaudio sink that will be used as the output device
     * for the headphones. You can use mbx_create_output_device_name_list()
     * to get a list of available devices.
     */
    MBX_CFG_HEADPHONES_DEVICE,
    /**
     * The name of the pulseaudio sink that will be used as the output device
     * for the speakers. You can use mbx_create_output_device_name_list()
     * to get a list of available devices.
     */
    MBX_CFG_SPEAKERS_DEVICE,
    /**
     * The path to the directory that contains the mp3 files.
     */
    MBX_CFG_MP3DIR
} mbx_config_var;

/**
 * Allocate a new mbx_config.
 *
 * Allocates a new, empty #mbx_config. A pointer to the newly allocated
 * #mbx_config is put in <tt>*cfg_p</tt>. The #mbx_config must be freed with
 * mbx_config_free().
 *
 * @param  cfg_p
 *         A pointer to the newly allocated mbx_config will be put in
 *         <tt>*cfg_p</tt>.
 *
 * @return #MBX_SUCCESS
 */
extern mbx_error_code mbx_config_new(mbx_config *cfg_p);

/**
 * Load a configuration file.
 *
 * Each line in the config file is a key value pair, separated by a space
 * character. A sample config file looks as follows:
 *
 * @verbatim

# This is a sample config file for music box

headphones alsa_output.pci-0000_00_1b.0.analog-stereo
speakers alsa_output.pci-0000_00_1b.0.analog-stereo
mp3dir /home/fabian/music/

   @endverbatim
 *
 * The lines in the config file must not be longer than 256 chars.
 *
 * @param  cfg
 *         The parameters read from the file are stored in <tt>cfg</tt>.
 *         Current parameters in <tt>cfg</tt> will be overwritten.
 * @param  path
 *         path to the configuration file.
 *
 * @return #MBX_SUCCESS,
 *         #MBX_CONFIG_FILE_OPEN_FAILED,
 *         #MBX_CONFIG_FILE_SYNTAX_ERROR
 */
extern mbx_error_code mbx_config_load_file(mbx_config cfg, const char *path);

/**
 * Set a configuration parameter.
 *
 * @param  cfg
 *         A copy of <tt>value</tt> will be stored in <tt>cfg</tt>.
 * @param  var
 *         The variable to be set, see #mbx_config_var for supported variables.
 * @param  value
 *         The value to be stored. The value will be copied.
 */
extern void mbx_config_set(mbx_config cfg, mbx_config_var var,
        const char *value);

/**
 * Check if a configuration parameter has a reasonable value.
 *
 * <ul>
 * <li>If <tt>var</tt> is #MBX_CFG_HEADPHONES_DEVICE or
 *     #MBX_CFG_SPEAKERS_DEVICE, the function checks if the device exists.
 * <li>If <tt>var</tt> is #MBX_CFG_MP3DIR, the function checks if the
 *     directory exists and can be opened.
 * </ul>
 *
 * @param  cfg
 *         The configuration where the variable is stored.
 * @param  var
 *         The variable to be checked.
 * @param  result
 *         Will be set to <tt>1</tt> if the value is ok, or <tt>0</tt>
 *         if the value is not ok.
 * @return #MBX_SUCCESS, #MBX_PULSEAUDIO_ERROR
 */
extern mbx_error_code mbx_config_check(mbx_config cfg, mbx_config_var var,
        int *result);

/**
 * Get the current value of a configuration variable.
 *
 * The value returned is a pointer to the internal value stored in <tt>cfg</tt>.
 * You must _not_ free it.
 *
 * If the value has never been set, NULL is returned.
 *
 * @param  cfg
 *         The configuration where the value is stored.
 * @param  var
 *         The requested variable
 * @return The string value of the configuration variable, or NULL if the
 *         variable is not configured. The return value is a pointer to
 *         internal data structure of <tt>cfg</tt>. You must not free it or
 *         modify the string's content.
 */
extern const char *mbx_config_get(mbx_config cfg, mbx_config_var var);

/**
 * Free the configuration.
 *
 * @param  cfg
 *         The configuration to be freed.
 */
extern void mbx_config_free(mbx_config cfg);

#endif
