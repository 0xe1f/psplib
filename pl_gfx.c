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

#include "image.h"

#include "pl_gfx.h"

#define SLICE_SIZE    16
#define BUF_WIDTH     512
#define VRAM_START    0x04000000
#define VRAM_UNCACHED 0x40000000

static unsigned int get_bytes_per_pixel(unsigned int format);

typedef struct pl_gfx_xf_fvertex_t
{
  short u, v;
  u16 color;
  float x, y, z;
} pl_gfx_xf_fvertex;

typedef struct pl_gfx_vertex_t
{
  unsigned short u, v;
  unsigned short color;
  short x, y, z;
} pl_gfx_vertex;

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

int pl_gfx_init(unsigned int format)
{
  unsigned int bytes_per_pixel = get_bytes_per_pixel(format);
  unsigned int vram_offset = 0;

  if (!bytes_per_pixel) return 0;

  _format = 0;
  _vram_alloc_ptr = (void*)(VRAM_START|VRAM_UNCACHED|0x00088000);

  /* Initialize draw buffer */
  int size = bytes_per_pixel * BUF_WIDTH * PL_GFX_SCREEN_HEIGHT;
  _draw_buffer = (void*)vram_offset;
  vram_offset += size;
  _disp_buffer = (void*)vram_offset;
  vram_offset += size;
  _depth_buffer = (void*)vram_offset;
  vram_offset += size;

  sceGuInit();
  sceGuStart(GU_DIRECT, _disp_list);
  sceGuDrawBuffer(format, _draw_buffer, BUF_WIDTH);
  sceGuDispBuffer(PL_GFX_SCREEN_WIDTH, PL_GFX_SCREEN_HEIGHT, _disp_buffer, BUF_WIDTH);
  sceGuDepthBuffer(_depth_buffer, BUF_WIDTH);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuOffset(0, 0);
  sceGuViewport(PL_GFX_SCREEN_WIDTH/2, PL_GFX_SCREEN_HEIGHT/2,
                PL_GFX_SCREEN_WIDTH,   PL_GFX_SCREEN_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuDisable(GU_ALPHA_TEST);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuEnable(GU_BLEND);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuEnable(GU_CULL_FACE);
  sceGuDisable(GU_LIGHTING);
  sceGuFrontFace(GU_CW);
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

void pl_gfx_put_image(const PspImage *image, int dx, int dy, int dw, int dh)
{
  int scissor_enabled = sceGuGetStatus(GU_SCISSOR_TEST);
  int texture_enabled = sceGuGetStatus(GU_TEXTURE_2D);

  sceKernelDcacheWritebackAll();

  if (scissor_enabled) sceGuDisable(GU_SCISSOR_TEST);
  if (!texture_enabled) sceGuEnable(GU_TEXTURE_2D);

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    sceGuClutMode(GU_PSM_5551, 0, 0xff, 0);
    sceGuClutLoad(image->PalSize >> 3, image->Palette);
  }

  sceGuTexMode(image->TextureFormat, 0, 0, GU_FALSE);
  sceGuTexImage(0, image->Width, image->Width, image->Width, image->Pixels);

  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexFilter(GU_LINEAR, GU_LINEAR);

  float sc_end, slsz_scaled, ddx = (float)dx;
  short start, end, sxr;
  float dxr;
  slsz_scaled = (float)(dw * SLICE_SIZE) / (float)image->Viewport.Width;

  start = image->Viewport.X;
  end = image->Viewport.X + image->Viewport.Width;
  sc_end = ddx + (float)dw;

  pl_gfx_xf_fvertex *tx_vert;

  for (; start < end; start += SLICE_SIZE, ddx += slsz_scaled)
  {
    tx_vert = (pl_gfx_xf_fvertex*)sceGuGetMemory(2 * sizeof(pl_gfx_xf_fvertex));

    sxr = start + SLICE_SIZE;
    dxr = ddx + slsz_scaled;
    tx_vert[0].u = start;                   /* source X left */
    tx_vert[1].u = (sxr > end) ? end : sxr ; /* source X right */
    tx_vert[0].x = ddx;                   /* dest. X left */
    tx_vert[1].x = (dxr > sc_end) ? sc_end : dxr; /* dest. X right */

    tx_vert[0].v = image->Viewport.Y;             /* sy top     */
    tx_vert[1].v = image->Viewport.Y + image->Viewport.Height; /* sy bottom  */
    tx_vert[0].y = (float)dy;                     /* dy top    */
    tx_vert[1].y = (float)dy + (float)dh;         /* dy bottom */

    tx_vert[0].color = tx_vert[1].color = 0;  /* not used */
    tx_vert[0].z = tx_vert[1].z = 0.0;        /* not used */

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_5551 | 
                               GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                   2, 0, tx_vert);
  }

  /* Restore states */
  if (!texture_enabled) sceGuDisable(GU_TEXTURE_2D);
  if (scissor_enabled) sceGuEnable(GU_SCISSOR_TEST);

  if (scissor_enabled)
    sceGuEnable(GU_SCISSOR_TEST);
}

void pl_gfx_begin()
{
  sceGuStart(GU_DIRECT, _disp_list);
}

void pl_gfx_end()
{
  sceGuFinish();
  sceGuSync(0, 0);
}

void pl_gfx_vsync()
{
  sceDisplayWaitVblankStart();
}

void pl_gfx_swap()
{
  sceGuSwapBuffers();
}
