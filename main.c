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
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include "hwi.h"

void
err_fatal(char *s) {

	(void) fprintf(stderr, "%s\n", s);
	exit(1);
}

int
main(int argc, char **argv) {

	(void) setlocale(LC_ALL, "");

	sys_info();
	cpu_info();
	mem_info();
	pci_info(VIDEO);
	pci_info(NET);
	drives_info();

	return (0);
}
