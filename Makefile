CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

UTILS_OBJ = obj/utils.o
CLIENT_OBJ = obj/tftp-client.o $(UTILS_OBJ)
SERVER_OBJ = obj/tftp-server.o $(UTILS_OBJ)

CLIENT_BIN = bin/tftp-client
SERVER_BIN = bin/tftp-server

ROOT_DIR = root_dir/*.txt
CLIENT_DIR = client_dir/*.txt

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN) $(CLIENT_OBJ) $(SERVER_OBJ) $(UTILS_OBJ)