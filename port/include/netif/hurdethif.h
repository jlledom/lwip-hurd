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

#ifndef LWIP_HURDETHIF_H
#define LWIP_HURDETHIF_H

#include <hurd/ports.h>

#include <lwip/netif.h>

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 */
struct hurdethif {
  struct eth_addr *ethaddr;
  device_t ether_port;
  struct port_info *readpt;
  mach_port_t readptname;
  char *devname;
};

err_t hurdethif_init(struct netif *netif);
err_t hurdethif_terminate(struct netif *netif);
error_t hurdethif_input_init();

#endif /* LWIP_HURDETHIF_H */
