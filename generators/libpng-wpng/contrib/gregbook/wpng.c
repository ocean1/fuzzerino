/*---------------------------------------------------------------------------

   wpng-gfz - simple PNG-writing program                             wpng.c
   
   Simplified png-writing program based on Gregp

   This program converts certain NetPBM binary files (grayscale and RGB,
   maxval = 255) to PNG.  Non-interlaced PNGs are written progressively;
   interlaced PNGs are read and written in one memory-intensive blast.

   Thanks to Jean-loup Gailly for providing the necessary trick to read
   interactive text from the keyboard while stdin is redirected.  Thanks
   to Cosmin Truta for Cygwin fixes.

   NOTE:  includes provisional support for PNM type "8" (portable alphamap)
          images, presumed to be a 32-bit interleaved RGBA format; no pro-
          vision for possible interleaved grayscale+alpha (16-bit) format.
          THIS IS UNLIKELY TO BECOME AN OFFICIAL NETPBM ALPHA FORMAT!

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2007, 2017 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------*/

#define PROGNAME  "weapng-gfz"
#define VERSION   "0.2.0"
#define APPNAME   "WeaPNGen"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>     /* for jmpbuf declaration in writepng.h */
#include <time.h>
#include <sys/random.h>

/* #define DEBUG  :  this enables the Trace() macros */

/* #define FORBID_LATIN1_CTRL  :  this requires the user to re-enter any
   text that includes control characters discouraged by the PNG spec; text
   that includes an escape character (27) must be re-entered regardless */

#include "writepng.h"   /* typedefs, common macros, writepng prototypes */



/* local prototypes */
static mainprog_info wpng_info;   /* lone global */


int main(int argc, char **argv)
{
    wpng_info.outfile = fopen("/dev/shm/wpng", "wb");
    writepng_init(&wpng_info);

    fclose(wpng_info.outfile);
    return 0;
}