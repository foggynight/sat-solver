#!/usr/bin/env lua

-- Generate a CNF expression for testing.
-- Copyright (C) 2026 Robert Coffey

var_count    = 30  -- total number of variables
clause_size  = 20  -- number of variables per clause
clause_count = 20  -- number of clauses

for i = 1, clause_count do
    io.write("(")

    if math.random(0, 1) == 0 then io.write("-") end
    io.write(math.random(1, var_count))

    for j = 1, clause_size - 1 do
       io.write("+")
       if math.random(0, 1) == 0 then io.write("-") end
       io.write(math.random(1, var_count))
    end

    io.write(")")
end

print()
