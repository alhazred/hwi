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
	int c;

	(void) setlocale(LC_ALL, "");

	if (argc == 1) {
		sys_info();
		cpu_info();
		mem_info();
		pci_info(VIDEO);
		pci_info(NET);
		drives_info();

		return (0);
	}

	while ((c = getopt(argc, argv, ":cdmnsvV")) != -1)  {
		switch (c)  {
		case 'c':
			cpu_info();
			break;
		case 'd':
			drives_info();
			break;

		case 'm':
			mem_info();
			break;
		case 'n':
			pci_info(NET);
			break;
		case 's':
			sys_info();
			break;
		case 'v':
			pci_info(VIDEO);
			break;
		case 'V':
			(void) fprintf(stderr, "hwi version %s\n", VERSION);
			break;
		default:
			(void) fprintf(stderr, "Usage: hwi [ -c | -d | -m | -n | -s | -v | -V ]\n");
		}
	}
	return (0);
}
