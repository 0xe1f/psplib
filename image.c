/* psplib/image.h: Image manipulation/saving/loading (legacy version)
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
#include <string.h>
#include <png.h>
#include <pspgu.h>

#include "video.h"
#include "image.h"

typedef unsigned char byte;

int FindPowerOfTwoLargerThan(int n);
int FindPowerOfTwoLargerThan2(int n);

/* Creates an image in memory */
PspImage* pspImageCreate(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int size = width * height * (bpp / 8);
  void *pixels = memalign(16, size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));

  if (!image)
  {
    free(pixels);
    return NULL;
  }

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  int i;
  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp / 8;
  image->FreeBuffer = 1;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));
  image->PalSize = (unsigned short)256;

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED:
    image->TextureFormat = GU_PSM_T8;
    break;
  case PSP_IMAGE_16BPP:
    image->TextureFormat = GU_PSM_5551;
    break;
  }

  return image;
}

/* Creates an image using portion of VRAM */
PspImage* pspImageCreateVram(int width, int height, int bpp)
{
  if (bpp != PSP_IMAGE_INDEXED && bpp != PSP_IMAGE_16BPP) return NULL;

  int i, size = width * height * (bpp / 8);
  void *pixels = pspVideoAllocateVramChunk(size);

  if (!pixels) return NULL;

  PspImage *image = (PspImage*)malloc(sizeof(PspImage));
  if (!image) return NULL;

  memset(pixels, 0, size);

  image->Width = width;
  image->Height = height;
  image->Pixels = pixels;

  image->Viewport.X = 0;
  image->Viewport.Y = 0;
  image->Viewport.Width = width;
  image->Viewport.Height = height;

  for (i = 1; i < width; i *= 2);
  image->PowerOfTwo = (i == width);
  image->BytesPerPixel = bpp >> 3;
  image->FreeBuffer = 0;
  image->Depth = bpp;
  memset(image->Palette, 0, sizeof(image->Palette));
  image->PalSize = (unsigned short)256;

  switch (image->Depth)
  {
  case PSP_IMAGE_INDEXED: image->TextureFormat = GU_PSM_T8;   break;
  case PSP_IMAGE_16BPP:   image->TextureFormat = GU_PSM_5551; break;
  }

  return image;
}

PspImage* pspImageCreateOptimized(int width, int height, int bpp)
{
  PspImage *image = pspImageCreate(FindPowerOfTwoLargerThan(width), height, bpp);
  if (image) image->Viewport.Width = width;

  return image;
}

/* Destroys image */
void pspImageDestroy(PspImage *image)
{
  if (image->FreeBuffer) free(image->Pixels);
  free(image);
}

PspImage* pspImageRotate(const PspImage *orig, int angle_cw)
{
  PspImage *final;

  /* Create image of appropriate size */
  switch(angle_cw)
  {
  case 0:
    return pspImageCreateCopy(orig);
  case 90:
    final = pspImageCreate(orig->Viewport.Height,
      orig->Viewport.Width, orig->Depth);
    break;
  case 180:
    final = pspImageCreate(orig->Viewport.Width,
      orig->Viewport.Height, orig->Depth);
    break;
  case 270:
    final = pspImageCreate(orig->Viewport.Height,
      orig->Viewport.Width, orig->Depth);
    break;
  default:
    return NULL;
  }

  int i, j, k, di = 0;
  int width = final->Width;
  int height = final->Height;
  int l_shift = orig->BytesPerPixel >> 1;
  
  const unsigned char *source = (unsigned char*)orig->Pixels;
  unsigned char *dest = (unsigned char*)final->Pixels;

  /* Skip to Y viewport starting point */
  source += (orig->Viewport.Y * orig->Width) << l_shift;

  /* Copy image contents */
  for (i = 0, k = 0; i < orig->Viewport.Height; i++)
  {
    /* Skip to the start of the X viewport */
    source += orig->Viewport.X << l_shift;

    for (j = 0; j < orig->Viewport.Width; j++, source += 1 << l_shift, k++)
    {
      switch(angle_cw)
      {
      case 90:
        di = (width - (k / height) - 1) + (k % height) * width;
        break;
      case 180:
        di = (height - i - 1) * width + (width - j - 1);
        break;
      case 270:
        di = (k / height) + (height - (k % height) - 1) * width;
        break;
      }

      /* Write to destination */
      if (orig->Depth == PSP_IMAGE_INDEXED) dest[di] = *source;
      else ((unsigned short*)dest)[di] = *(unsigned short*)source; /* 16 bpp */
    }

    /* Skip to the end of the line */
    source += (orig->Width - (orig->Viewport.X + orig->Viewport.Width)) << l_shift;
  }

  if (orig->Depth == PSP_IMAGE_INDEXED)
  {
    memcpy(final->Palette, orig->Palette, sizeof(orig->Palette));
    final->PalSize = orig->PalSize;
  }

  return final;
}

