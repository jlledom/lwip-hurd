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

#include <lwip_pfinet_S.h>

/* Return the list of devices in the format provided by SIOCGIFCONF
   in IFR, but don't return more then AMOUNT bytes. If AMOUNT is
   negative, there is no limit.  */
error_t
lwip_S_pfinet_siocgifconf (io_t port,
		      vm_size_t amount,
		      char **ifr,
		      mach_msg_type_number_t *len)
{
  return ENOTSUP;
}
