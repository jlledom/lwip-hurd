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
   along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Loopback devices module */

#include <netif/hurdloopif.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <string.h>

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
  free (netif_get_state (netif)->devname);
  mem_free (netif_get_state (netif));

  return 0;
}

/*
 * Set up the LwIP loopback interface
 *
 * This function should be passed as a parameter to netif_add() so it may be
 * called many times.
 */
error_t
hurdloopif_init (struct netif * netif)
{
  error_t err = 0;
  hurdloopif *loopif;

  /*
   * Replace the hook by a new one with the proper size.
   * The old one is in the stack and will be removed soon.
   */
  loopif = mem_malloc (sizeof (hurdloopif));
  if (loopif == NULL)
    {
      LWIP_DEBUGF (NETIF_DEBUG, ("hurdloopif_init: out of memory\n"));
      return ERR_MEM;
    }
  memset (loopif, 0, sizeof (hurdloopif));
  memcpy (loopif, netif_get_state (netif), sizeof (struct ifcommon));
  netif->state = loopif;

  loopif->devname = LOOP_DEV_NAME;
  loopif->type = ARPHRD_LOOPBACK;

  netif->mtu = TCP_MSS + 0x28;

  hurdloopif_device_set_flags (netif, IFF_UP | IFF_RUNNING | IFF_LOOPBACK);

  loopif->open = 0;
  loopif->close = 0;
  loopif->terminate = hurdloopif_terminate;
  loopif->update_mtu = hurdloopif_update_mtu;
  loopif->change_flags = hurdloopif_device_set_flags;

  return err;
}
