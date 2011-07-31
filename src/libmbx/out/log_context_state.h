#ifndef LOG_CONTEXT_STATE
#define LOG_CONTEXT_STATE

/* Write a debug message with the current pulseaudio context state */
extern void _mbx_log_context_state(pa_context_state_t state);

#endif
