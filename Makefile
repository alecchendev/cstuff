all: build run

build:
	clang -o main main.c

run:
	./main
