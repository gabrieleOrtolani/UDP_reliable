CC = gcc
CFLAGS = -Wall -I.


all:
	make server
	make client



server: server.c socket_list.c reliable.c
	$(CC) $(CFLAGS) -o server server.c socket_list.c reliable.c -lpthread -lm
client: client.c socket_list.c reliable.c
	$(CC) $(CFLAGS) -o client client.c socket_list.c reliable.c -lpthread -lm


clean:
	rm -f server
	rm -f client

.PHONY: clean
