/* psplib/ui.h: Simple user interface implementation (legacy version)
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

#ifndef _PSP_UI_H
#define _PSP_UI_H

#include "video.h"
#include "pl_menu.h"
#include "adhoc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PspUiMetric
{
  const PspImage *Background;
  const PspFont *Font;
  u64 CancelButton;
  u64 OkButton;
  int Left;
  int Top;
  int Right;
  int Bottom;
  u32 ScrollbarColor;
  u32 ScrollbarBgColor;
  int ScrollbarWidth;
  u32 TextColor;
  u32 SelectedColor;
  u32 SelectedBgColor;
  u32 StatusBarColor;
  int MenuFps;

  u32 DialogFogColor;

  u32 BrowserFileColor;
  u32 BrowserDirectoryColor;
  u32 BrowserScreenshotDelay;
  const char *BrowserScreenshotPath;

  int GalleryIconsPerRow;
  int GalleryIconMarginWidth;

  int MenuItemMargin;
  u32 MenuSelOptionBg;
  u32 MenuOptionBoxColor;
  u32 MenuOptionBoxBg;
  u32 MenuDecorColor;

  int TitlePadding;
  u32 TitleColor;
  u32 TabBgColor;
  int Animate;
} PspUiMetric;

typedef struct PspUiFileBrowser
{
  void (*OnRender)(const void *browser, const void *path);
  int  (*OnOk)(const void *browser, const void *file);
  int  (*OnCancel)(const void *gallery, const void *parent_dir);
  int  (*OnButtonPress)(const struct PspUiFileBrowser* browser, 
         const char *selected, u32 button_mask);
  const char **Filter;
  void *Userdata;
} PspUiFileBrowser;

typedef struct PspUiMenu
{
  void (*OnRender)(const void *uimenu, const void *item);
  int  (*OnOk)(const void *menu, const void *item);
  int  (*OnCancel)(const void *menu, const void *item);
  int  (*OnButtonPress)(const struct PspUiMenu *menu, pl_menu_item *item, 
         u32 button_mask);
  int  (*OnItemChanged)(const struct PspUiMenu *menu, pl_menu_item *item, 
         const pl_menu_option *option);
  pl_menu Menu;
} PspUiMenu;

typedef struct PspUiGallery
{
  void (*OnRender)(const void *gallery, const void *item);
  int  (*OnOk)(const void *gallery, const void *item);
  int  (*OnCancel)(const void *gallery, const void *item);
  int  (*OnButtonPress)(const struct PspUiGallery *gallery, pl_menu_item* item, 
          u32 button_mask);
  void *Userdata;
  pl_menu Menu;
} PspUiGallery;

typedef struct PspUiSplash
{
  void (*OnRender)(const void *splash, const void *null);
  int  (*OnCancel)(const void *splash, const void *null);
  int  (*OnButtonPress)(const struct PspUiSplash *splash, u32 button_mask);
  const char* (*OnGetStatusBarText)(const struct PspUiSplash *splash);
} PspUiSplash;

#define PSP_UI_YES     2
#define PSP_UI_NO      1
#define PSP_UI_CANCEL  0

#define PSP_UI_CONFIRM 1

char pspUiGetButtonIcon(u32 button_mask);

void pspUiOpenBrowser(PspUiFileBrowser *browser, const char *start_path);
void pspUiOpenGallery(PspUiGallery *gallery, const char *title);
void pspUiOpenMenu(PspUiMenu *uimenu, const char *title);
void pspUiSplashScreen(PspUiSplash *splash);

int pspUiAdhocHost(const char *name, PspMAC mac);
int pspUiAdhocJoin(PspMAC mac);

int  pspUiConfirm(const char *message);
int  pspUiYesNoCancel(const char *message);
void pspUiAlert(const char *message);
void pspUiFlashMessage(const char *message);
const pl_menu_item* pspUiSelect(const char *title, const pl_menu *menu);

void pspUiFadeout();

PspUiMetric UiMetric;

#ifdef __cplusplus
}
#endif

#endif  // _PSP_UI_H
