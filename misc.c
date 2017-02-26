/*
   Copyright (C) 1995,96,2000 Free Software Foundation, Inc.
   Written by Michael I. Bushnell, p/BSG.

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

#include "lwip-hurd.h"

/* Create a sock_user structure, initialized from SOCK and ISROOT.
   If NOINSTALL is set, don't put it in the portset.*/
struct sock_user *
make_sock_user (int sock, int isroot, int noinstall)
{
  error_t err;
  struct sock_user *user;

  if (noinstall)
    err = ports_create_port_noinstall (socketport_class, lwip_bucket,
				       sizeof (struct sock_user), &user);
  else
    err = ports_create_port (socketport_class, lwip_bucket,
			     sizeof (struct sock_user), &user);
  if (err)
    return 0;

  user->isroot = isroot;
  user->sock = sock;
  return user;
}

/* Nothing need be done here. */
void
clean_socketport (void *arg)
{
}

/* Nothing need be done here. */
void
clean_addrport (void *arg)
{
}
