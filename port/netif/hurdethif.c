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

#include <netif/hurdethif.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <error.h>
#include <device/device.h>
#include <device/net_status.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <lwip/ethip6.h>
#include <lwip/etharp.h>

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

/* Get the MAC address from an array of int */
#define GET_HWADDR_BYTE(x,n)  (((char*)x)[n])

static short ether_filter[] =
{
#ifdef NETF_IN
  /* We have to tell the packet filtering code that we're interested in
     incoming packets.  */
  NETF_IN, /* Header.  */
#endif
  NETF_PUSHLIT | NETF_NOP,
  1
};
static int ether_filter_len = sizeof (ether_filter) / sizeof (short);

static struct bpf_insn bpf_ether_filter[] =
{
    {NETF_IN|NETF_BPF, 0, 0, 0},		/* Header. */
    {BPF_LD|BPF_H|BPF_ABS, 0, 0, 12},		/* Load Ethernet type */
    {BPF_JMP|BPF_JEQ|BPF_K, 2, 0, 0x0806},	/* Accept ARP */
    {BPF_JMP|BPF_JEQ|BPF_K, 1, 0, 0x0800},	/* Accept IPv4 */
    {BPF_JMP|BPF_JEQ|BPF_K, 0, 1, 0x86DD},	/* Accept IPv6 */
    /*
     * And return an amount of bytes equal to:
     * MSS + IP and transport headers length + Ethernet header length
     */
    {BPF_RET|BPF_K, 0, 0, TCP_MSS + 0x28 + PBUF_LINK_HLEN},
    {BPF_RET|BPF_K, 0, 0, 0},			/* Or discard it all */
};
static int bpf_ether_filter_len = sizeof (bpf_ether_filter) / sizeof (short);

/* Bucket and class for the incoming data */
struct port_bucket *etherport_bucket;
struct port_class *etherread_class;

/* Thread for the incoming data */
static pthread_t input_thread;

/* Get the device flags */
static error_t
hurdethif_device_get_flags(struct netif *netif, uint16_t *flags)
{
  error_t err = 0;
  size_t count;
  struct net_status status;
  hurdethif *ethif;

  memset(&status, 0, sizeof(struct net_status));

  ethif = netif_get_state(netif);
  count = NET_STATUS_COUNT;
  err = device_get_status (ethif->ether_port,
                            NET_STATUS, (dev_status_t)&status, &count);
  if(err == D_INVALID_OPERATION)
  {
    /*
     * eth-multiplexer doesn't support setting flags.
     * We must ignore D_INVALID_OPERATION.
     */
    fprintf(stderr, "%s: hardware doesn't support getting flags.\n",
              ethif->devname);
    err = 0;
  }
  else if (err)
    error (2, err, "%s: Cannot set hardware flags", ethif->devname);

  *flags = status.flags;

  return err;
}

/* Set the device flags */
static error_t
hurdethif_device_set_flags(struct netif *netif, uint16_t flags)
{
  error_t err = 0;
  hurdethif *ethif;
  int sflags;

  sflags = flags;
  ethif = netif_get_state(netif);
  err = device_set_status (ethif->ether_port, NET_FLAGS, &sflags, 1);
  if(err == D_INVALID_OPERATION)
  {
    /*
     * eth-multiplexer doesn't support setting flags.
     * We must ignore D_INVALID_OPERATION.
     */
    fprintf(stderr, "%s: hardware doesn't support setting flags.\n",
              ethif->devname);
    err = 0;
  }
  else if (err)
    error (2, err, "%s: Cannot set hardware flags", ethif->devname);

  ethif->flags = flags;

  return err;
}

/* Use the device interface to access the device */
static err_t
hurdethif_device_open (struct netif *netif)
{
  err_t err;
  device_t master_device;
  hurdethif *ethif = netif_get_state(netif);

  LWIP_ASSERT ("hurdethif->ether_port == MACH_PORT_NULL",
                  ethif->ether_port == MACH_PORT_NULL);

  err = ports_create_port (etherread_class, etherport_bucket,
        sizeof (struct port_info), &ethif->readpt);
  LWIP_ASSERT ("err==0", err==0);
  ethif->readptname = ports_get_right (ethif->readpt);
  mach_port_insert_right (mach_task_self (), ethif->readptname,
                          ethif->readptname, MACH_MSG_TYPE_MAKE_SEND);

  mach_port_set_qlimit (mach_task_self (), ethif->readptname,
                          MACH_PORT_QLIMIT_MAX);

  master_device = file_name_lookup (ethif->devname, O_RDWR, 0);
  if (master_device != MACH_PORT_NULL)
    {
      /* The device name here is the path of a device file.  */
      err = device_open (master_device, D_WRITE | D_READ,
                          "eth", &ethif->ether_port);
      mach_port_deallocate (mach_task_self (), master_device);
      if (err)
        error (2, err, "device_open on %s", ethif->devname);

      err = device_set_filter (ethif->ether_port, ethif->readptname,
             MACH_MSG_TYPE_MAKE_SEND, 0,
             (filter_array_t)bpf_ether_filter, bpf_ether_filter_len);
      if (err)
        error (2, err, "device_set_filter on %s", ethif->devname);
    }
  else
    {
      /* No, perhaps a Mach device?  */
      int file_errno = errno;
      err = get_privileged_ports (0, &master_device);
      if (err)
      {
        error (0, file_errno, "file_name_lookup %s", ethif->devname);
        error (2, err, "and cannot get device master port");
      }
      err = device_open (master_device, D_WRITE | D_READ,
                          ethif->devname, &ethif->ether_port);
      mach_port_deallocate (mach_task_self (), master_device);
      if (err)
      {
        error (0, file_errno, "file_name_lookup %s", ethif->devname);
        error (2, err, "device_open(%s)", ethif->devname);
      }

      err = device_set_filter (ethif->ether_port, ethif->readptname,
             MACH_MSG_TYPE_MAKE_SEND, 0,
             (filter_array_t)ether_filter, ether_filter_len);
      if (err)
        error (2, err, "device_set_filter on %s", ethif->devname);
    }

  return ERR_OK;
}

