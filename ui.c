/* psplib/ui.c: Simple user interface implementation (legacy version)
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

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspgu.h>
#include <pspnet_adhocmatching.h>

#include "pl_psp.h"
#include "pl_file.h"
#include "ctrl.h"
#include "ui.h"
#include "font.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define UI_ANIM_FRAMES   8
#define UI_ANIM_FOG_STEP 0x0f

#define ADHOC_INITIALIZING  "Initializing Ad-hoc networking. Please wait..."
#define ADHOC_AWAITING_JOIN "Waiting for someone to join..."

#define CONTROL_BUTTON_MASK \
  (PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE | \
   PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT | PSP_CTRL_START)

static const char 
  *AlertDialogButtonTemplate   = "\026\001\020/\026\002\020 Close",
  *ConfirmDialogButtonTemplate = "\026\001\020 Confirm\t\026\002\020 Cancel",
  *YesNoCancelDialogButtonTemplate = 
    "\026\001\020 Yes\t\026"PSP_CHAR_SQUARE"\020 No\t\026\002\020 Cancel",

  *SelectorTemplate = "\026\001\020 Confirm\t\026\002\020 Cancel",

  *BrowserTemplates[] = {
    "\026\002\020 Cancel\t\026\001\020 Open",
    "\026\002\020 Cancel\t\026\001\020 Enter directory",
    "\026\002\020 Cancel\t\026\001\020 Open\t\026"PSP_CHAR_TRIANGLE"\020 Parent directory",
    "\026\002\020 Cancel\t\026\001\020 Enter directory\t\026"PSP_CHAR_TRIANGLE"\020 Parent directory"
   },

  *SplashStatusBarTemplate  = "\026\255\020/\026\256\020 Switch tabs",

  *OptionModeTemplate = 
    "\026\245\020/\026\246\020 Select\t\026\247\020/\026\002\020 Cancel\t\026\250\020/\026\001\020 Confirm";

enum
{
  BrowserTemplateOpenTop  = 0,
  BrowserTemplateEnterTop = 1,
  BrowserTemplateOpen     = 2,
  BrowserTemplateEnter    = 3,
};

void enter_directory(pl_file_path current_dir, 
                     const char *subdir);

#define BROWSER_TEMPLATE_COUNT 4

struct UiPos
{
  int Index;
  int Offset;
  const pl_menu_item *Top;
};

struct AdhocMatchEvent
{
  int NewEvent;
  int EventID;
  PspMAC EventMAC;
  PspMAC CurrentMAC;
  char OptData[512];
};

static struct AdhocMatchEvent _adhoc_match_event;

static void adhocMatchingCallback(int unk1, 
                                  int event, 
                                  unsigned char *mac2, 
                                  int opt_len, 
                                  void *opt_data);

#define ADHOC_PENDING     0
#define ADHOC_WAIT_CLI    1
#define ADHOC_WAIT_HOST   2
#define ADHOC_WAIT_EST    3
#define ADHOC_ESTABLISHED 4
#define ADHOC_EST_AS_CLI  5

#ifndef MATCHING_JOINED
#define MATCHING_JOINED      PSP_ADHOC_MATCHING_EVENT_JOIN
#define MATCHING_DISCONNECT  PSP_ADHOC_MATCHING_EVENT_LEFT
#define MATCHING_CANCELED    PSP_ADHOC_MATCHING_EVENT_CANCEL
#define MATCHING_SELECTED    PSP_ADHOC_MATCHING_EVENT_ACCEPT
#define MATCHING_REJECTED    PSP_ADHOC_MATCHING_EVENT_REJECT
#define MATCHING_ESTABLISHED PSP_ADHOC_MATCHING_EVENT_COMPLETE
#endif

/* TODO: dynamically allocate ?? */
static unsigned int __attribute__((aligned(16))) call_list[524288];//262144];

/* Gets status string - containing current time and battery information */
static void GetStatusString(char *status, int length)
{
  static char main_str[128], batt_str[32];
  pspTime time;

  /* Get current time */
  sceRtcGetCurrentClockLocalTime(&time);

  /* Get the battery/power-related information */
  if (!scePowerIsBatteryExist()) sprintf(batt_str, PSP_CHAR_POWER);
  else
  {
    /* If the battery's online, display charging stats */
    int batt_time = scePowerGetBatteryLifeTime();
    int batt_percent = scePowerGetBatteryLifePercent();
    int i, charging = scePowerIsBatteryCharging();

    static int percentiles[] = { 60, 30, 12, 0 };
    for (i = 0; i < 4; i++)
      if (batt_percent >= percentiles[i])
        break;

    /* Fix for when battery switches state from AC to batt */
    batt_time = (batt_time >= 0) ? batt_time : 0;

    sprintf(batt_str, "%c%3i%% (%02i:%02i)",
      (charging) ? *PSP_CHAR_POWER : *PSP_CHAR_FULL_BATT + i,
      batt_percent, batt_time / 60, batt_time % 60);
  }

  /* Write the rest of the string */
  sprintf(main_str, "\270%2i/%2i %02i%c%02i %s ",
    time.month, time.day, time.hour, (time.microseconds > 500000) ? ':' : ' ',
    time.minutes, batt_str);

  strncpy(status, main_str, length);
  status[length - 1] = '\0';
}

static inline void RenderStatus()
{
  static char status[128];
  GetStatusString(status, sizeof(status));

  int width = pspFontGetTextWidth(UiMetric.Font, status);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH - width, 0, status, PSP_COLOR_WHITE);
}

static void ReplaceIcons(char *string)
{
  char *ch;

  for (ch = string; *ch; ch++)
  {
    switch(*ch)
    {
    case '\001': *ch = pspUiGetButtonIcon(UiMetric.OkButton); break;
    case '\002': *ch = pspUiGetButtonIcon(UiMetric.CancelButton); break;
    }
  }
}

char pspUiGetButtonIcon(u32 button_mask)
{
  switch (button_mask)
  {
  case PSP_CTRL_CROSS:    return *PSP_CHAR_CROSS;
  case PSP_CTRL_CIRCLE:   return *PSP_CHAR_CIRCLE;
  case PSP_CTRL_TRIANGLE: return *PSP_CHAR_TRIANGLE;
  case PSP_CTRL_SQUARE:   return *PSP_CHAR_SQUARE;
  default:                return '?';
  }
}

void pspUiAlert(const char *message)
{
  PspImage *screen = NULL;
  int sx, sy, dx, dy, th, fh, mw, cw, w, h;
  int i, n = UI_ANIM_FRAMES;
  char *instr = strdup(AlertDialogButtonTemplate);
  ReplaceIcons(instr);

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  cw = pspFontGetTextWidth(UiMetric.Font, instr);
  fh = pspFontGetLineHeight(UiMetric.Font);
  th = pspFontGetTextHeight(UiMetric.Font, message);

  w = ((mw > cw) ? mw : cw) + 50;
  h = th + fh * 3;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  /* Intro animation */
  if (UiMetric.Animate)
  {
    /* Get copy of screen */
    screen = pspVideoGetVramBufferCopy();

    for (i = 0; i < n; i++)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
 }

  pspVideoBegin();

  if (UiMetric.Animate)
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
    COLOR(0,0,0,UI_ANIM_FOG_STEP*n));
  pspVideoFillRect(sx, sy, dx, dy, UiMetric.MenuOptionBoxBg);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - mw / 2, sy + fh * 0.5, message, 
    UiMetric.TextColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - cw / 2, dy - fh * 1.5, instr, 
    UiMetric.TextColor);
  pspVideoGlowRect(sx, sy, dx - 1, dy - 1, 
    COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP*n), 2);

  pspVideoEnd();

  /* Swap buffers */
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  SceCtrlData pad;

  /* Loop until X or O is pressed */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.OkButton || pad.Buttons & UiMetric.CancelButton)
      break;
  }
  
  if (!ExitPSP && UiMetric.Animate)
  {
	  /* Exit animation */
	  for (i = n - 1; i >= 0; i--)
	  {
		  pspVideoBegin();

		  /* Clear screen */
		  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

		  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

		  pspVideoEnd();

	    /* Swap buffers */
	    pspVideoWaitVSync();
	    pspVideoSwapBuffers();
		}
	}

  if (screen) pspImageDestroy(screen);
  free(instr);
}

int pspUiYesNoCancel(const char *message)
{
  PspImage *screen = NULL;
  int sx, sy, dx, dy, th, fh, mw, cw, w, h;
  int i, n = UI_ANIM_FRAMES;
  char *instr = strdup(YesNoCancelDialogButtonTemplate);
  ReplaceIcons(instr);

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  cw = pspFontGetTextWidth(UiMetric.Font, instr);
  fh = pspFontGetLineHeight(UiMetric.Font);
  th = pspFontGetTextHeight(UiMetric.Font, message);

  w = ((mw > cw) ? mw : cw) + 50;
  h = th + fh * 3;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  /* Intro animation */
  if (UiMetric.Animate)
  {
    /* Get copy of screen */
    screen = pspVideoGetVramBufferCopy();

    for (i = 0; i < n; i++)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
  }

  pspVideoBegin();

  if (UiMetric.Animate)
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
    COLOR(0,0,0,UI_ANIM_FOG_STEP*n));
  pspVideoFillRect(sx, sy, dx, dy, UiMetric.MenuOptionBoxBg);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - mw / 2, sy + fh * 0.5, message, 
    UiMetric.TextColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - cw / 2, dy - fh * 1.5, instr, 
    UiMetric.TextColor);
  pspVideoGlowRect(sx, sy, dx - 1, dy - 1, 
    COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP*n), 2);

  pspVideoEnd();

  /* Swap buffers */
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  SceCtrlData pad;

  /* Loop until X or O is pressed */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.OkButton || pad.Buttons & UiMetric.CancelButton 
      || pad.Buttons & PSP_CTRL_SQUARE) break;
  }

  if (!ExitPSP && UiMetric.Animate)
  {
	  /* Exit animation */
	  for (i = n - 1; i >= 0; i--)
	  {
		  pspVideoBegin();

		  /* Clear screen */
		  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

		  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

		  pspVideoEnd();

	    /* Swap buffers */
	    pspVideoWaitVSync();
	    pspVideoSwapBuffers();
		}
	}

  if (screen) pspImageDestroy(screen);
  free(instr);

  if (pad.Buttons & UiMetric.CancelButton) return PSP_UI_CANCEL;
  else if (pad.Buttons & PSP_CTRL_SQUARE) return PSP_UI_NO;
  else return PSP_UI_YES;
}

