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

#include <lwip_iioctl_S.h>

#include <lwip/sockets.h>

#include <lwip-hurd.h>
#include <lwip-util.h>
#include <netif/hurdethif.h>

/* Get the interface from its name */
struct netif *get_if (char *name)
{
  char ifname[16];
  struct netif *netif;

  memcpy (ifname, name, IFNAMSIZ-1);
  ifname[IFNAMSIZ-1] = 0;

  netif = netif_list;
  while(netif != 0)
  {
    if (strcmp (((struct hurdethif *)netif->state)->devname, ifname) == 0)
      break;

    netif = netif->next;
  }

  return netif;
}

enum siocgif_type
{
  ADDR,
  NETMASK,
  DSTADDR,
  BRDADDR
};

#define SIOCGIF(name, type)						\
  kern_return_t								\
  lwip_S_iioctl_siocgif##name (struct sock_user *user,                       \
        ifname_t ifnam,				\
        sockaddr_t *addr)				\
  {									\
    return siocgifXaddr (user, ifnam, addr, type);			\
  }

/* Get some sockaddr type of info.  */
static kern_return_t
siocgifXaddr (struct sock_user *user,
        ifname_t ifnam,
        sockaddr_t *addr,
        enum siocgif_type type)
{
  error_t err = 0;
  struct sockaddr_in *sin = (struct sockaddr_in *) addr;
  size_t buflen = sizeof(struct sockaddr);
  struct netif *netif;
  uint32_t addrs[4];

  if (!user)
    return EOPNOTSUPP;

  netif = get_if (ifnam);
  if (!netif)
    return ENODEV;

  if(type == DSTADDR)
    return EOPNOTSUPP;

  err = lwip_getsockname(user->sock->sockno, addr, (socklen_t*)&buflen);
  if(err)
    return err;

  if (sin->sin_family != AF_INET)
    err = EINVAL;
  else
  {
    inquire_device (netif, &addrs[0], &addrs[1], &addrs[2], &addrs[3], 0, 0, 0);
    sin->sin_addr.s_addr = addrs[type];
  }

  return err;
}

#define SIOCSIF(name, type)						\
  kern_return_t								\
  lwip_S_iioctl_siocsif##name (struct sock_user *user,                       \
			  ifname_t ifnam,				\
			  sockaddr_t addr)				\
  {									\
    return siocsifXaddr (user, ifnam, &addr, type);			\
  }

/* Set some sockaddr type of info.  */
static kern_return_t
siocsifXaddr (struct sock_user *user,
        ifname_t ifnam,
        sockaddr_t *addr,
        enum siocgif_type type)
{
  return EOPNOTSUPP;
}

/* 12 SIOCSIFADDR -- Set address of a network interface.  */
SIOCSIF (addr, ADDR);

/* 14 SIOCSIFDSTADDR -- Set point-to-point (peer) address of a network interface.  */
SIOCSIF (dstaddr, DSTADDR);

/* 16 SIOCSIFFLAGS -- Set flags of a network interface.  */
kern_return_t
lwip_S_iioctl_siocsifflags (struct sock_user *user,
        ifname_t ifnam,
        short flags)
{
  return EOPNOTSUPP;
}

/* 17 SIOCGIFFLAGS -- Get flags of a network interface.  */
kern_return_t
lwip_S_iioctl_siocgifflags (struct sock_user *user,
        char *name,
        short *flags)
{
  return EOPNOTSUPP;
}

/* 19 SIOCSIFBRDADDR -- Set broadcast address of a network interface.  */
SIOCSIF (brdaddr, BRDADDR);

/* 22 SIOCSIFNETMASK -- Set netmask of a network interface.  */
SIOCSIF (netmask, NETMASK);

/* 23 SIOCGIFMETRIC -- Get metric of a network interface.  */
kern_return_t
lwip_S_iioctl_siocgifmetric (struct sock_user *user,
        ifname_t ifnam,
        int *metric)
{
  return EOPNOTSUPP;
}

/* 24 SIOCSIFMETRIC -- Set metric of a network interface.  */
kern_return_t
lwip_S_iioctl_siocsifmetric (struct sock_user *user,
      ifname_t ifnam,
      int metric)
{
  return EOPNOTSUPP;
}

/* 25 SIOCDIFADDR -- Delete interface address.  */
kern_return_t
lwip_S_iioctl_siocdifaddr (struct sock_user *user,
        ifname_t ifnam,
        sockaddr_t addr)
{
  return EOPNOTSUPP;
}

/* 33 SIOCGIFADDR -- Get address of a network interface.  */
SIOCGIF (addr, ADDR);

/* 34 SIOCGIFDSTADDR -- Get point-to-point address of a network interface.  */
SIOCGIF (dstaddr, DSTADDR);

/* 35 SIOCGIFBRDADDR -- Get broadcast address of a network interface.  */
SIOCGIF (brdaddr, BRDADDR);

/* 37 SIOCGIFNETMASK -- Get netmask of a network interface.  */
SIOCGIF (netmask, NETMASK);

/* 39 SIOCGIFHWADDR -- Get the hardware address of a network interface.  */
error_t
lwip_S_iioctl_siocgifhwaddr (struct sock_user *user,
        ifname_t ifname,
        sockaddr_t *addr)
{
  return EOPNOTSUPP;
}

/* 51 SIOCGIFMTU -- Get mtu of a network interface.  */
error_t
lwip_S_iioctl_siocgifmtu (struct sock_user *user,
        ifname_t ifnam,
        int *mtu)
{
  return EOPNOTSUPP;
}

/* 51 SIOCSIFMTU -- Set mtu of a network interface.  */
error_t
lwip_S_iioctl_siocsifmtu (struct sock_user *user,
        ifname_t ifnam,
        int mtu)
{
  return EOPNOTSUPP;
}

/* 100 SIOCGIFINDEX -- Get index number of a network interface.  */
error_t
lwip_S_iioctl_siocgifindex (struct sock_user *user,
          ifname_t ifnam,
          int *index)
{
  return EOPNOTSUPP;
}

/* 101 SIOCGIFNAME -- Get name of a network interface from index number.  */
error_t
lwip_S_iioctl_siocgifname (struct sock_user *user,
        ifname_t ifnam,
        int *index)
{
  return EOPNOTSUPP;
}
