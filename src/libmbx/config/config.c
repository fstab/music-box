#include <errno.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/types.h>
#include <assert.h>
#include "libmbx/api.h"
#include "libmbx/common/log.h"
#include "libmbx/common/xmalloc.h"

/* Lines in the configuration file must not be longer than 256 bytes. */
#define CFG_FILE_MAX_LINE_LENGTH 256

struct _mbx_config {
    const char *headphones_output_device_name;
    const char *speakers_output_device_name;
    const char *mp3dir;
};

mbx_error_code mbx_config_new(mbx_config *cfg_p) {
    mbx_config cfg = (mbx_config) _mbx_xmalloc(sizeof(struct _mbx_config));
    bzero(cfg, sizeof(struct _mbx_config));
    *cfg_p = cfg;
    return MBX_SUCCESS;
}

/*
 * Each line in the config file is a key value pair, separated by a space
 * character. The following is a sample config file:
 * ----------------------------------------------------------------------------
 * # comment
 *
 * headphones alsa_output.pci-0000_00_1b.0.analog-stereo
 * speakers alsa_output.pci-0000_00_1b.0.analog-stereo
 * mp3dir /home/fabian/music/
 * ----------------------------------------------------------------------------
 */
mbx_error_code mbx_config_load_file(mbx_config cfg, const char *path) {
    char line[CFG_FILE_MAX_LINE_LENGTH];
    int linenum=0;
    mbx_error_code result = MBX_SUCCESS;
    FILE *fp = fopen(path, "r");
    if ( fp == NULL ) {
        mbx_log_warn(MBX_LOG_CONFIG, "Failed to open config file %s: %s", path,
            strerror(errno));
        return MBX_CONFIG_FILE_OPEN_FAILED;
    }
    while ( fgets(line, sizeof(line), fp) != NULL ) {
        char var[CFG_FILE_MAX_LINE_LENGTH], value[CFG_FILE_MAX_LINE_LENGTH];
        linenum++;
        if ( line[0] == '#' || line[0] == '\n' ) {
            continue;
        }
        if ( sscanf(line, "%s %s", var, value) != 2 ) {
            result = MBX_CONFIG_FILE_SYNTAX_ERROR;
        }
        if ( ! strcmp("speakers", var) ) {
            cfg->speakers_output_device_name = _mbx_xstrdup(value);
        }
        else if ( ! strcmp("headphones", var) ) {
            cfg->headphones_output_device_name = _mbx_xstrdup(value);
        }
        else if ( ! strcmp("mp3dir", var) ) {
            cfg->mp3dir = _mbx_xstrdup(value);
        }
        else {
            result = MBX_CONFIG_FILE_SYNTAX_ERROR;
        }
    }
    fclose(fp);
    return result;
}

void mbx_config_set(mbx_config cfg, mbx_config_var var, const char *value) {
    char *val = _mbx_xstrdup(value);
    switch ( var ) {
        case MBX_CFG_SPEAKERS_DEVICE:
            cfg->speakers_output_device_name = val;
            break;
        case MBX_CFG_HEADPHONES_DEVICE:
            cfg->headphones_output_device_name = val;
            break;
        case MBX_CFG_MP3DIR:
            cfg->mp3dir = val;
            break;
        default:
            assert("Unknown enum value for mbx_config_var" == NULL);
    }
}

mbx_error_code mbx_config_check(mbx_config cfg, mbx_config_var var,
        int *result) {
    DIR *mp3dir;
    switch ( var ) {
        case MBX_CFG_SPEAKERS_DEVICE:
            return mbx_output_device_exists(cfg->speakers_output_device_name,
                    result);
        case MBX_CFG_HEADPHONES_DEVICE:
            return mbx_output_device_exists(cfg->headphones_output_device_name,
                    result);
        case MBX_CFG_MP3DIR:
            *result = 0;
            if ( cfg->mp3dir != NULL ) {
                mp3dir = opendir(cfg->mp3dir);
                if ( mp3dir != NULL ) {
                    *result = 1;
                    closedir(mp3dir);
                }
            }
            return MBX_SUCCESS;
        default:
            assert("Unknown enum value for mbx_config_var" == NULL);
    }
}

const char *mbx_config_get(mbx_config cfg, mbx_config_var var) {
    switch ( var ) {
        case MBX_CFG_SPEAKERS_DEVICE:
            return cfg->speakers_output_device_name;
        case MBX_CFG_HEADPHONES_DEVICE:
            return cfg->headphones_output_device_name;
        case MBX_CFG_MP3DIR:
            return cfg->mp3dir;
        default:
            assert("Unknown enum value for mbx_config_var" == NULL);
    }
}

void mbx_config_free(mbx_config cfg) {
    _mbx_xfree((void *) cfg->speakers_output_device_name);
    _mbx_xfree((void *) cfg->headphones_output_device_name);
    _mbx_xfree((void *) cfg->mp3dir);
    bzero(cfg, sizeof(struct _mbx_config));
    _mbx_xfree(cfg);
}
