# Makefile for chirpy_tx tests

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.. -I.
LDFLAGS =

# Directories
SRC_DIR = ..
TEST_DIR = .

# Source files
CHIRPY_SRC = $(SRC_DIR)/chirpy_tx.c
UNITY_SRC = unity.c
TEST_SRC = test_main.c

# Object files
CHIRPY_OBJ = chirpy_tx.o
UNITY_OBJ = unity.o
TEST_OBJ = test_main.o

# Output binary
TEST_BIN = test_chirpy

.PHONY: all clean test run

all: $(TEST_BIN)

$(TEST_BIN): $(CHIRPY_OBJ) $(UNITY_OBJ) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(CHIRPY_OBJ): $(CHIRPY_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UNITY_OBJ): $(UNITY_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_OBJ): $(TEST_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_BIN)
	./$(TEST_BIN)

run: test

clean:
	rm -f $(CHIRPY_OBJ) $(UNITY_OBJ) $(TEST_OBJ) $(TEST_BIN)
