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
#include "vcrtcm_ioctl.h"

#define VCRTCM_DEVICE_OLD "/dev/pimmgr"
#define VCRTCM_DEVICE "/dev/vcrtcm"
#define VCRTCM_SYSFS_OLD "/sys/class/vcrtcm/pimmgr"
#define VCRTCM_SYSFS "/sys/class/vcrtcm/vcrtcm"

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
	int min_argc;
	int max_argc;
	char arghelp[MAX_ARGHELP_LEN];
	char description[MAX_DESCRIPTION_LEN];
	int (*func)(int argc, char **argv);
};

struct operation ops[] = {
	{
		.command = "instantiate",
		.min_argc = 1,
		.max_argc = 3,
		.arghelp = "<pim_name> [<xfer_mode> [<hints>]]",
		.description = "Create a new PCON of the given type.",
		.func = do_instantiate,
	},
	{
		.command = "pimtest",
		.min_argc = 2,
		.max_argc = 2,
		.arghelp = "<pim_name> <arg>",
		.description = "Call the PIM's test() callback function with the given argument.",
		.func = do_pimtest,
	},
	{
		.command = "destroy",
		.min_argc = 1,
		.max_argc = 2,
		.arghelp = "<pconid> [<force>]",
		.description = "Destroy the given PCON.",
		.func = do_destroy,
	},
	{
		.command = "info",
		.min_argc = 0,
		.max_argc = 0,
		.arghelp = "",
		.description = "List all available PIMs and their associated PCONs.",
		.func = do_info,
	},
	{
		.command = "help",
		.min_argc = 0,
		.max_argc = 0,
		.arghelp = "",
		.description = "Display this message.",
		.func = do_help,
	},
	{
		.command = "attach",
		.min_argc = 3,
		.max_argc = 3,
		.arghelp = "<pconid> <connid> <devpath>",
		.description = "Attach the given PCON to the connector in the given DRM devpath with the given DRM identifer.",
		.func = do_attach,
	},
	{
		.command = "detach",
		.min_argc = 1,
		.max_argc = 2,
		.arghelp = "<pconid> [<force>]",
		.description = "Detach the given PCON from its connector.",
		.func = do_detach,
	},
	{
		.command = "fps",
		.min_argc = 2,
		.max_argc = 2,
		.arghelp = "<pconid> <fps>",
		.description = "Set the given PCON's frame rate to the specified value.",
		.func = do_fps,
	},
	{
		.command = "xmit",
		.min_argc = 1,
		.max_argc = 1,
		.arghelp = "<pconid>",
		.description = "Tell the given PCON to transmit one frame.",
		.func = do_xmit,
	},
};

static int operation_count = sizeof(ops) / sizeof(struct operation);

