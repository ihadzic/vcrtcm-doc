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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <ftw.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "vcrtcm.h"
#include "vcrtcm_ioctl.h"

#define MAX_COMMAND_LEN 35
#define MAX_ARGHELP_LEN 128
#define MAX_DESCRIPTION_LEN 1024

/* Operation implementations */
int do_pimtest(int argc, char **argv);
int do_instantiate(int argc, char **argv);
int do_destroy(int argc, char **argv);
int do_info(int argc, char **argv);
int do_help(int argc, char **argv);
int do_attach(int argc, char **argv);
int do_detach(int argc, char **argv);
int do_fps(int argc, char **argv);
int do_xmit(int argc, char **argv);

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
		.description = "Create a new PCON of the given type.",
		.func = do_instantiate,
	},
	{
		.command = "pimtest",
		.argc = 2,
		.arghelp = "<pim_name> <arg>",
		.description = "Call the PIM's test() callback function with the given argument.",
		.func = do_pimtest,
	},
	{
		.command = "destroy",
		.argc = 1,
		.arghelp = "<pconid>",
		.description = "Destroy the given PCON.",
		.func = do_destroy,
	},
	{
		.command = "info",
		.argc = 0,
		.arghelp = "",
		.description = "List all available PIMs and their associated PCONs.",
		.func = do_info,
	},
	{
		.command = "help",
		.argc = 0,
		.arghelp = "",
		.description = "Display this message.",
		.func = do_help,
	},
	{
		.command = "attach",
		.argc = 3,
		.arghelp = "<pconid> <connid> <devpath>",
		.description = "Attach the given PCON to the connector in the given DRM devpath with the given DRM identifer.",
		.func = do_attach,
	},
	{
		.command = "detach",
		.argc = 1,
		.arghelp = "<pconid>",
		.description = "Detach the given PCON from its connector.",
		.func = do_detach,
	},
	{
		.command = "fps",
		.argc = 2,
		.arghelp = "<pconid> <fps>",
		.description = "Set the given PCON's frame rate to the specified value.",
		.func = do_fps,
	},
	{
		.command = "xmit",
		.argc = 1,
		.arghelp = "<pconid>",
		.description = "Tell the given PCON to transmit one frame.",
		.func = do_xmit,
	},
};

static int operation_count = sizeof(ops) / sizeof(struct operation);
static int found_pim = 0;
static int found_pcon = 0;

void print_usage(char *command)
{
	printf("usage: %s <command> [args]\n", command);
	printf("type \"%s help\" for more help\n", command);
}

int open_vcrtcm_device(void)
{
	int fd;

	fd = open(VCRTCM_DEVICE, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "error: cannot open %s: %s\n", VCRTCM_DEVICE, strerror(errno));
		exit(-1);
	}
	return fd;
}

