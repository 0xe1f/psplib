/* psplib/pl_perf.h: Performance benchmarking
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

#ifndef _PL_PERF_H
#define _PL_PERF_H

#include <psptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pl_perf_counter_t
{
  float ticks_per_second;
  int frame_count;
  u64 last_tick;
  float fps;
} pl_perf_counter;

void  pl_perf_init_counter(pl_perf_counter *counter);
float pl_perf_update_counter(pl_perf_counter *counter);

#ifdef __cplusplus
}
#endif

#endif // _PL_PERF_H
