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

#ifndef _PSP_IMAGE_H
#define _PSP_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define PSP_IMAGE_INDEXED 8
#define PSP_IMAGE_16BPP   16

typedef struct
{
  int X;
  int Y;
  int Width;
  int Height;
} PspViewport;

typedef struct
{
  int Width;
  int Height;
  void *Pixels;
  PspViewport Viewport;
  char FreeBuffer;
  char BytesPerPixel;
  char Depth;
  char PowerOfTwo;
  unsigned int TextureFormat;
  /* TODO: don't allocate if not necessary */
  unsigned short __attribute__((aligned(16))) Palette[256];
  unsigned short PalSize;
} PspImage;

/* Create/destroy */
PspImage* pspImageCreate(int width, int height, int bits_per_pixel);
PspImage* pspImageCreateVram(int width, int height, int bits_per_pixel);
PspImage* pspImageCreateOptimized(int width, int height, int bpp);
void      pspImageDestroy(PspImage *image);

PspImage* pspImageRotate(const PspImage *orig, int angle_cw);
PspImage* pspImageCreateThumbnail(const PspImage *image);
PspImage* pspImageCreateCopy(const PspImage *image);
void      pspImageClear(PspImage *image, unsigned int color);

PspImage* pspImageLoadPng(const char *path);
int       pspImageSavePng(const char *path, const PspImage* image);
PspImage* pspImageLoadPngFd(FILE *fp);
int       pspImageSavePngFd(FILE *fp, const PspImage* image);

int pspImageBlur(const PspImage *original, PspImage *blurred);
int pspImageDiscardColors(const PspImage *original);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_IMAGE_H
