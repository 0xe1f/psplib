// Copyright 2007-2015 Akop Karapetyan
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
