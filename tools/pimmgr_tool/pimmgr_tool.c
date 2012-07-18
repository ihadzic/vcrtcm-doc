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
#include "pimmgr_tool.h"
#include <ftw.h>

#define MAX_COMMAND_LEN 35

/* Operation implementations */
int do_instantiate(int argc, char **argv);
int do_destroy(int argc, char **argv);
int do_info(int argc, char **argv);

struct operation {
	char command[MAX_COMMAND_LEN];
	int argc;
	int (*func)(int argc, char **argv);
	
};

struct operation ops[] = {
	{
		.command = "instantiate",
		.argc = 1,
		.func = do_instantiate,
	},
	
	{
		.command = "destroy",
		.argc = 1,
		.func = do_destroy,
	},
	
	{
		.command = "info",
		.argc = 0,
		.func = do_info,
	}
};

static int operation_count = sizeof(ops) / sizeof(struct operation);

void print_usage(char *command)
{
	int i=0;
	int j=0;
	
	printf("Usage: %s [command] {args}\n\n", command);
	printf("Supported Commands:\n");
	
	for (i=0; i<operation_count; i++) {
		printf("\t%s", ops[i].command);
		for (j=0; j<ops[i].argc; j++)
			printf(" arg%i", j);
		printf("\n");
	}
}

int open_pimmgr_device(void)
{
	int fd = open(PIMMGR_DEVICE, O_RDONLY);
	
	if (fd < 0) {
		printf("Cannot open pimmgr file, exiting...\n");
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
	
	strncpy(args.arg1.pim_name, type, PIM_NAME_LEN);
	args.arg2.hints = 0;
	
	result = ioctl(fd, PIMMGR_IOC_INSTANTIATE, &args);
	
	printf("IOCTL result %li\n", result);
	
	if (IOCTL_RESULT_IS_ERR(result)) {
		if (result == PIMMGR_ERR_INVALID_PIM)
			printf("Invalid pim identifier.\n");
		else if (result == PIMMGR_ERR_NOT_AVAILABLE)
			printf("No pims of that type are available.\n");
		else if (result == PIMMGR_ERR_CANNOT_REGISTER)
			printf("Error registering pcon with vcrtcm\n");
		else if (result == PIMMGR_ERR_NOMEM)
			printf("Out of memory error while instantiating pcon\n");
		else
			printf("Unknown error occurred.\n");
		return 1;
	}
	
	printf("Success\n");
	printf("New pconid = %u\n", args.result1.pconid);
	
	return 0;
}

int do_destroy(int argc, char **argv)
{
	struct pimmgr_ioctl_args args;
	int fd = 0;
	long result = 0;
	
	fd = open_pimmgr_device();
	
	args.arg1.pconid = (uint32_t) atol(argv[0]);
	
	result = ioctl(fd, PIMMGR_IOC_DESTROY, &args);
	if (IOCTL_RESULT_IS_ERR(result)) {
		if (result == PIMMGR_ERR_INVALID_PCON)
			printf("Invalid pconid.\n");
		else
			printf("Unknown error occured.\n");
		return 1;
	}
	
	return 0;
}

int sysfs_find_pims_pcons(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{

}

int sysfs_find_pims(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	if (typeflag != FTW_D)
		return FTW_CONTINUE;
		
	if (ftwbuf->base == 0)
		return FTW_CONTINUE;
	
	if (ftwbuf->level == 1)
		printf("PIM: %s\n", fpath + ftwbuf->base);
	
	if (ftwbuf->level == 2)
		printf(" --> PCON: %s\n", fpath + ftwbuf->base);
		
	return FTW_CONTINUE;
}

int do_info(int argc, char **argv)
{
	nftw(PIMMGR_SYSFS_PIM_PATH, sysfs_find_pims, 32, FTW_ACTIONRETVAL);
	
	return 0;
}

int validate_args(struct operation *op, int argc)
{
	if (!(op->argc <= argc)) {
		printf("Not enough arguments...\n");
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
		if (strcmp(operation, ops[i].command) == 0 && validate_args(&ops[i], new_argc)) {
			return ops[i].func(new_argc, new_argv);
		}
	}
	
	printf("Bad command...\n");
	return 0;
}

