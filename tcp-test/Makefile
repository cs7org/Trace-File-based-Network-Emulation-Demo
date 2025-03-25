CC = gcc
CFLAGS = -lpthread -O1 -Wall -Werror

all: server client

client: client.o common.o
	$(CC) $(CC_FLAGS) -o $@ $^

server: server.o common.o
	$(CC) $(CC_FLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o client server
