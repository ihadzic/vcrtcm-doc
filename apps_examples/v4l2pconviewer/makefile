default: all

.PHONY: all clean

OBJ = v4l2pconviewer.o

CC = gcc
COMOPTS = -g -Wall
CCOPTS = $(COMOPTS) -I.
LDOPTS = $(COMOPTS) -L/usr/X11R6/lib -lX11 -lXext

OPTS = -I. -g -Wall -Werror

all: v4l2pconviewer

v4l2pconviewer:  $(OBJ)
	$(CC) $(LDOPTS) -o v4l2pconviewer $(OBJ)

clean:  
	rm -f v4l2pconviewer
	rm -f $(OBJ)

