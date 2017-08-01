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

#ifndef LWIP_HURDTUNIF_H
#define LWIP_HURDTUNIF_H

#include <hurd/ports.h>

#include <lwip/netif.h>
#include <netif/ifcommon.h>

struct hurdtunif
{
  struct ifcommon comm;

  struct trivfs_control *cntl;  /* Identify the tunnel device in use */
  file_t underlying;            /* Underlying node where the tunnel is bound */
  struct iouser *user;          /* Restrict the access to one user at a time */
};

struct port_class *tunnel_cntlclass;
struct port_class *tunnel_class;

err_t hurdtunif_init(struct netif *netif);
error_t hurdtunif_module_init();

#endif /* LWIP_HURDTUNIF_H */
