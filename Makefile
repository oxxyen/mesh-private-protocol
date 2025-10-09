# Makefile
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2 -pthread -D_GNU_SOURCE
CXXFLAGS = -Wall -Wextra -O2 -pthread -std=c++17
LDFLAGS = -lsqlite3 -lssl -lcrypto -lsignal-protocol-c -lpthread

SRC_DIR = src
BUILD_DIR = build
BIN_NAME = mesh_server

SRCS_C = $(SRC_DIR)/server.c
SRCS_CPP = $(SRC_DIR)/main.cpp
OBJS_C = $(SRCS_C:.c=.o)
OBJS_CPP = $(SRCS_CPP:.cpp=.o)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_NAME): $(OBJS_C) $(OBJS_CPP)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: all clean

all: $(BIN_NAME)

clean:
	rm -rf $(BUILD_DIR) $(BIN_NAME)

run: $(BIN_NAME)
	./$(BIN_NAME)