/* Creates a half-sized thumbnail of an image */
PspImage* pspImageCreateThumbnail(const PspImage *image)
{
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Viewport.Width >> 1,
    image->Viewport.Height >> 1, image->Depth)))
      return NULL;

  int dy = image->Viewport.Y + image->Viewport.Height;
  int dx = image->Viewport.X + image->Viewport.Width;

  for (i = image->Viewport.Y, p = 0; i < dy; i += 2)
    for (j = image->Viewport.X; j < dx; j += 2)
      if (image->Depth == PSP_IMAGE_INDEXED)
        ((unsigned char*)thumb->Pixels)[p++]
          = ((unsigned char*)image->Pixels)[(image->Width * i) + j];
      else
        ((unsigned short*)thumb->Pixels)[p++]
          = ((unsigned short*)image->Pixels)[(image->Width * i) + j];

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memcpy(thumb->Palette, image->Palette, sizeof(image->Palette));
    thumb->PalSize = image->PalSize;
  }

  return thumb;
}

int pspImageDiscardColors(const PspImage *original)
{
  if (original->Depth != PSP_IMAGE_16BPP) return 0;

  int y, x, gray;
  unsigned short *p;

  for (y = 0, p = (unsigned short*)original->Pixels; y < original->Height; y++)
    for (x = 0; x < original->Width; x++, p++)
    {
      gray = (RED(*p) * 3 + GREEN(*p) * 4 + BLUE(*p) * 2) / 9;
      *p = RGB(gray, gray, gray);
    }

  return 1;
}

int pspImageBlur(const PspImage *original, PspImage *blurred)
{
  if (original->Width != blurred->Width
    || original->Height != blurred->Height
    || original->Depth != blurred->Depth
    || original->Depth != PSP_IMAGE_16BPP) return 0;

  int r, g, b, n, i, y, x, dy, dx;
  unsigned short p;

  for (y = 0, i = 0; y < original->Height; y++)
  {
    for (x = 0; x < original->Width; x++, i++)
    {
      r = g = b = n = 0;
      for (dy = y - 1; dy <= y + 1; dy++)
      {
        if (dy < 0 || dy >= original->Height) continue;

        for (dx = x - 1; dx <= x + 1; dx++)
        {
          if (dx < 0 || dx >= original->Width) continue;

          p = ((unsigned short*)original->Pixels)[dx + dy * original->Width];
          r += RED(p);
          g += GREEN(p);
          b += BLUE(p);
          n++;
        }

        r /= n;
        g /= n;
        b /= n;
        ((unsigned short*)blurred->Pixels)[i] = RGB(r, g, b);
      }
    }
  }

  return 1;
}

/* Creates an exact copy of the image */
PspImage* pspImageCreateCopy(const PspImage *image)
{
  PspImage *copy;

  /* Create image */
  if (!(copy = pspImageCreate(image->Width, image->Height, image->Depth)))
    return NULL;

  /* Copy pixels */
  int size = image->Width * image->Height * image->BytesPerPixel;
  memcpy(copy->Pixels, image->Pixels, size);
  memcpy(&copy->Viewport, &image->Viewport, sizeof(PspViewport));
  memcpy(copy->Palette, image->Palette, sizeof(image->Palette));
  copy->PalSize = image->PalSize;

  return copy;
}

/* Clears an image */
void pspImageClear(PspImage *image, unsigned int color)
{
  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memset(image->Pixels, color & 0xff, image->Width * image->Height);
  }
  else if (image->Depth == PSP_IMAGE_16BPP)
  {
    int i;
    unsigned short *pixel = image->Pixels;
    for (i = image->Width * image->Height - 1; i >= 0; i--, pixel++)
      *pixel = color & 0xffff;
  }
}

