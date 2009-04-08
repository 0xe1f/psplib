/* psplib/pl_image.h: Image manipulation/saving/loading
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

#include "pl_image.h"
#include "pl_file.h"

#ifdef PSP
#include "pl_gfx.h"
#endif

static uint get_next_power_of_two(uint n);
static uint get_bitmap_size(const pl_image *image);
int copy_from_void(pl_image_format format,
                   const void *from,
                   uint32_t *to);
int copy_to_void(pl_image_format format,
                 uint32_t from,
                 void *to);

int pl_image_create(pl_image *image,
                    uint width,
                    uint height,
                    pl_image_format format,
                    uint8_t flags)
{
  uint bytes_per_pixel = pl_image_get_bytes_per_pixel(format);
  if (!bytes_per_pixel)
    return 0;

  /* Allocate memory */
  uint line_width = get_next_power_of_two(width);
  uint pitch = line_width * bytes_per_pixel;
  uint buf_len = pitch * height;
  void *buffer = NULL;

#ifdef PSP
  if (flags & PL_IMAGE_USE_VRAM) /* use VRAM */
    buffer = pl_gfx_vram_alloc(buf_len);
  else   /* use heap */
#endif
    buffer = memalign(16, buf_len);

  if (!buffer) return 0;

  memset(buffer, 0, buf_len);

  /* Init. structure */
  image->view.x = image->view.y = 0;
  image->view.w = width;
  image->view.h = height;
  image->height = height;
  image->line_width = line_width;
  image->pitch  = pitch;
  image->format = format;
  image->flags  = flags;
  image->bitmap = buffer;

  image->palette.palette = NULL;
  image->palette.colors = 0;
  image->palette.format = 0;

  return 1;
}

void pl_image_destroy(pl_image *image)
{
  /* Release bitmap */
  if (!(image->flags & PL_IMAGE_USE_VRAM))
    free(image->bitmap);

  /* Release palette */
  if (image->palette.palette)
    free(image->palette.palette);
}

int pl_image_palettize(pl_image *image,
                       pl_image_format pal_format,
                       uint pal_colors)
{
  if ((image->format != pl_image_indexed) || /* palette only valid for indexed images */
      (pal_colors == 0) ||              /* need at least 1 color */
      (pal_colors & 15) ||              /* number of colors must be divisible by 16 */
      (pal_format == pl_image_indexed)) /* 'indexed' not valid for palette format */
    return 0;

  /* Allocate pal. space */
  uint bytes_per_pixel = pl_image_get_bytes_per_pixel(pal_format);
  void *palette;
  if (!(palette = memalign(16, bytes_per_pixel * pal_colors)))
    return 0;

  /* Release current pal. */
  if (image->palette.palette)
    free(image->palette.palette);

  /* Reset image pal. */
  image->palette.format  = pal_format;
  image->palette.colors  = pal_colors;
  image->palette.palette = palette;

  return 1;
}

int pl_image_set_palette_color(pl_image *image,
                               uint index,
                               uint32_t color)
{
  if (image->format != pl_image_indexed ||
      index > image->palette.colors)
    return 0;

  switch (image->palette.format)
  {
  case pl_image_4444:
  case pl_image_5551:
    ((uint16_t*)(image->palette.palette))[index] = color & 0xffff;
    return 1;
  default:
    return 0;
  }
}

int pl_image_create_duplicate(const pl_image *original,
                              pl_image *copy)
{
  /* create image */
  if (!pl_image_create(copy,
                       original->line_width,
                       original->height,
                       original->format,
                       0)) /* TODO: all but vram flag */
    return 0;

  /* copy palette */
  if (original->palette.palette)
  {
    if (!pl_image_palettize(copy,
                            original->palette.format,
                            original->palette.colors))
    {
      pl_image_destroy(copy);
      return 0;
    }

    uint pal_size = copy->palette.colors *
                    pl_image_get_bytes_per_pixel(copy->palette.format);
    memcpy(copy->palette.palette,
           original->palette.palette,
           pal_size);
  }

  /* copy image */
  memcpy(copy->bitmap,
         original->bitmap,
         get_bitmap_size(copy));

  /* copy misc. attributes */
  copy->view = original->view;

  return 1;
}

