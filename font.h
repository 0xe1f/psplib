/* psplib/font.h: Rudimentary bitmap font implementation (legacy version)
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

#ifndef _PSP_FONT_H
#define _PSP_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_CHAR_ANALUP    "\251"
#define PSP_CHAR_ANALDOWN  "\252"
#define PSP_CHAR_ANALLEFT  "\253"
#define PSP_CHAR_ANALRIGHT "\254"
#define PSP_CHAR_UP        "\245"
#define PSP_CHAR_DOWN      "\246"
#define PSP_CHAR_LEFT      "\247"
#define PSP_CHAR_RIGHT     "\250"
#define PSP_CHAR_SQUARE    "\244"
#define PSP_CHAR_CROSS     "\241"
#define PSP_CHAR_CIRCLE    "\242"
#define PSP_CHAR_TRIANGLE  "\243"
#define PSP_CHAR_LTRIGGER  "\255"
#define PSP_CHAR_RTRIGGER  "\256"
#define PSP_CHAR_SELECT    "\257\260"
#define PSP_CHAR_START     "\261\262"

#define PSP_CHAR_UP_ARROW    "\272"
#define PSP_CHAR_DOWN_ARROW  "\273"

#define PSP_CHAR_POWER      "\267"
#define PSP_CHAR_EMPTY_BATT "\266"
#define PSP_CHAR_FULL_BATT  "\263"
#define PSP_CHAR_FLOPPY     "\274"
#define PSP_CHAR_TAPE       "\275"

#define PSP_CHAR_MS         "\271"

#define PSP_FONT_RESTORE 020
#define PSP_FONT_BLACK   021
#define PSP_FONT_RED     022
#define PSP_FONT_GREEN   023
#define PSP_FONT_BLUE    024
#define PSP_FONT_GRAY    025
#define PSP_FONT_YELLOW  026
#define PSP_FONT_MAGENTA 027
#define PSP_FONT_WHITE   030

struct PspFont
{
  unsigned char Height;
  unsigned char Ascent;
  struct
  {
    unsigned char Width;
    unsigned short *Char;
  } Chars[256];
};

typedef struct PspFont PspFont;

extern const PspFont PspStockFont;

int pspFontGetLineHeight(const PspFont *font);
int pspFontGetTextWidth(const PspFont *font, const char *string);
int pspFontGetTextHeight(const PspFont *font, const char *string);


#ifdef __cplusplus
}
#endif

#endif  // _PSP_FONT_H
