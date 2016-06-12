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
  return 1;
}

void pl_ui_destroy(pl_ui *ui)
{
}