int pl_image_compose_color(pl_image_format dest_format,
                           uint32_t *color,
                           uint8_t red,
                           uint8_t green,
                           uint8_t blue,
                           uint8_t alpha)
{
  switch (dest_format)
  {
  case pl_image_indexed:
    /* TODO: indexed color? */
    return 0;
  case pl_image_4444:
    *color = (((alpha >> 4) & 0x0F) << 12) |
             (((blue  >> 4) & 0x0F) << 8) |
             (((green >> 4) & 0x0F) << 4) |
                ((red >> 4) & 0x0F);
    return 1;
  case pl_image_5551:
    *color = (((alpha >> 7) & 0x01) << 15) |
             (((blue  >> 3) & 0x1F) << 10) |
             (((green >> 3) & 0x1F) << 5) |
                ((red >> 3) & 0x1F);
    return 1;
  default:
    return 0;
  }
}

int pl_image_split_color(pl_image_format src_format,
                         const pl_image_palette *src_palette,
                         uint32_t color,
                         uint8_t *red,
                         uint8_t *green,
                         uint8_t *blue,
                         uint8_t *alpha)
{
  switch (src_format)
  {
  case pl_image_indexed:
    /* Palette's format may not be indexed */
    if (src_palette->format == pl_image_indexed)
      return 0;

    /* Get actual color */
    copy_from_void(src_palette->format,
                   src_palette->palette + color *
                     pl_image_get_bytes_per_pixel(src_palette->format),
                   &color);

    /* Split color */
    return pl_image_split_color(src_palette->format,
                                NULL,
                                color,
                                red,
                                green,
                                blue,
                                alpha);
  case pl_image_4444:
    *red   = (color & 0x0F) * 0xFF/0x0F;
    *green = ((color >> 4) & 0x0F) * 0xFF/0x0F;
    *blue  = ((color >> 8) & 0x0F) * 0xFF/0x0F;
    *alpha = ((color >> 12) & 0x0F) * 0xFF/0x0F;
    return 1;
  case pl_image_5551:
    *red   = (color & 0x1F) * 0xFF/0x1F;
    *green = ((color >> 5) & 0x1F) * 0xFF/0x1F;
    *blue  = ((color >> 10) & 0x1F) * 0xFF/0x1F;
    *alpha = ((color >> 15) & 0x01) * 0xFF/0x01;
    return 1;
  default:
    return 0;
  }
}

int pl_image_load_png_stream(pl_image *image,
                             FILE *stream)
{
  /* hardcoded for now */
  pl_image_format format = pl_image_5551;

  const size_t nSigSize = 8;
  uint8_t signature[nSigSize];
  if (fread(signature, sizeof(uint8_t), nSigSize, stream) != nSigSize)
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

  png_init_io(pPngStruct, stream);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  if (!pl_image_create(image,
                       width,
                       height,
                       format,
                       0))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  uint8_t r, g, b, a;
  uint32_t color;
  int bytes_per_pixel = pl_image_get_bytes_per_pixel(format);
  void *line_ptr, *pel_ptr;

  for (y = 0, line_ptr = image->bitmap;
       y < height;
       y++, line_ptr += image->pitch)
  {
    png_byte *pRow = pRowTable[y];

    for (x = 0, pel_ptr = line_ptr;
         x < width;
         x++, pel_ptr += bytes_per_pixel)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          a = 0xff;
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
          a = 0xff;
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

      pl_image_compose_color(format, &color, r, g, b, a);
      copy_to_void(format, color, pel_ptr);
    }
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return 1;
}

