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
 * Copyright 2019 Alexander Eremin <alexander.r.eremin@gmail.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libdevinfo.h>
#include <sys/types.h>
#include <sys/pci.h>
#include <libdlpi.h>
#include <sys/socket.h>

#include "hwi.h"

static char *
get_physical_address_dlpi(char *nicname)
{
	uint_t physaddrlen = DLPI_PHYSADDR_MAX;
	uchar_t physaddr[DLPI_PHYSADDR_MAX];
	char *buf;
	dlpi_handle_t dh;

	if (dlpi_open(nicname, &dh, 0) != DLPI_SUCCESS) {
		err_fatal("dlpi_open() failed");
	}

	if (dlpi_get_physaddr(dh, DL_CURR_PHYS_ADDR, physaddr,
	    &physaddrlen) != DLPI_SUCCESS) {
		dlpi_close(dh);
		err_fatal("dlpi_get_physaddr() failed");
	}

	buf = _link_ntoa(physaddr, NULL, physaddrlen, 1);
	dlpi_close(dh);

	return (buf);
}

static int
devfs_entry(di_node_t node, di_minor_t minor, void *arg)
{
	char *drv;
	int ddm_type;
	char *cp;
	int *vid, *did;
	pci_regspec_t *reg;
	const char *vname, *dname;
	pcidb_vendor_t *pciv;
	pcidb_device_t *pcid;
	uint32_t bus0, bus1, bus2;

	cp = di_minor_nodetype(minor);
	if ((cp == NULL) || (strcmp(cp, type == VIDEO ?
	    DDI_NT_DISPLAY : DDI_NT_NET))) {
		return (DI_WALK_CONTINUE);
	}

	ddm_type = di_minor_type(minor);
	if (ddm_type == DDM_INTERNAL_PATH) {
		return (DI_WALK_CONTINUE);
	}

	drv = di_driver_name(node);
	if (drv == NULL) {
		return (DI_WALK_CONTINUE);
	}

	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node,
	    "vendor-id", &vid) > 0) {
		pciv = pcidb_lookup_vendor(pcidb, vid[0]);
		if (pciv == NULL)
			return (DI_WALK_CONTINUE);
		vname = pcidb_vendor_name(pciv);
	} else {
		vname = "unknown";
	}

	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node,
	    "device-id", &did) > 0) {
		pcid = pcidb_lookup_device_by_vendor(pciv, did[0]);
		if (pcid == NULL)
			return (DI_WALK_CONTINUE);
		dname = pcidb_device_name(pcid);
	} else {
		dname = "unknown";
	}

	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node,
	    "reg", (int **)&reg) > 0) {
		bus0 = PCI_REG_BUS_G(reg[0].pci_phys_hi);
		bus1 = PCI_REG_DEV_G(reg[0].pci_phys_hi);
		bus2 = PCI_REG_FUNC_G(reg[0].pci_phys_hi);
	}

	if (type == VIDEO) {
		(void) printf(BD"  Card:"RS" %s %s "BD"bus-ID:"
		    RS" 0%x:0%x.%x\n", vname, dname, bus0, bus1, bus2);
	} else if (type == NET) {
		num_eth++;
		char en[16];
		(void) sprintf(en, "%s%d", drv, di_instance(node));
		(void) printf(BD"%*s:"RS" %s "BD"Card:"RS" %s %s "
		    BD"mac:"RS" %s "BD"bus-ID:"RS" 0%x:0%x.%x\n",
		    num_eth == 1 ? 5 : 13, "IF", en, vname, dname,
		    get_physical_address_dlpi(en), bus0, bus1, bus2);
	}

	return (DI_WALK_CONTINUE);
}

void
pci_info(int cmd)
{
	di_node_t root;

	root = di_init("/", DINFOFORCE | DINFOSUBTREE | DINFOMINOR | DINFOPROP);
	if (root == DI_NODE_NIL) {
		err_fatal("di_init() failed");
	}

	pcidb = pcidb_open(PCIDB_VERSION);
	if (pcidb == NULL) {
		err_fatal("pcidb_open() failed");
	}

	type = cmd;
	(void) printf(type == VIDEO ? BD"Graphics:"RS  : BD"Network:"RS);
	(void) di_walk_minor(root, type == VIDEO ?
	    DDI_NT_DISPLAY : DDI_NT_NET, 0, NULL, devfs_entry);
	di_fini(root);
}
