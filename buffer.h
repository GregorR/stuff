#ifndef BUFFER_H
#define BUFFER_H

#include "helpers.h"

#define BUFFER_DEFAULT_SIZE 1024

/* auto-expanding buffer */
#define BUFFER(name, type) \
struct Buffer_ ## name { \
    size_t bufsz, bufused; \
    type *buf; \
}

BUFFER(char, char);
BUFFER(int, int);

/* initialize a buffer */
#define INIT_BUFFER(buffer) \
{ \
    (buffer).bufsz = BUFFER_DEFAULT_SIZE; \
    (buffer).bufused = 0; \
    SF((buffer).buf, malloc, NULL, (BUFFER_DEFAULT_SIZE * sizeof(*(buffer).buf))); \
}

/* free a buffer */
#define FREE_BUFFER(buffer) \
{ \
    if ((buffer).buf) free((buffer).buf); \
    (buffer).buf = NULL; \
}

/* the address of the free space in the buffer */
#define BUFFER_END(buffer) ((buffer).buf + (buffer).bufused)

/* mark new bytes in a buffer */
#define STEP_BUFFER(buffer, by) ((buffer).bufused += by)

/* the amount of space left in the buffer */
#define BUFFER_SPACE(buffer) ((buffer).bufsz - (buffer).bufused)

/* expand a buffer */
#define EXPAND_BUFFER(buffer) \
{ \
    (buffer).bufsz *= 2; \
    SF((buffer).buf, realloc, NULL, ((buffer).buf, (buffer).bufsz * sizeof(*(buffer).buf))); \
}

/* write a string to a buffer */
#define WRITE_BUFFER(buffer, string, len) \
{ \
    size_t _len = (len); \
    while (BUFFER_SPACE(buffer) < _len) { \
        EXPAND_BUFFER(buffer); \
    } \
    memcpy(BUFFER_END(buffer), string, _len * sizeof(*(buffer).buf)); \
    STEP_BUFFER(buffer, _len); \
}

#endif
