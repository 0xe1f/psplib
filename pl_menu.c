/* psplib/pl_menu.c: Simple menu implementation
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

#include "pl_menu.h"

static void
  destroy_item(pl_menu_item *item);
static pl_menu_item*
  find_last_item(const pl_menu *menu);
static pl_menu_option*
  find_last_option(const pl_menu_item *item);

int pl_menu_create(pl_menu *menu,
                   const pl_menu_def *def)
{
  menu->items = NULL;
  menu->selected = NULL;

  /* No definition; nothing more to do */
  if (!def) return 1;

  pl_menu_item *item;

  /* Initialize menu */
  for (; def->id || def->caption; def++)
  {
    /* Append the item */
    item = pl_menu_append_item(menu,
                               def->id,
                               def->caption);

    if (item)
    {
      /* Add the options */
      if (def->options)
      {
        const pl_menu_option_def *option_def;
        for (option_def = def->options; option_def->text; option_def++)
          pl_menu_append_option(item,
                                option_def->text,
                                option_def->value, 0);
      }

      /* Set help text */
      item->help_text = (def->help_text)
                        ? strdup(def->help_text) : NULL;
    }
  }

  return 1;
}

void pl_menu_destroy(pl_menu *menu)
{
  pl_menu_clear_items(menu);
}

void pl_menu_clear_items(pl_menu *menu)
{
  pl_menu_item *item, *next;

  for (item = menu->items; item; item = next)
  {
    next = item->next;
    destroy_item(item);
  }

  menu->items = NULL;
  menu->selected = NULL;
}

int pl_menu_remove_item(pl_menu *menu,
                        pl_menu_item *which)
{
  pl_menu_item *item;
  int found = 0;

  /* Make sure the item is in the menu */
  for (item = menu->items; item; item = item->next)
    if (item == which)
    { found = 1; break; }
  if (!found) return 0;

  /* Redirect pointers */
  if (item->prev)
    item->prev->next = item->next;
  else
    menu->items = item->next;

  if (item->next)
    item->next->prev = item->prev;

  if (menu->selected == item)
    menu->selected = item->next;

  /* Destroy the item */
  destroy_item(item);

  return 1;
}

static void destroy_item(pl_menu_item *item)
{
  if (item->caption)
    free(item->caption);
  if (item->help_text)
    free(item->help_text);

  pl_menu_clear_options(item);
  free(item);
}

void pl_menu_clear_options(pl_menu_item *item)
{
  pl_menu_option *option, *next;

  for (option = item->options; option; option = next)
  {
    next = option->next;
    free(option->text);
    free(option);
  }

  item->selected = NULL;
  item->options = NULL;
}

static pl_menu_item* find_last_item(const pl_menu *menu)
{
  if (!menu->items)
    return NULL;

  pl_menu_item *item;
  for (item = menu->items; item->next; item = item->next);

  return item;
}

static pl_menu_option* find_last_option(const pl_menu_item *item)
{
  if (!item->options)
    return NULL;

  pl_menu_option *option;
  for (option = item->options; option->next; option = option->next);

  return option;
}

pl_menu_item* pl_menu_append_item(pl_menu *menu,
                                  unsigned int id,
                                  const char *caption)
{
  pl_menu_item* item = (pl_menu_item*)malloc(sizeof(pl_menu_item));
  if (!item) return NULL;

  if (!caption)
    item->caption = NULL;
  else
  {
    if (!(item->caption = strdup(caption)))
    {
      free(item);
      return NULL;
    }
  }

  pl_menu_item *last = find_last_item(menu);
  item->help_text = NULL;
  item->id = id;
  item->param = NULL;
  item->options = NULL;
  item->selected = NULL;
  item->next = NULL;
  item->prev = last;

  if (last)
    last->next = item;
  else
    menu->items = item;

  return item;
}

pl_menu_item* pl_menu_find_item_by_index(const pl_menu *menu,
                                         int index)
{
  pl_menu_item *item;
  int i = 0;

  for (item = menu->items; item; item = item->next, i++)
    if (i == index)
      return item;

  return NULL;
}

pl_menu_item* pl_menu_find_item_by_id(const pl_menu *menu,
                                      unsigned int id)
{
  pl_menu_item *item;
  for (item = menu->items; item; item = item->next)
    if (item->id == id)
      return item;

  return NULL;
}

int pl_menu_get_item_count(const pl_menu *menu)
{
  int i = 0;
  pl_menu_item *item = menu->items;
  for (; item; item = item->next) i++;
  return i;
}

pl_menu_option* pl_menu_append_option(pl_menu_item *item,
                                      const char *text,
                                      const void *value,
                                      int select)
{
  pl_menu_option *option;
  if (!(option = (pl_menu_option*)malloc(sizeof(pl_menu_option))))
    return NULL;

  if (!(option->text = strdup(text)))
  {
    free(option);
    return NULL;
  }

  option->value = value;
  option->next = NULL;

  if (item->options)
  {
    pl_menu_option *last = find_last_option(item);
    last->next = option;
    option->prev = last;
  }
  else
  {
    item->options = option;
    option->prev = NULL;
  }

  if (select)
    item->selected = option;

  return option;
}

int pl_menu_get_option_count(pl_menu_item *item)
{
  int i;
  pl_menu_option *option;
  for (i = 0, option = item->options; option; i++, option = option->next);
  return i;
}

pl_menu_option* pl_menu_find_option_by_index(const pl_menu_item *item,
                                             int index)
{
  int i;
  pl_menu_option *option;

  for (i = 0, option = item->options;
       option && (i <= index);
       i++, option = option->next)
    if (i == index)
      return option;

  return NULL;
}

pl_menu_option* pl_menu_find_option_by_value(const pl_menu_item *item,
                                             const void *value)
{
  pl_menu_option *option;
  for (option = item->options; option; option = option->next)
    if (option->value == value)
      return option;

  return NULL;
}

pl_menu_option* pl_menu_select_option_by_index(pl_menu_item *item,
                                               int index)
{
  pl_menu_option *option = pl_menu_find_option_by_index(item,
                                                        index);
  if (!option)
    return NULL;

  return (item->selected = option);
}

pl_menu_option* pl_menu_select_option_by_value(pl_menu_item *item,
                                               const void *value)
{
  pl_menu_option *option = pl_menu_find_option_by_value(item,
                                                        value);
  if (!option)
    return NULL;

  return (item->selected = option);
}

int pl_menu_update_option(pl_menu_option *option,
                          const char *text,
                          const void *value)
{
  if (option->text)
    free(option->text);

  option->text = (text) ? strdup(text) : NULL;
  option->value = value;

  return (!text || option->text);
}

int pl_menu_set_item_caption(pl_menu_item *item,
                             const char *caption)
{
  if (item->caption)
    free(item->caption);
  item->caption = (caption) ? strdup(caption) : NULL;
  return (!caption || item->caption);
}

int pl_menu_set_item_help_text(pl_menu_item *item,
                               const char *help_text)
{
  if (item->help_text)
    free(item->help_text);
  item->help_text = (help_text) ? strdup(help_text) : NULL;
  return (!help_text || item->help_text);
}
