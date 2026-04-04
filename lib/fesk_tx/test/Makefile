# Makefile for fesk_tx tests

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -I.. -I../../base32
LDFLAGS =

# Directories
SRC_DIR = ..
BASE32_DIR = ../../base32
TEST_DIR = .

# Source files
FESK_TX_SRC = $(SRC_DIR)/fesk_tx.c
FESK_SESSION_SRC = $(SRC_DIR)/fesk_session.c
BASE32_SRC = $(BASE32_DIR)/base32.c
UNITY_SRC = unity.c
TEST_MAIN_SRC = test_main.c
TEST_SESSION_SRC = test_session_auto_encode.c

# Object files (shared)
FESK_TX_OBJ = fesk_tx.o
FESK_SESSION_OBJ = fesk_session.o
BASE32_OBJ = base32.o
UNITY_OBJ = unity.o

# Test-specific object files
TEST_MAIN_OBJ = test_main.o
TEST_SESSION_OBJ = test_session_auto_encode.o

# Output binaries
TEST_BIN_MAIN = test_fesk
TEST_BIN_SESSION = test_fesk_session

.PHONY: all clean test run

all: $(TEST_BIN_MAIN) $(TEST_BIN_SESSION)

# Build test_fesk (fesk_tx layer tests)
$(TEST_BIN_MAIN): $(FESK_TX_OBJ) $(UNITY_OBJ) $(TEST_MAIN_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Build test_fesk_session (session auto-encoding tests)
$(TEST_BIN_SESSION): $(FESK_TX_OBJ) $(FESK_SESSION_OBJ) $(BASE32_OBJ) $(UNITY_OBJ) $(TEST_SESSION_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile object files
$(FESK_TX_OBJ): $(FESK_TX_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(FESK_SESSION_OBJ): $(FESK_SESSION_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BASE32_OBJ): $(BASE32_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UNITY_OBJ): $(UNITY_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_MAIN_OBJ): $(TEST_MAIN_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_SESSION_OBJ): $(TEST_SESSION_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_BIN_MAIN) $(TEST_BIN_SESSION)
	@echo "Running fesk_tx tests..."
	./$(TEST_BIN_MAIN)
	@echo ""
	@echo "Running fesk_session auto-encoding tests..."
	./$(TEST_BIN_SESSION)

run: test

clean:
	rm -f $(FESK_TX_OBJ) $(FESK_SESSION_OBJ) $(BASE32_OBJ) $(UNITY_OBJ) $(TEST_MAIN_OBJ) $(TEST_SESSION_OBJ) $(TEST_BIN_MAIN) $(TEST_BIN_SESSION)
