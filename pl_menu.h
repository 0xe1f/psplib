/* psplib/pl_menu.h: Simple menu implementation
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

#ifndef _PL_MENU_H
#define _PL_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pl_menu_option_t
{
  char *text;
  const void *value;
  struct pl_menu_option_t *prev;
  struct pl_menu_option_t *next;
} pl_menu_option;

typedef struct pl_menu_item_t
{
  unsigned int id;
  char *caption;
  char *help_text;
  const void *param;
  struct pl_menu_option_t *selected;
  struct pl_menu_option_t *options;
  struct pl_menu_item_t *prev;
  struct pl_menu_item_t *next;
} pl_menu_item;

typedef struct pl_menu_t
{
  struct pl_menu_item_t *selected;
  struct pl_menu_item_t *items;
} pl_menu;

#define PL_MENU_ITEMS_BEGIN(ident) \
        pl_menu_def ident[] = {
#define PL_MENU_HEADER(text) \
        {0,"\t"text,NULL,NULL},
#define PL_MENU_ITEM(caption,id,option_list,help) \
        {id,caption,help,option_list},
#define PL_MENU_ITEMS_END \
        {0,NULL,NULL,NULL}};

#define PL_MENU_OPTIONS_BEGIN(ident) \
        pl_menu_option_def ident[] = {
#define PL_MENU_OPTION(text,param) \
        {text,(const void*)(param)},
#define PL_MENU_OPTIONS_END \
        {NULL,NULL}};

typedef struct pl_menu_option_def_t
{
  const char *text;
  const void *value;
} pl_menu_option_def;

typedef struct pl_menu_def_t
{
  unsigned int id;
  const char *caption;
  const char *help_text;
  pl_menu_option_def *options;
} pl_menu_def;

int  pl_menu_create(pl_menu *menu,
                    const pl_menu_def *def);
void pl_menu_destroy(pl_menu *menu);
void pl_menu_clear_items(pl_menu *menu);
pl_menu_item*
     pl_menu_append_item(pl_menu *menu,
                         unsigned int id,
                         const char *caption);
pl_menu_item*
     pl_menu_find_item_by_index(const pl_menu *menu,
                                int index);
pl_menu_item*
     pl_menu_find_item_by_id(const pl_menu *menu,
                             unsigned int id);
int  pl_menu_remove_item(pl_menu *menu,
                      pl_menu_item *which);
int  pl_menu_get_item_count(const pl_menu *menu);
int  pl_menu_set_item_caption(pl_menu_item *item,
                              const char *caption);
int  pl_menu_set_item_help_text(pl_menu_item *item,
                                const char *help_text);
void pl_menu_clear_options(pl_menu_item *item);
pl_menu_option*
     pl_menu_append_option(pl_menu_item *item,
                           const char *text,
                           const void *value,
                           int select);
int  pl_menu_get_option_count(pl_menu_item *item);
pl_menu_option*
     pl_menu_find_option_by_index(const pl_menu_item *item,
                                  int index);
pl_menu_option*
     pl_menu_find_option_by_value(const pl_menu_item *item,
                                  const void *value);
pl_menu_option*
     pl_menu_select_option_by_index(pl_menu_item *item,
                                    int index);
pl_menu_option*
     pl_menu_select_option_by_value(pl_menu_item *item,
                                    const void *value);
int  pl_menu_update_option(pl_menu_option *option,
                           const char *text,
                           const void *value);

#ifdef __cplusplus
}
#endif

#endif  // _PL_MENU_H
