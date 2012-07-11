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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "pimmgr_tool.h"

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
	uint32_t pconid = 0;
	char *type = argv[0];
	
	fd = open_pimmgr_device();
	
	strncpy(args.pim_name, type, PIM_NAME_LEN);
	args.hints = 0;
	
	pconid = ioctl(fd, PIMMGR_IOC_INSTANTIATE, &args);
	
	printf("Success\n");
	printf("New pconid = %u\n", pconid);
	
	return 0;
}

int do_destroy(int argc, char **argv)
{
	int fd = 0;
	int result = 0;
	uint32_t pconid = 0;
	
	fd = open_pimmgr_device();
	
	pconid = (uint32_t) atol(argv[0]);
	result = ioctl(fd, PIMMGR_IOC_DESTROY, pconid);
	
	if (result)
		printf("Success\n");
	else
		printf("Invalid pconid\n");
	
	return 0;
}

int do_info(int argc, char **argv)
{
	printf("Not Implemented Yet\n");
	
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
			printf("Executing %s...\n", ops[i].command);
			return ops[i].func(new_argc, new_argv);
		}
	}
	
	printf("Bad command...\n");
	return 0;
}

