/* psplib/pl_ui.h: Simple user interface implementation
   Copyright (C) 2009 Akop Karapetyan

   $Id: ui.h 409 2009-04-08 20:01:39Z jack $

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

