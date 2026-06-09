#include "DA.h"

#include <stdio.h>

void DA_error(const char *error_msg) {
    fprintf(stderr, "DA_error: %s\n", error_msg);
    exit(1);
}
