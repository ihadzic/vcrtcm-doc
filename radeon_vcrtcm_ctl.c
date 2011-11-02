/*
   Copyright (C) 2010 Alcatel-Lucent, Inc.
   Author: Ilija Hadzic <ihadzic@research.bell-labs.com>

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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <libdrm/drm.h>

#include "radeon_vcrtcm.h"

#define DRM_DRV_STRING "/dev/dri/card0"


void print_usage()
{
    printf("This is radeon/vcrtcm control utility (with an attitude).\n");
    printf("\nIf you make no mistake, the tool will setup the transmission\n");
    printf("your virtual CRTCs the way you want.\n");
    printf("If you make a mistake, you'll get what you deserve.\n");
    
    printf("\nUsage:\n\tradeon_vcrtcm_ctl <op_code> <device> <display_index> <op_code dependent arguments>\n");
    printf("\t<op_code>\n\t\t\t{nop,?,help,fps,stop,attach,detach}, default=nop\n"); 
    printf("\t<device>\n\t\t\tpath to the device node, default=/dev/dri/card0\n"); 
    printf("\t<display_index>\n\t\t\twhat else can it be, duh?, default=0\n"); 
    printf("\t<op_code dependent arguments>\n"); 
    printf("\t\tnop    -- no arguments\n"); 
    printf("\t\t?|help -- no arguments\n");
    printf("\t\tfps    -- <farme_rate in fps, default=15>\n"); 
    printf("\t\tstop   -- no arguments\n"); 
    printf("\t\tattach -- <major, default=0> <minor, default=0>, <flow, default=0>\n"); 
    printf("\t\tdetach -- no arguments\n"); 
    printf("\t\txmit   -- no arguments\n"); 
    printf("\nAll arguments are optional, but you may get verbally abused\n"); 
    printf("if you specify something that (combined with defaults) doesn't make sense.\n\n"); 
}

#define OP_CODE_TABLE_SIZE 8

/* Add new opcodes at the *end* of this table, then 
 * update the OP_CODE_TABLE_SIZE and the case
 * statement down in main()
 */
char *op_code_table[OP_CODE_TABLE_SIZE]={
    "nop", 
    "?",
    "help",
    "fps", 
    "stop", 
    "attach", 
    "detach",
    "xmit"
};

void cleanup(radeon_vcrtcm_ctl_descriptor_t *radeon_vcrtcm_ctl_descriptor)
{
    if (radeon_vcrtcm_ctl_descriptor -> user) free (radeon_vcrtcm_ctl_descriptor -> user); 
    free(radeon_vcrtcm_ctl_descriptor); 
}

void clue() {
    printf("Here's a clue: %s\n", strerror(errno));
    printf("Happy debugging.\n"); 
}

int parse_op_code(char *op_code_string) 
{
    
    int i; 
    
    for (i=0; i<OP_CODE_TABLE_SIZE; i++) {
	if (!strcmp(op_code_table[i],op_code_string)) return i; 
    }
    
    return -1; 
    
}

