/* psplib/pl_ctl.h: Controller routines
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

#ifndef _PL_CTL_H
#define _PL_CTL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <psptypes.h>

enum pl_ctl_polling_mode
{
  PL_CTL_NORMAL     = 0,
  PL_CTL_AUTOREPEAT
};

enum pl_ctl_buttons 
{
  PL_CTL_UP           = 0x0001,
  PL_CTL_DOWN         = 0x0002,
  PL_CTL_LEFT         = 0x0004,
  PL_CTL_RIGHT        = 0x0008,
  PL_CTL_CROSS        = 0x0010,
  PL_CTL_CIRCLE       = 0x0020,
  PL_CTL_SQUARE       = 0x0040,
  PL_CTL_TRIANGLE     = 0x0080,
  PL_CTL_LTRIGGER     = 0x0100,
  PL_CTL_RTRIGGER     = 0x0200,
  PL_CTL_SELECT       = 0x0400,
  PL_CTL_START        = 0x0800,
  PL_CTL_ANALOG_UP    = 0x1000,
  PL_CTL_ANALOG_DOWN  = 0x2000,
  PL_CTL_ANALOG_LEFT  = 0x4000,
  PL_CTL_ANALOG_RIGHT = 0x8000,
};

typedef struct pl_ctl_config_t
{
  unsigned int buttons;
  int stick_x;
  int stick_y;
  unsigned int polling_mode;
  unsigned int delay;
  unsigned int threshold;
  u64 push_time[12];
} pl_ctl_config;

int pl_ctl_init(pl_ctl_config *config, unsigned int polling_mode);
int pl_ctl_poll(pl_ctl_config *config);

#ifdef __cplusplus
}
#endif

#endif // _PL_CTL_H
