/*
   Copyright (C) 2011 Alcatel-Lucent, Inc.
   Author: Hans Christian Woithe <hans.woithe@alcatel-lucent.com>

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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <rfb/rfb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int v4l2fd = -1;
static char *v4l2str = "/dev/video0";
static unsigned v4l2width = 640;
static unsigned v4l2height = 480;
static unsigned int v4l2Bpp = 4;
static unsigned int v4l2fps = 60;
static unsigned char *v4l2buf = NULL;
static unsigned int v4l2bufsize = 0;

static int open_v4l2();
static int init_v4l2();
static int alloc_mem();
static int read_frame();
static int free_mem();
static int close_v4l2();
static double timestamp();

int main(int argc,char **argv)
{
	rfbScreenInfoPtr s;
	double t1, t2;
	double timeout;
	int nread;

	if (argc == 2)
		v4l2str = argv[1];
	printf("Using dev %s\n", v4l2str);
	if (argc == 3)
		v4l2fps = atoi(argv[2]);
	printf("Using fps %d\n", v4l2fps);

	if (open_v4l2() != 0)
		exit(EXIT_FAILURE);
	if (init_v4l2() != 0)
		exit(EXIT_FAILURE);
	if (alloc_mem() != 0)
		exit(EXIT_FAILURE);

	s = rfbGetScreen(&argc, argv, v4l2width, v4l2height, 8, 3, v4l2Bpp);
	s->desktopName = "v4l2tovnc";
	s->frameBuffer = (char *) v4l2buf;
	s->serverFormat.blueShift = 0;
	s->serverFormat.greenShift = 8;
	s->serverFormat.redShift = 16;

	timeout = 1.0 / v4l2fps;
	rfbInitServer(s);
	t1 = timestamp();
	while (rfbIsActive(s)) {
		t2 = timestamp();
		if ((t2 - t1) >= timeout) {
			nread = read_frame();
			if (nread < 0)
				break;
			rfbMarkRectAsModified(s, 0, 0, v4l2width, v4l2height);
			t1 = timestamp();
		}
		rfbProcessEvents(s, -1);
	}
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
	nread = read(v4l2fd, v4l2buf, v4l2bufsize);
	return nread;
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

static int close_v4l2()
{
	return close(v4l2fd);
}

static double timestamp()
{
	struct timeval t;
	double dt;
	gettimeofday(&t, NULL);
	dt = t.tv_sec + (t.tv_usec / 1000000.0);
	return dt;
}
