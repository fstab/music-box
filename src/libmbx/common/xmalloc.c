#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "xmalloc.h"
#include "log.h"

// return p unless out of memory
static void *unless_out_of_memory(void *p) {
    if ( p == NULL ) {
        mbx_log_fatal(MBX_LOG_XMALLOC, "Out of memory.\n");
        exit(-1);
    }
    return p;
}

void *_mbx_xmalloc(size_t size) {
    assert(size > 0);
    return unless_out_of_memory(malloc(size));
}

void *_mbx_xrealloc(void *p, size_t size) {
    assert(size > 0);
    return unless_out_of_memory(realloc(p, size));
}

char *_mbx_xstrdup(const char *s) {
    assert(s);
    return unless_out_of_memory(strdup(s));
}

void _mbx_xfree(void *p) {
    free(p);
}
