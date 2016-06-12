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

#ifndef _PL_GFX_H
#define _PL_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

#define PL_GFX_SCREEN_WIDTH  480
#define PL_GFX_SCREEN_HEIGHT 272

typedef u32 pl_color;
#define PL_MAKE_COLOR(r,g,b,a) \
  (((int)(r)&0xff)|(((int)(g)&0xff)<<8)|\
  (((int)(b)&0xff)<<16)|(((int)(a)&0xff)<<24))

#define PL_WHITE    (u32)0xffffffff
#define PL_BLACK    (u32)0xff000000
#define PL_GRAY     (u32)0xffcccccc
#define PL_DARKGRAY (u32)0xff777777
#define PL_RED      (u32)0xff0000ff
#define PL_GREEN    (u32)0xff00ff00
#define PL_BLUE     (u32)0xffff0000
#define PL_YELLOW   (u32)0xff00ffff
#define PL_MAGENTA  (u32)0xffff00ff

int  pl_gfx_init();
void pl_gfx_shutdown();

void pl_gfx_begin();
void pl_gfx_end();
void pl_gfx_vsync();
void pl_gfx_swap();

void* pl_gfx_vram_alloc(unsigned int bytes);
void  pl_gfx_put_image(const PspImage *image, int dx, int dy, int dw, int dh);

#ifdef __cplusplus
}
#endif

#endif  // PL_GFX_H
