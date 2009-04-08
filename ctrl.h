/* psplib/ctrl.h: Controller routines
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

#ifndef _PSP_CTRL_H
#define _PSP_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pspctrl.h>

/* These bits are currently unused */
#define PSP_CTRL_ANALUP    0x00000002
#define PSP_CTRL_ANALDOWN  0x00000004
#define PSP_CTRL_ANALLEFT  0x00000400
#define PSP_CTRL_ANALRIGHT 0x00000800

#define PSP_CTRL_NORMAL     0
#define PSP_CTRL_AUTOREPEAT 1

void pspCtrlInit();
int  pspCtrlGetPollingMode();
void pspCtrlSetPollingMode(int mode);
int  pspCtrlPollControls(SceCtrlData *pad);

#ifdef __cplusplus
}
#endif

#endif // _PSP_FILEIO_H
