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
#include <device/device.h>
#include <device/net_status.h>

#include <lwip-hurd.h>
#include <lwip-util.h>
#include <netif/ifcommon.h>

/* Get the interface from its name */
static struct netif *
get_if(char *name)
{
  char ifname[16];
  struct netif *netif;

  memcpy (ifname, name, IFNAMSIZ-1);
  ifname[IFNAMSIZ-1] = 0;

  netif = netif_list;
  while(netif != 0)
  {
    if (strcmp (netif_get_state(netif)->devname, ifname) == 0)
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
  error_t err = 0;
  struct sockaddr_in sin;
  size_t buflen = sizeof(struct sockaddr_in);
  struct netif *netif;
  uint32_t ipv4_addrs[5];

  if (!user)
    return EOPNOTSUPP;

  if (!user->isroot)
    return EPERM;

  netif = get_if (ifnam);

  if (!netif)
    return ENODEV;

  if(type == DSTADDR || type == BRDADDR)
    return EOPNOTSUPP;

  err = lwip_getsockname(user->sock->sockno,
                          (sockaddr_t*)&sin, (socklen_t*)&buflen);
  if(err)
    return err;

  if (sin.sin_family != AF_INET)
    err = EINVAL;
  else
  {
    inquire_device (netif, &ipv4_addrs[0], &ipv4_addrs[1],
                      &ipv4_addrs[2], &ipv4_addrs[3], &ipv4_addrs[4], 0, 0);

    ipv4_addrs[type] = ((struct sockaddr_in *)addr)->sin_addr.s_addr;

    err = configure_device (netif, ipv4_addrs[0], ipv4_addrs[1],
                      ipv4_addrs[2], ipv4_addrs[3], ipv4_addrs[4], 0, 0);
  }

  return err;
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
  error_t err = 0;
  int status_flags = flags;

  struct netif *netif;

  if (!user)
    return EOPNOTSUPP;

  netif = get_if (ifnam);

  if (!user->isroot)
    err = EPERM;
  else if (!netif)
    err = ENODEV;
  else
  {
    err = device_set_status (netif_get_state(netif)->ether_port,
                                NET_FLAGS, &status_flags, 1);
    if(err)
      fprintf(stderr, "%s: hardware doesn't support setting flags.\n",
                netif_get_state(netif)->devname);
  }

  return err;
}

/* 17 SIOCGIFFLAGS -- Get flags of a network interface.  */
kern_return_t
lwip_S_iioctl_siocgifflags (struct sock_user *user,
        char *name,
        short *flags)
{
  error_t err = 0;
  struct netif *netif;
  size_t count;
  struct net_status status;

  netif = get_if (name);
  if (!netif)
    err = ENODEV;
  else
  {
    count = NET_STATUS_COUNT;
    err = device_get_status (netif_get_state(netif)->ether_port,
                              NET_STATUS, (dev_status_t)&status, &count);
    if (err)
      fprintf(stderr, "%s: hardware doesn't support getting flags.\n",
                netif_get_state(netif)->devname);
    else
      *flags = status.flags;
  }

  return err;
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
  error_t err = 0;
  struct netif *netif;

  netif = get_if (ifnam);
  if (!netif)
    err = ENODEV;
  else
    {
      *metric = 0; /* Not supported.  */
    }

  return err;
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
  error_t err = 0;
  struct netif *netif;

  netif = get_if (ifname);
  if (!netif)
    err = ENODEV;
  else
  {
    memcpy(addr->sa_data, netif->hwaddr, netif->hwaddr_len);
    addr->sa_family = netif_get_state(netif)->type;
  }

  return err;
}

/* 51 SIOCGIFMTU -- Get mtu of a network interface.  */
error_t
lwip_S_iioctl_siocgifmtu (struct sock_user *user,
        ifname_t ifnam,
        int *mtu)
{
  error_t err = 0;
  struct netif *netif;

  netif = get_if (ifnam);
  if (!netif)
    err = ENODEV;
  else
  {
    *mtu = netif->mtu;
  }

  return err;
}

/* 51 SIOCSIFMTU -- Set mtu of a network interface.  */
error_t
lwip_S_iioctl_siocsifmtu (struct sock_user *user,
        ifname_t ifnam,
        int mtu)
{
  error_t err = 0;
  struct netif *netif;

  if (!user)
    return EOPNOTSUPP;

  if (!user->isroot)
    err = EPERM;

  if (mtu <= 0)
    err = EINVAL;

  netif = get_if (ifnam);
  if (!netif)
    err = ENODEV;
  else
  {
    err = netif_get_state(netif)->update_mtu(netif, mtu);
  }

  return err;
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
