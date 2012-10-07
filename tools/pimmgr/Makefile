CC=gcc
CFLAGS=-g

all: build

build: magic.txt
	$(CC) $(CFLAGS) pimmgr.c -o pimmgr

install:
	cp -v pimmgr /usr/local/bin/

magic.txt:
	ln -s /lib/modules/`uname -r`/source/drivers/gpu/drm/pimmgr/pimmgr_ioctl.h
	@echo 'DO NOT REMOVE THIS FILE' > magic.txt
	@chmod a-wx magic.txt
	@chmod a+r magic.txt

clobber: clean
	rm -f magic.txt
	rm -f pimmgr_ioctl.h

clean:
	rm -f *.o pimmgr
