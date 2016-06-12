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
