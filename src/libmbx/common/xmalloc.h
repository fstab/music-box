#ifndef MBX_XMALLOC
#define MBX_XMALLOC

#include <stddef.h>

extern void *_mbx_xmalloc(size_t size);
extern void *_mbx_xrealloc(void *p, size_t size);
extern char *_mbx_xstrdup(const char *s);
extern void _mbx_xfree(void *p);

#endif
