default: all
-include *.d

CC = gcc
CFLAGS = -MD
KERNEL_DIR = $(shell echo /lib/modules/`uname -r`/source/include/drm/vcrtcm)
HDR = vcrtcm_ioctl.h

all: vcrtcm

vcrtcm: vcrtcm.c $(HDR)
	$(CC) $(CFLAGS) vcrtcm.c -o vcrtcm

$(HDR): $(KERNEL_DIR)/$(HDR)
	ln -s $(KERNEL_DIR)/$(HDR)

install:
	sudo install vcrtcm /usr/local/bin

clean:
	rm -f *.o pimmgr vcrtcm $(HDR) magic.txt