void close_vcrtcm_device(int fd)
{
	if (close(fd) == -1) {
		fprintf(stderr, "error: cannot close %s: %s\n",
			VCRTCM_DEVICE, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int do_pimtest(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;
	char *type = argv[0];
	int pimid;

	fd = open_vcrtcm_device();
	pimid = sysfs_find_pimid(VCRTCM_SYSFS_PIM_PATH, type);
	if (pimid < 0) {
		fprintf(stderr, "error: could not find pim type in sysfs\n");
		return 1;
	}
	args.arg1.pimid = pimid;
	args.arg2.testarg = strtoul(argv[1], NULL, 0);
	result = ioctl(fd, VCRTCM_IOC_PIMTEST, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}
	printf("result = %ld\n", result);
	return 0;
}

int do_instantiate(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;
	char *type = argv[0];
	int pimid;

	fd = open_vcrtcm_device();
	pimid = sysfs_find_pimid(VCRTCM_SYSFS_PIM_PATH, type);
	if (pimid < 0) {
		fprintf(stderr, "error: could not find pim type in sysfs\n");
		return 1;
	}
	args.arg1.pimid = pimid;
	args.arg2.hints = 0;
	result = ioctl(fd, VCRTCM_IOC_INSTANTIATE, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}
	printf("%d\n", args.result1.pconid);
	return 0;
}

int do_destroy(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;

	fd = open_vcrtcm_device();
	args.arg1.pconid = strtoul(argv[0], NULL, 0);
	result = ioctl(fd, VCRTCM_IOC_DESTROY, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		switch (errno) {
			case EINVAL:
			case ENODEV:
				fprintf(stderr, "error: invalid pconid (%d)\n", args.arg1.pconid); break;
				break;
			default:
				fprintf(stderr, "non-vcrtcm error: %s\n", strerror(errno));
				break;
		}
		return 1;
	}
	printf("destroyed pcon %d\n", args.arg1.pconid);
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
		fclose(f);
	}
	return size;
}

/*
 * return the number of pcons found or a negative value on error
 */
int sysfs_find_pcons(const char *pim_path)
{
	static char contents[4096];
	char pcon_path[PATH_MAX];
	struct stat pcon_stat;
	struct dirent *de;
	DIR *d;
	int attached;
	int num_pcons;
	int result;

	num_pcons = 0;
	d = opendir(pim_path);
	if (!d) {
		fprintf(stderr, "error: cannot open %s: %s\n",
			pim_path, strerror(errno));
		return -1;
	}
	while ((de = readdir(d)) != NULL) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		snprintf(pcon_path, sizeof(pcon_path), "%s/%s",
			 pim_path, de->d_name);

		/* if the entry is not a directory then continue */
		if (stat(pcon_path, &pcon_stat) == -1) {
			fprintf(stderr, "error: cannot stat %s: %s\n",
				pcon_path, strerror(errno));
			continue;
		}
		if (!S_ISDIR(pcon_stat.st_mode))
			continue;

		num_pcons++;
		printf(" --> PCON: %s\n", de->d_name);
		sysfs_read_file(pcon_path, "description", contents);
		printf("       -> %s\n", contents);
		result = sysfs_read_file(pcon_path, "attached", contents);
		if (!result) {
			printf("       -> properties not implemented\n");
			continue;
		}
		attached = atoi(contents);
		if (attached) {
			sysfs_read_file(pcon_path, "fps", contents);
			printf("       -> attached, FPS = %s\n", contents);
		} else
			printf("       -> unattached\n");
	}
	closedir(d);
	return num_pcons;
}

/*
 * return the number of pims found or a negative value on error
 */
int sysfs_find_pims(const char *pims_path)
{
	char pim_path[PATH_MAX];
	struct dirent *de;
	DIR *d;
	int num_pims, num_pcons;

	num_pims = 0;
	d = opendir(pims_path);
	if (!d) {
		fprintf(stderr, "error: cannot open %s: %s\n",
			pims_path, strerror(errno));
		return -1;
	}
	while ((de = readdir(d)) != NULL) {
		if (!strcmp(de->d_name, ".") ||!strcmp(de->d_name, ".."))
			continue;
		num_pims++;
		printf("PIM: %s\n", de->d_name);
		snprintf(pim_path, sizeof(pim_path), "%s/%s",
			 pims_path, de->d_name);
		num_pcons = sysfs_find_pcons(pim_path);
		if (num_pcons < 0) {
			closedir(d);
			return num_pcons;
		} else if (num_pcons == 0)
			printf(" --> no pcons found\n");
	}
	closedir(d);
	return num_pims;
}

/*
 * query_sysfs for a pim with name given in pim_name
 * return the pimid if found, or a negative value if not found
 */
int sysfs_find_pimid(const char *pims_path, const char *pim_name)
{
	char pim_path[512];
	static char pimid_str[4096];
	int pimid;
	int size;

	snprintf(pim_path, 512, "%s/%s", pims_path, pim_name);

	size = sysfs_read_file(pim_path, "id", pimid_str);
	if (size > 0) {
		pimid = (int)strtol(pimid_str, NULL, 0);
		return pimid;
	}

	return -1;
}

int do_info(int argc, char **argv)
{
	int num_pims;

	num_pims = sysfs_find_pims(VCRTCM_SYSFS_PIM_PATH);
	if (num_pims < 0)
		return num_pims;
	if (num_pims == 0)
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
	if (op->argc > argc) {
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
	int i;
	struct operation *op = NULL;

	if (argc < 2) {
		print_usage(argv[0]);
		return 0;
	}
	operation = argv[1];
	for (i=0; i<operation_count; i++) {
		if (strncmp(operation, ops[i].command, strlen(operation)) == 0 &&
		    validate_args(&ops[i], new_argc)) {
			return ops[i].func(new_argc, new_argv);
		}
	}
	fprintf(stderr, "error: bad command\n");
	return 0;
}

int do_attach(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;
	struct stat st;

	fd = open_vcrtcm_device();
	args.arg1.pconid = strtoul(argv[0], NULL, 0);
	args.arg2.connid = strtoul(argv[1], NULL, 0);
	if (stat(argv[2], &st)) {
		fprintf(stderr, "cannot stat %s: %s\n", argv[2], strerror(errno));
		return 1;
	}
	args.arg3.major = major(st.st_rdev);
	args.arg4.minor = minor(st.st_rdev);
	result = ioctl(fd, VCRTCM_IOC_ATTACH, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		switch (errno) {
			case EINVAL:
			case ENODEV:
				fprintf(stderr, "error: invalid pconid (%d) or connid (%d)\n", args.arg1.pconid, args.arg2.connid); break;
				break;
			default:
				fprintf(stderr, "non-vcrtcm error: %s\n", strerror(errno));
				break;
		}
		return 1;
	}
	printf("attached pcon %d to connector %d\n", args.arg1.pconid, args.arg2.connid);
	return 0;
}

int do_detach(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;

	fd = open_vcrtcm_device();
	args.arg1.pconid = strtoul(argv[0], NULL, 0);
	result = ioctl(fd, VCRTCM_IOC_DETACH, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		switch (errno) {
			case EINVAL:
			case ENODEV:
				fprintf(stderr, "error: invalid pconid (%d)\n", args.arg1.pconid); break;
				break;
			default:
				fprintf(stderr, "non-vcrtcm error: %s\n", strerror(errno));
				break;
		}
		return 1;
	}
	printf("detached pcon %d\n", args.arg1.pconid);
	return 0;
}

int do_fps(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;

	fd = open_vcrtcm_device();
	args.arg1.pconid = strtoul(argv[0], NULL, 0);
	args.arg2.fps = strtoul(argv[1], NULL, 0);
	result = ioctl(fd, VCRTCM_IOC_FPS, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		switch (errno) {
			case EINVAL:
			case ENODEV:
				fprintf(stderr, "error: invalid pconid (%d)\n", args.arg1.pconid); break;
				break;
			default:
				fprintf(stderr, "non-vcrtcm error: %s\n", strerror(errno));
				break;
		}
		return 1;
	}
	printf("set pcon %d to %d f/s\n", args.arg1.pconid, args.arg2.fps);
	return 0;
}

int do_xmit(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long result;

	fd = open_vcrtcm_device();
	args.arg1.pconid = strtoul(argv[0], NULL, 0);
	result = ioctl(fd, VCRTCM_IOC_XMIT, &args);
	close_vcrtcm_device(fd);
	if (errno) {
		switch (errno) {
			case EINVAL:
			case ENODEV:
				fprintf(stderr, "error: invalid pconid (%d)\n", args.arg1.pconid); break;
				break;
			default:
				fprintf(stderr, "non-vcrtcm error: %s\n", strerror(errno));
				break;
		}
		return 1;
	}
	printf("forced pcon %d to xmit\n", args.arg1.pconid);
	return 0;
}
