/*
   Copyright (C) 2011 Alcatel-Lucent, Inc.
   Author: Hans Christian Woithe <Hans.Woithe@alcatel-lucent.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include <linux/videodev2.h>

#define TIMEOUT_USEC 100

static int v4l2fd = -1;
static char *v4l2str = "/dev/video0";
static unsigned int v4l2width = 640;
static unsigned int v4l2height = 480;
static unsigned char *v4l2buf = NULL;
static int v4l2bufsize = 0;

static int open_v4l2();
static int init_v4l2();
static int alloc_mem();
static int read_frame();
static int close_v4l2();
static int free_mem();

int main(int argc, char **argv)
{
	Display *d;
	Window w;
	XEvent e;
	GC gc;
	Visual *v;
	XImage *img;
	XWindowAttributes wa;
	long timeout;
	int black;
	int dpth;
	unsigned char *imgdata;
	int imgdatasize;
	int nread;

	if (argc == 2)
		v4l2str = argv[1];
	printf("Using dev %s\n", v4l2str);

	if (open_v4l2() != 0)
		exit(EXIT_FAILURE);
	if (init_v4l2() != 0)
		exit(EXIT_FAILURE);
	if (alloc_mem() != 0)
		exit(EXIT_FAILURE);

	d = XOpenDisplay(0);
	if (!d)
		exit(EXIT_FAILURE);
	black = BlackPixel(d, DefaultScreen(d));
	w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0,
				v4l2width, v4l2height, 0,
				black, black);
	XSelectInput(d, w, StructureNotifyMask);
	XStoreName(d, w, "v4l2pcon viewer");
	XMapRaised(d, w);
	gc = XCreateGC(d, w, 0, NULL);
	XGetWindowAttributes(d, w, &wa);
	v = wa.visual;
	dpth = wa.depth;
	if (dpth != 24)
		exit(EXIT_FAILURE);

	img = XCreateImage(d, v, dpth, ZPixmap, 0, v4l2buf, v4l2width,
				v4l2height, 16, 0);
	if (!img)
		exit(EXIT_FAILURE);

	for (;;) {
		nread = read_frame();
		if (nread > 0) {
			XPutImage(d, w, gc, img, 0, 0, 0, 0,
					v4l2width, v4l2height);
		}
		if (XPending(d) > 0)
			XNextEvent(d, &e);
		usleep(TIMEOUT_USEC);
	}

	free_mem();
	close_v4l2();
	return 0;
}

static int open_v4l2()
{
	struct stat s;
	int res;

	res = stat(v4l2str, &s);
	if (res != 0)
		return 1;
	if (!S_ISCHR(s.st_mode))
		return 2;
	v4l2fd = open(v4l2str, O_RDWR, 0);
	if (v4l2fd == -1)
		return 3;
	return 0;
}

static int init_v4l2()
{
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	int res;

	res = ioctl(v4l2fd, VIDIOC_QUERYCAP, &cap);
	if (res == -1)
		return 1;
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
		return 2;
	if (!(cap.capabilities & V4L2_CAP_READWRITE))
		return 3;

	memset(&fmt, '\0', sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
	fmt.fmt.pix.width = v4l2width;
	fmt.fmt.pix.height = v4l2height;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	res = ioctl(v4l2fd, VIDIOC_S_FMT, &fmt);
	if (res == -1)
		return 4;
	v4l2width = fmt.fmt.pix.width;
	v4l2height = fmt.fmt.pix.height;
	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR32)
		return 5;
	return 0;
}

static int alloc_mem()
{
	free_mem();
	v4l2bufsize = v4l2width * v4l2height * 4;
	v4l2buf = (unsigned char *) calloc(1, v4l2bufsize);
	if (!v4l2buf) {
		v4l2bufsize = 0;
		return 1;
	}
	return 0;
}

static int read_frame()
{
	ssize_t nread;
	struct timeval t;
	fd_set fds;
	int res;

	FD_ZERO(&fds);
	FD_SET(v4l2fd, &fds);
	t.tv_sec = 0;
	t.tv_usec = TIMEOUT_USEC;

	res = select(v4l2fd + 1, &fds, NULL, NULL, &t);
	if (res <= 0)
		return res;
	return nread = read(v4l2fd, v4l2buf, v4l2bufsize);
}

static int close_v4l2()
{
	return close(v4l2fd);
}

static int free_mem()
{
	if (v4l2buf) {
		free(v4l2buf);
		v4l2buf = NULL;
		v4l2bufsize = 0;
	}
	return 0;
}
