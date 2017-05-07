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

#include <lwip/sockets.h>

/* Create a sockaddr port.  Fill in *ADDR and *ADDRTYPE accordingly.
   The address should come from SOCK; PEER is 0 if we want this socket's
   name and 1 if we want the peer's name. */
error_t
make_sockaddr_port (int sock,
		    int peer,
		    mach_port_t *addr,
		    mach_msg_type_name_t *addrtype)
{
  union { struct sockaddr_storage storage; struct sockaddr sa; } buf;
  int buflen = sizeof buf;
  error_t err;
  struct sock_addr *addrstruct;

  if(peer)
    err = lwip_getpeername(sock, &buf.sa, (socklen_t*)&buflen);
  else
    err = lwip_getsockname(sock, &buf.sa, (socklen_t*)&buflen);
  if (err)
    return -err;

  err = ports_create_port (addrport_class, lwip_bucket,
			   (offsetof (struct sock_addr, address)
			    + buflen), &addrstruct);
  if (!err)
    {
      addrstruct->address.sa_family = buf.sa.sa_family;
      addrstruct->address.sa_len = buflen;
      memcpy (addrstruct->address.sa_data, buf.sa.sa_data,
	      buflen - offsetof (struct sockaddr, sa_data));
      *addr = ports_get_right (addrstruct);
      *addrtype = MACH_MSG_TYPE_MAKE_SEND;
    }

  ports_port_deref (addrstruct);

  return err;
}

struct socket *
sock_alloc (void)
{
  struct socket *sock;

  sock = malloc (sizeof *sock);
  if (!sock)
    return 0;
  memset (sock, 0, sizeof *sock);
  sock->identity = MACH_PORT_NULL;

  return sock;
}

/* This is called from the port cleanup function below, and on
   a newly allocated socket when something went wrong in its creation.  */
void
sock_release (struct socket *sock)
{
  lwip_close (sock->sockno);
  
  if (sock->identity != MACH_PORT_NULL)
    mach_port_destroy (mach_task_self (), sock->identity);

  free (sock);
}

/* Create a sock_user structure, initialized from SOCK and ISROOT.
   If NOINSTALL is set, don't put it in the portset.*/
struct sock_user *
make_sock_user (struct socket *sock, int isroot, int noinstall)
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

/*  Release the referenced socket. */
void
clean_socketport (void *arg)
{
  struct sock_user *const user = arg;
  
  sock_release(user->sock);
}

/* Nothing need be done here. */
void
clean_addrport (void *arg)
{
}
