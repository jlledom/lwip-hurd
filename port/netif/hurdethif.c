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
    {BPF_RET|BPF_K, 0, 0, 1500},		/* And return 1500 bytes */
    {BPF_RET|BPF_K, 0, 0, 0},			/* Or discard it all */
};
static int bpf_ether_filter_len = sizeof (bpf_ether_filter) / sizeof (short);

static err_t
open_device (struct netif *netif)
{
  err_t err;
  device_t master_device;
  struct ethernetif *ethernetif = netif->state;

  LWIP_ASSERT ("ethernetif->ether_port == MACH_PORT_NULL",
                  ethernetif->ether_port == MACH_PORT_NULL);

  err = ports_create_port (etherreadclass, etherport_bucket,
			   sizeof (struct port_info), &ethernetif->readpt);
  LWIP_ASSERT ("err==0", err==0);
  ethernetif->readptname = ports_get_right (ethernetif->readpt);
  mach_port_insert_right (mach_task_self (), ethernetif->readptname, ethernetif->readptname,
			  MACH_MSG_TYPE_MAKE_SEND);

  mach_port_set_qlimit (mach_task_self (), ethernetif->readptname, MACH_PORT_QLIMIT_MAX);

  master_device = file_name_lookup (ethernetif->devname, O_READ | O_WRITE, 0);
  if (master_device != MACH_PORT_NULL)
    {
      /* The device name here is the path of a device file.  */
      err = device_open (master_device, D_WRITE | D_READ, "eth", &ethernetif->ether_port);
      mach_port_deallocate (mach_task_self (), master_device);
      if (err)
	error (2, err, "device_open on %s", ethernetif->devname);

      err = device_set_filter (ethernetif->ether_port, ethernetif->readptname,
			       MACH_MSG_TYPE_MAKE_SEND, 0,
			       bpf_ether_filter, bpf_ether_filter_len);
      if (err)
	error (2, err, "device_set_filter on %s", ethernetif->devname);
    }
  else
    {
      /* No, perhaps a Mach device?  */
      int file_errno = errno;
      err = get_privileged_ports (0, &master_device);
      if (err)
	{
	  error (0, file_errno, "file_name_lookup %s", ethernetif->devname);
	  error (2, err, "and cannot get device master port");
	}
      err = device_open (master_device, D_WRITE | D_READ, ethernetif->devname, &ethernetif->ether_port);
      mach_port_deallocate (mach_task_self (), master_device);
      if (err)
	{
	  error (0, file_errno, "file_name_lookup %s", ethernetif->devname);
	  error (2, err, "device_open(%s)", ethernetif->devname);
	}

      err = device_set_filter (ethernetif->ether_port, ethernetif->readptname,
			       MACH_MSG_TYPE_MAKE_SEND, 0,
			       ether_filter, ether_filter_len);
      if (err)
	error (2, err, "device_set_filter on %s", ethernetif->devname);
    }

  return ERR_OK;
}

static err_t
close_device (struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;

  mach_port_deallocate (mach_task_self (), ethernetif->readptname);
  ethernetif->readptname = MACH_PORT_NULL;
  ports_destroy_right (ethernetif->readpt);
  ethernetif->readpt = NULL;
  device_close (ethernetif->ether_port);
  mach_port_deallocate (mach_task_self (), ethernetif->ether_port);
  ethernetif->ether_port = MACH_PORT_NULL;
  
  return ERR_OK;
}

/**
 * In this function, the hardware should be initialized.
 * Called from hurdethif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this hurdethif
 */
static err_t
low_level_init(struct netif *netif)
{
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = 0x52;
  netif->hwaddr[1] = 0x54;
  netif->hwaddr[2] = 0x00;
  netif->hwaddr[3] = 0xb5;
  netif->hwaddr[4] = 0x38;
  netif->hwaddr[5] = 0x41;

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  return open_device(netif);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this hurdethif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  error_t err;
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  int count;
  u8_t tried = 0;
  
  //TODO: initiate transfer();

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  for (q = p; q != NULL; q = q->next) {
    tried++;
    
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
      do
      {
        tried++;
        err = device_write (ethernetif->ether_port, D_NOWAIT, 0, q->payload, q->len, &count);
        if (err == EMACH_SEND_INVALID_DEST || err == EMIG_SERVER_DIED)
        {
          /* Device probably just died, try to reopen it.  */

          if (tried == 2)
            /* Too many tries, abort */
            break;

          close_device (netif);
          open_device (netif);
        }
            else
        {
          LWIP_ASSERT ("err==0", err==0);
          LWIP_ASSERT ("count == q->len", count == q->len);
        }
      }
    while (err);
  }
  
  //TODO: signal that packet should be sent();

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

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this hurdethif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif, struct net_rcv_msg *msg)
{
  struct pbuf *p, *q;
  u16_t len;

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
    for (q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
       * available data in the pbuf is given by the q->len
       * variable.
       * This does not necessarily have to be a memcpy, you can also preallocate
       * pbufs for a DMA-enabled MAC and after receiving truncate it to the
       * actually received size. In this case, ensure the tot_len member of the
       * pbuf is the sum of the chained pbuf len members.
       */
      memcpy (q->payload, msg->header, PBUF_LINK_HLEN);
      memcpy (q->payload + PBUF_LINK_HLEN,
                msg->packet + sizeof (struct packet_header),
                len - PBUF_LINK_HLEN);
      q->len = len;
    }
    //TODO: acknowledge that packet has been read();

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
    //TODO: drop packet();
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }

  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this hurdethif
 */
void
hurdethif_input(struct netif *netif, struct net_rcv_msg *msg)
{
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif, msg);
  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

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
    if (local_port == ((struct ethernetif*)netif->state)->readptname)
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

static void *
hurdethif_input_thread (void *arg)
{
  ports_manage_port_operations_one_thread (etherport_bucket,
					   hurdethif_demuxer,
					   0);
  return NULL;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this hurdethif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
hurdethif_init(struct netif *netif)
{
  err_t err;
  pthread_t thread;
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

  ethernetif = mem_malloc(sizeof(struct ethernetif));
  if (ethernetif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  ethernetif->devname = netif->state;
  netif->state = ethernetif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;

  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

  etherport_bucket = ports_create_bucket ();
  etherreadclass = ports_create_class (0, 0);
  
  /* initialize the input */
  err = pthread_create (&thread, NULL, hurdethif_input_thread, NULL);
  if (!err)
    pthread_detach (thread);
  else
  {
    errno = err;
    perror ("pthread_create");
  }

  /* initialize the hardware */
  return low_level_init(netif);
}
