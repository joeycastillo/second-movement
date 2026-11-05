CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Werror -g
SANITIZERS = -fsanitize=address,undefined

.PHONY: test clean

test: build/test_wumpus
	./build/test_wumpus

build/test_wumpus: test_wumpus.c movement.h ../../watch-faces/complication/wumpus_face.c ../../watch-faces/complication/wumpus_face.h
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZERS) -I. test_wumpus.c -o $@ $(LDFLAGS)

clean:
	rm -rf build
