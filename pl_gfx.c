/* psplib/pl_gfx.c: Graphics rendering routines
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

#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <math.h>

#include "pl_gfx.h"
#include "pl_image.h"

#define SLICE_SIZE    16
#define BUF_WIDTH     512
#define VRAM_START    0x04000000
#define VRAM_UNCACHED 0x40000000

typedef struct pl_gfx_vertex_t
{
  unsigned short u, v;
  unsigned short color;
  short x, y, z;
} pl_gfx_vertex;

static unsigned int get_bytes_per_pixel(unsigned int format);
static unsigned int get_texture_format(pl_image_format format);

static void *_vram_alloc_ptr = NULL;
static void *_disp_buffer    = NULL;
static void *_draw_buffer    = NULL;
static void *_depth_buffer   = NULL;
static unsigned int _format  = 0;

static unsigned int __attribute__((aligned(16))) _disp_list[262144];

static unsigned int get_bytes_per_pixel(unsigned int format)
{
  switch (format)
  {
  case GU_PSM_5650:
  case GU_PSM_5551:
  case GU_PSM_4444:
    return 2;
  case GU_PSM_8888:
    return 4;
  case GU_PSM_T8:
    return 1;
  default:
    return 0;
  }
}

static unsigned int get_texture_format(pl_image_format format)
{
  switch (format)
  {
  case pl_image_indexed:
    return GU_PSM_T8;
  case pl_image_5551:
    return GU_PSM_5551;
  case pl_image_4444:
    return GU_PSM_4444;
  default:
    return 0;
  }
}

int pl_gfx_init(unsigned int format)
{
  unsigned int bytes_per_pixel =
    get_bytes_per_pixel(format);
  unsigned int vram_offset = 0;

  if (!bytes_per_pixel) return 0;

  _format = 0;
  _vram_alloc_ptr = (void*)(VRAM_START|VRAM_UNCACHED|0x00088000);

  /* Initialize draw buffer */
  int size = bytes_per_pixel * BUF_WIDTH * PL_GFX_SCREEN_HEIGHT;
  _draw_buffer = (void*)vram_offset;
  vram_offset += size;
  /* TODO: bpp was originally 4 instead of 2 here */
  _disp_buffer = (void*)vram_offset;
  vram_offset += size;
  _depth_buffer = (void*)vram_offset;
  vram_offset += size;

  sceGuInit();
  sceGuStart(GU_DIRECT, _disp_list);
  sceGuDrawBuffer(format,
                  _draw_buffer,
                  BUF_WIDTH);
  sceGuDispBuffer(PL_GFX_SCREEN_WIDTH,
                  PL_GFX_SCREEN_HEIGHT,
                  _disp_buffer,
                  BUF_WIDTH);
  sceGuDepthBuffer(_depth_buffer,
                   BUF_WIDTH);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuOffset(0, 0);
  sceGuViewport(PL_GFX_SCREEN_WIDTH/2,
                PL_GFX_SCREEN_HEIGHT/2,
                PL_GFX_SCREEN_WIDTH,
                PL_GFX_SCREEN_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuDisable(GU_ALPHA_TEST);
  sceGuBlendFunc(GU_ADD,
                 GU_SRC_ALPHA,
                 GU_ONE_MINUS_SRC_ALPHA,
                 0, 0);
  sceGuEnable(GU_BLEND);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuEnable(GU_CULL_FACE);
  sceGuDisable(GU_LIGHTING);
  sceGuFrontFace(GU_CW);
/* TODO:  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT); */
/*  sceGuEnable(GU_SCISSOR_TEST); */
  sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
  sceGuAmbientColor(0xffffffff);
  sceGuFinish();
  sceGuSync(0,0);
  sceGuDisplay(GU_TRUE);

  return 1;
}

void pl_gfx_shutdown()
{
  sceGuTerm();
}

void* pl_gfx_vram_alloc(unsigned int bytes)
{
  void *ptr = _vram_alloc_ptr;
  _vram_alloc_ptr += bytes;

  /* Align pointer to 16 bytes */
  int rem = (unsigned int)_vram_alloc_ptr & 15;
  if (rem != 0)
    _vram_alloc_ptr += 16 - rem;

  return ptr;
}

void pl_video_put_image(const pl_image *image, 
                        int dx,
                        int dy,
                        int dw,
                        int dh)
{
  sceKernelDcacheWritebackAll();
  sceGuEnable(GU_TEXTURE_2D);

  /* TODO: Rewrite so that any format is rendered */
  if (image->format == pl_image_indexed)
  {
    sceGuClutMode(get_texture_format(image->palette.format),
                  0, 0xff, 0);
    sceGuClutLoad(image->palette.colors >> 3,
                  image->palette.palette);
  }

  sceGuTexMode(get_texture_format(image->format),
               0, 0, GU_FALSE);
  sceGuTexImage(0,
                image->line_width,
                image->line_width,
                image->line_width,
                image->bitmap);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexFilter(GU_LINEAR, GU_LINEAR);

  pl_gfx_vertex *vertices;
  int start, end, sc_end, slsz_scaled;
  slsz_scaled = ceil((float)dw * (float)SLICE_SIZE) / (float)image->view.w;

  start = image->view.x;
  end = image->view.x + image->view.w;
  sc_end = dx + dw;

  for (; start < end; start += SLICE_SIZE, dx += slsz_scaled)
  {
    vertices = (pl_gfx_vertex*)sceGuGetMemory(2 * sizeof(pl_gfx_vertex));

    vertices[0].u = start;
    vertices[0].v = image->view.y;
    vertices[1].u = start + SLICE_SIZE;
    vertices[1].v = image->view.h + image->view.y;

    vertices[0].x = dx; vertices[0].y = dy;
    vertices[1].x = dx + slsz_scaled; vertices[1].y = dy + dh;

    vertices[0].color
      = vertices[1].color
      = vertices[0].z 
      = vertices[1].z = 0;

    sceGuDrawArray(GU_SPRITES,
      GU_TEXTURE_16BIT|GU_COLOR_5551|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
  }

  sceGuDisable(GU_TEXTURE_2D);
}