int main(int argc, char **argv)
{
    
    radeon_vcrtcm_ctl_descriptor_t *radeon_vcrtcm_ctl_descriptor;
    int display_index = 0;
    char *dev_path = DRM_DRV_STRING;
    int op_code = 0; 
    int drm_fd;
    
    if (argc<=1) {
	printf("Hey, I could use some arguments, dude! How about typing 'radeon_vcrtcm_ctl ?' for a start ?\n\n"); 
	printf("Anyway, I'll do a 'nop' operation on display %d device %s for you,\n",
	       display_index, DRM_DRV_STRING); 
	printf("but I doubt you'll find the result particularly exciting.\n"); 
    }
    else op_code = parse_op_code(argv[1]);
    
    if (argc>=3) dev_path = argv[2];
    if (argc>=4) display_index = strtol(argv[3], NULL, 0); 
    
    printf("\n"); 
    
    if (!(radeon_vcrtcm_ctl_descriptor = malloc(sizeof(radeon_vcrtcm_ctl_descriptor_t)))) {
	printf("Can't allocate memory for ioctl decriptor\n");
	return 1;
    }
    
    switch (op_code) {
	
    case 0:
	printf("No operation, device %s\n", dev_path); 
	radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_NOP; 
	break; 
	
    case 1:
    case 2:
	print_usage(); 
	break; 
	
    case 3: 
	{
	    int fps = 15; 
	    
	    if (argc>=5) fps = strtol(argv[4], NULL, 0); 
	    if (fps >= RADEON_VCRTCM_FPS_HARD_LIMIT) {
		printf("So who in the right mind would want %d frames per second ?\n", fps); 
		printf("I ain't doing this for you. If you have a reason to do it,\n"); 
		printf("find out where the limits are and code them out. Happy hacking.\n");
		cleanup(radeon_vcrtcm_ctl_descriptor);
		return 1;
	    }
	    if (fps <= 0) {
		printf("I don't know which Universe do you, bloke, come from, but\n");
		printf("here on planet Earth, we only do positive frame rates.\n"); 
		cleanup(radeon_vcrtcm_ctl_descriptor);
		return 1;
	    }
	    printf("Transmission on, device %s, display %d, fps %d\n", 
		   dev_path, display_index, fps); 
	    radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_SET_RATE; 
	    radeon_vcrtcm_ctl_descriptor -> arg1.fps = fps; 
	    break;
	}
    case 4:
	printf("Transmission off, device %s, display %d\n", 
	       dev_path, display_index); 
	radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_SET_RATE; 
	break; 
	
    case 5: 
	{
	    int major = 0, minor = 0, flow = 0;
	    if (argc>=5) major = strtol(argv[4], NULL, 0); 
	    if (argc>=6) minor = strtol(argv[5], NULL, 0); 
	    if (argc>=7) flow  = strtol(argv[6], NULL, 0); 
	    printf("Attach, device %s, display %d, major %d, minor %d, flow %d\n",
		   dev_path, display_index, 
		   major, minor, flow);
	    radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_ATTACH; 
	    radeon_vcrtcm_ctl_descriptor -> arg1.major = major; 
	    radeon_vcrtcm_ctl_descriptor -> arg2.minor = minor; 
	    radeon_vcrtcm_ctl_descriptor -> arg3.flow  = flow; 
	    break; 
	}
    case 6: 
	printf("Detach, device %s, display %d\n",
	       dev_path, display_index); 
	radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_DETACH; 
	break; 
	
    case 7:
	printf("Force transmission, device %s, display %d\n", 
	       dev_path, display_index); 
	radeon_vcrtcm_ctl_descriptor -> op_code = RADEON_VCRTCM_CTL_OP_CODE_XMIT;
	break;

    default:
	printf("wtf. is %s ?\n", argv[1]); 
	cleanup(radeon_vcrtcm_ctl_descriptor);
	return 1;
    }
    
    radeon_vcrtcm_ctl_descriptor -> display_index = display_index; 
    
    printf("\n"); 
    
    drm_fd=open(dev_path, O_RDWR);
    if (drm_fd < 0) {
	printf("Sorry buddy, but I'm having problems opening the device file %s\n",
	       dev_path); 
	clue(); 
	cleanup(radeon_vcrtcm_ctl_descriptor);
	return 1; 
    }
    
    if ((op_code != 1) && (op_code !=2))
	if (ioctl(drm_fd, DRM_IOCTL_RADEON_VCRTCM, radeon_vcrtcm_ctl_descriptor) < 0) {
	    printf("Sorry dude, but kernel won't cooperate.\n"); 
	    printf("One of us (probably you) screwed up something.\n"); 
	    clue(); 
	    cleanup(radeon_vcrtcm_ctl_descriptor); 
	    close(drm_fd);
	    return 1;
	}
    
    close(drm_fd); 
    cleanup(radeon_vcrtcm_ctl_descriptor);
    return 0; 
}