/* Destroy our link to the device */
static err_t
hurdethif_device_close (struct netif *netif)
{
  hurdethif *ethif = netif_get_state(netif);

  mach_port_deallocate (mach_task_self (), ethif->readptname);
  ethif->readptname = MACH_PORT_NULL;
  ports_destroy_right (ethif->readpt);
  ethif->readpt = NULL;
  device_close (ethif->ether_port);
  mach_port_deallocate (mach_task_self (), ethif->ether_port);
  ethif->ether_port = MACH_PORT_NULL;
  
  return ERR_OK;
}

/*
 * In this function, the hardware should be initialized.
 * Called from hurdethif_init().
 */
static err_t
hurdethif_low_level_init(struct netif *netif)
{
  err_t err;
  size_t count = 2;
  int net_address[2];
  device_t ether_port;

  err = hurdethif_device_open(netif);
  LWIP_ASSERT ("err==0", err==0);

  /* et the MAC address */
  ether_port = netif_get_state(netif)->ether_port;
  err = device_get_status (ether_port, NET_ADDRESS, net_address, &count);
  LWIP_ASSERT ("count * sizeof (int) >= ETHARP_HWADDR_LEN",
                  count * sizeof (int) >= ETHARP_HWADDR_LEN);
  if (err)
    error (2, err, "%s: Cannot get hardware Ethernet address",
            netif_get_state(netif)->devname);
  net_address[0] = ntohl (net_address[0]);
  net_address[1] = ntohl (net_address[1]);

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = GET_HWADDR_BYTE(net_address,0);
  netif->hwaddr[1] = GET_HWADDR_BYTE(net_address,1);
  netif->hwaddr[2] = GET_HWADDR_BYTE(net_address,2);
  netif->hwaddr[3] = GET_HWADDR_BYTE(net_address,3);
  netif->hwaddr[4] = GET_HWADDR_BYTE(net_address,4);
  netif->hwaddr[5] = GET_HWADDR_BYTE(net_address,5);

  /* maximum transfer unit */
  netif->mtu = TCP_MSS + 0x28;

  /* Enable Ethernet multicasting */
  hurdethif_device_get_flags(netif, &netif_get_state(netif)->flags);
  netif_get_state(netif)->flags |= IFF_BROADCAST|IFF_ALLMULTI;
  hurdethif_device_set_flags(netif, netif_get_state(netif)->flags);

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP
                | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware
   * know to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  return 0;
}

