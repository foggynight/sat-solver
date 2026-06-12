#include "util.h"

#include <stdlib.h>
#include <string.h>

char *string_slice(const char *str, size_t i_start, size_t i_end) {
    char *copy = malloc(i_end - i_start + 1);
    if (!copy) { return NULL; }
    memcpy(copy, str + i_start, i_end - i_start);
    copy[i_end - i_start] = '\0';
    return copy;
}
