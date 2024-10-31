CFLAGS = -Wall -g
ARGS ?=

all: run

.PHONY: force
force:

build: force
	mkdir -p build
	clang $(CFLAGS) main.c -o build/main

run: build
	build/main
