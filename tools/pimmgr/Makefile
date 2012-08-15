CC=gcc
CFLAGS=-g

all: build

build:
	$(CC) $(CFLAGS) pimmgr.c -o pimmgr

install:
	cp -v pimmgr /usr/local/bin/

clean:
	rm -rvf *.o pimmgr
