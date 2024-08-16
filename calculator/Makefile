all: build run

build: src/main.c
	mkdir -p build
	clang -g src/main.c -o build/main

run:
	build/main

build-test: src/test.c src/main.c
	mkdir -p build
	clang -g src/test.c -o build/test

test: build-test
	build/test
