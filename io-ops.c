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

#include <sys/mman.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>

#include <lwip/sockets.h>

error_t
lwip_S_io_write (struct sock_user *user,
	    char *data,
	    size_t datalen,
	    off_t offset,
	    mach_msg_type_number_t *amount)
{
  int sent;
  int sockflags;
  struct iovec iov = { data, datalen };
  struct msghdr m = { msg_name: 0, msg_namelen: 0, msg_flags: 0,
		      msg_controllen: 0, msg_iov: &iov, msg_iovlen: 1 };

  if (!user)
    return EOPNOTSUPP;

  sockflags = lwip_fcntl(user->sock->sockno, F_GETFL, 0);
  if (sockflags & O_NONBLOCK)
    m.msg_flags |= MSG_DONTWAIT;
  sent = lwip_sendmsg(user->sock->sockno, &m, 0);

  if (sent >= 0)
  {
    *amount = sent;
  }
  
  return errno;
}

error_t
lwip_S_io_read (struct sock_user *user,
	   char **data,
	   size_t *datalen,
	   off_t offset,
	   mach_msg_type_number_t amount)
{
  error_t err;
  int alloced = 0;
  int flags;

  if (!user)
    return EOPNOTSUPP;

  /* Instead of this, we should peek and the socket and only
     allocate as much as necessary. */
  if (amount > *datalen)
    {
      *data = mmap (0, amount, PROT_READ|PROT_WRITE, MAP_ANON, 0, 0);
      if (*data == MAP_FAILED)
        /* Should check whether errno is indeed ENOMEM --
           but this can't be done in a straightforward way,
           because the glue headers #undef errno. */
        return ENOMEM;
      alloced = 1;
    }
  
  //Get flags
  flags = lwip_fcntl(user->sock->sockno, F_GETFL, 0);

  err = lwip_recv(user->sock->sockno, *data, amount,
                  (flags & O_NONBLOCK) ? MSG_DONTWAIT : 0);

  if (err < 0)
  {
    if (alloced)
      munmap (*data, amount);
  }
  else
  {
    *datalen = err;
    if (alloced && round_page (*datalen) < round_page (amount))
      munmap (*data + round_page (*datalen),
    round_page (amount) - round_page (*datalen));
    errno = 0;
  }
  
  return errno;
}

error_t
lwip_S_io_seek (struct sock_user *user,
	   off_t offset,
	   int whence,
	   off_t *newp)
{
  return user ? ESPIPE : EOPNOTSUPP;
}

error_t
lwip_S_io_readable (struct sock_user *user,
	       mach_msg_type_number_t *amount)
{
  error_t err;
  if (!user)
    return EOPNOTSUPP;
  
  err = lwip_ioctl(user->sock->sockno, FIONREAD, amount);
  
  if(err < 0)
    *amount = 0;
  
  return errno;
}

error_t
lwip_S_io_set_all_openmodes (struct sock_user *user,
			int bits)
{
  int opt;
  
  if (!user)
    return EOPNOTSUPP;

  if (bits & O_NONBLOCK)
    opt = 1;
  else
    opt = 0;
  
  lwip_ioctl(user->sock->sockno, FIONBIO, &opt);
  
  return errno;
}

error_t
lwip_S_io_get_openmodes (struct sock_user *user,
		    int *bits)
{
  if (!user)
    return EOPNOTSUPP;

  *bits = lwip_fcntl(user->sock->sockno, F_GETFL, 0);

  return errno;
}

error_t
lwip_S_io_set_some_openmodes (struct sock_user *user,
			 int bits)
{
  if (!user)
    return EOPNOTSUPP;

  if (bits & O_NONBLOCK)
  {
    int opt = 1;
    lwip_ioctl(user->sock->sockno, FIONBIO, &opt);
  }
    
  return errno;
}


error_t
lwip_S_io_clear_some_openmodes (struct sock_user *user,
		   	int bits)
{
  if (!user)
    return EOPNOTSUPP;

  if (bits & O_NONBLOCK)
  {
    int opt = 0;
    lwip_ioctl(user->sock->sockno, FIONBIO, &opt);
  }
    
  return errno;
}


