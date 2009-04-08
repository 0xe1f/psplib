/* psplib/pl_psp.h: Platform init/shutdown and various system routines
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

#ifndef _PL_PSP_H
#define _PL_PSP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  PSP_EXIT_CALLBACK
} pl_callback_type;

extern int ExitPSP;

int  pl_psp_init(const char *app_path);
void pl_psp_shutdown();
void pl_psp_set_clock_freq(int freq);
const char* pl_psp_get_app_directory();

int pl_psp_register_callback(pl_callback_type type,
                             void (*func)(void *param),
                             void *param);
int pl_psp_start_callback_thread();

#ifdef __cplusplus
}
#endif

#endif // _PL_PSP_H
