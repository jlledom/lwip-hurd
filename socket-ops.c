/*
   Copyright (C) 2017 Free Software Foundation, Inc.
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

#include <lwip_socket_S.h>

#include <lwip/sockets.h>
#include <lwip-hurd.h>

error_t
lwip_S_socket_create (struct trivfs_protid *master,
		 int sock_type,
		 int protocol,
		 mach_port_t *port,
		 mach_msg_type_name_t *porttype)
{
  struct sock_user *user;
  int sock;
  int isroot;
  int *domain;

  if (!master)
    return EOPNOTSUPP;

  domain = (int*)master->po->cntl->hook;
  sock = lwip_socket(*domain, sock_type, protocol);
  if(sock < 0) {
    return errno;
  }
  isroot = master->isroot;

  user = make_sock_user(sock, isroot, 0);
  *port = ports_get_right (user);
  *porttype = MACH_MSG_TYPE_MAKE_SEND;
  ports_port_deref(user);

  return errno;
}


/* Listen on a socket. */
error_t
lwip_S_socket_listen (struct sock_user *user, int queue_limit)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_accept (struct sock_user *user,
		 mach_port_t *new_port,
		 mach_msg_type_name_t *new_port_type,
		 mach_port_t *addr_port,
		 mach_msg_type_name_t *addr_port_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_connect (struct sock_user *user,
			struct sock_addr *addr)
{
  error_t err;

  if (!user || !addr)
    return EOPNOTSUPP;

  err = lwip_connect(user->sock, &addr->address, addr->address.sa_len);

  /* MiG should do this for us, but it doesn't. */
  if (!err)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  return err;
}

error_t
lwip_S_socket_bind (struct sock_user *user,
			struct sock_addr *addr)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_name (struct sock_user *user,
			mach_port_t *addr,
			mach_msg_type_name_t *addrPoly)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_peername (struct sock_user *user,
		   mach_port_t *addr_port,
		   mach_msg_type_name_t *addr_port_name)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_connect2 (struct sock_user *user,
			struct sock_user *sock2)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_create_address (mach_port_t server,
			 int sockaddr_type,
			 char *data,
			 mach_msg_type_number_t data_len,
			 mach_port_t *addr_port,
			 mach_msg_type_name_t *addr_port_type)
{
  error_t err;
  struct sock_addr *addrstruct;
  const struct sockaddr *const sa = (void *) data;

  if (sockaddr_type != AF_INET)
    return EAFNOSUPPORT;
  if (sa->sa_family != sockaddr_type
      || data_len < offsetof (struct sockaddr, sa_data))
    return EINVAL;

  err = ports_create_port (addrport_class, lwip_bucket,
			   (offsetof (struct sock_addr, address)
			    + data_len), &addrstruct);
  if (err)
    return err;

  memcpy (&addrstruct->address, data, data_len);

  /* BSD does not require incoming sa_len to be set, so we don't either.  */
  addrstruct->address.sa_len = data_len;

  *addr_port = ports_get_right (addrstruct);
  *addr_port_type = MACH_MSG_TYPE_MAKE_SEND;
  ports_port_deref (addrstruct);
  return 0;
}

error_t
lwip_S_socket_fabricate_address (mach_port_t server,
			    int sockaddr_type,
			    mach_port_t *addr_port,
			    mach_msg_type_name_t *addr_port_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_whatis_address (struct sock_addr *addr,
			 int *type,
			 char **data,
			 mach_msg_type_number_t *datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_shutdown (struct sock_user *user,
		   int direction)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_getopt (struct sock_user *user,
		 int level,
		 int option,
		 char **data,
		 size_t *datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_setopt (struct sock_user *user,
		 int level,
		 int option,
		 char *data,
		 size_t datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_send (struct sock_user *user,
	       struct sock_addr *addr,
	       int flags,
	       char *data,
	       size_t datalen,
	       mach_port_t *ports,
	       size_t nports,
	       char *control,
	       size_t controllen,
	       mach_msg_type_number_t *amount)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_recv (struct sock_user *user,
	       mach_port_t *addrport,
	       mach_msg_type_name_t *addrporttype,
	       int flags,
	       char **data,
	       size_t *datalen,
	       mach_port_t **ports,
	       mach_msg_type_name_t *portstype,
	       size_t *nports,
	       char **control,
	       size_t *controllen,
	       int *outflags,
	       mach_msg_type_number_t amount)
{
  return EOPNOTSUPP;
}