int pspUiConfirm(const char *message)
{
  PspImage *screen = NULL;
  int sx, sy, dx, dy, th, fh, mw, cw, w, h;
  int i, n = UI_ANIM_FRAMES;
  char *instr = strdup(ConfirmDialogButtonTemplate);
  ReplaceIcons(instr);

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  cw = pspFontGetTextWidth(UiMetric.Font, instr);
  fh = pspFontGetLineHeight(UiMetric.Font);
  th = pspFontGetTextHeight(UiMetric.Font, message);

  w = ((mw > cw) ? mw : cw) + 50;
  h = th + fh * 3;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  if (UiMetric.Animate)
  {
    /* Get copy of screen */
    screen = pspVideoGetVramBufferCopy();

    /* Intro animation */
    for (i = 0; i < n; i++)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
  }

  pspVideoBegin();

  if (UiMetric.Animate)
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
    COLOR(0,0,0,UI_ANIM_FOG_STEP*n));
  pspVideoFillRect(sx, sy, dx, dy, UiMetric.MenuOptionBoxBg);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - mw / 2, sy + fh * 0.5, message, 
    UiMetric.TextColor);
  pspVideoPrint(UiMetric.Font, SCR_WIDTH / 2 - cw / 2, dy - fh * 1.5, instr, 
    UiMetric.TextColor);
  pspVideoGlowRect(sx, sy, dx - 1, dy - 1, 
    COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP*n), 2);

  pspVideoEnd();

  /* Swap buffers */
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  SceCtrlData pad;

  /* Loop until X or O is pressed */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.OkButton || pad.Buttons & UiMetric.CancelButton)
      break;
  }

  if (!ExitPSP && UiMetric.Animate)
  {
	  /* Exit animation */
	  for (i = n - 1; i >= 0; i--)
	  {
		  pspVideoBegin();

		  /* Clear screen */
		  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

		  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

		  pspVideoEnd();

	    /* Swap buffers */
	    pspVideoWaitVSync();
	    pspVideoSwapBuffers();
		}
	}

  if (screen) pspImageDestroy(screen);
  free(instr);

  return pad.Buttons & UiMetric.OkButton;
}

void pspUiFlashMessage(const char *message)
{
  PspImage *screen = NULL;
  int sx, sy, dx, dy, fh, mw, mh, w, h;
  int i, n = UI_ANIM_FRAMES;

  mw = pspFontGetTextWidth(UiMetric.Font, message);
  fh = pspFontGetLineHeight(UiMetric.Font);
  mh = pspFontGetTextHeight(UiMetric.Font, message);

  w = mw + 50;
  h = mh + fh * 2;
  sx = SCR_WIDTH / 2 - w / 2;
  sy = SCR_HEIGHT / 2 - h / 2;
  dx = sx + w;
  dy = sy + h;

  if (UiMetric.Animate)
  {
    /* Get copy of screen */
    screen = pspVideoGetVramBufferCopy();

    /* Intro animation */
    for (i = 0; i < n; i++)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0,0,0,UI_ANIM_FOG_STEP*i));
  	  pspVideoFillRect(SCR_WIDTH/2-(((dx-sx)/n)*i)/2, 
  	    SCR_HEIGHT/2-(((dy-sy)/n)*i)/2, 
  	    SCR_WIDTH/2+(((dx-sx)/n)*i)/2, SCR_HEIGHT/2+(((dy-sy)/n)*i)/2,
  	    COLOR(RED_32(UiMetric.MenuOptionBoxBg),
  	      GREEN_32(UiMetric.MenuOptionBoxBg),
  	      BLUE_32(UiMetric.MenuOptionBoxBg),(0xff/n)*i));

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
  }

  pspVideoBegin();

  if (UiMetric.Animate)
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
    COLOR(0,0,0,UI_ANIM_FOG_STEP*n));
  pspVideoFillRect(sx, sy, dx, dy, UiMetric.MenuOptionBoxBg);
  pspVideoPrintCenter(UiMetric.Font,
    sx, sy + fh, dx, message, UiMetric.TextColor);
  pspVideoGlowRect(sx, sy, dx - 1, dy - 1, 
    COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP*n), 2);

  pspVideoEnd();

  /* Swap buffers */
  pspVideoWaitVSync();
  pspVideoSwapBuffers();

  if (screen) pspImageDestroy(screen);
}

