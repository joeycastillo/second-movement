test:
	rm -rf build
	mkdir build
	mkdir build/objs
	gcc -c -Werror -I. -Iunity/src/ -Imock/ -Isrc/ -I../complication -I../../lib/fesk_tx testunit_counter_face.c -o build/objs/testunit_counter_face.o
	gcc -c -Werror -I. -Iunity/src/ -Imock/ -Isrc/ -I../complication -I../../lib/fesk_tx testuptime_face.c -o build/objs/testuptime_face.o
	gcc -c -Werror -I. -Iunity/src/ -Isrc/ -DTEST unity/src/unity.c -o build/objs/unity.o
	gcc -c -Werror -Wno-format-overflow -I. -Imock/ -Iunity/src/ -Isrc/ -DTEST src/unit_counter_face.c -o build/objs/unit_counter_face.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -I../complication -I../../lib/fesk_tx -DTEST ../complication/uptime_face.c -o build/objs/uptime_face.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -I../complication -I../../lib/fesk_tx -I../../lib/base32 -DTEST ../../lib/fesk_tx/fesk_session.c -o build/objs/fesk_session.o
	gcc -c -Werror -I../../lib/base32 -DTEST ../../lib/base32/base32.c -o build/objs/base32.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -DTEST mock/watch.c -o build/objs/watch.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -DTEST mock/movement.c -o build/objs/movement.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -I../complication -I../../lib/fesk_tx -DTEST mock/fesk_tx.c -o build/objs/fesk_tx.o
	gcc -c -Werror -I. -Imock/ -Iunity/src/ -Isrc/ -DTEST src/bac.c -o build/objs/bac.o
	gcc -o build/testunit_counter_face.out build/objs/testunit_counter_face.o build/objs/unity.o build/objs/unit_counter_face.o build/objs/uptime_face.o build/objs/watch.o build/objs/bac.o build/objs/movement.o build/objs/fesk_session.o build/objs/fesk_tx.o build/objs/base32.o
	gcc -o build/testuptime_face.out build/objs/testuptime_face.o build/objs/unity.o build/objs/uptime_face.o build/objs/watch.o build/objs/movement.o build/objs/fesk_session.o build/objs/fesk_tx.o build/objs/base32.o
	./build/testunit_counter_face.out
	./build/testuptime_face.out
