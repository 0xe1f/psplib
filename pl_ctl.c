/* psplib/pl_ctl.c: Controller routines
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
#include <pspctrl.h>

#include "pl_ctl.h"

static int psp_controls_initialized = 0;
static const int
  monitored_buttons[] = 
  {    PSP_CTRL_UP,
    PSP_CTRL_DOWN,
    PSP_CTRL_LEFT,
    PSP_CTRL_RIGHT,
    PSP_CTRL_CROSS,
    PSP_CTRL_CIRCLE,
    PSP_CTRL_SQUARE,
    PSP_CTRL_TRIANGLE,
    PSP_CTRL_LTRIGGER,    PSP_CTRL_RTRIGGER,
    PSP_CTRL_SELECT,
    PSP_CTRL_START,
    0
  },
  virtual_buttons[] =
  {
    PL_CTL_UP,
    PL_CTL_DOWN,
    PL_CTL_LEFT,
    PL_CTL_RIGHT,
    PL_CTL_CROSS,
    PL_CTL_CIRCLE,
    PL_CTL_SQUARE,
    PL_CTL_TRIANGLE,
    PL_CTL_LTRIGGER,
    PL_CTL_RTRIGGER,
    PL_CTL_SELECT,
    PL_CTL_START,
  };

static void clear_controls(pl_ctl_config *config);

int pl_ctl_init(pl_ctl_config *config, 
                unsigned int polling_mode)
{
  if (!psp_controls_initialized)
  {
    /* Init PSP controller */
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    psp_controls_initialized = 1; /* Don't reinitialize */
  }

  config->buttons = 0;
  config->polling_mode = polling_mode;
  config->delay = 400;
  config->threshold = 50;

  clear_controls(config);

  return 1;
}

static void clear_controls(pl_ctl_config *config)
{
  if (config->polling_mode == PL_CTL_AUTOREPEAT)
  {
    SceCtrlData p;
    int i;
    u64 tick;
    u32 tick_res;
    /* Poll the controls */
    if (sceCtrlPeekBufferPositive(&p, 1))
    {      /* Get current tick count */
      sceRtcGetCurrentTick(&tick);
      tick_res = sceRtcGetTickResolution();

      /* Check each button */      for (i = 0; monitored_buttons[i]; i++)
        config->push_time[i] = (p.Buttons & monitored_buttons[i]) 
          ? tick + config->delay * (tick_res / 1000) : 0;
    }
  }
}

int pl_ctl_poll(pl_ctl_config *config)
{
  int i;
  SceCtrlData p;

  /* Poll the controls */
  if (!sceCtrlPeekBufferPositive(&p, 1))
    return 0;

  /* Clear state */
  config->buttons = 0;

  switch (config->polling_mode)
  {
  case PL_CTL_NORMAL:
    /* Set buttons */
    for (i = 0; monitored_buttons[i]; i++)
      if (p.Buttons & monitored_buttons[i])
        config->buttons |= virtual_buttons[i];
    break;
  case PL_CTL_AUTOREPEAT:
    {
      u64 tick;
      u32 tick_res;

      /* Get current tick count */
      sceRtcGetCurrentTick(&tick);
      tick_res = sceRtcGetTickResolution();

      /* Check each button */
      for (i = 0; monitored_buttons[i]; i++)
      {
        if (p.Buttons & monitored_buttons[i])
        {
          if (!config->push_time[i] || tick >= config->push_time[i])
          {
            /* Button was pushed for the first time, or time to repeat */
            config->buttons |= virtual_buttons[i];
            /* Compute next press time */
            config->push_time[i] = tick + ((config->push_time[i]) 
              ? config->threshold : config->delay) * (tick_res / 1000);
          }
        }
        else
          /* Button was released */
          config->push_time[i] = 0;        
      }
    }
    break;
  default:
    return 0;
  }

  /* Copy analog stick status */
  config->stick_x = p.Lx;
  config->stick_y = p.Ly;

  /* Set the bits based on analog stick status */
  if (p.Ly < 32)
    config->buttons |= PL_CTL_ANALOG_UP;
  else if (p.Ly >= 224)
    config->buttons |= PL_CTL_ANALOG_DOWN;

  if (p.Lx < 32)
    config->buttons |= PL_CTL_ANALOG_LEFT;
  else if (p.Lx >= 224)
    config->buttons |= PL_CTL_ANALOG_RIGHT;

  return 1;
}

