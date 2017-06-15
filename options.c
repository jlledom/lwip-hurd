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
  memset(&h->curint->device, 0, sizeof(struct netif));
  h->curint->address = INADDR_NONE;
  h->curint->netmask = INADDR_NONE;
  h->curint->peer = INADDR_NONE;
  h->curint->gateway = INADDR_NONE;

  return 0;
}

//Option parser
static error_t
parse_opt (int opt, char *arg, struct argp_state *state)
{
  error_t err = 0;
  struct parse_hook *h = state->hook;

  /* Return _ERR from this routine, and in the special case of OPT being
     ARGP_KEY_SUCCESS, remember to free H first.  */
#define RETURN(_err)                          \
  do { if (opt == ARGP_KEY_SUCCESS)           \
  { err = (_err); goto free_hook; }          \
       else                                   \
  return _err; } while (0)

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
        if (strcmp (in->name, arg) == 0)
          /* Re-use an old slot.  */
          {
            h->curint = in;
            return 0;
          }

      if (h->curint->name[0])
      /* The current interface slot is not available.  */
      {
        /* Add a new interface entry.  */
        err = parse_hook_add_interface (h);
      }
      in = h->curint;
      
      strncpy(in->name, arg, sizeof(h->curint->name));
      in->device.name[0] = h->num_interfaces / 10 + '0';
      in->device.name[1] = h->num_interfaces % 10 + '0';
      break;

    case 'a':
      if (arg)
      {
        h->curint->address = ADDR (arg, "address");
        if (!IN_CLASSA (ntohl (h->curint->address))
            && !IN_CLASSB (ntohl (h->curint->address))
            && !IN_CLASSC (ntohl (h->curint->address)))
          {
            if (IN_MULTICAST (ntohl (h->curint->address)))
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
        h->curint->address = ADDR ("0.0.0.0", "address");
        h->curint->netmask = ADDR ("255.0.0.0", "netmask");
        h->curint->gateway = INADDR_NONE;
      }
      break;

    case 'm':
      if (arg)
        h->curint->netmask = ADDR (arg, "netmask");
      else
        h->curint->netmask = INADDR_NONE;
      break;

    case 'p':
      if (arg)
        h->curint->peer = ADDR (arg, "peer");
      else
        h->curint->peer = INADDR_NONE;
      break;

    case 'g':
      if (arg)
      {
        /* Remove any possible other default gateway */
        for (in = h->interfaces; in < h->interfaces + h->num_interfaces;
             in++)
          in->gateway = INADDR_NONE;
        h->curint->gateway = ADDR (arg, "gateway");
      }
      else
        h->curint->gateway = INADDR_NONE;
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
      ifs = h;
      break;

    case ARGP_KEY_ERROR:
      /* Parsing error occurred, free everything. */
    free_hook:
      free (h->interfaces);
      free (h);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return err;
}

struct argp lwip_argp = { options, parse_opt, 0, doc };

struct argp *trivfs_runtime_argp = &lwip_argp;
