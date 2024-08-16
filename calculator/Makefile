all: build run

build: main.c
	mkdir -p build
	clang -g main.c -o build/main

run:
	build/main

build-test: test.c main.c
	mkdir -p build
	clang -g test.c -o build/test

test: build-test
	build/test
