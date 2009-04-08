/* psplib/pl_rewind.h: State rewinding routines
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

#ifndef _PL_REWIND_H
#define _PL_REWIND_H

#ifdef __cplusplus
extern "C" {
#endif

struct rewind_state;

typedef struct
{
  int state_data_size;
  int state_count;
  struct rewind_state *start;
  struct rewind_state *current;
  int (*save_state)(void *);
  int (*load_state)(void *);
  int (*get_state_size)();
} pl_rewind;

int  pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)());
void pl_rewind_realloc(pl_rewind *rewind);
void pl_rewind_destroy(pl_rewind *rewind);
void pl_rewind_reset(pl_rewind *rewind);
int  pl_rewind_save(pl_rewind *rewind);
int  pl_rewind_restore(pl_rewind *rewind);

#ifdef __cplusplus
}
#endif

#endif // _PL_REWIND_H
