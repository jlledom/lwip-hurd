/*
   Copyright (C) 1995,96,97,99,2000,02,07 Free Software Foundation, Inc.
   Written by Joan Lledó.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <lwip/sio.h>

sio_fd_t
sio_open(u8_t devnum)
{
	return 0;
}

void
sio_send(u8_t c, sio_fd_t fd)
{
	return;
}

u8_t
sio_recv(sio_fd_t fd)
{
	return -1;
}

u32_t
sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
	return -1;
}

u32_t
sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
	return -1;
}

u32_t
sio_write(sio_fd_t fd, u8_t *data, u32_t len)
{
	return -1;
}

void
sio_read_abort(sio_fd_t fd)
{
	return;
}
