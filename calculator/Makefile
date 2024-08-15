all: build run

build: main.c
	mkdir -p build
	clang main.c -o build/main

run:
	build/main

test: build
	build/main test

# test:
# 	clang -o test test.c
# 	./test
