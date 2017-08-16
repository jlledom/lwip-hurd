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

#ifndef LWIP_UTIL_H
#define LWIP_UTIL_H

#define LOOP_DEV_NAME   "lo"

#include <lwip/netif.h>

/* Prototype for module initialization callbacks */
typedef err_t (*module_init_t) (struct netif * netif);

void init_ifs (void *arg);

void inquire_device (struct netif *netif, uint32_t * addr, uint32_t * netmask,
		     uint32_t * peer, uint32_t * broadcast,
		     uint32_t * gateway, uint32_t * addr6,
		     uint8_t * addr6_prefix_len);
error_t configure_device (struct netif *netif, uint32_t addr,
			  uint32_t netmask, uint32_t peer, uint32_t broadcast,
			  uint32_t gateway, uint32_t * addr6,
			  uint8_t * addr6_prefix_len);

#endif /* LWIP_UTIL_H */
