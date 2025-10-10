CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LIBS = -lpthread -lsqlite3 -lssl -lcrypto -lncurses
TARGET = p2p_messenger

SOURCES = p2p_messenger.c client.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET) *.db

install-deps:
	sudo apt-get update
	sudo apt-get install libssl-dev libsqlite3-dev libncurses5-dev

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean install-deps debug