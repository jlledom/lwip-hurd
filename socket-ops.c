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

#include <sys/mman.h>
#include <hurd/fshelp.h>

#include <lwip/sockets.h>
#include <lwip-hurd.h>

error_t
lwip_S_socket_create (struct trivfs_protid *master,
		 int sock_type,
		 int protocol,
		 mach_port_t *port,
		 mach_msg_type_name_t *porttype)
{
  error_t err;
  struct sock_user *user;
  int sock;
  int isroot;
  int *domain;

  if (!master)
    return EOPNOTSUPP;
  
  if (sock_type != SOCK_STREAM
      && sock_type != SOCK_DGRAM
      && sock_type != SOCK_RAW)
    return EPROTOTYPE;

  domain = (int*)master->po->cntl->hook;
  sock = lwip_socket(*domain, sock_type, protocol);
  if(sock < 0) {
    return errno;
  }

  isroot = master->isroot;
  if (!isroot)
  {
    struct stat st;

    st.st_uid = lwip_owner;
    st.st_gid = lwip_group;

    err = fshelp_isowner (&st, master->user);
    if (! err)
      isroot = 1;
  }

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
  if (!user)
    return EOPNOTSUPP;

  lwip_listen(user->sock, queue_limit);

  return errno;
}

error_t
lwip_S_socket_accept (struct sock_user *user,
		 mach_port_t *new_port,
		 mach_msg_type_name_t *new_port_type,
		 mach_port_t *addr_port,
		 mach_msg_type_name_t *addr_port_type)
{
  struct sock_user *newuser;
  union { struct sockaddr_storage storage; struct sockaddr sa; } addr = {};
  error_t err;
  int sock, newsock;

  if (!user)
    return EOPNOTSUPP;

  sock = user->sock;

  err = lwip_S_socket_create_address (0, addr.sa.sa_family,
             (void *) &addr.sa, sizeof addr,
             addr_port, addr_port_type);
  if(err)
    return err;

  newsock = lwip_accept(sock, &addr.sa, (socklen_t*)&addr.sa.sa_len);

  if (newsock != -1)
	{
	  newuser = make_sock_user (newsock, user->isroot, 0);
	  *new_port = ports_get_right (newuser);
	  *new_port_type = MACH_MSG_TYPE_MAKE_SEND;
	  ports_port_deref (newuser);
	}

  return errno;
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

  return errno;
}

error_t
lwip_S_socket_bind (struct sock_user *user,
			struct sock_addr *addr)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;
  if (! addr)
    return EADDRNOTAVAIL;

  err = lwip_bind(user->sock, &addr->address, addr->address.sa_len);

  /* MiG should do this for us, but it doesn't. */
  if (!err)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  return errno;
}

error_t
lwip_S_socket_name (struct sock_user *user,
			mach_port_t *addr_port,
			mach_msg_type_name_t *addr_port_name)
{
  error_t err;
  
  if (!user)
    return EOPNOTSUPP;

  err = make_sockaddr_port(user->sock, 0, addr_port, addr_port_name);

  return err;
}

error_t
lwip_S_socket_peername (struct sock_user *user,
		   mach_port_t *addr_port,
		   mach_msg_type_name_t *addr_port_name)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  err = make_sockaddr_port (user->sock, 1, addr_port, addr_port_name);

  return err;
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

  if (sockaddr_type != AF_INET && sockaddr_type != AF_UNSPEC)
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
  if (!addr)
    return EOPNOTSUPP;

  *type = addr->address.sa_family;
  if (*datalen < addr->address.sa_len)
    *data = mmap (0, addr->address.sa_len,
		  PROT_READ|PROT_WRITE, MAP_ANON, 0, 0);
  *datalen = addr->address.sa_len;
  memcpy (*data, &addr->address, addr->address.sa_len);

  return 0;
}

error_t
lwip_S_socket_shutdown (struct sock_user *user,
		   int direction)
{
  if (!user)
    return EOPNOTSUPP;

  lwip_shutdown(user->sock, direction);

  return errno;
}

error_t
lwip_S_socket_getopt (struct sock_user *user,
		 int level,
		 int option,
		 char **data,
		 size_t *datalen)
{
  if (! user)
    return EOPNOTSUPP;

  int len = *datalen;
  lwip_getsockopt(user->sock, level, option, *data, (socklen_t*)&len);
  *datalen = len;

  return errno;
}

error_t
lwip_S_socket_setopt (struct sock_user *user,
		 int level,
		 int option,
		 char *data,
		 size_t datalen)
{
  if (! user)
    return EOPNOTSUPP;

  lwip_setsockopt(user->sock, level, option, data, datalen);

  return errno;
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
  int sent;
  struct iovec iov = { data, datalen };
  struct msghdr m = { msg_name: addr ? &addr->address : 0,
		      msg_namelen: addr ? addr->address.sa_len : 0,
		      msg_flags: flags,
		      msg_controllen: 0, msg_iov: &iov, msg_iovlen: 1 };

  if (!user)
    return EOPNOTSUPP;

  /* Don't do this yet, it's too bizarre to think about right now. */
  if (nports != 0 || controllen != 0)
    return EINVAL;

  sent = lwip_sendmsg(user->sock, &m, flags);

  /* MiG should do this for us, but it doesn't. */
  if (addr && sent >= 0)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  if (sent >= 0)
  {
    *amount = sent;
  }

  return errno;
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
  error_t err;
  int alloced = 0;
  union { struct sockaddr_storage storage; struct sockaddr sa; } addr = {};

  if (!user)
    return EOPNOTSUPP;

  /* Instead of this, we should peek and the socket and only
     allocate as much as necessary. */
  if (amount > *datalen)
    {
      *data = mmap (0, amount, PROT_READ|PROT_WRITE, MAP_ANON, 0, 0);
      if (*data == MAP_FAILED)
        /* Should check whether errno is indeed ENOMEM --
           but this can't be done in a straightforward way,
           because the glue headers #undef errno. */
        return ENOMEM;
      alloced = 1;
    }

  err = lwip_recv(user->sock, *data, amount, flags);

  if (err < 0)
  {
    if (alloced)
      munmap (*data, amount);
  }
  else
  {
    *datalen = err;
    if (alloced && round_page (*datalen) < round_page (amount))
      munmap (*data + round_page (*datalen),
    round_page (amount) - round_page (*datalen));

    err = lwip_S_socket_create_address (0, addr.sa.sa_family,
           (void *) &addr.sa, sizeof addr,
           addrport, addrporttype);
    if (err && alloced)
      munmap (*data, *datalen);

    *outflags = lwip_fcntl(user->sock, F_GETFL, 0);
    *nports = 0;
    *portstype = MACH_MSG_TYPE_COPY_SEND;
    *controllen = 0;
  }

  return errno;
}
