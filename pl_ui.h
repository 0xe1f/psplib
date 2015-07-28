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

#ifndef _PL_UI_H
#define _PL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pl_gfx.h"
#include "ctrl.h"

enum pl_ui_object_type
{
  PL_UI_FILE_VIEW,
  PL_UI_ICON_VIEW,
  PL_UI_MENU
};

typedef struct pl_ui_metrics_t
{
  pl_button ok_button;
  pl_button cancel_button;

  pl_color scrollbar_fg;
  pl_color scrollbar_bg;
  pl_color statusbar_fg;

  pl_color text_fg;
  pl_color selected_fg;
  pl_color selected_bg;

  pl_color tab_fg;
  pl_color tab_bg;

  pl_color fog_bg;

  int      scrollbar_width;
} pl_ui_metrics;

typedef struct pl_ui_file_view_metrics_t
{
  pl_color directory_fg;
  int      screenshot_delay;
  char    *screenshot_path;
} pl_ui_file_view_metrics;

typedef struct pl_ui_icon_view_metrics_t
{
  int icons_per_row;
  int margin_width;
} pl_ui_icon_view_metrics;

typedef struct pl_ui_menu_metrics_t
{
  pl_color option_window_fg;
  pl_color option_window_bg;
  pl_color option_selected_bg;
  int      item_margin;
} pl_ui_menu_metrics;

typedef struct pl_ui_tab_t
{
  char *tab_name;
  enum pl_ui_object_type type;
  void *ui_object;

  struct pl_ui_tab_t *previous;
  struct pl_ui_tab_t *next;
} pl_ui_tab;

typedef struct pl_ui_t
{
  pl_ui_tab *first_tab;
} pl_ui;

int  pl_ui_create(pl_ui *ui);
void pl_ui_destroy(pl_ui *ui);

#endif  // _PL_UI_H

