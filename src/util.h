#ifndef UTIL_H
#define UTIL_H

char *string_slice(const char *str, size_t i_start, size_t i_end);

#endif // UTIL_H


#ifdef UTIL_IMPL
#undef UTIL_IMPL

#include <stdlib.h>

char *string_slice(const char *str, size_t i_start, size_t i_end) {
    char *copy = malloc(i_end - i_start);
    if (!copy) { return NULL; }
    for (size_t i = i_start; i < i_end; ++i) {
        copy[i] = str[i];
    }
    return copy;
}

#endif // UTIL_IMPL
