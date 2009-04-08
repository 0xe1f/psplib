/* psplib/pl_util.h: Various utility functions
   Copyright (C) 2007-2009 Akop Karapetyan

   $Id$

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: 
     Email: dev@psp.akop.org
*/

#ifndef _PL_UTIL_H
#define _PL_UTIL_H

#include <psptypes.h>
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sequential image saves */
int pl_util_save_image_seq(const char *path,
                           const char *filename,
                           const PspImage *image);
int pl_util_save_vram_seq(const char *path,
                          const char *prefix);
int pl_util_date_compare(const ScePspDateTime *date1,
                         const ScePspDateTime *date2);
int pl_util_compute_crc32_buffer(const void *buf,
                                 size_t buf_len,
                                 uint32_t *crc_32);
int pl_util_compute_crc32_fd(FILE *file,
                             uint32_t *outCrc32);
int pl_util_compute_crc32_file(const char *path,
                               uint32_t *outCrc32);

#ifdef __cplusplus
}
#endif

#endif // _PL_UTIL_H
