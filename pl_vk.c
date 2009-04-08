/* psplib/pl_vk.c: Virtual keyboard implementation
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
#include <malloc.h>
#include <pspgu.h>
#include <string.h>
#include <pspkernel.h>
#include <stdio.h>

#include "video.h"

#include "pl_vk.h"

#define PL_VK_DELAY     400
#define PL_VK_THRESHOLD 50

#define PL_VK_BUTTON_BG        COLOR(0x44,0x44,0x44,0x33)
#define PL_VK_LAYOUT_BG        COLOR(0,0,0,0x22)
#define PL_VK_STUCK_COLOR      COLOR(0xff,0,0,0x33)
#define PL_VK_SELECTED_COLOR   COLOR(0xff,0xff,0xff,0x88)

static const int _button_map[] = 
{
  PSP_CTRL_UP,
  PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,
  PSP_CTRL_RIGHT,
  PSP_CTRL_CIRCLE,
  PSP_CTRL_TRIANGLE,
  0
};
static u64 _push_time[6];

typedef struct pl_vk_button_t
{
  uint16_t code;
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint8_t  is_sticky;
} pl_vk_button;

typedef struct pl_vk_sticky_t
{
  uint16_t code;
  uint8_t  status;
  uint16_t *key_index;
  uint8_t  index_count;
} pl_vk_sticky;

static void filter_repeats(SceCtrlData *pad);
static void render_to_display_list(pl_vk_layout *layout);

int pl_vk_load(pl_vk_layout *layout,
               const char *data_path,
               const char *image_path,
               int(*read_callback)(unsigned int),
               void(*write_callback)(unsigned int, int))
{
  /* Reset physical button status */
  int i;
  for (i = 0; _button_map[i]; i++)
    _push_time[i] = 0;

  /* Initialize */
  layout->keys = NULL;
  layout->stickies = NULL;
  layout->keyb_image = NULL;
  layout->key_count = layout->sticky_count =
    layout->offset_x = layout->offset_y =
    layout->selected = layout->held_down = 0;

  /* Load image */
  if (image_path && !(layout->keyb_image = pspImageLoadPng(image_path)))
    return 0;

  /* Init callbacks */
  layout->read_callback = read_callback;
  layout->write_callback = write_callback;

  /* Try opening file */
  FILE *file;
  if (!(file = fopen(data_path, "r")))
  {
    pl_vk_destroy(layout);
    return 0;
  }

  int code;
  uint16_t x, y, w, h;

  /* Determine button count */
  while ((fscanf(file, "0x%x\t%hi\t%hi\t%hi\t%hi\n",
                &code, &x, &y, &w, &h) == 5) && code)
    layout->key_count++;

  /* Rewind to start */
  rewind(file);

  layout->keys = (pl_vk_button*)malloc(layout->key_count *
                                       sizeof(pl_vk_button));
  if (!layout->keys)
  {
    fclose(file);
    pl_vk_destroy(layout);
    return 0;
  }

  /* Start reading keys */
  pl_vk_button *button;
  for (i = 0, button = layout->keys; i < layout->key_count; i++, button++)
  {
    if (fscanf(file, "0x%x\t%hi\t%hi\t%hi\t%hi\n",
               &code, &x, &y, &w, &h) < 5)
    {
      fclose(file);
      pl_vk_destroy(layout);
      return 0;
    }

    button->code = code & 0xffff;
    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->is_sticky = 0;
  }

  /* "Swallow" last record */
  fscanf(file, "0x%x\t%hi\t%hi\t%hi\t%hi\n",
         &code, &x, &y, &w, &h);

  /* Determine sticky count */
  long pos = ftell(file);
  while ((fscanf(file, "0x%x\t", &code) == 1) && code)
    layout->sticky_count++;

  /* Rewind again */
  fseek(file, pos, SEEK_SET);

  if (layout->sticky_count > 0)
  {
    layout->stickies = (pl_vk_sticky*)malloc(layout->sticky_count *
                                             sizeof(pl_vk_sticky));
    if (!layout->stickies)
    {
      fclose(file);
      pl_vk_destroy(layout);
      return 0;
    }

    /* Start reading stickies */
    pl_vk_sticky *sticky;
    for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
    {
      if (fscanf(file, "0x%x\t", &code) < 1)
      {
        fclose(file);
        pl_vk_destroy(layout);
        return 0;
      }

      sticky->code = code & 0xffff;
      sticky->status = 0;
      sticky->index_count = 0;
      sticky->key_index = NULL;

      int j;
      for (j = 0; j < layout->key_count; j++)
      {
        /* Mark sticky status */
        if (layout->keys[j].code == sticky->code)
          layout->keys[j].is_sticky = 1;

        /* Determine number of matching sticky keys */
        if (layout->keys[j].code == sticky->code) 
          sticky->index_count++;
      }

      /* Allocate space */
      if (sticky->index_count > 0)
      {
        sticky->key_index = (uint16_t*)malloc(sticky->index_count *
                                                    sizeof(uint16_t));
        if (!sticky->index_count)
        {
          fclose(file);
          pl_vk_destroy(layout);
          return 0;
        }
      }

      /* Save indices */
      int count;
      for (j = 0, count = 0; j < layout->key_count; j++)
        if (layout->keys[j].code == sticky->code)
          sticky->key_index[count++] = j;
    }
  }

  /* "Swallow" last record */
  fscanf(file, "0x%x\t", &code);

  /* Read the offsets */
  if (fscanf(file, "%hi\t%hi\n", 
             &layout->offset_x, &layout->offset_y) < 2)
  {
    fclose(file);
    pl_vk_destroy(layout);
    return 0;
  }

  render_to_display_list(layout);

  /* Close the file */
  fclose(file);
  return 1;
}

