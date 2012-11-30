CC = gcc
INCLUDES = -I/usr/include/libdrm
CFLAGS = -Wall -O2 $(INCLUDES)

SRC = gpu_resource_dump.c
TARGET = gpu_resource_dump
OBJS=$(SRC:.c=.o)

all: depend.make $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -ldrm

install:
	sudo cp -v $(TARGET) /usr/local/bin

depend.make: $(SRC)
	$(CC) -c -MM $(INCLUDES) $(SRC) > depend.make

clean:
	rm -f $(OBJS) *.bak *~ $(TARGET)

clobber: clean
	rm -f depend.make
	rm -f radeon_vcrtcm.h
force:

-include depend.make
