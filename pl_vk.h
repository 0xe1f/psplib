/* psplib/pl_vk.h: Virtual keyboard implementation
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

#ifndef _PL_VK_H
#define _PL_VK_H

#include "ctrl.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pl_vk_button_t;
struct pl_vk_sticky_t;

typedef struct pl_vk_layout_t
{
  uint16_t key_count;
  struct pl_vk_button_t *keys;
  uint16_t sticky_count;
  struct pl_vk_sticky_t *stickies;
  uint16_t offset_x;
  uint16_t offset_y;
  uint16_t selected;
  uint16_t held_down;
  PspImage *keyb_image;
  unsigned int __attribute__((aligned(16))) call_list[262144];
  int(*read_callback)(unsigned int code);
  void(*write_callback)(unsigned int code, int status);
} pl_vk_layout;

int  pl_vk_load(pl_vk_layout *layout,
                const char *data_path,
                const char *image_path,
                int(*read_callback)(unsigned int),
                void(*write_callback)(unsigned int, int));
void pl_vk_destroy(pl_vk_layout *layout);
void pl_vk_reinit(pl_vk_layout *layout);
void pl_vk_render(const pl_vk_layout *layout);
void pl_vk_navigate(pl_vk_layout *layout, SceCtrlData *pad);
void pl_vk_release_all(pl_vk_layout *layout);

#ifdef __cplusplus
}
#endif

#endif  // _PL_VK_H
