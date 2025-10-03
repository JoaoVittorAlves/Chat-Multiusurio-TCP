CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Executáveis
all: server client

server: server.c log.c log.h
	$(CC) $(CFLAGS) -o server server.c log.c

client: client.c log.c log.h
	$(CC) $(CFLAGS) -o client client.c log.c

clean:
	rm -f server client