void pspUiOpenBrowser(PspUiFileBrowser *browser, const char *start_path)
{
  pl_file *file;
  pl_file_list list;
  const pl_menu_item *sel, *last_sel;
  pl_menu_item *item;
  SceCtrlData pad;
  char *instructions[BROWSER_TEMPLATE_COUNT];
  int delay;
  PspImage *screenshot = NULL;
  int screenshot_width = 0;
  int screenshot_height = 0;

  /* Initialize instruction strings */
  int i;
  for (i = 0; i < BROWSER_TEMPLATE_COUNT; i++)
  {
    instructions[i] = strdup(BrowserTemplates[i]);
    ReplaceIcons(instructions[i]);
  }

  if (!start_path)
    start_path = pl_psp_get_app_directory();

  pl_file_path cur_path;
  if (!pl_file_is_directory(start_path))
    pl_file_get_parent_directory(start_path, cur_path, sizeof(cur_path));
  else
  {
    int copy_len = MIN(strlen(start_path), sizeof(cur_path) - 1);
    strncpy(cur_path, start_path, copy_len);
    cur_path[copy_len] = '\0';
  }

  const char *cur_file = pl_file_get_filename(start_path);
  struct UiPos pos;
  int lnmax, lnhalf;
  int sby, sbh, j, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy;
  int hasparent, is_dir;

  sx = UiMetric.Left;
  sy = UiMetric.Top + fh + UiMetric.TitlePadding;
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = dx - sx - UiMetric.ScrollbarWidth;
  h = dy - sy;

  pl_menu menu;
  pl_menu_create(&menu, NULL);

  memset(call_list, 0, sizeof(call_list));

  int sel_top = 0, last_sel_top = 0, fast_scroll;

  /* Begin browsing (outer) loop */
  while (!ExitPSP)
  {
    delay = UiMetric.BrowserScreenshotDelay;
    sel = last_sel = NULL;
    pos.Top = NULL;
    pl_menu_clear_items(&menu);

    /* Load list of files for the selected path */
    if (pl_file_get_file_list(&list, cur_path, browser->Filter) >= 0)
    {
      /* Check for a parent path, prepend .. if necessary */
      if ((hasparent =! pl_file_is_root_directory(cur_path)))
      {
        item = pl_menu_append_item(&menu, 0, "..");
        item->param = (void*)PL_FILE_DIRECTORY;
      }

      /* Add a menu item for each file */
      for (file = list.files; file; file = file->next)
      {
        /* Skip files that begin with '.' */
        if (file->name && file->name[0] == '.')
          continue;

        item = pl_menu_append_item(&menu, 0, file->name);
        item->param = (void*)(int)file->attrs;

        if (cur_file && strcmp(file->name, cur_file) == 0)
          sel = item;
      }

      cur_file = NULL;

      /* Destroy the file list */
      pl_file_destroy_file_list(&list);
    }
    else
    {
      /* Check for a parent path, prepend .. if necessary */
      if ((hasparent = !pl_file_is_root_directory(cur_path)))
      {
        item = pl_menu_append_item(&menu, 0, "..");
        item->param = (void*)PL_FILE_DIRECTORY;
      }
    }

    /* Initialize variables */
    lnmax = (dy - sy) / fh;
    lnhalf = lnmax >> 1;
    int item_count = pl_menu_get_item_count(&menu);
    sbh = (item_count > lnmax) 
          ? (int)((float)h * ((float)lnmax / (float)item_count)) : 0;

    pos.Index = pos.Offset = 0;

    if (!sel) 
    { 
      /* Select the first file/dir in the directory */
      if (menu.items && menu.items->next)
        sel=menu.items->next;
      else if (menu.items)
        sel=menu.items;
    }

    /* Compute index and offset of selected file */
    if (sel)
    {
      pos.Top = menu.items;
      for (item = menu.items; item != sel; item = item->next)
      {
        if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->next; } 
        else pos.Index++;
      }
    }

    pspVideoWaitVSync();

    /* Begin navigation (inner) loop */
    while (!ExitPSP)
    {
      if (!pspCtrlPollControls(&pad))
        continue;

      fast_scroll = 0;
      if (delay >= 0) delay--;
      if ((delay == 0)
        && sel
        && !screenshot
        && !((unsigned int)sel->param & PL_FILE_DIRECTORY)
        && UiMetric.BrowserScreenshotPath)
      {
        pl_file_path screenshot_path;
        sprintf(screenshot_path, "%s%s-00.png",
          UiMetric.BrowserScreenshotPath, sel->caption);
        screenshot = pspImageLoadPng(screenshot_path);
      }

      /* Check the directional buttons */
      if (sel)
      {
        if ((pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN) && sel->next)
        {
          if (pos.Index+1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->next; } 
          else pos.Index++;
          sel=sel->next;
          fast_scroll = pad.Buttons & PSP_CTRL_ANALDOWN;
        }
        else if ((pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP) && sel->prev)
        {
          if (pos.Index - 1 < 0) { pos.Offset--; pos.Top=pos.Top->prev; }
          else pos.Index--;
          sel = sel->prev;
          fast_scroll = pad.Buttons & PSP_CTRL_ANALUP;
        }
        else if (pad.Buttons & PSP_CTRL_LEFT)
        {
          for (i=0; sel->prev && i < lnhalf; i++)
          {
            if (pos.Index-1 < 0) { pos.Offset--; pos.Top=pos.Top->prev; }
            else pos.Index--;
            sel=sel->prev;
          }
        }
        else if (pad.Buttons & PSP_CTRL_RIGHT)
        {
          for (i=0; sel->next && i < lnhalf; i++)
          {
            if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top=pos.Top->next; }
            else pos.Index++;
            sel=sel->next;
          }
        }

        /* File/dir selection */
        if (pad.Buttons & UiMetric.OkButton)
        {
          if (((unsigned int)sel->param & PL_FILE_DIRECTORY))
          {
            enter_directory(cur_path, sel->caption);
            break;
          }
          else
          {
            int exit = 1;

            /* Selected a file */
            if (browser->OnOk)
            {
              char *file = malloc((strlen(cur_path) + strlen(sel->caption) + 1) * sizeof(char));
              sprintf(file, "%s%s", cur_path, sel->caption);
              exit = browser->OnOk(browser, file);
              free(file);
            }

            if (exit) goto exit_browser;
            else continue;
          }
        }
      }

      if (pad.Buttons & PSP_CTRL_TRIANGLE)
      {
        if (!pl_file_is_root_directory(cur_path))
        {
          enter_directory(cur_path, "..");
          break;
        }
      }
      else if (pad.Buttons & UiMetric.CancelButton)
      {
        if (browser->OnCancel)
          browser->OnCancel(browser, cur_path);
        goto exit_browser;
      }
      else if ((pad.Buttons & CONTROL_BUTTON_MASK) && browser->OnButtonPress)
      {
        char *file = NULL;
        int exit;

        if (sel)
        {
          file = malloc((strlen(cur_path) + strlen(sel->caption) + 1) * sizeof(char));
          sprintf(file, "%s%s", cur_path, sel->caption);
        }

        exit = browser->OnButtonPress(browser, 
          file, pad.Buttons & CONTROL_BUTTON_MASK);

        if (file) free(file);
        if (exit) goto exit_browser;
      }

      is_dir = (unsigned int)sel->param & PL_FILE_DIRECTORY;

      sceGuStart(GU_CALL, call_list);

      /* Draw current path */
      pspVideoPrint(UiMetric.Font, sx, UiMetric.Top, cur_path,
        UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);

      const char *instruction;
      if (hasparent)
        instruction = instructions[(is_dir)
          ? BrowserTemplateEnter : BrowserTemplateOpen];
      else
        instruction = instructions[(is_dir)
          ? BrowserTemplateEnterTop : BrowserTemplateOpenTop];

      pspVideoPrintCenter(UiMetric.Font,
        sx, SCR_HEIGHT - fh, dx, instruction, UiMetric.StatusBarColor);

      /* Draw scrollbar */
      if (sbh > 0)
      {
        sby = sy + (int)((float)(h - sbh) 
          * ((float)(pos.Offset + pos.Index) / (float)item_count));
        pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sy, dx, dy, 
          UiMetric.ScrollbarBgColor);
        pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sby, dx, sby + sbh, 
          UiMetric.ScrollbarColor);
      }

      /* Render the files */
      for (item = (pl_menu_item*)pos.Top, i = 0, j = sy; 
        item && i < lnmax; item = item->next, j += fh, i++)
      {
        if (item == sel) sel_top = j;

        pspVideoPrintClipped(UiMetric.Font, sx + 10, j, item->caption, w - 10, 
          "...", (item == sel) ? UiMetric.SelectedColor
            : ((unsigned int)item->param & PL_FILE_DIRECTORY)
            ? UiMetric.BrowserDirectoryColor : UiMetric.BrowserFileColor);
     }

      /* Render status information */
      RenderStatus();

      /* Perform any custom drawing */
      if (browser->OnRender)
        browser->OnRender(browser, "not implemented");

      sceGuFinish();

      if (screenshot)
      {
        screenshot_width = screenshot->Viewport.Width;
        screenshot_height = screenshot->Viewport.Height;
      }

      if (sel != last_sel && !fast_scroll && sel && last_sel
        && UiMetric.Animate)
      {
        /* Move animation */
        int f, n = 4;
        for (f = 1; f <= n; f++)
        {
          pspVideoBegin();

          /* Clear screen */
          if (!UiMetric.Background) pspVideoClearScreen();
          else pspVideoPutImage(UiMetric.Background, 0, 0, 
            UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

          /* Render screenshot */
          if (screenshot)
            pspVideoPutImageAlpha(screenshot,
                            UiMetric.Right - screenshot_width - UiMetric.ScrollbarWidth,
                            ((UiMetric.Bottom - UiMetric.Top) / 2 - 
                            screenshot_height / 2) + UiMetric.Top,
                            screenshot_width,
                            screenshot_height,
                            0xaa);

          /* Selection box */
          int box_top = last_sel_top-((last_sel_top-sel_top)/n)*f;
          pspVideoFillRect(sx, box_top, sx+w, box_top+fh,
            UiMetric.SelectedBgColor);

          sceGuCallList(call_list);

          pspVideoEnd();

          pspVideoWaitVSync();
          pspVideoSwapBuffers();
        }
      }

      pspVideoBegin();

      /* Clear screen */
      if (UiMetric.Background) 
        pspVideoPutImage(UiMetric.Background, 0, 0, 
          UiMetric.Background->Viewport.Width, UiMetric.Background->Height);
      else pspVideoClearScreen();

      /* Render screenshot */
      if (screenshot)
        pspVideoPutImageAlpha(screenshot,
                         UiMetric.Right - screenshot_width - UiMetric.ScrollbarWidth,
                         ((UiMetric.Bottom - UiMetric.Top) / 2 - 
                         screenshot_height / 2) + UiMetric.Top,
                         screenshot_width,
                         screenshot_height,
                         0xaa);

      /* Render selection box */
      if (sel) pspVideoFillRect(sx, sel_top, sx+w, sel_top+fh,
        UiMetric.SelectedBgColor);

      sceGuCallList(call_list);

      pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();

      if (last_sel != sel)
      {
        if (screenshot != NULL)
        {
          pspImageDestroy(screenshot);
          screenshot = NULL;
        }

        delay = UiMetric.BrowserScreenshotDelay;
      }

      last_sel = sel;
      last_sel_top = sel_top;
    }
  }

exit_browser:

  if (screenshot != NULL)
    pspImageDestroy(screenshot);

  /* Free instruction strings */
  for (i = 0; i < BROWSER_TEMPLATE_COUNT; i++)
    free(instructions[i]);

  pl_menu_destroy(&menu);
}

