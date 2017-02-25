/* Interface functions for the socket.defs interface.
   Copyright (C) 1995,96,97,99,2000,02,07 Free Software Foundation, Inc.
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

#include <lwip_socket_S.h>

#include <errno.h>


error_t
lwip_S_socket_create (struct trivfs_protid *master,
		 int sock_type,
		 int protocol,
		 mach_port_t *port,
		 mach_msg_type_name_t *porttype)
{
  return EOPNOTSUPP;
}


/* Listen on a socket. */
error_t
lwip_S_socket_listen (socket_t sock, int queue_limit)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_accept (socket_t sock,
		 mach_port_t *new_port,
		 mach_msg_type_name_t *new_port_type,
		 mach_port_t *addr_port,
		 mach_msg_type_name_t *addr_port_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_connect (socket_t sock,
		  mach_port_t addr)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_bind (socket_t sock,
	       mach_port_t addr)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_name ( socket_t sock,
			mach_port_t *addr,
			mach_msg_type_name_t *addrPoly)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_peername (socket_t sock,
		   mach_port_t *addr_port,
		   mach_msg_type_name_t *addr_port_name)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_connect2 (socket_t sock1,
		   socket_t sock2)
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
  return EOPNOTSUPP;
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
lwip_S_socket_whatis_address (mach_port_t addr,
			 int *type,
			 char **data,
			 mach_msg_type_number_t *datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_shutdown (socket_t sock,
		   int direction)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_getopt (socket_t sock,
		 int level,
		 int option,
		 char **data,
		 size_t *datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_setopt (socket_t sock,
		 int level,
		 int option,
		 char *data,
		 size_t datalen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_socket_send (socket_t sock,
	       mach_port_t addr,
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
lwip_S_socket_recv (socket_t sock,
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
