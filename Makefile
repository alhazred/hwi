#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright 2019 Alexander Eremin <alexander.r.eremin@gmail.com>
#

PROG = hwi

LDFLAGS = -lkstat -lsmbios -ldevinfo -ldlpi -lsocket
PCIDBLIB = /usr/lib/libpcidb.so.1

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

$(PROG): $(OBJ)
	$(CC) -Wall -o $@ $^ $(LDFLAGS) $(PCIDBLIB)

.PHONY: clean
clean:
	rm -f $(OBJ) $(PROG)