static const char *sysfs;

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
		fprintf(stderr, "warning: cannot open %s (%s), trying %s\n", VCRTCM_DEVICE, strerror(errno), VCRTCM_DEVICE_OLD);
		fd = open(VCRTCM_DEVICE_OLD, O_WRONLY);
		if (fd < 0) {
			fprintf(stderr, "error: cannot open %s: %s\n", VCRTCM_DEVICE_OLD, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	return fd;
}

int sysfs_read_file(const char *base_path, const char *file, char *contents)
{
	int size = 0;
	char path[PATH_MAX];
	FILE *f = NULL;

	snprintf(path, PATH_MAX, "%s/%s", base_path, file);
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
 * look in sysfs for pim with given name 
 * returns pimid or < 0
 */
int sysfs_find_pimid(const char *pim_name)
{
	char pim_path[PATH_MAX];
	static char pimid_str[4096];
	int pimid;
	int size;

	snprintf(pim_path, PATH_MAX, "%s/pims/%s", sysfs, pim_name);
	size = sysfs_read_file(pim_path, "id", pimid_str);
	if (size > 0) {
		pimid = (int)strtol(pimid_str, NULL, 0);
		return pimid;
	}
	return -1;
}

/*
 * look in sysfs for given pconid
 * returns boolean
 */
int sysfs_find_pconid(uint32_t pconid)
{
	char path[PATH_MAX];
	struct stat st;

	snprintf(path, PATH_MAX, "%s/pcons/%u", sysfs, pconid);
	if (stat(path, &st) < 0) {
		fprintf(stderr, "error: cannot stat %s: %s\n",
					path, strerror(errno));
		return 0;
	}
	if (!S_ISDIR(st.st_mode)) {
		fprintf(stderr, "error: %s not a directory\n", path);
		return 0;
	}
	return 1;
}

int do_pimtest(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long r;
	char *type = argv[0];
	int pimid;

	fd = open_vcrtcm_device();
	pimid = sysfs_find_pimid(type);
	if (pimid < 0) {
		fprintf(stderr, "error: no pim \"%s\" in sysfs\n", type);
		return EXIT_FAILURE;
	}
	memset(&args, 0, sizeof(args));
	args.arg1.pimid = pimid;
	args.arg2.testarg = strtoul(argv[1], NULL, 0);
	r = ioctl(fd, VCRTCM_IOC_PIMTEST, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	return 0;
}

int do_instantiate(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int xfer_mode;
	int hints;
	int fd;
	long r;
	char *type = argv[0];
	int pimid;

	xfer_mode = 0; // that's VCRTCM_XFERMODE_UNSPECIFIED
	if (argc > 1)
		xfer_mode = atoi(argv[1]);
	hints = 0;
	if (argc > 2)
		hints = atoi(argv[2]);
	fd = open_vcrtcm_device();
	pimid = sysfs_find_pimid(type);
	if (pimid < 0) {
		fprintf(stderr, "error: no pim \"%s\" in sysfs\n", type);
		return EXIT_FAILURE;
	}
	memset(&args, 0, sizeof(args));
	args.arg1.pimid = pimid;
	args.arg2.xfer_mode = xfer_mode;
	args.arg3.hints = hints;
	r = ioctl(fd, VCRTCM_IOC_INSTANTIATE, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("%d\n", args.result1.pconid);
	return 0;
}

int do_destroy(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int force;
	int fd;
	long r;
	uint32_t pconid;

	pconid = strtoul(argv[0], NULL, 0);
	force = 0;
	if (argc > 1)
		force = atoi(argv[1]);
	if (!sysfs_find_pconid(pconid)) {
		fprintf(stderr, "error: no pcon 0x%08x in sysfs\n", pconid);
		return EXIT_FAILURE;
	}
	fd = open_vcrtcm_device();
	memset(&args, 0, sizeof(args));
	args.arg1.pconid = pconid;
	args.arg2.force = force;
	r = ioctl(fd, VCRTCM_IOC_DESTROY, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("destroyed pcon %d\n", args.arg1.pconid);
	return 0;
}

static void
parse_connlink(char *path, int *conn_minor, int *connid)
{
	char *colon;
	char *slash;

	*conn_minor = -1;
	*connid = -1;
	colon = strrchr(path, ':');
	if (!colon)
		return;
	*connid = atoi(colon + 1);
	*colon = '\0';
	slash = strrchr(path, '/');
	if (!slash)
		return;
	*conn_minor = atoi(slash + 1);
}

static void
show_pcon(char *pcon_path, struct dirent *de)
{
	static char contents[4096];
	int r;
	int attached;

	printf("    PCON: %s\n", de->d_name);
	sysfs_read_file(pcon_path, "description", contents);
	printf("        %s\n", contents);
	r = sysfs_read_file(pcon_path, "attached", contents);
	if (!r) {
		printf("        properties not implemented\n");
		return;
	}
	attached = atoi(contents);
	if (attached) {
		int fps;
		char connlink_src_path[PATH_MAX];
		char connlink_dest_path[PATH_MAX];

		sysfs_read_file(pcon_path, "fps", contents);
		fps = atoi(contents);
		snprintf(connlink_src_path, sizeof(connlink_src_path),
			"%s/connector", pcon_path);
		if (readlink(connlink_src_path, connlink_dest_path, PATH_MAX) > 0) {
			int attach_minor;
			int conn_minor;
			int connid;

			connlink_dest_path[PATH_MAX-1] = '\0';
			parse_connlink(connlink_dest_path, &conn_minor, &connid);
			sysfs_read_file(pcon_path, "attach_minor", contents);
			attach_minor = atoi(contents);
			printf("        attached to connector %d of drm minor %d (attached via minor %d), f/s = %d\n", connid, conn_minor, attach_minor, fps);
		}
		else
			printf("        attached, f/s = %d\n", fps);
	} else
		printf("        unattached\n");
}

/*
 * return the number of pcons found or a negative value on error
 */
int sysfs_find_pcons_in(const char *dirpath, DIR *dir)
{
	struct dirent *de;
	int num_pcons;
	char pcon_path[PATH_MAX];

	num_pcons = 0;
	while ((de = readdir(dir)) != NULL) {
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			struct stat pcon_stat;

			snprintf(pcon_path, sizeof(pcon_path), "%s/%s",
				 dirpath, de->d_name);
			if (stat(pcon_path, &pcon_stat) == -1) {
				fprintf(stderr, "error: cannot stat %s: %s\n",
					pcon_path, strerror(errno));
				continue;
			}
			if (S_ISDIR(pcon_stat.st_mode)) {
				num_pcons++;
				show_pcon(pcon_path, de);
			}
		}
	}
	return num_pcons;
}

/*
 * return the number of pcons found or a negative value on error
 */
int sysfs_find_pcons(const char *pim_path)
{
	char pcons_path[PATH_MAX];
	const char *dirpath;
	DIR *dir;
	int num_pcons;

	snprintf(pcons_path, PATH_MAX, "%s/%s", pim_path, "pcons");
	dirpath = pcons_path;
	dir = opendir(dirpath);
	if (!dir) {
		/* it's the older sysfs layout */
		dirpath = pim_path;
		dir = opendir(dirpath);
		if (!dir) {
			fprintf(stderr, "error: cannot open %s: %s\n",
				pim_path, strerror(errno));
			return -1;
		}
	}
	num_pcons = sysfs_find_pcons_in(dirpath, dir);
	closedir(dir);
	return num_pcons;
}

/*
 * return the number of pims found or a negative value on error
 */
int sysfs_find_pims()
{
	char dir_path[PATH_MAX];
	char pim_path[PATH_MAX];
	struct dirent *de;
	DIR *d;
	int num_pims, num_pcons;

	snprintf(dir_path, sizeof(dir_path), "%s/pims", sysfs);
	d = opendir(dir_path);
	if (!d) {
		fprintf(stderr, "error: cannot open %s: %s\n",
			dir_path, strerror(errno));
		return -1;
	}
	num_pims = 0;
	while ((de = readdir(d)) != NULL) {
		if (!strcmp(de->d_name, ".") ||!strcmp(de->d_name, ".."))
			continue;
		num_pims++;
		printf("PIM: %s\n", de->d_name);
		snprintf(pim_path, sizeof(pim_path), "%s/%s",
			 dir_path, de->d_name);
		num_pcons = sysfs_find_pcons(pim_path);
		if (num_pcons < 0) {
			closedir(d);
			return num_pcons;
		} else if (num_pcons == 0)
			printf("    no pcons\n");
	}
	closedir(d);
	return num_pims;
}

int do_info(int argc, char **argv)
{
	int num_pims;

	num_pims = sysfs_find_pims();
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

void validate_args(struct operation *op, int argc)
{
	if (argc < op->min_argc) {
		printf("error: missing arguments\n");
		exit(EXIT_FAILURE);
	}
	if (argc > op->max_argc) {
		printf("error: too many arguments\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	char *operation;
	int new_argc = argc - 2;
	char **new_argv = argv + 2;
	int i;
	struct stat st;

	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	sysfs = VCRTCM_SYSFS;
	if (stat(sysfs, &st) < 0) {
		fprintf(stderr, "warning: cannot stat %s (%s), trying %s\n", sysfs, strerror(errno), VCRTCM_SYSFS_OLD);
		sysfs = VCRTCM_SYSFS_OLD;
		if (stat(sysfs, &st) < 0) {
			fprintf(stderr, "error: cannot stat %s: %s\n", sysfs, strerror(errno));
			return EXIT_FAILURE;
		}
	}
	operation = argv[1];
	for (i=0; i<operation_count; i++) {
		if (strncmp(operation, ops[i].command, strlen(operation)) == 0) {
		    validate_args(&ops[i], new_argc);
			return ops[i].func(new_argc, new_argv);
		}
	}
	fprintf(stderr, "error: unrecognized command \"%s\"\n", operation);
	return EXIT_FAILURE;
}

int do_attach(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long r;
	struct stat st;
	uint32_t pconid;

	pconid = strtoul(argv[0], NULL, 0);
	if (!sysfs_find_pconid(pconid)) {
		fprintf(stderr, "error: no pcon 0x%08x in sysfs\n", pconid);
		return EXIT_FAILURE;
	}
	if (stat(argv[2], &st)) {
		fprintf(stderr, "error: cannot stat %s: %s\n", argv[2], strerror(errno));
		return EXIT_FAILURE;
	}
	fd = open_vcrtcm_device();
	memset(&args, 0, sizeof(args));
	args.arg1.pconid = pconid;
	args.arg2.connid = strtoul(argv[1], NULL, 0);
	args.arg3.major = major(st.st_rdev);
	args.arg4.minor = minor(st.st_rdev);
	r = ioctl(fd, VCRTCM_IOC_ATTACH, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("attached pcon %d to connector %d\n", args.arg1.pconid, args.arg2.connid);
	return 0;
}

int do_detach(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int force;
	int fd;
	long r;
	uint32_t pconid;

	pconid = strtoul(argv[0], NULL, 0);
	force = 0;
	if (argc > 1)
		force = atoi(argv[1]);
	if (!sysfs_find_pconid(pconid)) {
		fprintf(stderr, "error: no pcon 0x%08x in sysfs\n", pconid);
		return EXIT_FAILURE;
	}
	fd = open_vcrtcm_device();
	memset(&args, 0, sizeof(args));
	args.arg1.pconid = pconid;
	args.arg2.force = force;
	r = ioctl(fd, VCRTCM_IOC_DETACH, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("detached pcon %d\n", args.arg1.pconid);
	return 0;
}

int do_fps(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long r;
	uint32_t pconid;

	pconid = strtoul(argv[0], NULL, 0);
	if (!sysfs_find_pconid(pconid)) {
		fprintf(stderr, "error: no pcon 0x%08x in sysfs\n", pconid);
		return EXIT_FAILURE;
	}
	fd = open_vcrtcm_device();
	memset(&args, 0, sizeof(args));
	args.arg1.pconid = pconid;
	args.arg2.fps = strtoul(argv[1], NULL, 0);
	r = ioctl(fd, VCRTCM_IOC_FPS, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("set pcon %d to %d f/s\n", args.arg1.pconid, args.arg2.fps);
	return 0;
}

int do_xmit(int argc, char **argv)
{
	struct vcrtcm_ioctl_args args;
	int fd;
	long r;
	uint32_t pconid;

	pconid = strtoul(argv[0], NULL, 0);
	if (!sysfs_find_pconid(pconid)) {
		fprintf(stderr, "error: no pcon 0x%08x in sysfs\n", pconid);
		return EXIT_FAILURE;
	}
	fd = open_vcrtcm_device();
	memset(&args, 0, sizeof(args));
	args.arg1.pconid = pconid;
	r = ioctl(fd, VCRTCM_IOC_XMIT, &args);
	if (r < 0) {
		fprintf(stderr, "error: ioctl failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	close(fd);
	printf("forced pcon %d to xmit\n", args.arg1.pconid);
	return 0;
}
