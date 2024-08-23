CFLAGS = -g

# Check if DEBUG is set from command line
ifdef DEBUG
    CFLAGS += -DDEBUG
endif

all: run

# Using this as prerequisite to force rebuild
# so that we can easily rebuild the project
# with debug logs with a CLI flag.
.PHONY: force
force:

build: force
	mkdir -p build
	clang $(CFLAGS) src/main.c -o build/main

run: build
	build/main

build-test: force
	mkdir -p build
	clang $(CFLAGS) src/test.c -o build/test

test: build-test
	build/test
