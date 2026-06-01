// DA - Dynamic Array
// Copyright (C) 2026 Robert Coffey

#ifndef DA_H
#define DA_H

#include <stdlib.h>

// e.g. typedef DA(T) DA_T;
#define DA(T)                                   \
    struct {                                    \
        T *items;                               \
        size_t count;                           \
        size_t capacity;                        \
        size_t offset;                          \
    }

#define DA_FREE(xs) do { if ((xs).capacity > 0) { free(xs.items); } } while (0)

// e.g.
//   typedef DA(int) DA_int;
//   DA_int ns;
//   DA_APPEND(ns, 1);
//   DA_APPEND(ns, 2);
//   DA_APPEND(ns, 3);
#define DA_APPEND(xs, x)                                                             \
    do {                                                                             \
        if ((xs).count >= (xs).capacity) {                                           \
            if ((xs).capacity == 0) (xs).capacity = 64;                              \
            else                    (xs).capacity *= 2;                              \
            (xs).items = realloc((xs).items, (xs).capacity * sizeof(*((xs).items))); \
            if (!(xs).items) DA_error("failed to realloc items");                    \
        }                                                                            \
        (xs).items[(xs).count++] = (x);                                              \
    } while (0)

#define DA_INDEX(xs, i) (xs)[(i) + (xs).offset]
#define DA_NEXT(xs) (xs).items[(xs).offset]
#define DA_DEQUE(xs) do { (xs).offset += 1; } while (0)
#define DA_REQUE(xs) do { (xs).offset -= 1; } while (0)

#define DA_CONTAINS(xs, x, p)                           \
    do {                                                \
        *(p) = false;                                   \
        for (size_t i_ = 0; i_ < (xs).count; ++i_) {    \
            if ((xs).items[i_] == (x)) *(p) = true;     \
        }                                               \
    } while (0)

typedef DA(char) DA_char;

void DA_error(const char *error_msg);

#endif // DA_H


#ifdef DA_IMPL
#undef DA_IMPL

#include <stdio.h>

void DA_error(const char *error_msg) {
    fprintf(stderr, "DA_error: %s\n", error_msg);
    exit(1);
}

#endif
