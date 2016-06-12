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

typedef u64 pl_button;
typedef u64 pl_button_mask;

#define PL_CTRL_BUTTON_MASK
void pspCtrlInit();
int  pspCtrlGetPollingMode();
void pspCtrlSetPollingMode(int mode);
int  pspCtrlPollControls(SceCtrlData *pad);

#ifdef __cplusplus
}
#endif

#endif // _PSP_FILEIO_H
