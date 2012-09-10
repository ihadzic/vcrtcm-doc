/*
 * Copyright (C) 2012 Alcatel-Lucent, Inc.
 * Author: Ilija Hadzic <ihadzic@research.bell-labs.com>
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

void print_usage()
{
	fprintf(stderr, "GPU Resource Dump\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\tgpu_resource_dump -? | ");
	fprintf(stderr, "[-n node_path]\n");
}

void print_connector_status(int status)
{
	switch (status) {
	case DRM_MODE_CONNECTED:
		printf("connected   ");
	break;
	case DRM_MODE_DISCONNECTED:
		printf("disconnected");
		break;
	case DRM_MODE_UNKNOWNCONNECTION:
		printf("unknown     ");
		break;
	default:
		printf("failed      ");
	}
}

void print_connector_type(int type) 
{
	switch (type) {
	case DRM_MODE_CONNECTOR_Unknown:
		printf("unknown    ");
		break;
	case DRM_MODE_CONNECTOR_VGA:
		printf("VGA        ");
		break;
	case DRM_MODE_CONNECTOR_DVII:
		printf("DVI-I      ");
		break;
	case DRM_MODE_CONNECTOR_DVID:
		printf("DVI-D      ");
		break;
	case DRM_MODE_CONNECTOR_DVIA:
		printf("DVI-A      ");
		break;
	case DRM_MODE_CONNECTOR_Composite:
		printf("composite  ");
		break;
	case DRM_MODE_CONNECTOR_SVIDEO:
		printf("s-video    ");
		break;
	case DRM_MODE_CONNECTOR_LVDS:
		printf("LVDS       ");
		break;
	case DRM_MODE_CONNECTOR_Component:
		printf("component  ");
		break;
	case DRM_MODE_CONNECTOR_9PinDIN:
		printf("9pinDIN    ");
		break;
	case DRM_MODE_CONNECTOR_DisplayPort:
		printf("DisplayPort");
		break;
	case DRM_MODE_CONNECTOR_HDMIA:
		printf("HDMI-A     ");
		break;
	case DRM_MODE_CONNECTOR_HDMIB:
		printf("HDMI-B     ");
		break;
	case DRM_MODE_CONNECTOR_TV:
		printf("TV         ");
		break;
	case DRM_MODE_CONNECTOR_eDP:
		printf("eDP        ");
		break;
	default:
		printf("unknown    ");
	}
}

void
print_used_crtc(int fd, drmModeConnector *connector, drmModeRes *resources)
{
	drmModeEncoder *encoder;
	int i;
	
	encoder = drmModeGetEncoder(fd, connector->encoder_id);
	if (!encoder)
		return;
	for (i = 0; i < resources->count_crtcs; i++) {
		if (resources->crtcs[i] == encoder->crtc_id)
			break;
	}
	drmModeFreeEncoder(encoder);
	if (i < resources->count_crtcs)
		printf("used_crtc=%d:%d", resources->crtcs[i], i);
}

int
print_possible_crtcs(int fd, drmModeEncoder *encoder, drmModeRes *resources)
{
	uint32_t possible_crtcs = encoder->possible_crtcs;
	int i;
	int r = 0;

	for (i = 0; i < resources->count_crtcs; i++) {
		if ((possible_crtcs >> i) & 0x1) {
			printf("possible_crtc=%d:%d ", resources->crtcs[i], i);
			r = 1;
		}
	}
	return r;
}

int main(int argc, char **argv)
{
	char *node_path;
	drmModeRes *resources;
	drmModeConnector *connector;
	int r, i;
	int fd;

	node_path = NULL;
	while ((r = getopt(argc, argv, "n:?")) != -1) {
		switch (r) {
		case 'n':
			node_path = optarg;
			break;
		case '?':
		default:
			print_usage();
			return 0;
		}
	}
	if (!node_path)
		node_path = "/dev/dri/card0";
	fprintf(stderr, "node_path = %s\n", node_path);
	fd = open(node_path, 0);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	resources = drmModeGetResources(fd);
	if (!resources) {
		perror("drmModeGetResources");
		close(fd);
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(fd, resources->connectors[i]);
		if (connector) {
			int j;
			printf("connector=%d ", connector->connector_id);
			print_connector_type(connector->connector_type);
			printf(" ");
			print_connector_status(connector->connection);
			printf(" ");
			if (connector->connection == DRM_MODE_CONNECTED) {
				printf("used_encoder=%d ",
				       connector->encoder_id);
				print_used_crtc(fd, connector, resources);
				printf(" ");
			}
			for (j = 0; j < connector->count_encoders; j++) {
				drmModeEncoder *encoder;

				printf("possible_encoder=%d [ ",
				       connector->encoders[j]);
				encoder = drmModeGetEncoder(fd,
						connector->encoders[j]);
				if (encoder) {
					int r;
					r = print_possible_crtcs(fd,
						encoder, resources);
					if (!r)
						printf(" ");
					drmModeFreeEncoder(encoder);
				}
				printf("] ");
			}
			printf("\n");
		} else
			printf("-----\n");
		drmModeFreeConnector(connector);
	}

	drmModeFreeResources(resources);
	close(fd);
	return 0;
}