void pspUiOpenGallery(PspUiGallery *gallery, const char *title)
{
  pl_menu *menu = &(gallery->Menu);
  const pl_menu_item *top, *item;
  SceCtrlData pad;
  pl_menu_item *sel = menu->selected;

  int sx, sy, dx, dy,
    orig_w = 272, orig_h = 228, // defaults
    fh, c, i, j,
    sbh, sby,
    w, h, 
    icon_w, icon_h, 
    grid_w, grid_h,
    icon_idx, icon_off,
    rows, vis_v, vis_s,
    icons;
  const pl_menu_item *last_sel = NULL;

  /* Find first icon and save its width/height */
  for (item = menu->items; item; item = item->next)
  {
    if (item->param)
    {
      orig_w = ((PspImage*)item->param)->Viewport.Width;
      orig_h = ((PspImage*)item->param)->Height;
      break;
    }
  }

  fh = pspFontGetLineHeight(UiMetric.Font);
  sx = UiMetric.Left;
  sy = UiMetric.Top + ((title) ? fh + UiMetric.TitlePadding : 0);
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = (dx - sx) - UiMetric.ScrollbarWidth; // visible width
  h = dy - sy; // visible height
  icon_w = (w - UiMetric.GalleryIconMarginWidth 
    * (UiMetric.GalleryIconsPerRow - 1)) / UiMetric.GalleryIconsPerRow; // icon width
  icon_h = (int)((float)icon_w 
    / ((float)orig_w / (float)orig_h)); // icon height
  grid_w = icon_w + UiMetric.GalleryIconMarginWidth; // width of the grid
  grid_h = icon_h + (fh * 2); // half-space for margin + 1 line of text 
  icons = pl_menu_get_item_count(menu); // number of icons total
  rows = ceil((float)icons / (float)UiMetric.GalleryIconsPerRow); // number of rows total
  vis_v = h / grid_h; // number of rows visible at any time
  vis_s = UiMetric.GalleryIconsPerRow * vis_v; // max. number of icons visible on screen at any time
  int max_w = ((float)icon_w * 1.5); /* Maximized width  */
  int max_h = ((float)icon_h * 1.5); /* Maximized height */

  icon_idx = 0;
  icon_off = 0;
  top = menu->items;

  if (!sel)
  {
    /* Select the first icon */
    sel = menu->items;
  }
  else
  {
    /* Find the selected icon */
    for (item = menu->items; item; item = item->next)
    {
      if (item == sel) 
        break;

      if (++icon_idx >= vis_s)
      {
        icon_idx=0;
        icon_off += vis_s;
        top = item;
      }
    }
    
    if (item != sel)
    {
      /* Icon not found; reset to first icon */
      sel = menu->items;
      top = menu->items;
      icon_idx = 0;
      icon_off = 0;
    }
  }

  /* Compute height of scrollbar */
  sbh = ((float)vis_v / (float)(rows + (rows % vis_v))) * (float)h;

  /* Compute update frequency */
  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  memset(call_list, 0, sizeof(call_list));
  int sel_left = 0 /*, max_left = 0 */;
  int sel_top = 0 /*, max_top = 0 */;

  pspVideoWaitVSync();

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    /* Check the directional buttons */
    if (sel)
    {
      if (pad.Buttons & PSP_CTRL_RIGHT && sel->next)
      {
        sel = sel->next;
        if (++icon_idx >= vis_s)
        {
          icon_idx = 0;
          icon_off += vis_s;
          top = sel;
        }
      }
      else if (pad.Buttons & PSP_CTRL_LEFT && sel->prev)
      {
        sel = sel->prev;
        if (--icon_idx < 0)
        {
          icon_idx = vis_s-1;
          icon_off -= vis_s;
          for (i = 0; i < vis_s && top; i++) top = top->prev;
        }
      }
      else if (pad.Buttons & PSP_CTRL_DOWN)
      {
        for (i = 0; sel->next && i < UiMetric.GalleryIconsPerRow; i++)
        {
          sel = sel->next;
          if (++icon_idx >= vis_s)
          {
            icon_idx = 0;
            icon_off += vis_s;
            top = sel;
          }
        }
      }
      else if (pad.Buttons & PSP_CTRL_UP)
      {
        for (i = 0; sel->prev && i < UiMetric.GalleryIconsPerRow; i++)
        {
          sel = sel->prev;
          if (--icon_idx < 0)
          {
            icon_idx = vis_s-1;
            icon_off -= vis_s;
            for (j = 0; j < vis_s && top; j++) top = top->prev;
          }
        }
      }

      if (pad.Buttons & UiMetric.OkButton)
      {
        pad.Buttons &= ~UiMetric.OkButton;
        if (!gallery->OnOk || gallery->OnOk(gallery, sel))
          break;
      }
    }

    if (pad.Buttons & UiMetric.CancelButton)
    {
      pad.Buttons &= ~UiMetric.CancelButton;
      if (gallery->OnCancel)
        gallery->OnCancel(gallery, sel);
      break;
    }

    if ((pad.Buttons & CONTROL_BUTTON_MASK) && gallery->OnButtonPress)
      if (gallery->OnButtonPress(gallery, sel, pad.Buttons & CONTROL_BUTTON_MASK))
          break;

    if (last_sel != sel && last_sel && sel && sel->param && UiMetric.Animate)
    {
      /* "Implode" animation */
      int f = 1, n = 2;
//      for (f = n - 1; f > 0; f--)
//      {
        pspVideoBegin();

        /* Clear screen */
        if (!UiMetric.Background) pspVideoClearScreen();
        else pspVideoPutImage(UiMetric.Background, 0, 0, 
          UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

        sceGuCallList(call_list); 

        pspVideoEnd();

        /* Render the menu items */
        for (i = sy, item = top; item && i + grid_h < dy; i += grid_h)
          for (j = sx, c = 0; item && c < UiMetric.GalleryIconsPerRow; j += grid_w, c++, item = item->next)
            if (item->param && item != last_sel)
            {
              pspVideoBegin();
              pspVideoPutImage((PspImage*)item->param, j, i, icon_w, icon_h);
              pspVideoEnd();
            }

        pspVideoBegin();

        pspVideoPutImage((PspImage*)last_sel->param, 
          sel_left-(icon_w+((max_w-icon_w)/n)*f)/2, 
          sel_top-(icon_h+((max_h-icon_h)/n)*f)/2, 
          icon_w+((max_w-icon_w)/n)*f, 
          icon_h+((max_h-icon_h)/n)*f);

        pspVideoEnd();

        /* Swap buffers */
        pspVideoWaitVSync();
        pspVideoSwapBuffers();
//      }
    }

    sceGuStart(GU_CALL, call_list);

    /* Draw title */
    if (title)
    {
      pspVideoPrint(UiMetric.Font, UiMetric.Left, UiMetric.Top, 
        title, UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);
    }

    /* Draw scrollbar */
    if (sbh < h)
    {
      sby = sy + (((float)icon_off / (float)UiMetric.GalleryIconsPerRow) 
        / (float)(rows + (rows % vis_v))) * (float)h;
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, 
        sy, dx, dy, UiMetric.ScrollbarBgColor);
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, 
        sby, dx, sby+sbh, UiMetric.ScrollbarColor);
    }

    /* Draw instructions */
    if (sel && sel->help_text)
    {
      static char help_copy[PL_FILE_MAX_PATH_LEN];
      strncpy(help_copy, sel->help_text, PL_FILE_MAX_PATH_LEN - 1);
      help_copy[PL_FILE_MAX_PATH_LEN - 1] = '\0';
      ReplaceIcons(help_copy);

      pspVideoPrintCenter(UiMetric.Font, 
        0, SCR_HEIGHT - fh, SCR_WIDTH, help_copy, UiMetric.StatusBarColor);
    }

    /* Render non-image components of each item */
    for (i = sy, item = top; item && i + grid_h < dy; i += grid_h)
    {
      for (j = sx, c = 0; item && c < UiMetric.GalleryIconsPerRow; j += grid_w, c++, item = item->next)
      {
        if (item != sel)
        {
          pspVideoShadowRect(j - 1, i - 1, j + icon_w, i + icon_h, PSP_COLOR_BLACK, 3);
          pspVideoDrawRect(j - 1, i - 1, j + icon_w, i + icon_h, UiMetric.TextColor);

          if (item->caption)
          {
            int cap_pos = j + icon_w / 2 
              - pspFontGetTextWidth(UiMetric.Font, item->caption) / 2;
            pspVideoPrint(UiMetric.Font, cap_pos, 
              i + icon_h + (fh / 2), item->caption, UiMetric.TextColor);
          }
        }
        else
        {
          sel_left = j + icon_w / 2;
          sel_top = i + icon_h / 2;

          sel_left = (sel_left-max_w/2 < sx) ? sx+max_w/2 : sel_left;
          sel_top = (sel_top-max_h/2 < UiMetric.Top) 
            ? UiMetric.Top+max_h/2 : sel_top;
          sel_left = (sel_left+max_w/2 > dx) ? dx-max_w/2 : sel_left;
          sel_top = (sel_top+max_h/2 > dy) ? dy-max_h/2 : sel_top;
        }
      }
    }

    /* Render status information */
    RenderStatus();

    /* Perform any custom drawing */
    if (gallery->OnRender)
      gallery->OnRender(gallery, sel);

    sceGuFinish();

    if (last_sel != sel && last_sel && sel && sel->param && UiMetric.Animate)
    {
      /* Popup animation */
      int f = 1, n = 2;
//      for (f = 1; f < n; f++)
//      {
        pspVideoBegin();

        /* Clear screen */
        if (!UiMetric.Background) pspVideoClearScreen();
        else pspVideoPutImage(UiMetric.Background, 0, 0, 
          UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

        sceGuCallList(call_list); 

        pspVideoEnd();

        /* Render the menu items */
        for (i = sy, item = top; item && i + grid_h < dy; i += grid_h)
          for (j = sx, c = 0; item && c < UiMetric.GalleryIconsPerRow; j += grid_w, c++, item = item->next)
            if (item->param && item != sel)
            {
              pspVideoBegin();
              pspVideoPutImage((PspImage*)item->param, j, i, icon_w, icon_h);
              pspVideoEnd();
            }

        pspVideoBegin();

        pspVideoPutImage((PspImage*)sel->param, 
          sel_left-(icon_w+((max_w-icon_w)/n)*f)/2, 
          sel_top-(icon_h+((max_h-icon_h)/n)*f)/2, 
          icon_w+((max_w-icon_w)/n)*f, 
          icon_h+((max_h-icon_h)/n)*f);

        pspVideoEnd();

        /* Swap buffers */
        pspVideoWaitVSync();
        pspVideoSwapBuffers();
//      }
    }

    pspVideoBegin();

    /* Clear screen */
    if (!UiMetric.Background) pspVideoClearScreen();
    else pspVideoPutImage(UiMetric.Background, 0, 0, 
      UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

    sceGuCallList(call_list); 

    pspVideoEnd();

    /* Render the menu items */
    for (i = sy, item = top; item && i + grid_h < dy; i += grid_h)
      for (j = sx, c = 0; item && c < UiMetric.GalleryIconsPerRow; j += grid_w, c++, item = item->next)
        if (item->param && item != sel)
        {
          pspVideoBegin();
          pspVideoPutImage((PspImage*)item->param, j, i, icon_w, icon_h);
          pspVideoEnd();
        }

    pspVideoBegin();

    if (sel && sel->param)
    {
      pspVideoPutImage((PspImage*)sel->param, sel_left-max_w/2, sel_top-max_h/2,
        max_w, max_h);
      pspVideoGlowRect(sel_left-max_w/2, sel_top-max_h/2,
        sel_left+max_w/2 - 1, sel_top+max_h/2 - 1,
        COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP * UI_ANIM_FRAMES), 2);
    }

    if (sel && sel->caption)
    {
      int cap_left = sel_left
        - pspFontGetTextWidth(UiMetric.Font, sel->caption) / 2;
      pspVideoPrint(UiMetric.Font, cap_left, 
        sel_top + max_h/2 - (fh + (fh - UiMetric.Font->Ascent)), sel->caption, 
        UiMetric.TextColor);
    }

    pspVideoEnd();

    last_sel = sel;

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
  }

  menu->selected = sel;
}

