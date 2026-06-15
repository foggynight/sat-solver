////////////////////////////////////////////////////////////////////////////////
//
//  Utility Functions
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stddef.h>

// TODO: Should this be wrapped in do/while?
#define UNREACHABLE()                           \
    assert(0 && "unreachable");                 \
    __builtin_unreachable();

void newline(void);
void error_msg(const char *msg, ...);

char *string_slice(const char *str, size_t i_start, size_t i_end);

#endif // UTIL_H
