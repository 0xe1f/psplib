/* psplib/pl_ui.c: Simple user interface implementation
   Copyright (C) 2007-2009 Akop Karapetyan

   $Id: ui.c 409 2009-04-08 20:01:39Z jack $

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

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspgu.h>

#include "image.h"
#include "pl_ui.h"

#include "pl_psp.h"
#include "pl_file.h"
#include "font.h"

static const pl_ui_metrics default_ui_metrics = 
{
  PSP_CTRL_CROSS,                      // OK
  PSP_CTRL_CIRCLE,                     // Cancel
  PL_GRAY,                             // Scrollbar fg
  PL_MAKE_COLOR(0x44,0xff,0xff,0xff),  // Scrollbar bg
  PL_WHITE,                            // Statusbar_fg
  PL_GRAY,                             // Text fg
  PL_MAKE_COLOR(0xf7,0xc2,0x50,0xFF),  // Selected fg
  PL_MAKE_COLOR(0x46,0x98,0xce,0x99),  // Selected bg
  PL_WHITE,                            // Tab fg
  PL_MAKE_COLOR(0xa4,0xa4,0xa4,0xff),  // Tab bg
  PL_MAKE_COLOR(0x59,0x91,0x38,0xBB),  // Fog bg
  10                                   // Scrollbar_width
};

static const pl_ui_file_view_metrics default_file_view_metrics = 
{
  PL_YELLOW, // Directory_fg
  30,        // Screenshot delay
  NULL       // Screenshot path
};

static const pl_ui_icon_view_metrics default_icon_view_metrics = 
{
  5, // Icons per row
  8  // Margin width;
};

static const pl_ui_menu_metrics default_menu_metrics = 
{
  PL_GRAY,                            // Option window fg
  PL_MAKE_COLOR(0x46,0x98,0xce,0xCC), // Option window bg
  PL_BLACK,                           // Option selected bg
  20                                  // Item margin
};

int  pl_ui_create(pl_ui *ui)
{
}

void pl_ui_destroy(pl_ui *ui)
{
}

