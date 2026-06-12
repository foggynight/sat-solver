#!/usr/bin/env python3

# Generate a CNF expression for testing.
# Copyright (C) 2026 Robert Coffey

import random

var_count = 200      # total number of variables
clause_size = 200    # number of variables per clause
clause_count = 200  # number of clauses

if __name__ == "__main__":
    expr = ''

    for i in range(clause_count):
        expr += '('

        if random.choice([True, False]): expr += '-'
        expr += str(random.randint(1, var_count))

        for j in range(clause_size - 1):
            expr += '+'
            if random.choice([True, False]): expr += '-'
            expr += str(random.randint(1, var_count))

        expr += ')'

    print(expr)
