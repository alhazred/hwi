/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Portions Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Copyright 2019 Alexander Eremin <alexander.r.eremin@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <libdevinfo.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <libdiskmgt.h>
#include <sys/dkio.h>

#include "hwi.h"

di_node_t root_node = DI_NODE_NIL;
di_prom_handle_t h_prom = DI_PROM_HANDLE_NIL;
di_devlink_handle_t hdev_link = NULL;

int
lookup_node_strings(di_node_t d_node, char *name, char **str)
{
	int ret = -1;

	if (h_prom != DI_PROM_HANDLE_NIL) {
		ret = di_prom_prop_lookup_strings(h_prom, d_node, name, str);
	}
	if (ret <= 0) {
		ret = di_prop_lookup_strings(DDI_DEV_T_ANY, d_node, name, str);
	}

	return (ret);
}

void
prt_hd_info(di_node_t node, const char *disk_path)
{
	char *vendor;
	char *product;
	char *serial;
	char *disk;
	int devfd;
	struct dk_minfo media;
	struct dk_cinfo dkinfo;
	char dsk[MAXPATHLEN];
	char *dtype;
	double dsize;

	if (lookup_node_strings(node, "inquiry-vendor-id",
			(char **)&vendor) <= 0)
		vendor = "unknown";

	if (lookup_node_strings(node, "inquiry-product-id",
			(char **)&product) <= 0)
		product = "unknown";

	if (lookup_node_strings(node, "inquiry-serial-no",
			(char **)&serial) <= 0)
		serial = "unknown";

	disk = basename((char *)disk_path);

	disk[strlen(disk) - 2] = '\0';

	(void) sprintf(dsk, "%sp0", disk_path);

	devfd = open(dsk, O_RDONLY);

	if (ioctl(devfd, DKIOCINFO, &dkinfo) == 0) {
		switch (dkinfo.dki_ctype) {
			case DKC_DIRECT:
				dtype = "ATA";
				break;
			case DKC_SCSI_CCS:
				dtype = "SCSI";
				break;
			default:
				dtype = "UNKNOWN";
		}
	}
	if ((ioctl(devfd, DKIOCGMEDIAINFO, &media)) == 0) {
		dsize = (double)(media.dki_capacity *
		    media.dki_lbsize / 1024.0 / 1024.0 / 1024.0);
	}
	num_dsk++;

	(void) printf("%*d: "BD"id:"RS" %s "BD"type:"RS" %s "BD"vendor:"RS
	    " %s "BD"product:"RS" %s "BD"serial:"RS" %.15s "BD"size:"RS
	    " %7.2f GiB\n", num_dsk == 1 ? 5 : 12, num_dsk, disk, dtype,
	    vendor, product, serial, dsize);
}

int
check_hdlink(di_devlink_t devlink, void *arg)
{
	const char *link_path;
	di_node_t node;

	link_path = di_devlink_path(devlink);
	if (strncmp(link_path, "/dev/rdsk/", 10) == 0) {
		if (strcmp("s2", strrchr(link_path, 's')) == 0) {
			node = (di_node_t)arg;
			prt_hd_info(node, link_path);
			return (DI_WALK_TERMINATE);
		}
	}

	return (DI_WALK_CONTINUE);
}

int
check_hddev(di_node_t node, di_minor_t minor, void *arg)
{
	char *minor_path;
	int ret;

	ret = strncmp("ddi_block:cdrom", di_minor_nodetype(minor), 15);
	if (ret != 0) {
		minor_path = di_devfs_minor_path(minor);
		if (minor_path) {
			(void) di_devlink_walk(hdev_link, NULL, minor_path, 0,
				node, check_hdlink);
			di_devfs_path_free(minor_path);
		}
	}

	return (DI_WALK_CONTINUE);
}



void
drives_info()
{
	if (root_node == DI_NODE_NIL) {
		root_node = di_init("/",
			DINFOSUBTREE | DINFOMINOR | DINFOPROP | DINFOLYR);

		if (root_node == DI_NODE_NIL) {
			if (root_node != DI_NODE_NIL) {
				di_fini(root_node);
				root_node = DI_NODE_NIL;
			}
			err_fatal("di_init() failed");
		}
	}

	if (h_prom == DI_PROM_HANDLE_NIL) {
		h_prom = di_prom_init();
	}

	if (hdev_link == NULL) {
		if ((hdev_link = di_devlink_init(NULL, 0)) == NULL) {
			if (root_node != DI_NODE_NIL) {
				di_fini(root_node);
				root_node = DI_NODE_NIL;
			}
			err_fatal("di_devlink_init() failed");
		}
	}
	(void) printf(BD"Drives:"RS);

	(void) di_walk_minor(root_node, "ddi_block",
			0, NULL, check_hddev);

	if (hdev_link != NULL) {
		(void) di_devlink_fini(&hdev_link);
		hdev_link = NULL;
	}
	if (root_node != DI_NODE_NIL) {
		di_fini(root_node);
		root_node = DI_NODE_NIL;
	}
}