void pspUiOpenMenu(PspUiMenu *uimenu, const char *title)
{
  struct UiPos pos;
  pl_menu *menu = &(uimenu->Menu);
  const pl_menu_item *item;
  SceCtrlData pad;
  const pl_menu_option *temp_option;
  int lnmax;
  int sby, sbh, i, j, k, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy, sel_top = 0, last_sel_top = 0;
  int max_item_w = 0, item_w;
  int option_mode, max_option_w = 0;
  int arrow_w = pspFontGetTextWidth(UiMetric.Font, "\272");
  int anim_frame = 0, anim_incr = 1;
  pl_menu_item *sel = menu->selected, *last_sel = NULL;

  sx = UiMetric.Left;
  sy = UiMetric.Top + ((title) ? (fh + UiMetric.TitlePadding) : 0);
  dx = UiMetric.Right;
  dy = UiMetric.Bottom;
  w = dx - sx - UiMetric.ScrollbarWidth;
  h = dy - sy;

  memset(call_list, 0, sizeof(call_list));

  /* Determine width of the longest caption */
  for (item = menu->items; item; item = item->next)
  {
    if (item->caption)
    {
      item_w = pspFontGetTextWidth(UiMetric.Font, item->caption);
      if (item_w > max_item_w) 
        max_item_w = item_w;
    }
  }

  /* Initialize variables */
  lnmax = (dy - sy) / fh;
  int item_count = pl_menu_get_item_count(menu);
  sbh = (item_count > lnmax) 
        ? (int)((float)h * ((float)lnmax / (float)item_count)) : 0;

  pos.Index = 0;
  pos.Offset = 0;
  pos.Top = NULL;
  option_mode = 0;
  temp_option = NULL;

  int cur_x=0, min_x=0, max_x=0;
  int cur_y=0, min_y=0, max_y=0;

  /* Find first selectable item */
  if (!sel)
  {
    for (sel = menu->items; sel; sel = sel->next)
      if (sel->caption && sel->caption[0] != '\t')
        break;
  }

  /* Compute index and offset of selected file */
  pos.Top = menu->items;
  for (item = menu->items; item != sel; item = item->next)
  {
    if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; } 
    else pos.Index++;
  }

  pspVideoWaitVSync();
  pl_menu_item *last;
  struct UiPos last_valid;

  /* Compute update frequency */
  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  int fast_scroll;

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    fast_scroll = 0;
    anim_frame += (UiMetric.Animate) ? anim_incr : 0;
    if (anim_frame > 2 || anim_frame < 0) 
      anim_incr *= -1;

    /* Check the directional buttons */
    if (sel)
    {
      if (pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALDOWN;

        if (option_mode)
        {
          if (temp_option->next)
            temp_option = temp_option->next;
        }
        else
        {
          if (sel->next)
          {
            last = sel;
            last_valid = pos;

            for (;;)
            {
              if (pos.Index + 1 >= lnmax)
              {
                pos.Offset++;
                pos.Top = pos.Top->next;
              }
              else pos.Index++;

              sel = sel->next;

              if (!sel)
              {
                sel = last;
                pos = last_valid;
                break;
              }

              if (sel->caption && sel->caption[0] != '\t')
                break;
            }
          }
        }
      }
      else if (pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALUP;

        if (option_mode)
        {
          if (temp_option->prev)
            temp_option = temp_option->prev;
        }
        else
        {
          if (sel->prev)
          {
            last = sel;
            last_valid = pos;

            for (;;)
            {
              if (pos.Index - 1 < 0)
              {
                pos.Offset--;
                pos.Top = pos.Top->prev;
              }
              else pos.Index--;

              sel = sel->prev;

              if (!sel)
              {
                sel = last;

                pos.Index = 0;
                pos.Offset = 0;
                pos.Top = menu->items;

                for (item = menu->items; item != sel; item = item->next)
                {
                  if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; } 
                  else pos.Index++;
                }

                break;
              }

              if (sel->caption && sel->caption[0] != '\t')
                break;
            }
          }
        }
      }

      /* Recompute box bounds if scrolling in option mode */
      if (option_mode && (pad.Buttons &
        (PSP_CTRL_UP|PSP_CTRL_ANALUP|PSP_CTRL_DOWN|PSP_CTRL_ANALDOWN)))
      {
        cur_x = sx + max_item_w + UiMetric.MenuItemMargin + 10;
        min_y = sy + pos.Index * fh;
        cur_y = min_y + fh / 2;
        max_y = sy + (pos.Index  + 1) * fh;
        min_x = cur_x - UiMetric.MenuItemMargin;
        max_x = cur_x + max_option_w + UiMetric.MenuItemMargin;
        cur_x += pspFontGetTextWidth(UiMetric.Font, " >");
        if (sel->selected && sel->selected->text)
          cur_x += pspFontGetTextWidth(UiMetric.Font, sel->selected->text);

        const pl_menu_option *option;
        for (option = temp_option; option && min_y >= sy; option = option->prev, min_y -= fh);
        for (option = temp_option->next; option && max_y < dy; option = option->next, max_y += fh);
        max_y += fh;
      }

      if (option_mode)
      {
        if (pad.Buttons & PSP_CTRL_RIGHT || pad.Buttons & UiMetric.OkButton)
        {
          option_mode = 0;

          /* If the callback function refuses the change, restore selection */
          if (!uimenu->OnItemChanged || uimenu->OnItemChanged(uimenu, sel, temp_option)) 
            sel->selected = (pl_menu_option*)temp_option;
        }
        else if (pad.Buttons & PSP_CTRL_LEFT  || pad.Buttons & UiMetric.CancelButton)
        {
          option_mode = 0;

          if (pad.Buttons & UiMetric.CancelButton)
            pad.Buttons &= ~UiMetric.CancelButton;
        }

        if (!option_mode)
        {
          if (UiMetric.Animate)
          {
            /* Deflation animation */
            for (i = UI_ANIM_FRAMES - 1; i >= 0; i--)
            {
          	  pspVideoBegin();
              if (!UiMetric.Background) pspVideoClearScreen();
                else pspVideoPutImage(UiMetric.Background, 0, 0, 
                  UiMetric.Background->Viewport.Width, UiMetric.Background->Height);
                
          	  pspVideoCallList(call_list);

              /* Perform any custom drawing */
              if (uimenu->OnRender)
                uimenu->OnRender(uimenu, sel);

          	  /* Clear screen */
          	  pspVideoFillRect(cur_x - ((cur_x - min_x) / UI_ANIM_FRAMES) * i,
          	    cur_y - ((cur_y - min_y) / UI_ANIM_FRAMES) * i, 
          	    cur_x + ((max_x - cur_x) / UI_ANIM_FRAMES) * i, 
          	    cur_y + ((max_y - cur_y) / UI_ANIM_FRAMES) * i,
                UiMetric.MenuOptionBoxBg);

              /* Selected option for the item */
              if (sel->selected && sel->selected->text)
              pspVideoPrint(UiMetric.Font, 
                sx + max_item_w + UiMetric.MenuItemMargin + 10, 
                sy + pos.Index * fh, sel->selected->text, UiMetric.SelectedColor);

          	  pspVideoEnd();

              /* Swap buffers */
              pspVideoWaitVSync();
              pspVideoSwapBuffers();
          	}
          }
        }
      }
      else
      {
        if ((pad.Buttons & PSP_CTRL_RIGHT) 
          && sel->options && sel->options->next)
        {
          option_mode = 1;
          max_option_w = 0;
          int width;
          const pl_menu_option *option;

          /* Find the longest option caption */
          for (option = sel->options; option; option = option->next)
            if (option->text && (width = pspFontGetTextWidth(UiMetric.Font, option->text)) > max_option_w) 
              max_option_w = width;

          temp_option = (sel->selected) ? sel->selected : sel->options;

          /* Determine bounds */
          cur_x = sx + max_item_w + UiMetric.MenuItemMargin + 10;
          min_y = sy + pos.Index * fh;
          cur_y = min_y + fh / 2;
          max_y = sy + (pos.Index  + 1) * fh;
          min_x = cur_x - UiMetric.MenuItemMargin;
          max_x = cur_x + max_option_w + UiMetric.MenuItemMargin;
          cur_x += pspFontGetTextWidth(UiMetric.Font, " >");
          if (sel->selected && sel->selected->text)
            cur_x += pspFontGetTextWidth(UiMetric.Font, sel->selected->text);

          for (option = temp_option; option && min_y >= sy; option = option->prev, min_y -= fh);
          for (option = temp_option->next; option && max_y < dy; option = option->next, max_y += fh);
          max_y += fh;

          if (UiMetric.Animate)
          {
            /* Expansion animation */
            for (i = 0; i <= UI_ANIM_FRAMES; i++)
            {
          	  pspVideoBegin();

              if (!UiMetric.Background) pspVideoClearScreen();
                else pspVideoPutImage(UiMetric.Background, 0, 0, 
                  UiMetric.Background->Viewport.Width, 
                  UiMetric.Background->Height);

          	  pspVideoCallList(call_list);

              /* Perform any custom drawing */
              if (uimenu->OnRender)
                uimenu->OnRender(uimenu, sel);

          	  pspVideoFillRect(cur_x - ((cur_x - min_x) / UI_ANIM_FRAMES) * i,
          	    cur_y - ((cur_y - min_y) / UI_ANIM_FRAMES) * i, 
          	    cur_x + ((max_x - cur_x) / UI_ANIM_FRAMES) * i, 
          	    cur_y + ((max_y - cur_y) / UI_ANIM_FRAMES) * i,
                UiMetric.MenuOptionBoxBg);

          	  pspVideoEnd();

              /* Swap buffers */
              pspVideoWaitVSync();
              pspVideoSwapBuffers();
          	}
        	}
        }
        else if (pad.Buttons & UiMetric.OkButton)
        {
          if (!uimenu->OnOk || uimenu->OnOk(uimenu, sel))
            break;
        }
      }
    }

    if (!option_mode)
    {
      if (pad.Buttons & UiMetric.CancelButton)
      {
        if (uimenu->OnCancel) 
          uimenu->OnCancel(uimenu, sel);
        break;
      }

      if ((pad.Buttons & CONTROL_BUTTON_MASK) && uimenu->OnButtonPress)
      {
        if (uimenu->OnButtonPress(uimenu, sel, pad.Buttons & CONTROL_BUTTON_MASK))
            break;
      }
    }

    /* Render to a call list */
    sceGuStart(GU_CALL, call_list);

    /* Draw instructions */
    if (sel)
    {
      const char *dirs = NULL;

      if (!option_mode && sel->help_text)
      {
        static char help_copy[PL_FILE_MAX_PATH_LEN];
        strncpy(help_copy, sel->help_text, PL_FILE_MAX_PATH_LEN - 1);
        help_copy[PL_FILE_MAX_PATH_LEN - 1] = '\0';
        ReplaceIcons(help_copy);

        dirs = help_copy;
      }
      else if (option_mode)
      {
        static char help_copy[PL_FILE_MAX_PATH_LEN];
        strncpy(help_copy, OptionModeTemplate, PL_FILE_MAX_PATH_LEN - 1);
        help_copy[PL_FILE_MAX_PATH_LEN - 1] = '\0';
        ReplaceIcons(help_copy);

        dirs = help_copy;
      }

      if (dirs) 
        pspVideoPrintCenter(UiMetric.Font, 
          0, SCR_HEIGHT - fh, SCR_WIDTH, dirs, UiMetric.StatusBarColor);
    }

    /* Draw title */
    if (title)
    {
      pspVideoPrint(UiMetric.Font, UiMetric.Left, UiMetric.Top, 
        title, UiMetric.TitleColor);
      pspVideoDrawLine(UiMetric.Left, UiMetric.Top + fh - 1, UiMetric.Left + w, 
        UiMetric.Top + fh - 1, UiMetric.TitleColor);
    }

    /* Render the menu items */
    for (item = pos.Top, i = 0, j = sy; item && i < lnmax; item = item->next, j += fh, i++)
    {
      if (item->caption)
      {
  	    /* Section header */
  	    if (item->caption[0] == '\t')
    		{
    		  // if (i != 0) j += fh / 2;
          pspVideoPrint(UiMetric.Font, sx, j, item->caption + 1, UiMetric.TitleColor);
          pspVideoDrawLine(sx, j + fh - 1, sx + w, j + fh - 1, UiMetric.TitleColor);
    		  continue;
    		}

        if (item == sel) sel_top = j;

        /* Item caption */
        pspVideoPrint(UiMetric.Font, sx + 10, j, item->caption, 
          (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);

        if (!option_mode || item != sel)
        {
          /* Selected option for the item */
          if (item->selected)
          {
            k = sx + max_item_w + UiMetric.MenuItemMargin + 10;
            k += pspVideoPrint(UiMetric.Font, k, j, item->selected->text, 
              (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);

            if (!option_mode && item == sel)
              if (sel->options && sel->options->next)
                pspVideoPrint(UiMetric.Font, k + anim_frame, j, " >", UiMetric.MenuDecorColor);
          }
        }
      }
    }

    /* Render status information */
    RenderStatus();

    /* Draw scrollbar */
    if (sbh > 0)
    {
      sby = sy + (int)((float)(h - sbh) * ((float)(pos.Offset + pos.Index) / (float)item_count));
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sy, dx, dy, UiMetric.ScrollbarBgColor);
      pspVideoFillRect(dx - UiMetric.ScrollbarWidth, sby, dx, sby + sbh, UiMetric.ScrollbarColor);
    }

    /* End writing to call list */
    sceGuFinish();

    if (!option_mode && !fast_scroll && sel && last_sel
      && UiMetric.Animate && last_sel != sel)
    {
      /* Move animation */
      int f, n = 4;
      for (f = 1; f <= n; f++)
      {
        pspVideoBegin();

        /* Clear screen */
        if (!UiMetric.Background) pspVideoClearScreen();
        else pspVideoPutImage(UiMetric.Background, 0, 0, 
          UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

        int box_top = last_sel_top-((last_sel_top-sel_top)/n)*f;
        pspVideoFillRect(sx, box_top, sx+w, box_top+fh, 
          UiMetric.SelectedBgColor);

        sceGuCallList(call_list);

        /* Perform any custom drawing */
        if (uimenu->OnRender)
          uimenu->OnRender(uimenu, sel);

        pspVideoEnd();

        /* Swap buffers */
        pspVideoWaitVSync();
        pspVideoSwapBuffers();
      }
    }

    /* Begin direct rendering */
    pspVideoBegin();

    /* Clear screen */
    if (!UiMetric.Background) pspVideoClearScreen();
    else pspVideoPutImage(UiMetric.Background, 0, 0, 
      UiMetric.Background->Viewport.Width, UiMetric.Background->Height);

    /* Draw the highlight for selected item */
    if (!option_mode)
      pspVideoFillRect(sx, sel_top, sx+w, sel_top+fh, 
        UiMetric.SelectedBgColor);

    pspVideoCallList(call_list);

    /* Perform any custom drawing */
    if (uimenu->OnRender)
      uimenu->OnRender(uimenu, sel);

    /* Render menu options */
    if (option_mode)
    {
      k = sx + max_item_w + UiMetric.MenuItemMargin + 10;
      int arrow_x = min_x + (UiMetric.MenuItemMargin / 2 - arrow_w / 2);
      const pl_menu_option *option;

      /* Background */
      pspVideoFillRect(min_x, min_y, max_x, max_y, UiMetric.MenuOptionBoxBg);
      pspVideoFillRect(min_x, sy + pos.Index * fh, max_x, 
        sy + (pos.Index + 1) * fh, UiMetric.MenuSelOptionBg);
      pspVideoGlowRect(min_x, min_y, max_x - 1, max_y - 1, 
        COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP * UI_ANIM_FRAMES), 2);

      /* Render selected item + previous items */
      i = sy + pos.Index * fh;
      for (option = temp_option; option && i >= sy; option = option->prev, i -= fh)
        pspVideoPrint(UiMetric.Font, k, i, option->text, (option == temp_option) 
          ? UiMetric.SelectedColor : UiMetric.MenuOptionBoxColor);

      /* Up arrow */
      if (option) pspVideoPrint(UiMetric.Font, arrow_x, 
          i + fh + anim_frame, PSP_CHAR_UP_ARROW, UiMetric.MenuDecorColor);

      /* Render following items */
      i = sy + (pos.Index  + 1) * fh;
      for (option = temp_option->next; option && i < dy; option = option->next, i += fh)
        pspVideoPrint(UiMetric.Font, k, i, option->text, 
          UiMetric.MenuOptionBoxColor);

      /* Down arrow */
      if (option) pspVideoPrint(UiMetric.Font, arrow_x, i - fh - anim_frame, 
          PSP_CHAR_DOWN_ARROW, UiMetric.MenuDecorColor);
    }

    pspVideoEnd();

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();

    last_sel = sel;
    last_sel_top = sel_top;
  }

  menu->selected = sel;
}

