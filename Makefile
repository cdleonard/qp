CFLAGS=-Wall -Wdeclaration-after-statement -Werror -g -I.
CC=gcc

all: check docs

.PHONY: \
	all \
	check \
	clean \
	docs \

test: test.c qp.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ \
		$(shell pkg-config --libs --cflags check)

clean:
	rm -rf test

check: test
	./test

docs:
	doxygen
