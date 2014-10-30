CC=cc #-DDEBUG
CFLAGS = -Wall 

all: compile

compile: main

main: main.o shell.o
	$(CC) $^ $(CFLAGS) -o $@

run: main
	./main
