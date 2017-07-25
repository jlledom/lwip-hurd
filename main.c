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
#include <lwip_pfinet_S.h>
#include <lwip_iioctl_S.h>

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
        (routine = lwip_pfinet_server_routine (inp)) ||
        (routine = lwip_iioctl_server_routine (inp)) ||
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
        (routine = lwip_pfinet_server_routine (inp)) ||
        (routine = lwip_iioctl_server_routine (inp)) ||
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

void
translator_bind (int portclass, const char *name)
{
  struct trivfs_control *cntl;
  error_t err = 0;
  mach_port_t right;
  file_t file = file_name_lookup (name, O_CREAT|O_NOTRANS, 0666);

  if (file == MACH_PORT_NULL)
    err = errno;

  if (! err) {
    if (lwip_protid_portclasses[portclass] != MACH_PORT_NULL)
      error (1, 0, "Cannot bind one protocol to multiple nodes.\n");

    err = trivfs_add_protid_port_class (&lwip_protid_portclasses[portclass]);
    if (err)
      error (1, 0, "error creating control port class");

    err = trivfs_add_control_port_class (&lwip_cntl_portclasses[portclass]);
    if (err)
      error (1, 0, "error creating control port class");

    err = trivfs_create_control (file, lwip_cntl_portclasses[portclass],
          lwip_bucket,
          lwip_protid_portclasses[portclass],
          lwip_bucket, &cntl);
  }

  if (! err)
    {
      right = ports_get_send_right (cntl);
      err = file_set_translator (file, 0, FS_TRANS_EXCL | FS_TRANS_SET,
              0, 0, 0, right, MACH_MSG_TYPE_COPY_SEND);
      mach_port_deallocate (mach_task_self (), right);
    }

  if (err)
    error (1, err, "%s", name);

  ports_port_deref (cntl);
}

int
main (int argc, char **argv)
{
  error_t err;
  struct stat st;
  mach_port_t bootstrap;
  
  lwip_bucket = ports_create_bucket ();
  addrport_class = ports_create_class (clean_addrport, 0);
  socketport_class = ports_create_class (clean_socketport, 0);
  lwip_bootstrap_portclass = PORTCLASS_INET;
  
  mach_port_allocate (mach_task_self(),
                          MACH_PORT_RIGHT_RECEIVE, &fsys_identity);

  /* Init the input thread */
  hurdethif_input_init();

  /* Parse options.  When successful, this configures the interfaces
     before returning */
  argp_parse (&lwip_argp, argc, argv, 0,0,0);

  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (-1, 0, "Must be started as a translator");

  /* Create portclass to install on the bootstrap port. */
  if(lwip_protid_portclasses[lwip_bootstrap_portclass] != MACH_PORT_NULL)
    error(1, 0, "No portclass left to assign to bootstrap port");

  err = trivfs_add_protid_port_class (
          &lwip_protid_portclasses[lwip_bootstrap_portclass]);
  if (err)
    error (1, 0, "error creating control port class");

  err = trivfs_add_control_port_class (
          &lwip_cntl_portclasses[lwip_bootstrap_portclass]);
  if (err)
    error (1, 0, "error creating control port class");

  /* Reply to our parent */
  err = trivfs_startup (bootstrap, 0,
            lwip_cntl_portclasses[lwip_bootstrap_portclass],
            lwip_bucket,
            lwip_protid_portclasses[lwip_bootstrap_portclass],
            lwip_bucket, &lwipcntl);
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

  ports_manage_port_operations_multithread (lwip_bucket, lwip_demuxer,
              30 * 1000, 2 * 60 * 1000, 0);

  return 0;
}
