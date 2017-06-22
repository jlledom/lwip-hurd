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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <fcntl.h>
#include <argp.h>
#include <sys/mman.h>
#include <hurd/trivfs.h>

#include <lwip_io_S.h>
#include <lwip_socket_S.h>
#include <lwip-hurd.h>
#include <options.h>

#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <netif/hurdethif.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern boolean_t mighello_server
  (mach_msg_header_t * InHeadP, mach_msg_header_t * OutHeadP);

extern struct argp lwip_argp;

extern struct netif *netif_list;

int trivfs_fstype = FSTYPE_MISC;
int trivfs_fsid = 0;
int trivfs_support_read = 0;
int trivfs_support_write = 0;
int trivfs_support_exec = 0;
int trivfs_allow_open = O_READ | O_WRITE;

void
trivfs_modify_stat (struct trivfs_protid *cred, io_statbuf_t * st)
{
}

error_t
trivfs_goaway (struct trivfs_control *fsys, int flags)
{
  exit (0);
}

int
lwip_demuxer (mach_msg_header_t * inp, mach_msg_header_t * outp)
{
  struct port_info *pi;
  
  //Clear errno to prevent raising previous errors again
  errno = 0;

  /* We have several classes in one bucket, which need to be demuxed
     differently.  */
  if (MACH_MSGH_BITS_LOCAL (inp->msgh_bits) ==
      MACH_MSG_TYPE_PROTECTED_PAYLOAD)
    pi = ports_lookup_payload (lwip_bucket,
			       inp->msgh_protected_payload,
			       socketport_class);
  else
    pi = ports_lookup_port (lwip_bucket,
			    inp->msgh_local_port,
			    socketport_class);

  if (pi)
  {
    ports_port_deref (pi);

    mig_routine_t routine;
    if ((routine = lwip_io_server_routine (inp)) ||
        (routine = lwip_socket_server_routine (inp)) ||
        (routine = NULL, trivfs_demuxer (inp, outp)))
      {
        if (routine)
          (*routine) (inp, outp);
        return TRUE;
      }
    else
      return FALSE;
  }
  else
  {
    mig_routine_t routine;
    if ((routine = lwip_socket_server_routine (inp)) ||
        (routine = NULL, trivfs_demuxer (inp, outp)))
      {
        if (routine)
          (*routine) (inp, outp);
        return TRUE;
      }
    else
      return FALSE;
  }

  return 0;
}

static int
check_valid_ip_config(struct parse_interface *in)
{
  if(in->address == INADDR_NONE)
    return 0;

  if (in->gateway != INADDR_NONE
    && (in->gateway & in->netmask) != (in->address & in->netmask))
    in->gateway = INADDR_NONE;

  return 1;
}

static void
remove_ifs()
{
  struct netif *netif;

  while(netif_list != 0)
  {
    netif = netif_list;
    netif_remove(netif);

    /* Free the interface and its hook */
    free(((struct hurdethif*)netif->state)->devname);
    free(netif->state);
    free(netif);
  }

  return;
}

void
init_ifs(void *arg)
{
  struct parse_interface *in;
  struct parse_hook *ifs;
  struct netif *netif;
  ip4_addr_t ipaddr, netmask, gw;

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

    ipaddr.addr = in->address;
    netmask.addr = in->netmask;
    gw.addr = in->gateway;

    netif = malloc(sizeof(struct netif));
    memset(netif, 0, sizeof(struct netif));
    strncpy(netif->name, in->lwip_name, LWIP_NAME_LEN);

    /* Fifth parameter (in->name) is a hook */
    netif_add(netif, &ipaddr, &netmask, &gw,
            in->dev_name, hurdethif_init, tcpip_input);

    netif_set_up(netif);

#if LWIP_IPV6
    netif_create_ip6_linklocal_address(netif, 1);
#endif
  }

  /* Set the first interface with valid gateway as default */
  if (in->gateway != INADDR_NONE)
  {
    netif_set_default(netif);
  }

  /* Free the hook */
  free (ifs->interfaces);
  free (ifs);

  return;
}

int
main (int argc, char **argv)
{
  error_t err;
  struct stat st;
  mach_port_t bootstrap;
  int domain;
  
  lwip_bucket = ports_create_bucket ();
  addrport_class = ports_create_class (clean_addrport, 0);
  socketport_class = ports_create_class (clean_socketport, 0);
  
  mach_port_allocate (mach_task_self(),
                          MACH_PORT_RIGHT_RECEIVE, &fsys_identity);

  err = trivfs_add_protid_port_class (&lwip_protid_portclass);
  if (err)
    error (1, 0, "error creating control port class");

  err = trivfs_add_control_port_class (&lwip_cntl_portclass);
  if (err)
    error (1, 0, "error creating control port class");

  /* Parse options.  When successful, this configures the interfaces
     before returning */
  argp_parse (&lwip_argp, argc, argv, 0,0,0);

  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (-1, 0, "Must be started as a translator");

  /* Reply to our parent */
  err = trivfs_startup (bootstrap, 0, lwip_cntl_portclass, lwip_bucket,
                          lwip_protid_portclass, lwip_bucket, &lwipcntl);
  mach_port_deallocate (mach_task_self (), bootstrap);
  if (err)
  {
    return (-1);
  }
  
  /* Initialize status from underlying node.  */
  lwip_owner = lwip_group = 0;
  err = io_stat (lwipcntl->underlying, &st);
  if (! err)
  {
    lwip_owner = st.st_uid;
    lwip_group = st.st_gid;
  }

  //Set the domain of this node. FIXME: Get the proper domain
  domain = PF_INET;
  lwipcntl->hook = (void*)&domain;

  ports_manage_port_operations_multithread (lwip_bucket, lwip_demuxer,
              30 * 1000, 2 * 60 * 1000, 0);

  return 0;
}
