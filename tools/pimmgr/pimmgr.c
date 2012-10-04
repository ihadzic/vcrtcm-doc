/*
 * Copyright (C) 2012 Alcatel-Lucent, Inc.
 * Author: Bill Katsak <william.katsak@alcatel-lucent.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "pimmgr.h"
#include <ftw.h>
#include <errno.h>

#define MAX_COMMAND_LEN 35
#define MAX_ARGHELP_LEN 128
#define MAX_DESCRIPTION_LEN 1024

/* Operation implementations */
int do_instantiate(int argc, char **argv);
int do_destroy(int argc, char **argv);
int do_info(int argc, char **argv);
int do_help(int argc, char **argv);

struct operation {
	char command[MAX_COMMAND_LEN];
	int argc;
	char arghelp[MAX_ARGHELP_LEN];
	char description[MAX_DESCRIPTION_LEN];
	int (*func)(int argc, char **argv);
};

struct operation ops[] = {
	{
		.command = "instantiate",
		.argc = 1,
		.arghelp = "<pim_name>",
		.description = "Create a new PCON instance of the PIM identified by pim_name.",
		.func = do_instantiate,
	},

	{
		.command = "destroy",
		.argc = 1,
		.arghelp = "<pconid>",
		.description = "Destroy the PCON instance identified by pconid.",
		.func = do_destroy,
	},

	{
		.command = "info",
		.argc = 0,
		.arghelp = "",
		.description = "List all available PIMs and their associated PCON instances.",
		.func = do_info,
	},

	{
		.command = "help",
		.argc = 0,
		.arghelp = "",
		.description = "Display this message.",
		.func = do_help,
	}
};

static int operation_count = sizeof(ops) / sizeof(struct operation);
static int found_pim = 0;
static int found_pcon = 0;

void print_usage(char *command)
{
	printf("usage: %s <command> [args]\n", command);
	printf("type \"%s help\" for more help\n", command);
}

int open_pimmgr_device(void)
{
	int fd = open(PIMMGR_DEVICE, O_WRONLY);

	if (fd < 0) {
		fprintf(stderr, "error: cannot open %s: %s\n", PIMMGR_DEVICE, strerror(errno));
		exit(-1);
	}

	return fd;
}

int do_instantiate(int argc, char **argv)
{
	struct pimmgr_ioctl_args args;
	int fd = 0;
	long result = 0;
	char *type = argv[0];

	fd = open_pimmgr_device();

	strncpy(args.arg1.pim_name, type, PIM_NAME_MAXLEN);
	args.arg2.hints = 0;

	result = ioctl(fd, PIMMGR_IOC_INSTANTIATE, &args);

	if (errno) {
		switch (errno) {
			case EINVAL:
				fprintf(stderr, "error: invalid pim identifier \"%s\"\n", type); break;
			case ENODEV:
				fprintf(stderr, "error: no available pcons of type \"%s\"\n", type); break;
			case EBUSY:
				fprintf(stderr, "error: unable to register pcon with vcrtcm\n"); break;
			case ENOMEM:
				fprintf(stderr, "error: out of memory while instantiating pcon\n"); break;
			case EMFILE:
				fprintf(stderr, "error: the system is out of pconids\n"); break;
			default:
				fprintf(stderr, "non-pimmgr error: %s\n", strerror(errno));
		}
		return 1;
	}

	printf("%u\n", args.result1.pconid);

	return 0;
}

int do_destroy(int argc, char **argv)
{
	struct pimmgr_ioctl_args args;
	int fd = 0;
	long result = 0;

	fd = open_pimmgr_device();

	args.arg1.pconid = (uint32_t) atoll(argv[0]);

	result = ioctl(fd, PIMMGR_IOC_DESTROY, &args);
	if (errno) {
		switch (errno) {
			case EINVAL:
				fprintf(stderr, "error: invalid pconid (%i)\n", args.arg1.pconid); break;
			default:
				fprintf(stderr, "non-pimmgr error: %s\n", strerror(errno));
		}
		return 1;
	}

	printf("destroyed pcon with id %u\n", args.arg1.pconid);
	return 0;
}

int sysfs_read_file(const char *base_path, const char *file, char *contents)
{
	int size = 0;
	char path[512];
	FILE *f = NULL;

	snprintf(path, 512, "%s/%s", base_path, file);
	f = fopen(path, "r");

	if (f) {
		size = fread(contents, sizeof(char), 4096, f);
		if (size && contents[size-1] == '\n')
			contents[size-1] = '\0';
	}
	return size;
}

int sysfs_find_pcons(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	static char contents[4096];
	
	if (ftwbuf->level == 1 && typeflag == FTW_D) {
		int result = 0;
		int attached = 0;

		printf(" --> PCON: %s\n", fpath + ftwbuf->base);
		found_pcon = 1;
		sysfs_read_file(fpath, "description", contents);
		printf("       -> %s\n", contents);

		result = sysfs_read_file(fpath, "attached", contents);
		if (!result) {
			printf("       -> properties not implemented\n");
			return FTW_CONTINUE;
		}
		attached = atoi(contents);
		if (attached) {
			sysfs_read_file(fpath, "fps", contents);
			printf("       -> attached, FPS = %s\n", contents);
		} else {
			printf("       -> unattached\n");
		}
	}

	return FTW_CONTINUE;
}

int sysfs_find_pims(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	if (ftwbuf->level == 1) {
		printf("PIM: %s\n", fpath + ftwbuf->base);
		found_pim = 1;
		found_pcon = 0;
		nftw(fpath, sysfs_find_pcons, 32, FTW_ACTIONRETVAL);
		if (!found_pcon)
			printf(" --> no pcons found\n");
	}

	return FTW_CONTINUE;
}

int do_info(int argc, char **argv)
{
	nftw(PIMMGR_SYSFS_PIM_PATH, sysfs_find_pims, 32, FTW_ACTIONRETVAL);
	if (!found_pim)
		printf("no pims found\n");
	return 0;
}

int do_help(int argc, char **argv)
{
	int i;

	printf("available commands:\n");
	for (i=0; i<operation_count; i++) {
		printf("\t%s %s\n", ops[i].command, ops[i].arghelp);
		printf("\t\t- %s\n", ops[i].description);
	}

	return 0;
}

int validate_args(struct operation *op, int argc)
{
	if (!(op->argc <= argc)) {
		printf("error: missing arguments\n");
		exit(0);
	}

	return 1;
}

int main(int argc, char **argv)
{
	char *operation;
	int new_argc = argc - 2;
	char **new_argv = argv + 2;
	int i = 0;
	struct operation *op = NULL;

	if (argc < 2) {
		print_usage(argv[0]);
		return 0;
	}

	operation = argv[1];

	for (i=0; i<operation_count; i++)
	{
		if (strncmp(operation, ops[i].command, strlen(operation)) == 0 &&
		    validate_args(&ops[i], new_argc)) {
			return ops[i].func(new_argc, new_argv);
		}
	}

	fprintf(stderr, "error: bad command\n");
	return 0;
}

