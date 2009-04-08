/* psplib/pl_perf.c: Performance benchmarking
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

#include <pspkernel.h>
#include <psprtc.h>

#include "pl_perf.h"

void  pl_perf_init_counter(pl_perf_counter *counter)
{
  counter->fps = 0;
  counter->frame_count = 0;
  counter->ticks_per_second = (float)sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&counter->last_tick);
}

float pl_perf_update_counter(pl_perf_counter *counter)
{
  u64 current_tick;
  sceRtcGetCurrentTick(&current_tick);

  counter->frame_count++;
  if (current_tick - counter->last_tick >= 
      counter->ticks_per_second)
  {
    /* A second elapsed; recompute FPS */
    counter->fps = (float)counter->frame_count 
      / (float)((current_tick - counter->last_tick) / counter->ticks_per_second);
    counter->last_tick = current_tick;
    counter->frame_count = 0;
  }

  return counter->fps;
}
