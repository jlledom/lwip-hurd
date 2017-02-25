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

#include <lwip_io_S.h>

#include <errno.h>

error_t
lwip_S_io_write (struct sock_user *user,
	    char *data,
	    size_t datalen,
	    off_t offset,
	    mach_msg_type_number_t *amount)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_read (struct sock_user *user,
	   char **data,
	   size_t *datalen,
	   off_t offset,
	   mach_msg_type_number_t amount)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_seek (struct sock_user *user,
	   off_t offset,
	   int whence,
	   off_t *newp)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_readable (struct sock_user *user,
	       mach_msg_type_number_t *amount)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_set_all_openmodes (struct sock_user *user,
			int bits)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_get_openmodes (struct sock_user *user,
		    int *bits)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_set_some_openmodes (struct sock_user *user,
			 int bits)
{
  return EOPNOTSUPP;
}


error_t
lwip_S_io_clear_some_openmodes (struct sock_user *user,
		   	int bits)
{
  return EOPNOTSUPP;
}


error_t
lwip_S_io_select (sock_user_t io_object,
			mach_port_t reply,
			mach_msg_type_name_t replyPoly,
			int *select_type)
{
  return EOPNOTSUPP;
}


error_t
lwip_S_io_select_timeout (sock_user_t io_object,
			mach_port_t reply,
			mach_msg_type_name_t replyPoly,
			timespec_t timeout,
			int *select_type)
{
  return EOPNOTSUPP;
}


error_t
lwip_S_io_stat (struct sock_user *user,
	   struct stat *st)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_reauthenticate (struct sock_user *user,
		     mach_port_t rend)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_restrict_auth (struct sock_user *user,
		    mach_port_t *newobject,
		    mach_msg_type_name_t *newobject_type,
		    uid_t *uids, size_t uidslen,
		    uid_t *gids, size_t gidslen)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_duplicate (struct sock_user *user,
		mach_port_t *newobject,
		mach_msg_type_name_t *newobject_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_identity (struct sock_user *user,
	       mach_port_t *id,
	       mach_msg_type_name_t *idtype,
	       mach_port_t *fsys,
	       mach_msg_type_name_t *fsystype,
	       ino_t *fileno)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_revoke (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_async (struct sock_user *user,
	    mach_port_t notify,
	    mach_port_t *id,
	    mach_msg_type_name_t *idtype)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_mod_owner (struct sock_user *user,
		pid_t owner)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_get_owner (struct sock_user *user,
		pid_t *owner)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_get_icky_async_id (struct sock_user *user,
			mach_port_t *id,
			mach_msg_type_name_t *idtype)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_server_version (struct sock_user *user,
		     char *name,
		     int *major,
		     int *minor,
		     int *edit)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_pathconf (struct sock_user *user,
	       int name,
	       int *value)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_map (struct sock_user *user,
	  mach_port_t *rdobj,
	  mach_msg_type_name_t *rdobj_type,
	  mach_port_t *wrobj,
	  mach_msg_type_name_t *wrobj_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_map_cntl (struct sock_user *user,
	       mach_port_t *obj,
	       mach_msg_type_name_t *obj_type)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_get_conch (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_release_conch (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_eofnotify (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_prenotify (struct sock_user *user,
		vm_offset_t start,
		vm_offset_t end)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_postnotify (struct sock_user *user,
		 vm_offset_t start,
		 vm_offset_t end)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_readnotify (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_readsleep (struct sock_user *user)
{
  return EOPNOTSUPP;
}

error_t
lwip_S_io_sigio (struct sock_user *user)
{
  return EOPNOTSUPP;
}
