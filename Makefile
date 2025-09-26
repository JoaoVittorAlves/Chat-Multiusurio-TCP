CC = gcc
CFLAGS = -Wall -pthread

all: main

main: main.c log.c log.h
	$(CC) $(CFLAGS) main.c log.c -o main

clean:
	rm -f main
