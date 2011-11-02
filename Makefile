CC = gcc
INCLUDES = -I../
CFLAGS = -Wall -O2 $(INCLUDES)

SRC1 = radeon_vcrtcm_ctl.c
TARGET1 = radeon_vcrtcm_ctl
OBJS1=$(SRC1:.c=.o)

# nothing for now under SRC2, add second app later (if desired)
# we used to have one, but now this is legacy but I am not
# ripping the makefile infrastructure now only to reinstate it later
SRC2 = 
TARGET2 = 
OBJS2=$(SRC2:.c=.o)

SRC = $(SRC1) $(SRC2)
TARGET = $(TARGET1) $(TARGET2)
OBJS = $(OBJS1) $(OBJS2)

all: depend.make $(TARGET)

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2)

depend.make: magic.txt $(SRC)
	$(CC) -c -MM $(INCLUDES) $(SRC) > depend.make

magic.txt:
	ln -s /lib/modules/`uname -r`/source/drivers/gpu/drm/radeon/radeon_vcrtcm.h
	@echo 'DO NOT REMOVE THIS FILE' > magic.txt
	@chmod a-wx magic.txt
	@chmod a+r magic.txt

clean:
	rm -f $(OBJS) *.bak *~ $(TARGET)

clobber: clean
	rm -f depend.make
	rm -f magic.txt
	rm -f radeon_vcrtcm.h
force:

-include depend.make
