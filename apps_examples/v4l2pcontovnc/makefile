default: all

.PHONY: all clean

CC = gcc
CCOPTS = $(COMOPTS) -g -Wall -I. -lvncserver
LDOPTS = $(COMOPTS) 


all: v4l2tovnc

v4l2tovnc:  $(OBJ)
	$(CC) $(CCOPTS) $(LDOPTS) -o v4l2tovnc v4l2tovnc.c

clean:  
	rm -f v4l2tovnc
