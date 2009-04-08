/* psplib/font.c: Rudimentary bitmap font implementation (legacy version)
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

#include "font.h"
#include "stockfont.h"

int pspFontGetLineHeight(const PspFont *font)
{
  return font->Height;
}

int pspFontGetTextWidth(const PspFont *font, const char *string)
{
  const unsigned char *ch;
  int width, max, w;

  for (ch = (unsigned char*)string, width = 0, max = 0; *ch; ch++)
  {
    /* Tab = 4 spaces (TODO) */
    if (*ch == '\t') w = font->Chars[(unsigned char)' '].Width * 4; 
    /* Newline */
    else if (*ch == '\n') width = w = 0;
    /* Special char */
    else if (*ch < 32) w = 0; 
    /* Normal char */
    else w = font->Chars[(unsigned char)(*ch)].Width;

    width += w;
    if (width > max) max = width;
  }

  return max;
}

int pspFontGetTextHeight(const PspFont *font, const char *string)
{
  const char *ch;
  int lines;

  for (ch = string, lines = 1; *ch; ch++)
    if (*ch == '\n') lines++;

  return lines * font->Height;
}

