CFLAGS=-Wall -g -I.
CC=gcc

all: check

.PHONY: \
	all \
	clean \
	check \

test: test.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ \
		$(shell pkg-config --libs --cflags check)

clean:
	rm -rf test

check: test
	./test
