CC = gcc
CFLAGS = -W -I.


all:
	make server
	make client



server: server.c socket_list.c reliable2.c
	$(CC) $(CFLAGS) -o server server.c socket_list.c reliable2.c -lpthread -lm
client: client.c socket_list.c reliable2.c
	$(CC) $(CFLAGS) -o client client.c socket_list.c reliable2.c -lpthread -lm


clean:
	rm -f server
	rm -f client

.PHONY: clean
