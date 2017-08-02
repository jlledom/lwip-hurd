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

#include <netif/hurdtunif.h>

#include <hurd/trivfs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <error.h>

#include <lwip-hurd.h>

static void
enqueue(struct pbufqueue *q, struct pbuf *p)
{
  if(q->tail)
    q->tail->next = p;

  q->tail = p;
  q->tail->next = 0;

  if(!q->head)
    q->head = q->tail;

  q->len++;
}

static struct pbuf*
dequeue(struct pbufqueue *q)
{
  struct pbuf *ret;

  if(q->head)
  {
    ret = q->head;
    q->head = q->head->next;
    q->len--;
  }

  if(!q->head)
    q->tail = 0;

  return ret;
}

/*
 * Update the interface's MTU
 */
error_t hurdtunif_update_mtu(struct netif *netif, uint32_t mtu)
{
  error_t err = 0;

  netif->mtu = mtu;

  return err;
}

static error_t
hurdtunif_device_set_flags(struct netif *netif, uint16_t flags)
{
  error_t err = 0;
  struct ifcommon *tunif;

  tunif = netif_get_state(netif);
  tunif->flags = flags;

  return err;
}

/*
 * Release all resources of this netif.
 *
 * Returns 0 on success.
 */
error_t
hurdtunif_terminate(struct netif *netif)
{
  struct hurdtunif *tunif = (struct hurdtunif*)netif_get_state(netif);

  /* Clear the queue */
  while (dequeue(&tunif->queue) != 0);
  pthread_cond_destroy (&tunif->read);
  pthread_cond_destroy (&tunif->select);
  pthread_mutex_destroy (&tunif->lock);

  /* Free the interface and its hook */
  free (netif_get_state(netif)->devname);
  mem_free (netif_get_state(netif));

  return 0;
}

err_t
hurdtunif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  err_t err = 0;
  struct hurdtunif *tunif;

  tunif = (struct hurdtunif *)netif_get_state(netif);

  pthread_mutex_lock (&tunif->lock);

  /* Avoid unlimited growth.  */
  if(tunif->queue.len > 128)
    dequeue(&tunif->queue);

  enqueue(&tunif->queue, p);

  if (tunif->read_blocked)
  {
    tunif->read_blocked = 0;
    pthread_cond_broadcast (&tunif->read);
    pthread_cond_broadcast (&tunif->select);
  }

  pthread_mutex_unlock (&tunif->lock);

  return err;
}

err_t
hurdtunif_init(struct netif *netif)
{
  err_t err = 0;
  struct hurdtunif *tunif;
  char *base_name, *name = netif->state;

  tunif = mem_malloc(sizeof(struct hurdtunif));
  if (tunif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("hurdtunif_init: out of memory\n"));
    return ERR_MEM;
  }
  memset(tunif, 0, sizeof(struct hurdtunif));

  base_name = strrchr (name, '/');
  if (base_name)
    /* The user provided a path */
    base_name++;
  else
    /* The user provided a name for the tunnel. We'll create it at /dev */
    base_name = name;

  if (base_name != name)
    tunif->comm.devname = strdup (name);
  else
    /* Setting up the translator at /dev/tunX.  */
    asprintf (&tunif->comm.devname, "/dev/%s", base_name);

  netif->state = tunif;
  tunif->comm.type = ARPHRD_PPP;

  netif->mtu = 1500;

  hurdtunif_device_set_flags(netif,
                              IFF_UP|IFF_RUNNING|IFF_POINTOPOINT|IFF_NOARP);

  netif->flags = NETIF_FLAG_LINK_UP;

  netif->output = hurdtunif_output;
  tunif->comm.terminate = hurdtunif_terminate;
  tunif->comm.update_mtu = hurdtunif_update_mtu;
  tunif->comm.change_flags = hurdtunif_device_set_flags;

  /* Bind the translator to tdev->devname */
  tunif->underlying = file_name_lookup (tunif->comm.devname,
                                          O_CREAT|O_NOTRANS, 0664);

  if (tunif->underlying == MACH_PORT_NULL)
    error (2, 1, "%s", base_name);

  err = trivfs_create_control (tunif->underlying, tunnel_cntlclass,
                                lwip_bucket, tunnel_class, lwip_bucket,
                                &tunif->cntl);

  if (! err)
    {
      mach_port_t right = ports_get_send_right (tunif->cntl);
      err = file_set_translator (tunif->underlying, 0, FS_TRANS_EXCL
                                  | FS_TRANS_SET, 0, 0, 0, right,
                                  MACH_MSG_TYPE_COPY_SEND);
      mach_port_deallocate (mach_task_self (), right);
    }

  if (err)
    error (2, err, "%s", base_name);

  /* We'll need to get the netif from trivfs operations*/
  tunif->cntl->hook = netif;

  /* Output queue initialization */
  tunif->queue.head = tunif->queue.tail = 0;
  tunif->queue.len = 0;
  pthread_mutex_init (&tunif->lock, NULL);
  pthread_cond_init (&tunif->read, NULL);
  pthread_cond_init (&tunif->select, NULL);
  tunif->read_blocked = 0;

  return err;
}

