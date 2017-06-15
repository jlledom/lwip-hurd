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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <argp.h>

#include <lwip/netif.h>
#include <lwip/sockets.h>

/* Used to describe a particular interface during argument parsing.  */
struct parse_interface
{
  /* The network interface in question.  */
  struct netif device;
  char name[256];

  /* New values to apply to it. (IPv4) */
  uint32_t address, netmask, peer, gateway;
};

/* Used to hold data during argument parsing.  */
struct parse_hook
{
  /* A list of specified interfaces and their corresponding options.  */
  struct parse_interface *interfaces;
  size_t num_interfaces;

  /* Interface to which options apply.  If the device field isn't filled in
     then it should be by the next --interface option.  */
  struct parse_interface *curint;
};

/* Lwip translator options.  Used for both startup and runtime.  */
static const struct argp_option options[] =
{
  {"interface", 'i', "DEVICE",   0,  "Network interface to use", 1},
  {0,0,0,0,"These apply to a given interface:", 2},
  {"address",   'a', "ADDRESS",  OPTION_ARG_OPTIONAL, "Set the network address"},
  {"netmask",   'm', "MASK",     OPTION_ARG_OPTIONAL, "Set the netmask"},
  {"gateway",   'g', "ADDRESS",  OPTION_ARG_OPTIONAL, "Set the default gateway"},
  {0}
};

static const char doc[] = "Interface-specific options before the first \
interface specification apply to the first following interface; otherwise \
they apply to the previously specified interface.";

struct parse_hook *ifs;

#endif // OPTIONS_H