/*
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 */
static err_t
hurdethif_low_level_output(struct netif *netif, struct pbuf *p)
{
  error_t err;
  hurdethif *ethif = netif_get_state(netif);
  int count;
  u8_t tried;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  LWIP_ASSERT ("p->next==0", p->next==0);
  tried = 0;
  /* Send the data from the pbuf to the interface, one pbuf at a
   time. The size of the data in each pbuf is kept in the ->len
   variable. */
  do
    {
      tried++;
      err = device_write(ethif->ether_port, D_NOWAIT, 0,
                          p->payload, p->len, &count);
      if (err == EMACH_SEND_INVALID_DEST || err == EMIG_SERVER_DIED)
      {
        /* Device probably just died, try to reopen it.  */

        if (tried == 2)
          /* Too many tries, abort */
          break;

        hurdethif_device_close (netif);
        hurdethif_device_open (netif);
      }
          else
      {
        LWIP_ASSERT ("err==0", err==0);
        LWIP_ASSERT ("count == p->len", count == p->len);
      }
    }
  while (err);

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t*)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/*
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 */
static struct pbuf *
hurdethif_low_level_input(struct netif *netif, struct net_rcv_msg *msg)
{
  struct pbuf *p, *q;
  u16_t len;
  u16_t off;
  u16_t next_read;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = PBUF_LINK_HLEN
    + msg->packet_type.msgt_number - sizeof (struct packet_header);

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    q = p;
    off = 0;
    do
    {
      /* Read enough bytes to fill this pbuf in the chain. The
       * available data in the pbuf is given by the q->len
       * variable.
       * This does not necessarily have to be a memcpy, you can also preallocate
       * pbufs for a DMA-enabled MAC and after receiving truncate it to the
       * actually received size. In this case, ensure the tot_len member of the
       * pbuf is the sum of the chained pbuf len members.
       */
      if(off < PBUF_LINK_HLEN)
      {
        /* We still haven't ended copying the header */
        next_read = (off + q->len) > PBUF_LINK_HLEN ?
                    (PBUF_LINK_HLEN - off) : q->len;
        memcpy (q->payload, msg->header + off, next_read);

        if((off + q->len) > PBUF_LINK_HLEN)
          memcpy (q->payload + PBUF_LINK_HLEN,
                  msg->packet + sizeof (struct packet_header),
                  q->len - next_read);
      }
      else
        /* The header is copyied yet */
        memcpy (q->payload, msg->packet +
        sizeof (struct packet_header) + off - PBUF_LINK_HLEN,
            q->len);

      off += q->len;

      if (q->tot_len == q->len)
        break;
      else
        q = q->next;
    } while(1);

    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if (((u8_t*)p->payload)[0] & 1) {
      /* broadcast or multicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
      /* unicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }

  return p;
}

/*
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 */
void
hurdethif_input(struct netif *netif, struct net_rcv_msg *msg)
{
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = hurdethif_low_level_input(netif, msg);
  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    /*
     * pass all packets to ethernet_input, which decides
     * what packets it supports
     */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("hurdethif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

/* Demux incoming RPCs from the device */
int
hurdethif_demuxer (mach_msg_header_t *inp,
		  mach_msg_header_t *outp)
{
  struct net_rcv_msg *msg = (struct net_rcv_msg *) inp;
  struct netif *netif;
  mach_port_t local_port;

  if (inp->msgh_id != NET_RCV_MSG_ID)
    return 0;

  if (MACH_MSGH_BITS_LOCAL (inp->msgh_bits) ==
      MACH_MSG_TYPE_PROTECTED_PAYLOAD)
  {
    struct port_info *pi = ports_lookup_payload (NULL,
             inp->msgh_protected_payload,
             NULL);
    if (pi)
    {
      local_port = pi->port_right;
      ports_port_deref (pi);
    }
    else
      local_port = MACH_PORT_NULL;
  }
  else
    local_port = inp->msgh_local_port;

  for (netif = netif_list; netif; netif = netif->next)
    if (local_port == netif_get_state(netif)->readptname)
      break;

  if (!netif)
  {
    if (inp->msgh_remote_port != MACH_PORT_NULL)
      mach_port_deallocate (mach_task_self (), inp->msgh_remote_port);
    return 1;
  }
  
  hurdethif_input(netif, msg);

  return 1;
}

/*
 * Update the interface's MTU and the BPF filter
 */
error_t hurdethif_update_mtu(struct netif *netif, uint32_t mtu)
{
  error_t err = 0;

  netif->mtu = mtu;

  bpf_ether_filter[5].k = mtu + PBUF_LINK_HLEN;

  return err;
}

/*
 * Release all resources of this netif.
 *
 * Returns 0 on success.
 */
error_t
hurdethif_terminate(struct netif *netif)
{
  /* Free the interface and its hook */
  mem_free (netif_get_state(netif)->devname);
  mem_free (netif_get_state(netif));

  return 0;
}

/*
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add() so it may be
 * called many times.
 */
err_t
hurdethif_init(struct netif *netif)
{
  hurdethif *ethif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

  ethif = mem_malloc(sizeof(hurdethif));
  if (ethif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("hurdethif_init: out of memory\n"));
    return ERR_MEM;
  }
  memset(ethif, 0, sizeof(hurdethif));

  ethif->devname = mem_malloc(strlen(netif->state)+1);
  if (ethif->devname == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("hurdethif_init: out of memory\n"));
    return ERR_MEM;
  }
  memset(ethif->devname, 0, strlen(netif->state)+1);

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd,
                    LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  strncpy(ethif->devname, netif->state, strlen(netif->state));
  netif->state = ethif;

  ethif->type = ARPHRD_ETHER;

  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = hurdethif_low_level_output;

  ethif->terminate = hurdethif_terminate;
  ethif->update_mtu = hurdethif_update_mtu;
  ethif->change_flags = hurdethif_device_set_flags;

  /* initialize the hardware */
  return hurdethif_low_level_init(netif);
}

static void *
hurdethif_input_thread (void *arg)
{
  ports_manage_port_operations_one_thread (etherport_bucket,
            hurdethif_demuxer,
            0);

  return 0;
}

/*
 * Init the thread for the incoming data.
 *
 * This function should be called once.
 */
error_t
hurdethif_module_init()
{
  error_t err;
  etherport_bucket = ports_create_bucket ();
  etherread_class = ports_create_class (0, 0);
  
  err = pthread_create (&input_thread, 0, hurdethif_input_thread, 0);
  if (!err)
    pthread_detach (input_thread);
  else
  {
    errno = err;
    perror ("pthread_create");
  }

  return err;
}