error_t
hurdtunif_module_init()
{
  error_t err = 0;

  trivfs_add_control_port_class (&tunnel_cntlclass);
  trivfs_add_protid_port_class (&tunnel_class);

  return err;
}

/* If a new open with read and/or write permissions is requested,
   restrict to exclusive usage.  */
static error_t
check_open_hook (struct trivfs_control *cntl, struct iouser *user, int flags)
{
  struct netif *netif;
  struct hurdtunif *tunif;

  for (netif = netif_list; netif; netif = netif->next)
  {
    tunif = (struct hurdtunif*)netif_get_state(netif);
    if (tunif->cntl == cntl)
      break;
  }

  if (netif && flags != O_NORW)
  {
    if (tunif->user)
      return EBUSY;
    else
      tunif->user = user;
  }

  return 0;
}

/* When a protid is destroyed, check if it is the current user.
   If yes, release the interface for other users.  */
static void
pi_destroy_hook (struct trivfs_protid *cred)
{
  struct netif *netif;
  struct hurdtunif *tunif;

  if (cred->pi.class != tunnel_class)
    return;

  netif = (struct netif*)cred->po->cntl->hook;
  tunif = (struct hurdtunif *) netif_get_state(netif);

  if (tunif->user == cred->user)
    tunif->user = 0;
}

/* If this variable is set, it is called every time a new peropen
   structure is created and initialized. */
error_t (*trivfs_check_open_hook)(struct trivfs_control *,
				  struct iouser *, int)
     = check_open_hook;

/* If this variable is set, it is called every time a protid structure
   is about to be destroyed. */
void (*trivfs_protid_destroy_hook) (struct trivfs_protid *) = pi_destroy_hook;

/* Read data from an IO object.  If offset is -1, read from the object
   maintained file pointer.  If the object is not seekable, offset is
   ignored.  The amount desired to be read is in AMOUNT.  */
error_t
trivfs_S_io_read (struct trivfs_protid *cred,
                  mach_port_t reply, mach_msg_type_name_t reply_type,
                  char **data, mach_msg_type_number_t *data_len,
                  loff_t offs, size_t amount)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return EINVAL;
}

/*
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this hurdethif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
hurdtunif_low_level_input(char *data, mach_msg_type_number_t datalen)
{
  struct pbuf *p, *q;
  u16_t off;

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, datalen, PBUF_POOL);

  if (p != NULL) {
    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    q = p;
    off = 0;
    do
    {
      memcpy (q->payload, data, q->len);

      off += q->len;

      if (q->tot_len == q->len)
        break;
      else
        q = q->next;
    } while(1);
  }

  return p;
}

/* Write data to an IO object.  If offset is -1, write at the object
   maintained file pointer.  If the object is not seekable, offset is
   ignored.  The amount successfully written is returned in amount.  A
   given user should not have more than one outstanding io_write on an
   object at a time; servers implement congestion control by delaying
   responses to io_write.  Servers may drop data (returning ENOBUFS)
   if they receive more than one write when not prepared for it.  */
error_t
trivfs_S_io_write (struct trivfs_protid *cred,
                   mach_port_t reply,
                   mach_msg_type_name_t replytype,
                   char *data,
                   mach_msg_type_number_t datalen,
                   off_t offset,
                   mach_msg_type_number_t *amount)
{
  struct netif *netif;
  struct pbuf *p;

  /* Deny access if they have bad credentials. */
  if (!cred)
    return EOPNOTSUPP;

  else if (!(cred->po->openmodes & O_WRITE))
    return EBADF;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  netif = (struct netif*)cred->po->cntl->hook;

  /* move received packet into a new pbuf */
  p = hurdtunif_low_level_input(data, datalen);
  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    /* pass it to the stack */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("trivfs_S_io_write: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }

  *amount = datalen;

  return 0;
}

/* Tell how much data can be read from the object without blocking for
   a "long time" (this should be the same meaning of "long time" used
   by the nonblocking flag.  */
kern_return_t
trivfs_S_io_readable (struct trivfs_protid *cred,
                      mach_port_t reply, mach_msg_type_name_t replytype,
                      mach_msg_type_number_t *amount)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return EINVAL;
}

