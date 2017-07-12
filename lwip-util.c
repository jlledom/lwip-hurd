/*
   Copyright (C) 2000, 2007 Free Software Foundation, Inc.
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

#include <lwip-util.h>

#include <error.h>

#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <lwip/netifapi.h>

#include <lwip-hurd.h>
#include <options.h>
#include <netif/hurdethif.h>

static int
check_valid_ip_config(struct parse_interface *in)
{
  if(in->address.addr == INADDR_NONE)
    return 0;

  if (in->gateway.addr != INADDR_NONE
    && (in->gateway.addr & in->netmask.addr)
      != (in->address.addr & in->netmask.addr))
    in->gateway.addr = INADDR_NONE;

  return 1;
}

void
remove_ifs()
{
  struct netif *netif;

  while(netif_list != 0)
  {
    netif = netif_list;
    netifapi_netif_remove (netif);

    hurdethif_terminate (netif);
    free (netif);
  }

  return;
}

void
init_ifs(void *arg)
{
  struct parse_interface *in;
  struct parse_hook *ifs;
  struct netif *netif;
  int8_t ipv6_addr_idx;

  if(netif_list != 0)
    remove_ifs();

  /*
   * Go through the list backwards. For LwIP
   * to create its list in the proper order.
   */
  ifs = (struct parse_hook*) arg;
  for (in = ifs->interfaces + ifs->num_interfaces - 1;
        in >= ifs->interfaces; in--)
  {
    if(!check_valid_ip_config(in))
      continue;

    netif = malloc(sizeof(struct netif));
    memset(netif, 0, sizeof(struct netif));
    strncpy(netif->name, in->lwip_name, LWIP_NAME_LEN);

    /*
     * Create a new interface and configre IPv4.
     *
     * Fifth parameter (in->name) is a hook.
     */
    netifapi_netif_add(netif, &in->address, &in->netmask, &in->gateway,
            in->dev_name, hurdethif_init, tcpip_input);

    /* Add IPv6 configuration */
    netif->ip6_autoconfig_enabled = 1;
    netif_create_ip6_linklocal_address(netif, 1);

    /* Add user given unicast address */
    if(!ip6_addr_isany(&in->address6))
    {
      netif_add_ip6_address(netif, &in->address6, &ipv6_addr_idx);

      /* First use DAD to make sure nobody else has it */
      if(ipv6_addr_idx >= 0)
        netif_ip6_addr_set_state(netif, ipv6_addr_idx, IP6_ADDR_TENTATIVE);
      else
        error (1, 0, "No more free slots for new IPv6 addresses");
    }

    //Up the inerface
    netifapi_netif_set_up(netif);
  }

  /* Set the first interface with valid gateway as default */
  if (in->gateway.addr != INADDR_NONE)
  {
    netifapi_netif_set_default(netif);
  }

  /* Free the hook */
  free (ifs->interfaces);
  free (ifs);

  return;
}

static void
update_if(struct netif *netif)
{
}

void
inquire_device (struct netif *netif, uint32_t *addr, uint32_t *netmask,
                uint32_t *peer, uint32_t *broadcast, uint32_t *gateway,
                uint32_t *addr6, uint8_t *addr6_prefix_len)
{
  int i;

  if(netif)
  {
    if(addr)
      *addr = netif->ip_addr.u_addr.ip4.addr;

    if(netmask)
      *netmask = netif->netmask.u_addr.ip4.addr;

    if(peer)
      *peer = INADDR_NONE;

    if(broadcast)
      *broadcast =
          netif->ip_addr.u_addr.ip4.addr | ~netif->netmask.u_addr.ip4.addr;

    if(gateway)
      *gateway = netif->gw.u_addr.ip4.addr;

    if(addr6)
      for(i=0; i< LWIP_IPV6_NUM_ADDRESSES; i++)
      {
        *(addr6 + i*4 + 0) = netif->ip6_addr[i].u_addr.ip6.addr[0];
        *(addr6 + i*4 + 1) = netif->ip6_addr[i].u_addr.ip6.addr[1];
        *(addr6 + i*4 + 2) = netif->ip6_addr[i].u_addr.ip6.addr[2];
        *(addr6 + i*4 + 3) = netif->ip6_addr[i].u_addr.ip6.addr[3];
      }

    if(addr6_prefix_len)
      for(i=0; i< LWIP_IPV6_NUM_ADDRESSES; i++)
        *(addr6_prefix_len + i) = 64;
  }
}

error_t
configure_device (struct netif *netif, uint32_t addr, uint32_t netmask,
                  uint32_t peer, uint32_t broadcast, uint32_t gateway,
                  uint32_t *addr6, uint8_t *addr6_prefix_len)
{
  error_t err = 0;

  return err;
}
