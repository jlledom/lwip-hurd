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
#  You should have received a copy of the GNU General Public License
#  along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.

dir		= lwip
makemode	= server

PORTDIR = $(srcdir)/port

SRCS =
IFSRCS =
MIGSRCS =
OBJS =

# If we have a configured tree, include the configuration so that we
# can conditionally build translators.
ifneq (,$(wildcard ../config.make))
 include ../config.make
endif

ifeq ($(HAVE_LIBLWIP), yes)
SRCS		= main.c io-ops.c socket-ops.c pfinet-ops.c iioctl-ops.c port-objs.c \
						options.c lwip-util.c
IFSRCS	= ifcommon.c hurdethif.c hurdloopif.c hurdtunif.c
MIGSRCS		= ioServer.c socketServer.c pfinetServer.c iioctlServer.c
OBJS		= $(patsubst %.S,%.o,$(patsubst %.c,%.o,\
			$(SRCS) $(IFSRCS) $(MIGSRCS)))

# cpp doesn't automatically make dependencies for -imacros dependencies. argh.
lwip_io_S.h ioServer.c lwip_socket_S.h socketServer.c: mig-mutate.h

target = lwip
endif

HURDLIBS= trivfs fshelp ports ihash shouldbeinlibc iohelp
LDLIBS = -lpthread $(liblwip_LIBS)

include ../Makeconf

vpath %.c $(PORTDIR) \
		$(PORTDIR)/netif

CFLAGS += -I$(PORTDIR)/include $(liblwip_CFLAGS)

CPPFLAGS += -imacros $(srcdir)/config.h
MIGCOMSFLAGS += -prefix lwip_
mig-sheader-prefix = lwip_
io-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h
socket-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h
iioctl-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h

$(OBJS): config.h
