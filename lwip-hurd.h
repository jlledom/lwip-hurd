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

#ifndef LWIP_HURD_H
#define LWIP_HURD_H

#include <hurd/ports.h>
#include <hurd/trivfs.h>

#include <lwip/sockets.h>

struct port_bucket *lwip_bucket;
struct port_class *socketport_class;
struct port_class *addrport_class;

struct port_class *lwip_protid_portclass;
struct port_class *lwip_cntl_portclass;

mach_port_t fsys_identity;

/* Trivfs control structure for lwip.  */
struct trivfs_control *lwipcntl;

struct socket
{
  int sockno;
  mach_port_t identity;
  uint_fast32_t refcnt;
};

/* Multiple sock_user's can point to the same socket. */
struct sock_user
{
  struct port_info pi;
  int isroot;
  struct socket *sock;
};

/* Socket address ports. */
struct sock_addr
{
  struct port_info pi;
  struct sockaddr address;
};

/* Owner of the underlying node.  */
uid_t lwip_owner;

/* Group of the underlying node.  */
uid_t lwip_group;

struct socket *sock_alloc (void);
void sock_release (struct socket *);

void clean_addrport (void*);
void clean_socketport (void*);

struct sock_user *make_sock_user (struct socket*, int, int, int);
error_t make_sockaddr_port (int, int,mach_port_t*, mach_msg_type_name_t*);

#endif
