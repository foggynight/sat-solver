.PHONY: all
all:
	gcc -o sat-solver -Ofast src/main.c -Wall -Wextra -Wpedantic

.PHONY: debug
debug:
	gcc -o sat-solver -g -O0 src/main.c -Wall -Wextra -Wpedantic
