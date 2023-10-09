CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

CLIENT_OBJ = obj/tftp-client.o
SERVER_OBJ = obj/tftp-server.o
UTILS_OBJ = obj/utils.o

CLIENT_BIN = bin/tftp-client
SERVER_BIN = bin/tftp-server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN) $(CLIENT_OBJ) $(SERVER_OBJ) $(UTILS_OBJ)