static error_t
lwip_io_select_common (struct sock_user *user,
		  mach_port_t reply,
		  mach_msg_type_name_t reply_type,
		  struct timeval *tv, int *select_type)
{
  fd_set readset, writeset, exceptset;
  fd_set *lreadset, *lwriteset, *lexceptset;
  int ret;
  int sock;

  if (!user)
    return EOPNOTSUPP;

  if(user->sock->sockno > FD_SETSIZE)
  {
    *select_type = 0;
    /* Glibc doesn't expect to get an error in this case */
    return ESUCCESS;
  }

  ports_interrupt_self_on_notification (user, reply, MACH_NOTIFY_DEAD_NAME);

  sock = user->sock->sockno;

  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_ZERO(&exceptset);
  lreadset = lwriteset = lexceptset = 0;

  if(*select_type & SELECT_READ)
  {
    FD_SET(sock, &readset);
    lreadset = &readset;
  }
  if(*select_type & SELECT_WRITE)
  {
    FD_SET(sock, &writeset);
    lwriteset = &writeset;
  }
  if(*select_type & SELECT_URG)
  {
    FD_SET(sock, &exceptset);
    lexceptset = &exceptset;
  }

  *select_type = 0;

  ret = lwip_select(sock+1, lreadset, lwriteset, lexceptset, tv);
  if(ret > 0)
  {
    if(FD_ISSET(sock, &readset))
      *select_type |= SELECT_READ;

    if(FD_ISSET(sock, &writeset))
      *select_type |= SELECT_WRITE;

    if(FD_ISSET(sock, &exceptset))
      *select_type |= SELECT_URG;
  }

  return errno;
}

error_t
lwip_S_io_select (struct sock_user *user,
	     mach_port_t reply,
	     mach_msg_type_name_t reply_type,
	     int *select_type)
{
  return lwip_io_select_common (user, reply, reply_type, NULL, select_type);
}

error_t
lwip_S_io_select_timeout (struct sock_user *user,
		     mach_port_t reply,
		     mach_msg_type_name_t reply_type,
		     struct timespec ts,
		     int *select_type)
{
  struct timeval tv, current_tv;
  
  TIMESPEC_TO_TIMEVAL(&tv, &ts);
  gettimeofday(&current_tv, 0);
  
  tv.tv_sec -= current_tv.tv_sec;
  tv.tv_usec -= current_tv.tv_usec;
  
  return lwip_io_select_common (user, reply, reply_type, &tv, select_type);
}


error_t
lwip_S_io_stat (struct sock_user *user,
	   struct stat *st)
{
  if (!user)
    return EOPNOTSUPP;

  memset (st, 0, sizeof(struct stat));

  st->st_fstype = FSTYPE_SOCKET;
  st->st_fsid = getpid ();
  st->st_ino = user->sock->sockno;

  st->st_mode = S_IFSOCK | ACCESSPERMS;
  st->st_blksize = 512;		/* ???? */

  return 0;
}

