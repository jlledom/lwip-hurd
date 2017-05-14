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
   along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef HURD_LWIP_LWIPOPTS_H
#define HURD_LWIP_LWIPOPTS_H

//An OS is present
#define NO_SYS    0

//Do not rename Sockets API functions
#define LWIP_COMPAT_SOCKETS	0

//Use Glibc malloc()/free()
#define MEM_LIBC_MALLOC   1
#define MEMP_MEM_MALLOC   1
#define MEM_USE_POOLS     0
#define MEM_ALIGNMENT     4

//Enable modules
#define LWIP_DHCP   1
#define LWIP_AUTOIP 1
#define LWIP_SNMP   1
#define LWIP_IGMP   1
#define PPP_SUPPORT 1
#define LWIP_IPV6   1

//Debug mode
#ifdef LWIP_DEBUG
#define SOCKETS_DEBUG   LWIP_DBG_ON
#define ETHARP_DEBUG   LWIP_DBG_ON
#endif

//Disable stats
#define LWIP_STATS          0
#define LWIP_STATS_DISPLAY  0

//Enable/Disable checksum generation for each netif
#define LWIP_CHECKSUM_CTRL_PER_NETIF  1

//Enable SO_RCVBUF and FIONREAD command for ioctl()
#define LWIP_SO_RCVBUF  1

//Only send complete packets to the device
#define LWIP_NETIF_TX_SINGLE_PBUF 1

// Glibc sends more than one packet in a row during an ARP resolution
#define ARP_QUEUEING    1
#define ARP_QUEUE_LEN   10

#endif
