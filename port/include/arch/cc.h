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
   along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdlib.h> /* rand() */
#include <stdio.h> /* printf() */

/* Use Glibc's fcntl() macros */
#include <fcntl.h>

/* Use Glibc's <errno.h> */
#include <errno.h>
#define LWIP_ERR_T  int

/* System provides its own struct timeval */
#define LWIP_TIMEVAL_PRIVATE  0
#include <sys/time.h>

/* Use our own <sys/socket.h> */
#include <sys/socket.h>
#include <netinet/in.h>

/* We need INT_MAX */
#include <limits.h>

/* We use poll() instead of select()*/
#include <poll.h>

/* printf() codes */
#define S16_F "d"
#define U16_F "u"
#define X16_F "x"

#define S32_F "d"
#define U32_F "u"
#define X32_F "x"

#ifdef LWIP_DEBUG
#define LWIP_PLATFORM_DIAG(x) do { printf x; fflush(NULL);} while(0)
#endif

/* Redefine LWIP_ERROR to not abort the process if the assertion failed */
#define LWIP_ERROR(message, expression, handler) \
  do { \
    if (!(expression)) { \
      printf("Assertion \"%s\" failed at line %d in %s\n", \
        message, __LINE__, __FILE__); fflush(NULL); \
      handler;} \
  } while(0)

#endif /* LWIP_ARCH_CC_H */
