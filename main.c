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

#include <lwip-hurd.h>
#include <lwip_io_S.h>
#include <lwip_socket_S.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern boolean_t mighello_server
  (mach_msg_header_t * InHeadP, mach_msg_header_t * OutHeadP);

int trivfs_fstype = FSTYPE_MISC;
int trivfs_fsid = 0;
int trivfs_support_read = 1;
int trivfs_support_write = 1;
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

error_t
trivfs_S_io_read (struct trivfs_protid *cred,
		  mach_port_t reply, mach_msg_type_name_t reply_type,
		  char **data, mach_msg_type_number_t * data_len,
		  off_t offs, mach_msg_type_number_t amount)
{

  /* Deny access if they have bad credentials.  */
  if (!cred)
    return EOPNOTSUPP;
  else if (!(cred->po->openmodes & O_READ))
    return EBADF;

  return 0;
}

error_t
trivfs_S_io_write (struct trivfs_protid * cred,
		   mach_port_t reply, mach_msg_type_name_t reply_type,
		   char **data, mach_msg_type_number_t * data_len,
		   off_t offs, mach_msg_type_number_t amount)
{

  /* Deny access if they have bad credentials.  */
  if (!cred)
    return EOPNOTSUPP;
  else if (!(cred->po->openmodes & O_READ))
    return EBADF;

  return 0;
}

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{

  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  switch (key)
    {
    case ARGP_KEY_INIT:
    case ARGP_KEY_SUCCESS:
    case ARGP_KEY_ERROR:
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	{
	  /* First argument */
	}

      break;
    case ARGP_KEY_END:
      /* Done parsing */
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

/* The options we understand. */
static struct argp_option options[] = {
  {0}
};

struct argp argst = {
  options, parse_opt, 0, "A translator providing access to LwIP TCP/IP stack."
};

int
lwip_demuxer (mach_msg_header_t * inp, mach_msg_header_t * outp)
{
  struct port_info *pi;

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
}

int
main (int argc, char **argv)
{
  error_t err;
  mach_port_t bootstrap;
  
  lwip_bucket = ports_create_bucket ();
  addrport_class = ports_create_class (0, 0);
  socketport_class = ports_create_class (0, 0);

  // Program arguments parsing
  argp_parse (&argst, argc, argv, 0, 0, 0);

  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (-1, 0, "Must be started as a translator");

  /* Reply to our parent */
  err = trivfs_startup (bootstrap, 0, 0, lwip_bucket, 0, lwip_bucket, &lwipcntl);
  mach_port_deallocate (mach_task_self (), bootstrap);
  if (err)
    {
      return (-1);
    }

  ports_manage_port_operations_multithread (lwip_bucket, lwip_demuxer,
					    30 * 1000, 2 * 60 * 1000, 0);

  return 0;
}