static void filter_repeats(SceCtrlData *pad)
{
  SceCtrlData p = *pad;
  int i;
  u64 tick;
  u32 tick_res;

  /* Get current tick count */
  sceRtcGetCurrentTick(&tick);
  tick_res = sceRtcGetTickResolution();

  /* Check each button */
  for (i = 0; i < _button_map[i]; i++)
  {
    if (p.Buttons & _button_map[i])
    {
      if (!_push_time[i] || tick >= _push_time[i])
      {
        /* Button was pushed for the first time, or time to repeat */
        pad->Buttons |= _button_map[i];
        /* Compute next press time */
        _push_time[i] = tick + ((_push_time[i]) ? PL_VK_THRESHOLD : PL_VK_DELAY)
          * (tick_res / 1000);
      }
      else
      {
        /* No need to repeat yet */
        pad->Buttons &= ~_button_map[i];
      }
    }
    else
    {
      /* Button was released */
      pad->Buttons &= ~_button_map[i];
      _push_time[i] = 0;
    }
  }
}

void pl_vk_release_all(pl_vk_layout *layout)
{
  /* Release 'held down' key */
  if (layout->held_down && layout->write_callback)
  {
    layout->write_callback(layout->held_down, 0);
    layout->held_down = 0;
  }

  /* Unset all sticky keys */
  int i;
  pl_vk_sticky *sticky;
  for (i = 0, sticky = layout->stickies; 
       i < layout->sticky_count; 
       i++, sticky++)
  {
    sticky->status = 0;
    if (layout->write_callback) 
      layout->write_callback(sticky->code, 0);
  }
}

void pl_vk_reinit(pl_vk_layout *layout)
{
  int i;
  for (i = 0; _button_map[i]; i++)
    _push_time[i] = 0;
  layout->held_down = 0;

  pl_vk_sticky *sticky;
  for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
    sticky->status = (layout->read_callback) 
                     ? layout->read_callback(sticky->code) : 0;
}

void pl_vk_navigate(pl_vk_layout *layout,
                    SceCtrlData *pad)
{
  int i;
  filter_repeats(pad);

  if ((pad->Buttons & PSP_CTRL_SQUARE)
    && layout->write_callback && !layout->held_down)
  {
    /* Button pressed */
    layout->held_down = layout->keys[layout->selected].code;
    layout->write_callback(layout->held_down, 1);

    /* Unstick if the key is stuck */
    if (layout->keys[layout->selected].is_sticky)
    {
      pl_vk_sticky *sticky;
      for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
      {
        /* Active sticky key; toggle status */
        if (sticky->status && 
          layout->keys[layout->selected].code == sticky->code)
        {
          sticky->status = !sticky->status;
          break;
        }
      }
    }
  }
  else if (!(pad->Buttons & PSP_CTRL_SQUARE) 
    && layout->write_callback && layout->held_down)
  {
    /* Button released */
    layout->write_callback(layout->held_down, 0);
    layout->held_down = 0;
  }

  if (pad->Buttons & PSP_CTRL_RIGHT)
  {
    if (layout->selected + 1 < layout->key_count
      && layout->keys[layout->selected].y == layout->keys[layout->selected + 1].y)
        layout->selected++;
  }
  else if (pad->Buttons & PSP_CTRL_LEFT)
  {
    if (layout->selected > 0 
      && layout->keys[layout->selected].y == layout->keys[layout->selected - 1].y)
        layout->selected--;
  }
  else if (pad->Buttons & PSP_CTRL_DOWN)
  {
    /* Find first button on the next row */
    for (i = layout->selected + 1; 
         i < layout->key_count && layout->keys[i].y == layout->keys[layout->selected].y; 
         i++);

    if (i < layout->key_count)
    {
      /* Find button below the current one */
      int r = i;
      for (; i < layout->key_count && layout->keys[i].y == layout->keys[r].y; i++)
        if (layout->keys[i].x + layout->keys[i].w / 2 >= layout->keys[layout->selected].x + layout->keys[layout->selected].w / 2) 
          break;

      layout->selected = (i < layout->key_count && layout->keys[r].y == layout->keys[i].y) 
                         ? i : i - 1;
    }
  }
  else if (pad->Buttons & PSP_CTRL_UP)
  {
    /* Find first button on the previous row */
    for (i = layout->selected - 1; 
         i >= 0 && layout->keys[i].y == layout->keys[layout->selected].y; 
         i--);

    if (i >= 0)
    {
      /* Find button above the current one */
      int r = i;
      for (; i >= 0 && layout->keys[i].y == layout->keys[r].y; i--)
        if (layout->keys[i].x + layout->keys[i].w / 2 <= 
            layout->keys[layout->selected].x + layout->keys[layout->selected].w / 2) 
          break;

      layout->selected = (i >= 0 && layout->keys[r].y == layout->keys[i].y) 
                         ? i : i + 1;
    }
  }

  if (layout->write_callback)
  {
    if (pad->Buttons & PSP_CTRL_CIRCLE && 
        layout->keys[layout->selected].is_sticky)
    {
      pl_vk_sticky *sticky;
      for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
      {
        /* Sticky key; toggle status */
        if (layout->keys[layout->selected].code == sticky->code)
        {
          sticky->status = !sticky->status;
          layout->write_callback(sticky->code, sticky->status);
          break;
        }
      }
    }
    else if (pad->Buttons & PSP_CTRL_TRIANGLE)
    {
      /* Unset all sticky keys */
      pl_vk_sticky *sticky;
      for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
      {
        sticky->status = 0;
        layout->write_callback(sticky->code, 0);
      }
    }
  }

  /* Unset used buttons */
  for (i = 0; _button_map[i]; i++) 
    pad->Buttons &= ~_button_map[i];
}

