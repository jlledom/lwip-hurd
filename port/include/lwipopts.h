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
#define LWIP_COMPAT_SOCKETS   0

//Use Glibc malloc()/free()
#define MEM_LIBC_MALLOC   1
#define MEMP_MEM_MALLOC   1
#define MEM_USE_POOLS     0
#define MEM_ALIGNMENT     4

//Only send complete packets to the device
#define LWIP_NETIF_TX_SINGLE_PBUF 1

/* Randomize local ports */
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS  1

/* Available sockets */
#define MEMP_NUM_NETCONN  32

// Glibc sends more than one packet in a row during an ARP resolution
#define ARP_QUEUEING    1
#define ARP_QUEUE_LEN   10

/* Netif API is needed to add or remove interfaces on run time */
#define LWIP_NETIF_API  1

/* IPv4 stuff */
#define IP_FORWARD  1

/* SLAAC support and other IPv6 stuff */
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS 1
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 1
#define LWIP_IPV6_AUTOCONFIG          1
#define LWIP_IPV6_FORWARD             1
#define MEMP_NUM_MLD6_GROUP           16
#define LWIP_IPV6_NUM_ADDRESSES       6

/* TCP tuning */
#define TCP_MSS     1460

//Disable stats
#define LWIP_STATS          0
#define LWIP_STATS_DISPLAY  0

/* Enable all socket operations */
#define LWIP_SO_SNDTIMEO            1
#define LWIP_SO_RCVTIMEO            1
#define LWIP_SO_RCVBUF              1
#define LWIP_SO_LINGER              1
#define SO_REUSE                    1
#define LWIP_MULTICAST_TX_OPTIONS   1

/* Enable modules */
#define LWIP_ARP              1
#define LWIP_ETHERNET         1
#define LWIP_IPV4             1
#define LWIP_ICMP             1
#define LWIP_IGMP             1
#define LWIP_RAW              1
#define LWIP_UDP              1
#define LWIP_UDPLITE          1
#define LWIP_TCP              1
#define LWIP_IPV6             1
#define LWIP_ICMP6            1
#define LWIP_IPV6_MLD         1
#define LWIP_NETIF_LOOPBACK   1

/* Debug mode */
#ifdef LWIP_DEBUG
#define ETHARP_DEBUG      LWIP_DBG_OFF
#define NETIF_DEBUG       LWIP_DBG_OFF
#define PBUF_DEBUG        LWIP_DBG_OFF
#define API_LIB_DEBUG     LWIP_DBG_OFF
#define API_MSG_DEBUG     LWIP_DBG_OFF
#define SOCKETS_DEBUG     LWIP_DBG_OFF
#define ICMP_DEBUG        LWIP_DBG_OFF
#define IGMP_DEBUG        LWIP_DBG_OFF
#define INET_DEBUG        LWIP_DBG_OFF
#define IP_DEBUG          LWIP_DBG_OFF
#define IP_REASS_DEBUG    LWIP_DBG_OFF
#define RAW_DEBUG         LWIP_DBG_OFF
#define MEM_DEBUG         LWIP_DBG_OFF
#define MEMP_DEBUG        LWIP_DBG_OFF
#define SYS_DEBUG         LWIP_DBG_OFF
#define TIMERS_DEBUG      LWIP_DBG_OFF
#define TCP_DEBUG         LWIP_DBG_OFF
#define TCP_INPUT_DEBUG   LWIP_DBG_OFF
#define TCP_FR_DEBUG      LWIP_DBG_OFF
#define TCP_RTO_DEBUG     LWIP_DBG_OFF
#define TCP_CWND_DEBUG    LWIP_DBG_OFF
#define TCP_WND_DEBUG     LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG  LWIP_DBG_OFF
#define TCP_RST_DEBUG     LWIP_DBG_OFF
#define TCP_QLEN_DEBUG    LWIP_DBG_OFF
#define UDP_DEBUG         LWIP_DBG_OFF
#define TCPIP_DEBUG       LWIP_DBG_OFF
#define SLIP_DEBUG        LWIP_DBG_OFF
#define DHCP_DEBUG        LWIP_DBG_OFF
#define AUTOIP_DEBUG      LWIP_DBG_OFF
#define DNS_DEBUG         LWIP_DBG_OFF
#define IP6_DEBUG         LWIP_DBG_OFF
#endif

#endif
