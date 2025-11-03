# Makefile for fesk_tx tests

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -I..
LDFLAGS =

# Directories
SRC_DIR = ..
TEST_DIR = .

# Source files
FESK_SRC = $(SRC_DIR)/fesk_tx.c
UNITY_SRC = unity.c
TEST_SRC = test_main.c

# Object files
FESK_OBJ = fesk_tx.o
UNITY_OBJ = unity.o
TEST_OBJ = test_main.o

# Output binary
TEST_BIN = test_fesk

.PHONY: all clean test run

all: $(TEST_BIN)

$(TEST_BIN): $(FESK_OBJ) $(UNITY_OBJ) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(FESK_OBJ): $(FESK_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UNITY_OBJ): $(UNITY_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_OBJ): $(TEST_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_BIN)
	./$(TEST_BIN)

run: test

clean:
	rm -f $(FESK_OBJ) $(UNITY_OBJ) $(TEST_OBJ) $(TEST_BIN)
