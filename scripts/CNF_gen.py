#!/usr/bin/env python3

# Generate a CNF expression for testing.
# Copyright (C) 2026 Robert Coffey

import random

var_names = (
    [chr(ord('A') + i) for i in range(26)]
    + [chr(ord('a') + i) for i in range(26)]
)

term_count = 100
term_size  = 100

if __name__ == "__main__":
    expr = ''

    for i in range(term_count):
        expr += '('

        if random.choice([True, False]):
            expr += '-'
        expr += var_names[random.randint(0, len(var_names) - 1)]

        for j in range(term_size - 1):
            expr += '+'
            if random.choice([True, False]):
                expr += '-'
            expr += var_names[random.randint(0, len(var_names) - 1)]

        expr += ')'

    print(expr)