void pl_vk_render(const pl_vk_layout *layout)
{
  int off_x, off_y, i, j;
  const pl_vk_button *button;

  pspVideoCallList(layout->call_list);

  off_x = (SCR_WIDTH / 2 - layout->keyb_image->Viewport.Width / 2);
  off_y = (SCR_HEIGHT / 2 - layout->keyb_image->Viewport.Height / 2);

  if (layout->keyb_image)
    pspVideoPutImage(layout->keyb_image, 
                     off_x, off_y,
                     layout->keyb_image->Viewport.Width,
                     layout->keyb_image->Viewport.Height);
  else
  {
    /* TODO: render the entire layout */
  }

  off_x += layout->offset_x;
  off_y += layout->offset_y;

  /* Highlight sticky buttons */
  pl_vk_sticky *sticky;
  for (i = 0, sticky = layout->stickies; 
       i < layout->sticky_count; 
       i++, sticky++)
  {
    if (sticky->status)
    {
      for (j = 0; j < sticky->index_count; j++)
      {
        button = &(layout->keys[sticky->key_index[j]]);
        pspVideoFillRect(off_x + button->x + 1,
                         off_y + button->y + 1,
                         off_x + button->x + button->w,
                         off_y + button->y + button->h,
                         PL_VK_STUCK_COLOR);
      }
    }
  }

  /* Highlight selected button */
  button = &(layout->keys[layout->selected]);
  pspVideoFillRect(off_x + button->x + 1,
                   off_y + button->y + 1,
                   off_x + button->x + button->w,
                   off_y + button->y + button->h,
                   PL_VK_SELECTED_COLOR);
}

void pl_vk_destroy(pl_vk_layout *layout)
{
  /* Destroy sticky keys */
  if (layout->stickies)
  {
    int i;
    pl_vk_sticky *sticky;
    for (i = 0, sticky = layout->stickies; i < layout->sticky_count; i++, sticky++)
      free(sticky->key_index);
    free(layout->stickies);
  }

  if (layout->keys)
    free(layout->keys);

  if (layout->keyb_image)
    pspImageDestroy(layout->keyb_image);
}

static void render_to_display_list(pl_vk_layout *layout)
{
  /* Render the virtual keyboard to a call list */
  memset(layout->call_list, 0, sizeof(layout->call_list));

  sceGuStart(GU_CALL, layout->call_list);

  const pl_vk_button *button;
  int off_x, off_y;
  int i;

  off_x = (SCR_WIDTH / 2 - layout->keyb_image->Viewport.Width / 2);
  off_y = (SCR_HEIGHT / 2 - layout->keyb_image->Viewport.Height / 2);

  pspVideoFillRect(off_x,
                   off_y,
                   off_x + layout->keyb_image->Viewport.Width + 1,
                   off_y + layout->keyb_image->Viewport.Height + 1,
                   PL_VK_LAYOUT_BG);

  off_x += layout->offset_x;
  off_y += layout->offset_y;

  for (i = 0, button = layout->keys; i < layout->key_count; i++, button++)
    pspVideoFillRect(off_x + button->x, 
                     off_y + button->y,
                     off_x + button->x + button->w + 1,
                     off_y + button->y + button->h + 1,
                     PL_VK_BUTTON_BG);

  sceGuFinish();
}
