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
