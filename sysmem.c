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
#include <unistd.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <libgen.h>
#include <ctype.h>
#include <errno.h>
#include <smbios.h>
#include <sys/types.h>

#include "hwi.h"

static int
do_slots(smbios_hdl_t *shp, const smbios_struct_t *sp, void *arg)
{
	smbios_memarray_t ma;

	if (sp->smbstr_type == SMB_TYPE_MEMARRAY) {
		(void) smbios_info_memarray(shp, sp->smbstr_id, &ma);
		memslots += ma.smbma_ndevs;
	}
	return (0);
}

void
sys_info()
{
	int oflags = SMB_O_NOCKSUM | SMB_O_NOVERS;
	id_t id;
	int i, err;
	smbios_system_t sys;
	smbios_hdl_t *shp;
	smbios_info_t info;
	smbios_struct_t s_mb;
	smbios_info_t mb;
	smbios_bios_t bios;
	smbios_ipmi_t ipmi;

	if ((shp = smbios_open(NULL, SMB_VERSION, oflags, &err)) == NULL) {
		err_fatal("smbios_open() failed");
	}

	if ((id = smbios_info_system(shp, &sys)) != SMB_ERR &&
	    smbios_info_common(shp, id, &info) != SMB_ERR) {
		(void) printf(BD"Machine:   System:"RS" %s "BD"product:"RS
		    " %s "BD"v:"RS" %s "BD"serial:"RS" %s\n",
		    info.smbi_manufacturer, info.smbi_product,
		    info.smbi_version[0] == '\0' ? "None" : info.smbi_version,
		    info.smbi_serial);

		(void) printf(BD"%17s"RS, "UUID: ");
		for (i = 0; i < sys.smbs_uuidlen; i++) {
			(void) printf("%02x", sys.smbs_uuid[i]);
			if (i == 3 || i == 5 || i == 7 || i == 9)
				(void) printf("-");
		}
		(void) printf("\n");
	}

	if ((smbios_lookup_type(shp, SMB_TYPE_BASEBOARD, &s_mb)) != SMB_ERR &&
	    smbios_info_common(shp, s_mb.smbstr_id, &mb) != SMB_ERR) {
		(void) printf(BD"%15s:"RS" %s "BD"model:"RS" %s "BD"serial:"
		    RS" %s\n", "Mobo", mb.smbi_manufacturer,
		    mb.smbi_product, mb.smbi_serial);
	}

	(void) smbios_info_bios(shp, &bios);
	(void) printf(BD"%15s:"RS" %s "BD"v:"RS" %s "BD"date:"RS" %s\n",
	    "BIOS", bios.smbb_vendor, bios.smbb_version, bios.smbb_reldate);

	(void) smbios_info_ipmi(shp, &ipmi);
	(void) printf(BD"%14s: IPMI version:"RS" %u.%u\n", "BMC",
	    ipmi.smbip_vers.smbv_major, ipmi.smbip_vers.smbv_minor);

	(void) smbios_iter(shp, do_slots, NULL);

	smbios_close(shp);
}

void
mem_info()
{
	long pagesize, npages;
	pagesize = sysconf(_SC_PAGESIZE);
	npages = sysconf(_SC_PHYS_PAGES);
	const int64_t mbyte = 1024 * 1024;
	int64_t ii = (int64_t)pagesize * npages;

	(void) printf(BD"Memory:    Memory size:"RS" %ld Megabytes "BD
	    "devices:" RS " %u\n",
	    (long)((ii + mbyte - 1) / mbyte), memslots);
}
