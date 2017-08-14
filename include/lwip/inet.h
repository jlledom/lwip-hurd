/**
 * @file
 * This file (together with sockets.h) aims to provide structs and functions from
 * - arpa/inet.h
 * - netinet/in.h
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_INET_H
#define LWIP_HDR_INET_H

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If your port already typedef's in_addr_t, define IN_ADDR_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(in_addr_t) && !defined(IN_ADDR_T_DEFINED)
typedef u32_t in_addr_t;
#endif

#define SIN_ZERO_LEN sizeof (struct sockaddr) - \
                           __SOCKADDR_COMMON_SIZE - \
                           sizeof (in_port_t) - \
                           sizeof (struct in_addr) \

#if LWIP_IPV4

#define inet_addr_from_ip4addr(target_inaddr, source_ipaddr) ((target_inaddr)->s_addr = ip4_addr_get_u32(source_ipaddr))
#define inet_addr_to_ip4addr(target_ipaddr, source_inaddr)   (ip4_addr_set_u32(target_ipaddr, (source_inaddr)->s_addr))
/* ATTENTION: the next define only works because both s_addr and ip4_addr_t are an u32_t effectively! */
#define inet_addr_to_ip4addr_p(target_ip4addr_p, source_inaddr)   ((target_ip4addr_p) = (ip4_addr_t*)&((source_inaddr)->s_addr))

/* directly map this to the lwip internal functions */
#define inet_addr(cp)                   ipaddr_addr(cp)
#define inet_aton(cp, addr)             ip4addr_aton(cp, (ip4_addr_t*)addr)
#define inet_ntoa(addr)                 ip4addr_ntoa((const ip4_addr_t*)&(addr))
#define inet_ntoa_r(addr, buf, buflen)  ip4addr_ntoa_r((const ip4_addr_t*)&(addr), buf, buflen)

#endif /* LWIP_IPV4 */

#if LWIP_IPV6
#define inet6_addr_from_ip6addr(target_in6addr, source_ip6addr) {(target_in6addr)->__in6_u.__u6_addr32[0] = (source_ip6addr)->addr[0]; \
                                                                 (target_in6addr)->__in6_u.__u6_addr32[1] = (source_ip6addr)->addr[1]; \
                                                                 (target_in6addr)->__in6_u.__u6_addr32[2] = (source_ip6addr)->addr[2]; \
                                                                 (target_in6addr)->__in6_u.__u6_addr32[3] = (source_ip6addr)->addr[3];}
#define inet6_addr_to_ip6addr(target_ip6addr, source_in6addr)   {(target_ip6addr)->addr[0] = (source_in6addr)->__in6_u.__u6_addr32[0]; \
                                                                 (target_ip6addr)->addr[1] = (source_in6addr)->__in6_u.__u6_addr32[1]; \
                                                                 (target_ip6addr)->addr[2] = (source_in6addr)->__in6_u.__u6_addr32[2]; \
                                                                 (target_ip6addr)->addr[3] = (source_in6addr)->__in6_u.__u6_addr32[3];}
/* ATTENTION: the next define only works because both in6_addr and ip6_addr_t are an u32_t[4] effectively! */
#define inet6_addr_to_ip6addr_p(target_ip6addr_p, source_in6addr)   ((target_ip6addr_p) = (ip6_addr_t*)(source_in6addr))

/* directly map this to the lwip internal functions */
#define inet6_aton(cp, addr)            ip6addr_aton(cp, (ip6_addr_t*)addr)
#define inet6_ntoa(addr)                ip6addr_ntoa((const ip6_addr_t*)&(addr))
#define inet6_ntoa_r(addr, buf, buflen) ip6addr_ntoa_r((const ip6_addr_t*)&(addr), buf, buflen)

#endif /* LWIP_IPV6 */


#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_INET_H */
