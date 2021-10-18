CFLAGS=-Wall -g -I.
CC=gcc

all: check

.PHONY: \
	all \
	clean \
	check \

show-config:
	@echo "HAVE_LIBCHECK=$(HAVE_LIBCHECK)"
	@echo "HAVE_LIBNL=$(HAVE_LIBNL)"
	@echo "HAVE_PKG_CONFIG=$(HAVE_PKG_CONFIG)"
	@echo "HAVE_NPM=$(HAVE_NPM)"

test: test.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ \
		$(shell pkg-config --libs --cflags check)

clean:
	rm -rf test

check: test
	./test
