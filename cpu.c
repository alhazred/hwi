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
 * Portions Copyright (c) 2012 DEY Storage Systems, Inc.  All rights reserved.
 * Portions Copyright 2012 Nexenta Systems, Inc.  All rights reserved.
 * Portions Copyright 2019 Joyent, Inc.
 * Copyright 2019 Alexander Eremin <alexander.r.eremin@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <kstat.h>
#include <libintl.h>
#include <locale.h>
#include <libgen.h>
#include <ctype.h>
#include <errno.h>

#include "hwi.h"

static struct link *pchips = NULL;
static struct link *vcpus = NULL;

static void
add_link(struct link **ins, struct link *item)
{
	item->l_next = *ins;
	*ins = item;
}

static void *
get_link(void *list, int id, struct link ***insp)
{
	struct link **ins = list;
	struct link *l;

	while ((l = *ins) != NULL) {
		if (l->l_id == id)
			return (l->l_ptr);
		if (l->l_id > id)
			break;
		ins = &l->l_next;
	}
	if (insp != NULL)
		*insp = ins;
	return (NULL);
}

static char *
uniq_spc(char *str)
{
	char *from, *to;
	int spc = 0;
	to = from = str;

	while (1) {
		if (spc && *from == ' ' && to[-1] == ' ') {
			++from;
		} else {
			spc = (*from == ' ') ? 1 : 0;
			*to++ = *from++;
			if (!to[-1])
				break;
		}
	}
	return (str);
}

void
cpu_info()
{
	kstat_ctl_t *kc;
	kstat_t *ksp;
	kstat_named_t *knp;
	struct vcpu *vc;
	struct pchip *chip, *p;
	struct link **lins, **pins;
	struct link *la, *lb;
	struct vcpu *vcp;
	int online = 0;

	if ((kc = kstat_open()) == NULL) {
		err_fatal("kstat_open() failed");
	}
	if ((ksp = kstat_lookup(kc, "cpu_info", -1, NULL)) == NULL) {
		err_fatal("kstat_lookup() failed");
	}
	for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next) {
		if (strcmp(ksp->ks_module, "cpu_info") != 0)
			continue;
		if (kstat_read(kc, ksp, NULL) == NULL)
			err_fatal("kstat_read() failed");

		vc = get_link(&vcpus, ksp->ks_instance, &lins);
		if (vc == NULL) {
			vc = calloc(1, sizeof (struct vcpu));
			vc->v_link.l_id = ksp->ks_instance;
			vc->v_link_pchip.l_id = ksp->ks_instance;
			vc->v_link.l_ptr = vc;
			vc->v_link_pchip.l_ptr = vc;
			add_link(lins, &vc->v_link);
		}

		if ((knp = kstat_data_lookup(ksp, "state")) != NULL) {
			vc->v_state = strdup(knp->value.c);
		} else {
			vc->v_state = "unknown";
		}
		if ((knp = kstat_data_lookup(ksp, "cpu_type")) != NULL) {
			vc->v_cpu_type = strdup(knp->value.c);
		}
		if ((knp = kstat_data_lookup(ksp, "fpu_type")) != NULL) {
			vc->v_fpu_type = strdup(knp->value.c);
		}
		if ((knp = kstat_data_lookup(ksp, "clock_MHz")) != NULL) {
			vc->v_clock_mhz = knp->value.l;
		}
		if ((knp = kstat_data_lookup(ksp, "brand")) == NULL) {
			vc->v_brand = "unknown";
		} else {
			vc->v_brand = strdup(knp->value.str.addr.ptr);
		}
		if ((knp = kstat_data_lookup(ksp, "model")) != NULL) {
			vc->v_model = knp->value.l;
		}
		if ((knp = kstat_data_lookup(ksp, "family")) != NULL) {
			vc->v_family = knp->value.l;
		}
		if ((knp = kstat_data_lookup(ksp, "stepping")) != NULL) {
			vc->v_step = knp->value.l;
		}
		if ((knp = kstat_data_lookup(ksp, "vendor_id")) == NULL) {
			vc->v_vendor = "unknown";
		} else {
			vc->v_vendor = strdup(knp->value.str.addr.ptr);
		}
		if ((knp = kstat_data_lookup(ksp, "chip_id")) != NULL) {
			vc->v_pchip_id = knp->value.l;
		}

		chip = get_link(&pchips, vc->v_pchip_id, &lins);
		if (chip == NULL) {
			chip = calloc(1, sizeof (struct pchip));
			chip->p_link.l_id = vc->v_pchip_id;
			chip->p_link.l_ptr = chip;
			add_link(lins, &chip->p_link);
		}
		vc->v_pchip = chip;

		(void) get_link(&chip->p_vcpus, vc->v_link.l_id, &pins);
		add_link(pins, &vc->v_link_pchip);
		chip->p_nvcpu++;
	}

	(void) kstat_close(kc);

	for (la = pchips; la != NULL; la = la->l_next) {
		p = la->l_ptr;
		for (lb = p->p_vcpus; lb != NULL; lb = lb->l_next) {
			vcp = lb->l_ptr;
			if (strcmp(vcp->v_state, "on-line") == 0) {
				online++;
				break;
			}
		}
	}

	(void) printf(BD"CPU(s):"RS"    %d x %d core %s "BD"family:"
		RS"%ld "BD"model:"RS" %ld "BD"step:"RS" %ld "BD"type:"RS
		" %s "BD"fpu:"RS" %s\n", online, p->p_nvcpu,
		uniq_spc(vc->v_brand), vc->v_family, vc->v_model,
		vc->v_step, vc->v_cpu_type, vc->v_fpu_type);
}
