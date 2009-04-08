/* psplib/ctrl.c: Controller routines (legacy version)
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

#include <time.h>
#include <psptypes.h>
#include <psprtc.h>

#include "ctrl.h"

#define PSP_CTRL_DELAY     400
#define PSP_CTRL_THRESHOLD 50
#define PSP_CTRL_BUTTONS   12

static int PollingMode;
static u64 PushTime[PSP_CTRL_BUTTONS];
static const int ButtonMap[PSP_CTRL_BUTTONS] = 
{
  PSP_CTRL_UP,
  PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,
  PSP_CTRL_RIGHT,
  PSP_CTRL_CROSS,
  PSP_CTRL_CIRCLE,
  PSP_CTRL_SQUARE,
  PSP_CTRL_TRIANGLE,
  PSP_CTRL_LTRIGGER,
  PSP_CTRL_RTRIGGER,
  PSP_CTRL_SELECT,
  PSP_CTRL_START,
};

void pspCtrlInit()
{
  /* Init PSP controller */
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  PollingMode = PSP_CTRL_NORMAL;
}

int pspCtrlGetPollingMode()
{
  return PollingMode;
}

void pspCtrlSetPollingMode(int mode)
{
  /* If autorepeat is being shut off, wait until it's "safe" */
  if (PollingMode == PSP_CTRL_AUTOREPEAT)
  {
    int i;
    SceCtrlData p;
    u64 tick;
    int wait;

    do
    {
      wait = 0;

      if (!sceCtrlPeekBufferPositive(&p, 1))
        break;

      /* Get current tick count */
      sceRtcGetCurrentTick(&tick);

      /* If at least one button is being held, wait until */
      /* next autorepeat interval, or until it's released */
      for (i = 0; i < PSP_CTRL_BUTTONS; i++)
        if (tick < PushTime[i] && (p.Buttons & ButtonMap[i]))
          { wait = 1; break; }
    }
    while (wait);
  }

  PollingMode = mode;

  /* If autorepeat is being turned on, initialize autorepeat data */
  if (mode == PSP_CTRL_AUTOREPEAT)
  {
    SceCtrlData p;
    int i;
    u64 tick;
    u32 tick_res;

    /* Poll the controls */
    if (sceCtrlPeekBufferPositive(&p, 1))
    {
      /* Get current tick count */
      sceRtcGetCurrentTick(&tick);
      tick_res = sceRtcGetTickResolution();

      /* Check each button */
      for (i = 0; i < PSP_CTRL_BUTTONS; i++)
        PushTime[i] = (p.Buttons & ButtonMap[i]) ? tick + PSP_CTRL_DELAY * (tick_res / 1000) : 0;
    }
  }
}

int pspCtrlPollControls(SceCtrlData *pad)
{
  int stat;

  /* Simulate button autorepeat */
  if (PollingMode == PSP_CTRL_AUTOREPEAT)
  {
    SceCtrlData p;
    int stat, i;
    u64 tick;
    u32 tick_res;

    /* Poll the controls */
    if (!(stat = sceCtrlPeekBufferPositive(&p, 1)))
      return stat;

    /* Get current tick count */
    sceRtcGetCurrentTick(&tick);
    tick_res = sceRtcGetTickResolution();

    /* Check each button */
    for (i = 0; i < PSP_CTRL_BUTTONS; i++)
    {
      if (p.Buttons & ButtonMap[i])
      {
        if (!PushTime[i] || tick >= PushTime[i])
        {
          /* Button was pushed for the first time, or time to repeat */
          pad->Buttons |= ButtonMap[i];
          /* Compute next press time */
          PushTime[i] = tick + ((PushTime[i]) ? PSP_CTRL_THRESHOLD : PSP_CTRL_DELAY)
            * (tick_res / 1000);
        }
        else
        {
          /* No need to repeat yet */
          pad->Buttons &= ~ButtonMap[i];
        }
      }
      else
      {
        /* Button was released */
        pad->Buttons &= ~ButtonMap[i];
        PushTime[i] = 0;        
      }
    }

    /* Copy analog stick status */
    pad->Lx = p.Lx;
    pad->Ly = p.Ly;

    /* Unset the analog stick bits */
    pad->Buttons &= ~(PSP_CTRL_ANALUP 
      | PSP_CTRL_ANALDOWN 
      | PSP_CTRL_ANALLEFT 
      | PSP_CTRL_ANALRIGHT);

    /* Set the bits based on analog stick status */
    if (pad->Ly < 32) pad->Buttons |= PSP_CTRL_ANALUP;
    else if (pad->Ly >= 224) pad->Buttons |= PSP_CTRL_ANALDOWN;
    if (pad->Lx < 32) pad->Buttons |= PSP_CTRL_ANALLEFT;
    else if (pad->Lx >= 224) pad->Buttons |= PSP_CTRL_ANALRIGHT;

    return stat;
  }

  /* Default is normal behavior */
  if (!(stat = sceCtrlPeekBufferPositive(pad, 1)))
    return stat;

  /* Unset the analog stick bits */
  pad->Buttons &= ~(PSP_CTRL_ANALUP 
    | PSP_CTRL_ANALDOWN 
    | PSP_CTRL_ANALLEFT 
    | PSP_CTRL_ANALRIGHT);

  /* Set the bits based on analog stick status */
  if (pad->Ly < 32) pad->Buttons |= PSP_CTRL_ANALUP;
  else if (pad->Ly >= 224) pad->Buttons |= PSP_CTRL_ANALDOWN;
  if (pad->Lx < 32) pad->Buttons |= PSP_CTRL_ANALLEFT;
  else if (pad->Lx >= 224) pad->Buttons |= PSP_CTRL_ANALRIGHT;

  return stat;
}