/* SELECT_TYPE is the bitwise OR of SELECT_READ, SELECT_WRITE, and SELECT_URG.
   Block until one of the indicated types of i/o can be done "quickly", and
   return the types that are then available.  ID_TAG is returned as passed; it
   is just for the convenience of the user in matching up reply messages with
   specific requests sent.  */
static error_t
io_select_common (struct trivfs_protid *cred,
		  mach_port_t reply,
		  mach_msg_type_name_t reply_type,
		  struct timespec *tsp, int *type)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return EINVAL;
}

error_t
trivfs_S_io_select (struct trivfs_protid *cred,
                    mach_port_t reply,
                    mach_msg_type_name_t reply_type,
                    int *type)
{
  return io_select_common (cred, reply, reply_type, NULL, type);
}

error_t
trivfs_S_io_select_timeout (struct trivfs_protid *cred,
			    mach_port_t reply,
			    mach_msg_type_name_t reply_type,
			    struct timespec ts,
			    int *type)
{
  return io_select_common (cred, reply, reply_type, &ts, type);
}

/* Change current read/write offset */
error_t
trivfs_S_io_seek (struct trivfs_protid *cred,
                  mach_port_t reply, mach_msg_type_name_t reply_type,
                  off_t offs, int whence, off_t *new_offs)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return ESPIPE;
}

/* Change the size of the file.  If the size increases, new blocks are
   zero-filled.  After successful return, it is safe to reference mapped
   areas of the file up to NEW_SIZE.  */
error_t
trivfs_S_file_set_size (struct trivfs_protid *cred,
                        mach_port_t reply, mach_msg_type_name_t reply_type,
                        off_t size)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return size == 0 ? 0 : EINVAL;
}

/* These four routines modify the O_APPEND, O_ASYNC, O_FSYNC, and
   O_NONBLOCK bits for the IO object. In addition, io_get_openmodes
   will tell you which of O_READ, O_WRITE, and O_EXEC the object can
   be used for.  The O_ASYNC bit affects icky async I/O; good async
   I/O is done through io_async which is orthogonal to these calls. */
error_t
trivfs_S_io_set_all_openmodes(struct trivfs_protid *cred,
                              mach_port_t reply,
                              mach_msg_type_name_t reply_type,
                              int mode)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return 0;
}

error_t
trivfs_S_io_set_some_openmodes (struct trivfs_protid *cred,
                                mach_port_t reply,
                                mach_msg_type_name_t reply_type,
                                int bits)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return 0;
}

error_t
trivfs_S_io_clear_some_openmodes (struct trivfs_protid *cred,
                                  mach_port_t reply,
                                  mach_msg_type_name_t reply_type,
                                  int bits)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return 0;
}

error_t
trivfs_S_io_get_owner (struct trivfs_protid *cred,
                       mach_port_t reply,
                       mach_msg_type_name_t reply_type,
                       pid_t *owner)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  *owner = 0;
  return 0;
}

error_t
trivfs_S_io_mod_owner (struct trivfs_protid *cred,
                       mach_port_t reply, mach_msg_type_name_t reply_type,
                       pid_t owner)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return EINVAL;
}

/* Return objects mapping the data underlying this memory object.  If
   the object can be read then memobjrd will be provided; if the
   object can be written then memobjwr will be provided.  For objects
   where read data and write data are the same, these objects will be
   equal, otherwise they will be disjoint.  Servers are permitted to
   implement io_map but not io_map_cntl.  Some objects do not provide
   mapping; they will set none of the ports and return an error.  Such
   objects can still be accessed by io_read and io_write.  */
error_t
trivfs_S_io_map (struct trivfs_protid *cred,
		 mach_port_t reply,
		 mach_msg_type_name_t replyPoly,
		 memory_object_t *rdobj,
		 mach_msg_type_name_t *rdtype,
		 memory_object_t *wrobj,
		 mach_msg_type_name_t *wrtype)
{
  if (!cred)
    return EOPNOTSUPP;

  if (cred->pi.class != tunnel_class)
    return EOPNOTSUPP;

  return EINVAL;
}
