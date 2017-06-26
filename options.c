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

#include <options.h>

#include <stdlib.h>
#include <argp.h>
#include <argz.h>

#include <lwip-hurd.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <netif/hurdethif.h>

/* Adds an empty interface slot to H, and sets H's current interface to it, or
   returns an error. */
static error_t
parse_hook_add_interface (struct parse_hook *h)
{
  struct parse_interface *new =
    realloc (h->interfaces,
      (h->num_interfaces + 1) * sizeof (struct parse_interface));
  if (! new)
    return ENOMEM;

  h->interfaces = new;
  h->num_interfaces++;
  h->curint = new + h->num_interfaces - 1;
  memset(&h->curint->dev_name, 0, DEV_NAME_LEN);
  memset(&h->curint->lwip_name, 0, LWIP_NAME_LEN);
  h->curint->address.addr = INADDR_NONE;
  h->curint->netmask.addr = INADDR_NONE;
  h->curint->peer.addr = INADDR_NONE;
  h->curint->gateway.addr = INADDR_NONE;

  return 0;
}

//Option parser
static error_t
parse_opt (int opt, char *arg, struct argp_state *state)
{
  error_t err = 0;
  struct parse_hook *h = state->hook;

  /* Return _ERR from this routine */
#define RETURN(_err)                          \
  do { return _err; } while (0)

  /* Print a parsing error message and (if exiting is turned off) return the
     error code ERR.  */
#define PERR(err, fmt, args...)               \
  do { argp_error (state, fmt , ##args); RETURN (err); } while (0)

  /* Like PERR but for non-parsing errors.  */
#define FAIL(rerr, status, perr, fmt, args...)  \
  do{ argp_failure (state, status, perr, fmt , ##args); RETURN (rerr); } while(0)

  /* Parse STR and return the corresponding  internet address.  If STR is not
     a valid internet address, signal an error mentioned TYPE.  */
#undef	ADDR
#define ADDR(str, type)                         \
  ({ unsigned long addr = inet_addr (str);      \
     if (addr == INADDR_NONE) PERR (EINVAL, "Malformed %s", type);  \
     addr; })

  if (!arg && state->next < state->argc
      && (*state->argv[state->next] != '-'))
    {
      arg = state->argv[state->next];
      state->next ++;
    }

  switch (opt)
    {
      struct parse_interface *in;

    case 'i':
      /* An interface.  */
      err = 0;

      /* First see if a previously specified one is being re-specified.  */
      for (in = h->interfaces; in < h->interfaces + h->num_interfaces; in++)
        if (strcmp (in->dev_name, arg) == 0)
          /* Re-use an old slot.  */
          {
            h->curint = in;
            return 0;
          }

      if (h->curint->dev_name[0])
      /* The current interface slot is not available.  */
      {
        /* Add a new interface entry.  */
        err = parse_hook_add_interface (h);
      }
      in = h->curint;

      strncpy(in->dev_name, arg, DEV_NAME_LEN);
      in->lwip_name[0] = h->num_interfaces / 10 + '0';
      in->lwip_name[1] = h->num_interfaces % 10 + '0';
      break;

    case 'a':
      if (arg)
      {
        h->curint->address.addr = ADDR (arg, "address");
        if (!IN_CLASSA (ntohl (h->curint->address.addr))
            && !IN_CLASSB (ntohl (h->curint->address.addr))
            && !IN_CLASSC (ntohl (h->curint->address.addr)))
          {
            if (IN_MULTICAST (ntohl (h->curint->address.addr)))
              FAIL (EINVAL, 1, 0,
                    "%s: Cannot set interface address to multicast address",
                     arg);
            else
              FAIL (EINVAL, 1, 0,
                    "%s: Illegal or undefined network address", arg);
          }
      }
      else
      {
        h->curint->address.addr = ADDR ("0.0.0.0", "address");
        h->curint->netmask.addr = ADDR ("255.0.0.0", "netmask");
        h->curint->gateway.addr = INADDR_NONE;
      }
      break;

    case 'm':
      if (arg)
        h->curint->netmask.addr = ADDR (arg, "netmask");
      else
        h->curint->netmask.addr = INADDR_NONE;
      break;

    case 'p':
      if (arg)
        h->curint->peer.addr = ADDR (arg, "peer");
      else
        h->curint->peer.addr = INADDR_NONE;
      break;

    case 'g':
      if (arg)
      {
        h->curint->gateway.addr = ADDR (arg, "gateway");
      }
      else
        h->curint->gateway.addr = INADDR_NONE;
      break;

    case '4':
      translator_bind (PORTCLASS_INET, arg);

      /* Install IPv6 port class on bootstrap port. */
      lwip_bootstrap_portclass = PORTCLASS_INET6;
      break;

    case '6':
      translator_bind (PORTCLASS_INET6, arg);
      break;

    case 'A':
      if (arg)
      {
        if (ip6addr_aton(arg, &h->curint->address6) <= 0)
          PERR (EINVAL, "Malformed address");

        if (ip6_addr_ismulticast (&h->curint->address6))
          FAIL (EINVAL, 1, 0, "%s: Cannot set interface address to "
          "multicast address", arg);
      }
      else
        ip6_addr_set_zero (&h->curint->address6);
      break;

    case ARGP_KEY_INIT:
      /* Initialize our parsing state.  */
      h = malloc (sizeof (struct parse_hook));
      if (! h)
        FAIL (ENOMEM, 11, ENOMEM, "option parsing");

      h->interfaces = 0;
      h->num_interfaces = 0;
      err = parse_hook_add_interface (h);
      if (err)
        FAIL (err, 12, err, "option parsing");

      state->hook = h;
      break;

    case ARGP_KEY_SUCCESS:
      /* Inititalize LwIP */
      tcpip_init(init_ifs, h);
      break;

    case ARGP_KEY_ERROR:
      /* Parsing error occurred, free everything. */
      free (h->interfaces);
      free (h);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return err;
}

error_t
trivfs_append_args (struct trivfs_control *fsys, char **argz, size_t *argz_len)
{
  error_t err = 0;
  struct netif *netif;
  int i;

#define ADD_OPT(fmt, args...)           \
  do { char buf[100];                   \
       if (! err) {                     \
         snprintf (buf, sizeof buf, fmt , ##args);      \
         err = argz_add (argz, argz_len, buf); } } while (0)
#define ADD_ADDR_OPT(name, addr)        \
  do { struct in_addr i;                \
       i.s_addr = (addr);               \
       ADD_OPT ("--%s=%s", name, inet_ntoa (i)); } while (0)

  netif = netif_list;
  while(netif != 0)
  {
    ADD_OPT ("--interface=%s", ((struct hurdethif*)netif->state)->devname);
    if (netif->ip_addr.u_addr.ip4.addr != INADDR_NONE)
      ADD_ADDR_OPT ("address", netif->ip_addr.u_addr.ip4.addr);
    if (netif->netmask.u_addr.ip4.addr != INADDR_NONE)
      ADD_ADDR_OPT ("netmask", netif->netmask.u_addr.ip4.addr);
    if (netif->gw.u_addr.ip4.addr != INADDR_NONE)
      ADD_ADDR_OPT ("gateway", netif->gw.u_addr.ip4.addr);
    for(i=0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
      if (!ip6_addr_isany(&netif->ip6_addr[i].u_addr.ip6))
        ADD_OPT("--address6=%s", ip6addr_ntoa(&netif->ip6_addr[i].u_addr.ip6));

    netif = netif->next;
  }

#undef ADD_ADDR_OPT

#undef ADD_OPT
  return err;
}

struct argp lwip_argp = { options, parse_opt, 0, doc };

struct argp *trivfs_runtime_argp = &lwip_argp;