int pl_image_save_png_stream(const pl_image *image,
                             FILE *stream)
{
  unsigned char *bitmap;
  int i, j;
  uint8_t r, g, b, a;
  int width = image->view.w;
  int height = image->view.h;
  int bytes_per_pixel = pl_image_get_bytes_per_pixel(image->format);
  void *line_ptr, *pel_ptr;
  uint32_t color;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  for (i = 0, line_ptr = image->bitmap + (image->view.y * image->pitch);
       i < height;
       i++, line_ptr += image->pitch)
  {
    /* Skip to the start of the viewport */
    for (j = 0, pel_ptr = line_ptr + (image->view.x * bytes_per_pixel);
         j < width;
         j++, pel_ptr += bytes_per_pixel)
    {
      copy_from_void(image->format, pel_ptr, &color);
      pl_image_split_color(image->format,
                           &image->palette,
                           color,
                           &r,&g,&b,&a);

      bitmap[i * width * 3 + j * 3 + 0] = r;
      bitmap[i * width * 3 + j * 3 + 1] = g;
      bitmap[i * width * 3 + j * 3 + 2] = b;
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
    buf[y] = (uint8_t*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, stream );
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

int pl_image_load(pl_image *image,
                  const char *path)
{
  int status = 0;
  FILE *stream = NULL;

  if (pl_file_is_of_type(path, "png"))
  {
    if ((stream = fopen(path, "r")))
      status = pl_image_load_png_stream(image,
                                        stream);
  }
  else status = 0;

  if (stream) fclose(stream);
  return status;
}

int pl_image_save(const pl_image *image,
                  const char *path)
{
  int status = 0;
  FILE *stream = NULL;

  if (pl_file_is_of_type(path, "png"))
  {
    if ((stream = fopen(path, "w")))
      status = pl_image_save_png_stream(image,
                                        stream);
  }
  else status = 0;

  if (stream) fclose(stream);
  return status;
}

int pl_image_clear(pl_image *image,
                   uint8_t red,
                   uint8_t green,
                   uint8_t blue,
                   uint8_t alpha)
{
  uint32_t color;
  int x, y;
  void *line_ptr, *pel_ptr;
  uint bytes_per_pixel = pl_image_get_bytes_per_pixel(image->format);

  if (!pl_image_compose_color(image->format,
                              &color,
                              red,
                              green,
                              blue,
                              alpha))
    return 0;

  for (y = 0, line_ptr = image->bitmap;
       y < image->height;
       y++, line_ptr += image->pitch)
  {
    for (x = 0, pel_ptr = line_ptr;
         x < image->line_width;
         x++, pel_ptr += bytes_per_pixel)
    {
      copy_to_void(image->format,
                   color,
                   pel_ptr);
    }
  }

  return 1;
}

int pl_image_create_thumbnail(const pl_image *original,
                              pl_image *thumb)
{
  /* create image */
  if (!pl_image_create(thumb,
                       original->view.w / 2,
                       original->view.h / 2,
                       original->format,
                       0)) /* TODO: all but vram flag */
    return 0;

  /* copy palette */
  if (original->format == pl_image_indexed &&
      original->palette.palette)
  {
    if (!pl_image_palettize(thumb,
                            original->palette.format,
                            original->palette.colors))
    {
      pl_image_destroy(thumb);
      return 0;
    }

    uint pal_size = thumb->palette.colors *
                    pl_image_get_bytes_per_pixel(thumb->palette.format);
    memcpy(thumb->palette.palette,
           original->palette.palette,
           pal_size);
  }

  int x, y;
  uint32_t color;
  uint bytes_per_pixel =
    pl_image_get_bytes_per_pixel(original->format);
  void *slp = original->bitmap + original->view.y * original->pitch;
  void *spp;
  void *dlp = thumb->bitmap;
  void *dpp;

  /* copy bitmap */
  for (y = 0;
       y < thumb->view.h;
       y++, dlp += thumb->pitch, slp += (original->pitch << 1))
  {
    for (x = 0, dpp = dlp, spp = slp + original->view.x * bytes_per_pixel;
         x < thumb->view.w;
         x++, dpp += bytes_per_pixel, spp += (bytes_per_pixel << 1))
    {
      copy_from_void(original->format, spp, &color);
      copy_to_void(thumb->format, color, dpp);
    }
  }

  return 1;
}

#if 0
int pl_image_rotate(const pl_image *original,
                    pl_image *rotated,
                    int angle_cw)
{
  int status = 0;

  /* Create image */
  switch(angle_cw)
  {
  case 0:   /* just duplicate */
    return pl_image_create_duplicate(original,
                                     rotated);
  case 90:
  case 270:
    status = pl_image_create(rotated,
                             original->view.h,
                             original->view.w,
                             original->format,
                             0);
    break;
  case 180:
    status = pl_image_create(rotated,
                             original->view.w,
                             original->view.h,
                             original->format,
                             0);
    break;
  default:
    return 0;
  }

  if (!status) return 0;

  /* copy palette */
  if (original->format == pl_image_indexed &&
      original->palette.palette)
  {
    if (!pl_image_palettize(rotated,
                            original->palette.format,
                            original->palette.colors))
    {
      pl_image_destroy(rotated);
      return 0;
    }

    uint pal_size = rotated->palette.colors *
                    pl_image_get_bytes_per_pixel(rotated->palette.format);
    memcpy(rotated->palette.palette,
           original->palette.palette,
           pal_size);
  }

  int x, y;
  uint32_t color;
  uint bytes_per_pixel =
    pl_image_get_bytes_per_pixel(original->format);
  void *slp = original->bitmap + original->view.y * original->pitch;
  void *spp, *dpp;

  /* copy bitmap */
  for (y = 0;
       y < original->view.h;
       y++, slp += original->pitch)
  {
    for (x = 0, spp = slp + original->view.x * bytes_per_pixel;
         x < original->view.w;
         x++, spp += bytes_per_pixel)
    {
      copy_from_void(original->format, spp, &color);
      switch(angle_cw)
      {
      case 90:
/*
        dpp = rotated->bitmap + (rotated->line_width - (k / rotated->height) - 1) *
              rotated->pitch;
        dpp += (k % rotated->height) * rotated->pitch;
*/
//        di = (width - (k / height) - 1) + (k % height) * width;
        break;
      case 180:
/*
(height - i - 1) * width + 
(width - j - 1);
*/
        dpp = rotated->bitmap + (rotated->height - y - 1) * rotated->pitch;
        dpp += (rotated->line_width - x - 1) * bytes_per_pixel;
        break;
      case 270:
//        di = (k / height) + (height - (k % height) - 1) * width;
        break;
      }
      copy_to_void(rotated->format, color, dpp);
    }
  }

  return 1;
}
#endif

static uint get_next_power_of_two(uint n)
{
  uint i;
  for (i = 0x10; i < n; i <<= 1);
  return i;
}

static uint get_bitmap_size(const pl_image *image)
{
  return image->pitch * image->height;
}

int copy_from_void(pl_image_format format,
                   const void *from,
                   uint32_t *to)
{
  switch (pl_image_get_bytes_per_pixel(format))
  {
  case 1:
    *to = *(uint8_t*)from;
    return 1;
  case 2:
    *to = *(uint16_t*)from;
    return 1;
  case 4:
    *to = *(uint32_t*)from;
  default:
    return 0;
  }
}

int copy_to_void(pl_image_format format,
                 uint32_t from,
                 void *to)
{
  switch (pl_image_get_bytes_per_pixel(format))
  {
  case 1:
    *(uint8_t*)to = (uint8_t)from;
    return 1;
  case 2:
    *(uint16_t*)to = (uint16_t)from;
    return 1;
  case 4:
    *(uint32_t*)to = (uint32_t)from;
    return 1;
  default:
    return 0;
  }
}
