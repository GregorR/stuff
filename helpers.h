#ifndef HELPERS_H
#define HELPERS_H

/* SF: safely use functions that fail with errno without pulling your hair out */
#define SF(into, func, bad, args) \
{ \
    (into) = func args; \
    if ((into) == (bad)) { \
        perror(#func); \
        exit(1); \
    } \
}

#endif
