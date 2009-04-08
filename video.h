/* psplib/video.h: Graphics rendering routines (legacy version)
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

#ifndef _PSP_VIDEO_H
#define _PSP_VIDEO_H

#include <psptypes.h>

#include "font.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_COLOR_WHITE	   (u32)0xffffffff
#define PSP_COLOR_BLACK	   (u32)0xff000000
#define PSP_COLOR_GRAY	   (u32)0xffcccccc
#define PSP_COLOR_DARKGRAY (u32)0xff777777
#define PSP_COLOR_RED	     (u32)0xff0000ff
#define PSP_COLOR_GREEN	   (u32)0xff00ff00
#define PSP_COLOR_BLUE	   (u32)0xffff0000
#define PSP_COLOR_YELLOW   (u32)0xff00ffff
#define PSP_COLOR_MAGENTA  (u32)0xffff00ff

#define PSP_VIDEO_UNSCALED    0
#define PSP_VIDEO_FIT_HEIGHT  1
#define PSP_VIDEO_FILL_SCREEN 2

#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272

#define COLOR(r,g,b,a) (((int)(r)&0xff)|(((int)(g)&0xff)<<8)|\
  (((int)(b)&0xff)<<16)|(((int)(a)&0xff)<<24))

#define RGB(r,g,b)   (((((b)>>3)&0x1F)<<10)|((((g)>>3)&0x1F)<<5)|\
  (((r)>>3)&0x1F)|0x8000)
#define RED(pixel)   ((((pixel))&0x1f)*0xff/0x1f)
#define GREEN(pixel) ((((pixel)>>5)&0x1f)*0xff/0x1f)
#define BLUE(pixel)  ((((pixel)>>10)&0x1f)*0xff/0x1f)

#define RGB_32(r,g,b)    COLOR(r,g,b,0xff)
#define RGBA_32(r,g,b,a) COLOR(r,g,b,a)

#define RED_32(c)   ((c)&0xff)
#define GREEN_32(c) (((c)>>8)&0xff)
#define BLUE_32(c)  (((c)>>16)&0xff)
#define ALPHA_32(c) (((c)>>24)&0xff)

extern const unsigned int PspFontColor[];

typedef struct PspVertex
{
  unsigned int color;
  short x, y, z;
} PspVertex;

void pspVideoInit();
void pspVideoShutdown();
void pspVideoClearScreen();
void pspVideoWaitVSync();
void pspVideoSwapBuffers();

void pspVideoBegin();
void pspVideoEnd();

void pspVideoDrawLine(int sx, int sy, int dx, int dy, u32 color);
void pspVideoDrawRect(int sx, int sy, int dx, int dy, u32 color);
void pspVideoFillRect(int sx, int sy, int dx, int dy, u32 color);

int pspVideoPrint(const PspFont *font, int sx, int sy, const char *string, u32 color);
int pspVideoPrintCenter(const PspFont *font, int sx, int sy, int dx, const char *string, u32 color);
int pspVideoPrintN(const PspFont *font, int sx, int sy, const char *string, int count, u32 color);
int pspVideoPrintClipped(const PspFont *font, int sx, int sy, const char* string, int max_w, char* clip, u32 color);
int pspVideoPrintNRaw(const PspFont *font, int sx, int sy, const char *string, int count, u32 color);
int pspVideoPrintRaw(const PspFont *font, int sx, int sy, const char *string, u32 color);

void pspVideoPutImage(const PspImage *image, int dx, int dy, int dw, int dh);
void pspVideoPutImageAlpha(const PspImage *image, int dx, int dy, int dw, int dh,
                           unsigned char alpha);

void pspVideoGlowRect(int sx, int sy, int dx, int dy, u32 color, int radius);
void pspVideoShadowRect(int sx, int sy, int dx, int dy, u32 color, int depth);

PspImage* pspVideoGetVramBufferCopy();

void pspVideoBeginList(void *list);
void pspVideoCallList(const void *list);

void* pspVideoAllocateVramChunk(unsigned int bytes);

unsigned int pspVideoGetVSyncFreq();

#ifdef __cplusplus
}
#endif

#endif  // _PSP_VIDEO_H
