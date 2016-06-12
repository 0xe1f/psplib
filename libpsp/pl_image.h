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

#ifndef _PL_IMAGE_H
#define _PL_IMAGE_H

#include <stdio.h>
#include <psptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PL_IMAGE_USE_VRAM  0x01

#define uint unsigned int

typedef enum pl_image_format_t
{
  pl_image_indexed = 0x01, /* & 0x07 gives    */
  pl_image_4444    = 0x02, /* bytes per pixel */
  pl_image_5551    = 0x12,
} pl_image_format;

typedef struct pl_image_palette_t
{
  void *palette;
  uint  colors;
  pl_image_format
        format;
} pl_image_palette;

typedef struct pl_image_view_t
{
  uint x;
  uint y;
  uint w;
  uint h;
} pl_image_view;

typedef struct pl_image_t
{
  pl_image_view
          view;
  uint    height;
  uint    line_width;
  uint    pitch;
  pl_image_format
          format;
  pl_image_palette
          palette;
  uint8_t flags;
  void   *bitmap;
} pl_image;

int  pl_image_create(pl_image *image,
                     uint width,
                     uint height,
                     pl_image_format format,
                     uint8_t flags);
void pl_image_destroy(pl_image *image);

int  pl_image_palettize(pl_image *image,
                        pl_image_format pal_format,
                        uint pal_colors);
int  pl_image_set_palette_color(pl_image *image,
                                uint index,
                                uint32_t color);

uint pl_image_get_depth(const pl_image *image);

int  pl_image_compose_color(pl_image_format dest_format,
                            uint32_t *color,
                            uint8_t red,
                            uint8_t green,
                            uint8_t blue,
                            uint8_t alpha);
int  pl_image_split_color(pl_image_format src_format,
                          const pl_image_palette *src_palette,
                          uint32_t color,
                          uint8_t *red,
                          uint8_t *green,
                          uint8_t *blue,
                          uint8_t *alpha);

int  pl_image_load_png_stream(pl_image *image,
                              FILE *stream);
int  pl_image_save_png_stream(const pl_image *image,
                              FILE *stream);

int  pl_image_load(pl_image *image,
                   const char *path);
int  pl_image_save(const pl_image *image,
                   const char *path);

int  pl_image_create_duplicate(const pl_image *original,
                               pl_image *copy);
int  pl_image_create_thumbnail(const pl_image *original,
                               pl_image *thumb);

#define pl_image_get_bytes_per_pixel(format) \
  ((format) & 0x07)

int pl_image_clear(pl_image *image,
                   uint8_t red,
                   uint8_t green,
                   uint8_t blue,
                   uint8_t alpha);
#if 0
int pl_image_rotate(const pl_image *original,
                    pl_image *rotated,
                    int angle_cw);
#endif

#undef uint

#ifdef __cplusplus
}
#endif

#endif  // _PL_IMAGE_H