void pspUiSplashScreen(PspUiSplash *splash)
{
  SceCtrlData pad;
  int fh = pspFontGetLineHeight(UiMetric.Font);

  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.CancelButton)
    {
      if (splash->OnCancel) splash->OnCancel(splash, NULL);
      break;
    }

    if ((pad.Buttons & CONTROL_BUTTON_MASK) && splash->OnButtonPress)
    {
      if (splash->OnButtonPress(splash, pad.Buttons & CONTROL_BUTTON_MASK))
          break;
    }

    pspVideoBegin();

    /* Clear screen */
    if (UiMetric.Background) 
      pspVideoPutImage(UiMetric.Background, 0, 0, 
        UiMetric.Background->Viewport.Width, UiMetric.Background->Height);
    else 
      pspVideoClearScreen();

    /* Draw instructions */
    const char *dirs = (splash->OnGetStatusBarText)
      ? splash->OnGetStatusBarText(splash)
      : SplashStatusBarTemplate;
    pspVideoPrintCenter(UiMetric.Font, UiMetric.Left,
      SCR_HEIGHT - fh, UiMetric.Right, dirs, UiMetric.StatusBarColor);

    /* Render status information */
    RenderStatus();

    /* Perform any custom drawing */
    if (splash->OnRender)
      splash->OnRender(splash, NULL);

    pspVideoEnd();

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
  }
}

