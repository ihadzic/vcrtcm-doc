default: all
-include *.d

CC = gcc
CFLAGS = -MD -Wall
KERNEL_DIR = $(shell echo /lib/modules/`uname -r`/source/include/drm/vcrtcm)
HDR = vcrtcm_ioctl.h

all: vcrtcm

vcrtcm: vcrtcm.c $(HDR)
	$(CC) $(CFLAGS) vcrtcm.c -o vcrtcm

$(HDR): $(KERNEL_DIR)/$(HDR)
	ln -s $(KERNEL_DIR)/$(HDR)

install:
	sudo install vcrtcm /usr/local/bin
	sudo rm -f /usr/local/bin/pimmgr
	sudo rm -f /usr/local/bin/radeon_vcrtcm_ctl

clean:
	rm -f *.o pimmgr vcrtcm $(HDR) magic.txt *.d
