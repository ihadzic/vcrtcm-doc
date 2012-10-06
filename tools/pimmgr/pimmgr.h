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

#ifndef PIMMGR_TOOL
#define PIMMGR_TOOL

#define PIMMGR_MAGIC 'K'
#define PIMMGR_IOC_INSTANTIATE _IOW(PIMMGR_MAGIC, 1, int)
#define PIMMGR_IOC_DESTROY _IOW(PIMMGR_MAGIC, 2, int)

#define PIM_NAME_MAXLEN 33
#define PIMMGR_DEVICE "/dev/pimmgr"
#define PIMMGR_SYSFS "/sys/class/vcrtcm/pimmgr"
#define PIMMGR_SYSFS_PIM_PATH PIMMGR_SYSFS "/pims"
#define PIMMGR_SYSFS_PCON_PATH PIMMGR_SYSFS "/pcons"


#include <stdint.h>

struct pimmgr_ioctl_args {
	union {
		char pim_name[PIM_NAME_MAXLEN];
		int pconid;
	} arg1;

	union {
		uint32_t hints;
	} arg2;

	union {
		int pconid;
	} result1;
};

#endif
