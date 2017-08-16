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

/* Loopback devices module */

#include <netif/hurdloopif.h>

#include <net/if.h>
#include <net/if_arp.h>

#include <lwip-util.h>

/* Set the device flags */
static error_t
hurdloopif_device_set_flags (struct netif *netif, uint16_t flags)
{
  error_t err = 0;
  hurdloopif *loopif;

  loopif = netif_get_state (netif);
  loopif->flags = flags;

  return err;
}

/*
 * Update the interface's MTU
 */
error_t
hurdloopif_update_mtu (struct netif * netif, uint32_t mtu)
{
  error_t err = 0;

  netif->mtu = mtu;

  return err;
}

/*
 * Release all resources of this netif.
 *
 * Returns 0 on success.
 */
error_t
hurdloopif_terminate (struct netif * netif)
{
  /* Free the hook */
  mem_free (netif->state);

  return 0;
}

/*
 * Set up the LwIP loopback interface
 *
 * This function should be passed as a parameter to netif_add() so it may be
 * called many times.
 */
err_t
hurdloopif_init (struct netif * netif)
{
  err_t err = 0;
  hurdloopif *loopif;

  loopif = netif_get_state (netif);

  loopif->devname = LOOP_DEV_NAME;
  loopif->type = ARPHRD_LOOPBACK;

  netif->mtu = TCP_MSS + 0x28;

  hurdloopif_device_set_flags (netif, IFF_UP | IFF_RUNNING | IFF_LOOPBACK);

  loopif->terminate = hurdloopif_terminate;
  loopif->update_mtu = hurdloopif_update_mtu;
  loopif->change_flags = hurdloopif_device_set_flags;

  return err;
}
