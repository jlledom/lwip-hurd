#   Copyright (C) 2017 Free Software Foundation, Inc.
#
#   This file is part of the GNU Hurd.
#
#   The GNU Hurd is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2, or (at
#   your option) any later version.
#
#   The GNU Hurd is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.

dir			= lwip
makemode	= server

PORTDIR = $(srcdir)/port
LIBLWIPDIR = $(srcdir)/../liblwip

SRCS		= main.c io-ops.c socket-ops.c pfinet-ops.c iioctl-ops.c port-objs.c \
						options.c lwip-util.c
IFSRCS	= hurdethif.c hurdloopif.c hurdtunif.c
MIGSRCS		= ioServer.c socketServer.c pfinetServer.c iioctlServer.c
OBJS		= $(patsubst %.S,%.o,$(patsubst %.c,%.o,\
				$(SRCS) $(IFSRCS) $(MIGSRCS)))

HURDLIBS= lwip trivfs fshelp ports ihash shouldbeinlibc iohelp
LDLIBS = -lpthread

target = lwip

include ../Makeconf

vpath %.c $(PORTDIR) \
		$(PORTDIR)/netif

CFLAGS += -I$(PORTDIR)/include \
		-I$(LIBLWIPDIR)/dist/include \
		-I$(LIBLWIPDIR)/port/include

CPPFLAGS += -imacros $(srcdir)/config.h
MIGCOMSFLAGS += -prefix lwip_
mig-sheader-prefix = lwip_
io-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h
socket-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h
iioctl-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h

# cpp doesn't automatically make dependencies for -imacros dependencies. argh.
lwip_io_S.h ioServer.c lwip_socket_S.h socketServer.c: mig-mutate.h
$(OBJS): config.h