error_t
lwip_S_io_reauthenticate (struct sock_user *user,
		     mach_port_t rend)
{
  struct sock_user *newuser;
  uid_t gubuf[20], ggbuf[20], aubuf[20], agbuf[20];
  uid_t *gen_uids, *gen_gids, *aux_uids, *aux_gids;
  size_t genuidlen, gengidlen, auxuidlen, auxgidlen;
  error_t err;
  size_t i, j;
  auth_t auth;
  mach_port_t newright;

  if (!user)
    return EOPNOTSUPP;

  genuidlen = gengidlen = auxuidlen = auxgidlen = 20;
  gen_uids = gubuf;
  gen_gids = ggbuf;
  aux_uids = aubuf;
  aux_gids = agbuf;

  newuser = make_sock_user (user->sock, 0, 1, 0);

  auth = getauth ();
  newright = ports_get_send_right (newuser);
  assert (newright != MACH_PORT_NULL);

  do
    err = auth_server_authenticate (auth,
				    rend,
				    MACH_MSG_TYPE_COPY_SEND,
				    newright,
				    MACH_MSG_TYPE_COPY_SEND,
				    &gen_uids, &genuidlen,
				    &aux_uids, &auxuidlen,
				    &gen_gids, &gengidlen,
				    &aux_gids, &auxgidlen);
  while (err == EINTR);

  mach_port_deallocate (mach_task_self (), rend);
  mach_port_deallocate (mach_task_self (), newright);
  mach_port_deallocate (mach_task_self (), auth);

  if (err)
    newuser->isroot = 0;
  else
    /* Check permission as fshelp_isowner would do.  */
    for (i = 0; i < genuidlen; i++)
    {
      if (gen_uids[i] == 0 || gen_uids[i] == lwip_owner)
        newuser->isroot = 1;
      if (gen_uids[i] == lwip_group)
        for (j = 0; j < gengidlen; j++)
          if (gen_gids[j] == lwip_group)
            newuser->isroot = 1;
    }

  mach_port_move_member (mach_task_self (), newuser->pi.port_right,
			 lwip_bucket->portset);

  ports_port_deref (newuser);

  if (gubuf != gen_uids)
    munmap (gen_uids, genuidlen * sizeof (uid_t));
  if (ggbuf != gen_gids)
    munmap (gen_gids, gengidlen * sizeof (uid_t));
  if (aubuf != aux_uids)
    munmap (aux_uids, auxuidlen * sizeof (uid_t));
  if (agbuf != aux_gids)
    munmap (aux_gids, auxgidlen * sizeof (uid_t));

  return 0;
}

error_t
lwip_S_io_restrict_auth (struct sock_user *user,
		    mach_port_t *newobject,
		    mach_msg_type_name_t *newobject_type,
		    uid_t *uids, size_t uidslen,
		    uid_t *gids, size_t gidslen)
{
  struct sock_user *newuser;
  int i, j;
  int isroot;

  if (!user)
    return EOPNOTSUPP;

  isroot = 0;
  if (user->isroot)
    /* Check permission as fshelp_isowner would do.  */
    for (i = 0; i < uidslen; i++)
    {
      if (uids[i] == 0 || uids[i] == lwip_owner)
        isroot = 1;
      if (uids[i] == lwip_group)
        for (j = 0; j < gidslen; j++)
          if (gids[j] == lwip_group)
            isroot = 1;
    }

  newuser = make_sock_user (user->sock, isroot, 0, 0);
  *newobject = ports_get_right (newuser);
  *newobject_type = MACH_MSG_TYPE_MAKE_SEND;
  ports_port_deref (newuser);

  return 0;
}

error_t
lwip_S_io_duplicate (struct sock_user *user,
		mach_port_t *newobject,
		mach_msg_type_name_t *newobject_type)
{
  struct sock_user *newuser;
  if (!user)
    return EOPNOTSUPP;

  newuser = make_sock_user (user->sock, user->isroot, 0, 0);
  *newobject = ports_get_right (newuser);
  *newobject_type = MACH_MSG_TYPE_MAKE_SEND;
  ports_port_deref (newuser);

  return 0;
}

error_t
lwip_S_io_identity (struct sock_user *user,
	       mach_port_t *id,
	       mach_msg_type_name_t *idtype,
	       mach_port_t *fsys,
	       mach_msg_type_name_t *fsystype,
	       ino_t *fileno)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  if (user->sock->identity == MACH_PORT_NULL)
  {
    err = mach_port_allocate (mach_task_self (), MACH_PORT_RIGHT_RECEIVE,
      &user->sock->identity);
    if (err)
    {
      return err;
    }
  }

  *id = user->sock->identity;
  *idtype = MACH_MSG_TYPE_MAKE_SEND;
  *fsys = fsys_identity;
  *fsystype = MACH_MSG_TYPE_MAKE_SEND;
  *fileno = user->sock->sockno;

  return 0;
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
