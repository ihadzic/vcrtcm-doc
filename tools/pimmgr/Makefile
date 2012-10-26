default: all

CC=gcc
CFLAGS=
KERNEL_DIR = $(shell echo /lib/modules/`uname -r`/source/drivers/gpu/drm/vcrtcm)
HDR = vcrtcm_ioctl.h

all: pimmgr

pimmgr: pimmgr.c $(HDR)
	$(CC) $(CFLAGS) pimmgr.c -o pimmgr

$(HDR): $(KERNEL_DIR)/$(HDR)
	ln -s $(KERNEL_DIR)/$(HDR)

install:
	sudo install pimmgr /usr/local/bin

clean:
	rm -f *.o pimmgr $(HDR) magic.txt