/* Loads an image from a file */
PspImage* pspImageLoadPng(const char *path)
{
  FILE *fp = fopen(path,"rb");
  if(!fp) return NULL;

  PspImage *image = pspImageLoadPngFd(fp);
  fclose(fp);

  return image;
}

/* Saves an image to a file */
int pspImageSavePng(const char *path, const PspImage* image)
{
  FILE *fp = fopen( path, "wb" );
	if (!fp) return 0;

  int stat = pspImageSavePngFd(fp, image);
  fclose(fp);

  return stat;
}

#define IRGB(r,g,b,a)   (((((b)>>3)&0x1F)<<10)|((((g)>>3)&0x1F)<<5)|\
  (((r)>>3)&0x1F)|(a?0x8000:0))

/* Loads an image from an open file descriptor (16-bit PNG)*/
PspImage* pspImageLoadPngFd(FILE *fp)
{
  const size_t nSigSize = 8;
  byte signature[nSigSize];
  if (fread(signature, sizeof(byte), nSigSize, fp) != nSigSize)
    return 0;

  if (!png_check_sig(signature, nSigSize))
    return 0;

  png_struct *pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!pPngStruct)
    return 0;

  png_info *pPngInfo = png_create_info_struct(pPngStruct);
  if(!pPngInfo)
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  if (setjmp(pPngStruct->jmpbuf))
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  png_init_io(pPngStruct, fp);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  PspImage *image;

  int mod_width = FindPowerOfTwoLargerThan2(width);
  if (!(image = pspImageCreate(mod_width, height, PSP_IMAGE_16BPP)))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  image->Viewport.Width = width;

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  byte r, g, b, a;
  unsigned short *out = image->Pixels;

  for (y=0; y<height; y++)
  {
    png_byte *pRow = pRowTable[y];

    for (x=0; x<width; x++)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          a = 1;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          r = g = b = pRow[0];
          a = pRow[1];
          pRow += 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = 1;
          pRow += 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = pRow[3];
          pRow += 4;
          break;
        default:
          r = g = b = a = 0;
          break;
      }

//      *out++ = IRGB(r,g,b,a);
      *out++ = IRGB(r,g,b,a);
    }

    out += (mod_width - width);
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return image;
}

/* Saves an image to an open file descriptor (16-bit PNG)*/
int pspImageSavePngFd(FILE *fp, const PspImage* image)
{
  unsigned char *bitmap;
  int i, j, width, height;

  width = image->Viewport.Width;
  height = image->Viewport.Height;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    const unsigned char *pixel;
    pixel = (unsigned char*)image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport */
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(image->Palette[*pixel]);
      }
      /* Skip to the end of the line */
      pixel += image->Width - (image->Viewport.X + width);
    }
  }
  else
  {
    const unsigned short *pixel;
    pixel = (unsigned short*)image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport */
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(*pixel);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(*pixel);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(*pixel);
      }
      /* Skip to the end of the line */
      pixel += image->Width - (image->Viewport.X + width);
    }
  }

  png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL );

  if (!pPngStruct)
  {
    free(bitmap);
    return 0;
  }

  png_info *pPngInfo = png_create_info_struct( pPngStruct );
  if (!pPngInfo)
  {
    png_destroy_write_struct( &pPngStruct, NULL );
    free(bitmap);
    return 0;
  }

  png_byte **buf = (png_byte**)malloc(height * sizeof(png_byte*));
  if (!buf)
  {
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    free(bitmap);
    return 0;
  }

  unsigned int y;
  for (y = 0; y < height; y++)
    buf[y] = (byte*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, fp );
  png_set_IHDR( pPngStruct, pPngInfo, width, height, 8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info( pPngStruct, pPngInfo );
  png_write_image( pPngStruct, buf );
  png_write_end( pPngStruct, pPngInfo );

  png_destroy_write_struct( &pPngStruct, &pPngInfo );
  free(buf);
  free(bitmap);

  return 1;
}

int FindPowerOfTwoLargerThan(int n)
{
  int i;
  for (i = n; i < n; i *= 2);
  return i;
}

int FindPowerOfTwoLargerThan2(int n)
{
  int i;
  for (i = 1; i < n; i *= 2);
  return i;
}
