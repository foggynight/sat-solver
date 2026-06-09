.PHONY: all
all:
	gcc -o sat-solver -Ofast src/*.c -Wall -Wextra -Wpedantic

.PHONY: debug
debug:
	gcc -o sat-solver -g -O0 src/*.c -Wall -Wextra -Wpedantic
