#include <string.h>
#include <pulse/pulseaudio.h>
#include "libmbx/common/log.h"

void log_context_state(pa_context_state_t state) {
    switch  (state) {
        case PA_CONTEXT_READY:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The connection to the pulseaudio server is "
                "established. The pulseaudio context is ready to execute "
                "operations.");
            break;
        case PA_CONTEXT_UNCONNECTED:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The pulseaudio context hasn't been connected "
                "yet.");
            break;
        case PA_CONTEXT_CONNECTING:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "A pulseaudio connection is being "
                "established.");
            break;
        case PA_CONTEXT_AUTHORIZING:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The pulseaudio client is authorizing itself "
                "to the daemon.");
            break;
        case PA_CONTEXT_SETTING_NAME:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The pulseaudio client is passing its "
                "application name to the daemon.");
            break;
        case PA_CONTEXT_FAILED:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The pulseaudio connection failed or was "
                "disconnected.");
            break;
        case PA_CONTEXT_TERMINATED:
            mbx_log_debug(MBX_LOG_AUDIO_OUTPUT, "The pulseaudio connection was terminated "
                "cleanly.");
            break;
        default:
            mbx_log_warn(MBX_LOG_AUDIO_OUTPUT, "Unknown context state: %d", state);
    }
}
