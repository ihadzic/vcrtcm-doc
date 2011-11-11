default: all

.PHONY: all clean

OBJ = v4l2ctdviewer.o

CC = gcc
COMOPTS = -g -Wall
CCOPTS = $(COMOPTS) -I.
LDOPTS = $(COMOPTS) -L/usr/X11R6/lib -lX11 -lXext

OPTS = -I. -g -Wall -Werror

all: v4l2ctdviewer

v4l2ctdviewer:  $(OBJ)
	$(CC) $(LDOPTS) -o v4l2ctdviewer $(OBJ)

clean:  
	rm -f v4l2ctdviewer
	rm -f $(OBJ)

