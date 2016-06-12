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