const pl_menu_item* pspUiSelect(const char *title, const pl_menu *menu)
{
  const pl_menu_item *sel, *item, *last_sel = NULL;
  struct UiPos pos;
  int lnmax, lnhalf;
  int i, j, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy;
  int anim_frame = 0, anim_incr = 1;
  int arrow_w = pspFontGetTextWidth(UiMetric.Font, PSP_CHAR_DOWN_ARROW);
  int widest = 100;
  int sel_top = 0, last_sel_top = 0;
  SceCtrlData pad;

  char *help_text = strdup(SelectorTemplate);
  ReplaceIcons(help_text);

  memset(call_list, 0, sizeof(call_list));

  /* Determine width of the longest caption */
  for (item = menu->items; item; item = item->next)
  {
    if (item->caption)
    {
      int item_w = pspFontGetTextWidth(UiMetric.Font, item->caption);
      if (item_w > widest) 
        widest = item_w;
    }
  }

  widest += UiMetric.MenuItemMargin * 2;

  sx = SCR_WIDTH - widest;
  sy = UiMetric.Top;
  dx = SCR_WIDTH;
  dy = UiMetric.Bottom;
  w = dx - sx;
  h = dy - sy;

  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  /* Initialize variables */
  lnmax = (dy - sy) / fh;
  lnhalf = lnmax >> 1;

  sel = menu->items;
  pos.Top = menu->items;
  pos.Index = pos.Offset = 0;

  pspVideoWaitVSync();

  /* Compute update frequency */
  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  /* Get copy of screen */
  PspImage *screen = pspVideoGetVramBufferCopy();

  if (UiMetric.Animate)
  {
    /* Intro animation */
    for (i = 0; i < UI_ANIM_FRAMES; i++)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw right frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0, 0, 0, UI_ANIM_FOG_STEP * i));
  	  pspVideoFillRect(SCR_WIDTH - (i * (widest / UI_ANIM_FRAMES)), 
        0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
  }

  int fast_scroll;

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (!pspCtrlPollControls(&pad))
      continue;

    fast_scroll = 0;

    /* Incr/decr animation frame */
    anim_frame += (UiMetric.Animate) ? anim_incr : 0;
    if (anim_frame > 2 || anim_frame < 0) 
      anim_incr *= -1;

    /* Check the directional buttons */
    if (sel)
    {
      if ((pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN) 
        && sel->next)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALDOWN;
        if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; } 
        else pos.Index++;
        sel = sel->next;
      }
      else if ((pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP) 
        && sel->prev)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALUP;
        if (pos.Index - 1 < 0) { pos.Offset--; pos.Top = pos.Top->prev; }
        else pos.Index--;
        sel = sel->prev;
      }
      else if (pad.Buttons & PSP_CTRL_LEFT)
      {
        for (i = 0; sel->prev && i < lnhalf; i++)
        {
          if (pos.Index - 1 < 0) { pos.Offset--; pos.Top = pos.Top->prev; }
          else pos.Index--;
          sel = sel->prev;
        }
      }
      else if (pad.Buttons & PSP_CTRL_RIGHT)
      {
        for (i = 0; sel->next && i < lnhalf; i++)
        {
          if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; }
          else pos.Index++;
          sel=sel->next;
        }
      }

      if (pad.Buttons & UiMetric.OkButton) break;
    }

    if (pad.Buttons & UiMetric.CancelButton) { sel = NULL; break; }

    /* Render to a call list */
    sceGuStart(GU_CALL, call_list);

    /* Apply fog and draw frame */
    pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
      COLOR(0, 0, 0, UI_ANIM_FOG_STEP * UI_ANIM_FRAMES));
    pspVideoGlowRect(sx, 0, dx - 1, SCR_HEIGHT - 1, 
      COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP * UI_ANIM_FRAMES), 2);

    /* Title */
    if (title)
      pspVideoPrintCenter(UiMetric.Font, sx, 0, dx,
        title, UiMetric.TitleColor);

    /* Render the items */
    for (item = (pl_menu_item*)pos.Top, i = 0, j = sy; 
      item && i < lnmax; item = item->next, j += fh, i++)
    {
      if (item == sel) sel_top = j;
      pspVideoPrintClipped(UiMetric.Font, sx + 10, j, item->caption, w - 10, 
        "...", (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);
    }

    /* Up arrow */
    if (pos.Top && pos.Top->prev) pspVideoPrint(UiMetric.Font, 
      SCR_WIDTH - arrow_w * 2, sy + anim_frame, 
      PSP_CHAR_UP_ARROW, UiMetric.MenuDecorColor);

    /* Down arrow */
    if (item) pspVideoPrint(UiMetric.Font, SCR_WIDTH - arrow_w * 2, 
        dy - fh - anim_frame, PSP_CHAR_DOWN_ARROW, UiMetric.MenuDecorColor);

    /* Shortcuts */
    pspVideoPrintCenter(UiMetric.Font, sx, SCR_HEIGHT - fh, dx,
      help_text, UiMetric.StatusBarColor);

    sceGuFinish();

    if (sel != last_sel && !fast_scroll && sel && last_sel 
      && UiMetric.Animate)
    {
      /* Move animation */
      int f, n = 4;
      for (f = 1; f <= n; f++)
      {
        pspVideoBegin();

        /* Clear screen */
        pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
        pspVideoFillRect(sx, 0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

        /* Selection box */
        int box_top = last_sel_top-((last_sel_top-sel_top)/n)*f;
        pspVideoFillRect(sx, box_top, sx + w, box_top + fh, 
          UiMetric.SelectedBgColor);

        sceGuCallList(call_list);

        pspVideoEnd();

        pspVideoWaitVSync();
        pspVideoSwapBuffers();
      }
    }

    pspVideoBegin();

    /* Clear screen */
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
    pspVideoFillRect(sx, 0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

    if (sel) pspVideoFillRect(sx, sel_top, sx + w, sel_top + fh, 
      UiMetric.SelectedBgColor);

    sceGuCallList(call_list);

    pspVideoEnd();

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();

    last_sel = sel;
    last_sel_top = sel_top;
  }

  if (UiMetric.Animate)
  {
    /* Exit animation */
    for (i = UI_ANIM_FRAMES - 1; i >= 0; i--)
    {
  	  pspVideoBegin();

  	  /* Clear screen */
  	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

  	  /* Apply fog and draw right frame */
  	  pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
  	    COLOR(0, 0, 0, UI_ANIM_FOG_STEP * i));
  	  pspVideoFillRect(SCR_WIDTH - (i * (widest / UI_ANIM_FRAMES)), 
        0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

  	  pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
  	}
  }

  free(help_text);
  pspImageDestroy(screen);

  return sel;
}

static void adhocMatchingCallback(int unk1, 
                                  int event, 
                                  unsigned char *mac2, 
                                  int opt_len, 
                                  void *opt_data)
{
  _adhoc_match_event.NewEvent = 1;
  _adhoc_match_event.EventID = event;
  memcpy(_adhoc_match_event.EventMAC, mac2, 
    sizeof(unsigned char) * 6);
  strncpy(_adhoc_match_event.OptData, opt_data, sizeof(char) * opt_len);
  _adhoc_match_event.OptData[opt_len] = '\0';
}

int pspUiAdhocHost(const char *name, PspMAC mac)
{
  /* Check the wlan switch */
  if (!pspAdhocIsWLANEnabled())
  {
    pspUiAlert("Error: WLAN switch is turned off"); 
    return 0;
  }

  pspUiFlashMessage(ADHOC_INITIALIZING);
  _adhoc_match_event.NewEvent = 0;

  /* Initialize ad-hoc networking */
  if (!pspAdhocInit("ULUS99999", adhocMatchingCallback))
  {
    pspUiAlert("Ad-hoc networking initialization failed"); 
    return 0;
  }

  /* Wait for someone to join */
  pspUiFlashMessage(ADHOC_AWAITING_JOIN);

  int state = ADHOC_WAIT_CLI;
  PspMAC selected;

  /* Loop until someone joins or host cancels */
  while (!ExitPSP)
  {
    SceCtrlData pad;

    if (!pspCtrlPollControls(&pad))
      continue;

    if (pad.Buttons & UiMetric.CancelButton) 
      break;

    if (_adhoc_match_event.NewEvent)
    {
      _adhoc_match_event.NewEvent = 0;

      switch(_adhoc_match_event.EventID)
      {
      case MATCHING_JOINED:
        break;
      case MATCHING_DISCONNECT:
      case MATCHING_CANCELED:
        if (pspAdhocIsMACEqual(selected, _adhoc_match_event.EventMAC))
          state = ADHOC_WAIT_CLI;
        break;
      case MATCHING_SELECTED:
        if (state == ADHOC_WAIT_CLI)
        {
          memcpy(selected, _adhoc_match_event.EventMAC, 
            sizeof(unsigned char) * 6);
          sceKernelDelayThread(1000000/60);
          pspAdhocSelectTarget(selected);
          state = ADHOC_WAIT_EST;
        }
        break;
      case MATCHING_ESTABLISHED:
        if (state == ADHOC_WAIT_EST)
        {
          if (pspAdhocIsMACEqual(selected, _adhoc_match_event.EventMAC))
          {
            state = ADHOC_ESTABLISHED;
            goto established;
          }
        }
        break;
      }
    }

    /* Wait if needed */
    sceKernelDelayThread(1000000/60);
  }

established:

  if (state == ADHOC_ESTABLISHED)
  {
    sceKernelDelayThread(1000000);

    PspMAC my_mac;
    pspAdhocGetOwnMAC(my_mac);
    memcpy(mac, selected, sizeof(unsigned char) * 6);

    if (!pspAdhocConnect(my_mac))
      return 0;

    return 1;
  }

  /* Shutdown ad-hoc networking */
  pspAdhocShutdown();
  return 0;
}

int pspUiAdhocJoin(PspMAC mac)
{
  /* Check the wlan switch */
  if (!pspAdhocIsWLANEnabled())
  {
    pspUiAlert("Error: WLAN switch is turned off"); 
    return 0;
  }

  char *title = "Select host";

  /* Get copy of screen */
  PspImage *screen = pspVideoGetVramBufferCopy();

  pspUiFlashMessage(ADHOC_INITIALIZING);
  _adhoc_match_event.NewEvent = 0;

  /* Initialize ad-hoc networking */
  if (!pspAdhocInit("ULUS99999", adhocMatchingCallback))
  {
    pspUiAlert("Ad-hoc networking initialization failed"); 
    pspImageDestroy(screen);
    return 0;
  }

  /* Initialize menu */
  pl_menu menu;
  pl_menu_create(&menu, NULL);

  int state = ADHOC_PENDING;
  const pl_menu_item *sel, *item, *last_sel = NULL;
  struct UiPos pos;
  int lnmax, lnhalf;
  int i, j, h, w, fh = pspFontGetLineHeight(UiMetric.Font);
  int sx, sy, dx, dy;
  int anim_frame = 0, anim_incr = 1;
  int arrow_w = pspFontGetTextWidth(UiMetric.Font, PSP_CHAR_DOWN_ARROW);
  int widest = 100;
  int sel_top = 0, last_sel_top = 0;
  SceCtrlData pad;
  PspMAC selected;

  char *help_text = strdup(SelectorTemplate);
  ReplaceIcons(help_text);

  memset(call_list, 0, sizeof(call_list));

  /* Determine width of the longest caption */
  for (item = menu.items; item; item = item->next)
  {
    if (item->caption)
    {
      int item_w = pspFontGetTextWidth(UiMetric.Font, item->caption);
      if (item_w > widest) 
        widest = item_w;
    }
  }

  widest += UiMetric.MenuItemMargin * 2;

  sx = SCR_WIDTH - widest;
  sy = UiMetric.Top;
  dx = SCR_WIDTH;
  dy = UiMetric.Bottom;
  w = dx - sx;
  h = dy - sy;

  u32 ticks_per_sec, ticks_per_upd;
  u64 current_tick, last_tick;

  /* Initialize variables */
  lnmax = (dy - sy) / fh;
  lnhalf = lnmax >> 1;

  sel = menu.items;
  pos.Top = menu.items;
  pos.Index = pos.Offset = 0;

  pspVideoWaitVSync();

  /* Compute update frequency */
  ticks_per_sec = sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&last_tick);
  ticks_per_upd = ticks_per_sec / UiMetric.MenuFps;

  if (UiMetric.Animate)
  {
    /* Intro animation */
    for (i = 0; i < UI_ANIM_FRAMES; i++)
    {
      pspVideoBegin();

      /* Clear screen */
      pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

      /* Apply fog and draw right frame */
      pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
        COLOR(0, 0, 0, UI_ANIM_FOG_STEP * i));
      pspVideoFillRect(SCR_WIDTH - (i * (widest / UI_ANIM_FRAMES)), 
        0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

      pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
    }
  }

  int fast_scroll, found_psp;

  /* Begin navigation loop */
  while (!ExitPSP)
  {
    if (_adhoc_match_event.NewEvent)
    {
      found_psp = 0;
      pl_menu_item *adhoc_item;
      _adhoc_match_event.NewEvent = 0;

      if (_adhoc_match_event.EventID == MATCHING_JOINED)
      {
        /* Make sure the machine isn't already on the list */
        for (adhoc_item = menu.items; adhoc_item; adhoc_item = adhoc_item->next)
          if (adhoc_item->param && pspAdhocIsMACEqual((unsigned char*)adhoc_item->param, 
            _adhoc_match_event.EventMAC))
          {
            found_psp = 1;
            break;
          }

        if (!found_psp)
        {
          /* Create item */
          adhoc_item = pl_menu_append_item(&menu, 0, _adhoc_match_event.OptData);

          /* Add MAC */
          unsigned char *opp_mac = (unsigned char*)malloc(6 * sizeof(unsigned char));
          memcpy(opp_mac, _adhoc_match_event.EventMAC, sizeof(unsigned char) * 6);
          adhoc_item->param = opp_mac;

          if (!pos.Top) 
            sel = pos.Top = menu.items;
        }
      }
      else if (_adhoc_match_event.EventID == MATCHING_DISCONNECT)
      {
        /* Make sure the machine IS on the list */
        for (adhoc_item = menu.items; adhoc_item; adhoc_item = adhoc_item->next)
          if (adhoc_item->param && pspAdhocIsMACEqual((unsigned char*)adhoc_item->param, 
            _adhoc_match_event.EventMAC))
          {
            found_psp = 1; 
            break; 
          }

        if (found_psp)
        {
          /* Free MAC & destroy item */
          free((void*)adhoc_item->param);
          pl_menu_remove_item(&menu, adhoc_item);

          /* Reset items */
          sel = pos.Top = menu.items;
          pos.Index = pos.Offset = 0;
        }
      }
      else if (_adhoc_match_event.EventID == MATCHING_REJECTED)
      {
        /* Host rejected connection */
        if (state == ADHOC_WAIT_HOST)
        {
          state = ADHOC_PENDING;
        }
      }
      else if (_adhoc_match_event.EventID == MATCHING_ESTABLISHED)
      {
        if (state == ADHOC_WAIT_HOST)
        {
          state = ADHOC_EST_AS_CLI;
          break;
        }
      }
    }

    /* Delay */
    sceKernelDelayThread(1000000/60);

    if (!pspCtrlPollControls(&pad))
      continue;

    fast_scroll = 0;

    /* Incr/decr animation frame */
    anim_frame += (UiMetric.Animate) ? anim_incr : 0;
    if (anim_frame > 2 || anim_frame < 0) 
      anim_incr *= -1;

    /* Check the directional buttons */
    if (sel)
    {
      if ((pad.Buttons & PSP_CTRL_DOWN || pad.Buttons & PSP_CTRL_ANALDOWN) 
        && sel->next)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALDOWN;
        if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; } 
        else pos.Index++;
        sel = sel->next;
      }
      else if ((pad.Buttons & PSP_CTRL_UP || pad.Buttons & PSP_CTRL_ANALUP) 
        && sel->prev)
      {
        fast_scroll = pad.Buttons & PSP_CTRL_ANALUP;
        if (pos.Index - 1 < 0) { pos.Offset--; pos.Top = pos.Top->prev; }
        else pos.Index--;
        sel = sel->prev;
      }
      else if (pad.Buttons & PSP_CTRL_LEFT)
      {
        for (i = 0; sel->prev && i < lnhalf; i++)
        {
          if (pos.Index - 1 < 0) { pos.Offset--; pos.Top = pos.Top->prev; }
          else pos.Index--;
          sel = sel->prev;
        }
      }
      else if (pad.Buttons & PSP_CTRL_RIGHT)
      {
        for (i = 0; sel->next && i < lnhalf; i++)
        {
          if (pos.Index + 1 >= lnmax) { pos.Offset++; pos.Top = pos.Top->next; }
          else pos.Index++;
          sel=sel->next;
        }
      }

      if (pad.Buttons & UiMetric.OkButton) 
      {
        if (state == ADHOC_PENDING)
        {
          state = ADHOC_WAIT_HOST;
          memcpy(selected, sel->param, sizeof(unsigned char) * 6);
          pspAdhocSelectTarget(selected);
        }
      }
    }

    if (pad.Buttons & UiMetric.CancelButton) { sel = NULL; break; }

    /* Render to a call list */
    sceGuStart(GU_CALL, call_list);

    /* Apply fog and draw frame */
    pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
      COLOR(0, 0, 0, UI_ANIM_FOG_STEP * UI_ANIM_FRAMES));
    pspVideoGlowRect(sx, 0, dx - 1, SCR_HEIGHT - 1, 
      COLOR(0xff,0xff,0xff,UI_ANIM_FOG_STEP * UI_ANIM_FRAMES), 2);

    /* Title */
    if (title)
      pspVideoPrintCenter(UiMetric.Font, sx, 0, dx,
        title, UiMetric.TitleColor);

    /* Render the items */
    for (item = (pl_menu_item*)pos.Top, i = 0, j = sy; 
      item && i < lnmax; item = item->next, j += fh, i++)
    {
      if (item == sel) sel_top = j;
      pspVideoPrintClipped(UiMetric.Font, sx + 10, j, item->caption, w - 10, 
        "...", (item == sel) ? UiMetric.SelectedColor : UiMetric.TextColor);
    }

    /* Up arrow */
    if (pos.Top && pos.Top->prev) pspVideoPrint(UiMetric.Font, 
      SCR_WIDTH - arrow_w * 2, sy + anim_frame, 
      PSP_CHAR_UP_ARROW, UiMetric.MenuDecorColor);

    /* Down arrow */
    if (item) pspVideoPrint(UiMetric.Font, SCR_WIDTH - arrow_w * 2, 
        dy - fh - anim_frame, PSP_CHAR_DOWN_ARROW, UiMetric.MenuDecorColor);

    /* Shortcuts */
    pspVideoPrintCenter(UiMetric.Font, sx, SCR_HEIGHT - fh, dx,
      help_text, UiMetric.StatusBarColor);

    sceGuFinish();

    if (sel != last_sel && !fast_scroll && sel && last_sel 
      && UiMetric.Animate)
    {
      /* Move animation */
      int f, n = 4;
      for (f = 1; f <= n; f++)
      {
        pspVideoBegin();

        /* Clear screen */
        pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
        pspVideoFillRect(sx, 0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

        /* Selection box */
        int box_top = last_sel_top-((last_sel_top-sel_top)/n)*f;
        pspVideoFillRect(sx, box_top, sx + w, box_top + fh, 
          UiMetric.SelectedBgColor);

        sceGuCallList(call_list);

        pspVideoEnd();

        pspVideoWaitVSync();
        pspVideoSwapBuffers();
      }
    }

    pspVideoBegin();

    /* Clear screen */
    pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);
    pspVideoFillRect(sx, 0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

    if (sel) pspVideoFillRect(sx, sel_top, sx + w, sel_top + fh, 
      UiMetric.SelectedBgColor);

    sceGuCallList(call_list);

    pspVideoEnd();

    /* Wait if needed */
    do { sceRtcGetCurrentTick(&current_tick); }
    while (current_tick - last_tick < ticks_per_upd);
    last_tick = current_tick;

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();

    last_sel = sel;
    last_sel_top = sel_top;
  }

  if (UiMetric.Animate)
  {
    /* Exit animation */
    for (i = UI_ANIM_FRAMES - 1; i >= 0; i--)
    {
      pspVideoBegin();

      /* Clear screen */
      pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

      /* Apply fog and draw right frame */
      pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, 
        COLOR(0, 0, 0, UI_ANIM_FOG_STEP * i));
      pspVideoFillRect(SCR_WIDTH - (i * (widest / UI_ANIM_FRAMES)), 
        0, dx, SCR_HEIGHT, UiMetric.MenuOptionBoxBg);

      pspVideoEnd();

      /* Swap buffers */
      pspVideoWaitVSync();
      pspVideoSwapBuffers();
    }
  }

  free(help_text);
  pspImageDestroy(screen);

  /* Free memory used for MACs; menu resources */
  for (item = menu.items; item; item=item->next)
    if (item->param) free((void*)item->param);
  pl_menu_destroy(&menu);

  if (state == ADHOC_EST_AS_CLI)
  {
    memcpy(mac, selected, sizeof(unsigned char) * 6);

    if (!pspAdhocConnect(selected))
      return 0;

    return 1;
  }

  /* Shut down ad-hoc networking */
  pspAdhocShutdown();
  return 0;
}

void pspUiFadeout()
{
  /* Get copy of screen */
  PspImage *screen = pspVideoGetVramBufferCopy();

  /* Exit animation */
  int i, alpha;
  for (i = 0; i < UI_ANIM_FRAMES; i++)
  {
	  pspVideoBegin();

	  /* Clear screen */
	  pspVideoPutImage(screen, 0, 0, screen->Viewport.Width, screen->Height);

	  /* Apply fog */
	  alpha = (0x100/UI_ANIM_FRAMES)*i-1;
	  if (alpha > 0) 
	    pspVideoFillRect(0, 0, SCR_WIDTH, SCR_HEIGHT, COLOR(0,0,0,alpha));

	  pspVideoEnd();

    /* Swap buffers */
    pspVideoWaitVSync();
    pspVideoSwapBuffers();
	}

  pspImageDestroy(screen);
}

void enter_directory(pl_file_path current_dir, 
                     const char *subdir)
{
  pl_file_path new_path;
  pl_file_open_directory(current_dir,
                         subdir,
                         new_path,
                         sizeof(new_path));
  strcpy(current_dir, new_path);
}
