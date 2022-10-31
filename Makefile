CFLAGS=-Wall -Wdeclaration-after-statement -Werror -g -I.
CC=gcc

all: check docs

.PHONY: \
	all \
	check \
	docs \

test: test.c qp.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ \
		$(shell pkg-config --libs --cflags check)

check: test
	./test

docs:
	doxygen

KDIR_CURRENT=/lib/modules/`uname -r`/build
qp_kmod_test__current.ko: qp_kmod_test.c Kbuild
	[ -d $(KDIR_CURRENT) ]
	$(MAKE) -C $(KDIR_CURRENT) M=$(PWD) modules
	cp qp_kmod_test.ko $@

KDIR_UBUNTU_GENERIC=/usr/src/`dpkg-query --showformat='$${Depends}' --show linux-headers-generic`
qp_kmod_test__ubuntu_generic.ko: qp_kmod_test.c Kbuild
	[ -d $(KDIR_UBUNTU_GENERIC) ]
	$(MAKE) -C $(KDIR_UBUNTU_GENERIC) M=$(PWD) modules
	cp qp_kmod_test.ko $@
