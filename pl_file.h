/* psplib/pl_file.h: File and directory query routines
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

#ifndef _PL_FILE_H
#define _PL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PL_FILE_DIRECTORY     0x01
#define PL_FILE_MAX_PATH_LEN  1024

typedef char pl_file_path[PL_FILE_MAX_PATH_LEN];

typedef struct pl_file_t
{
  char *name;
  unsigned char attrs;
  struct pl_file_t *next;
} pl_file;

typedef struct pl_file_list_t
{
  struct pl_file_t *files;
} pl_file_list;

void pl_file_get_parent_directory(const char *path,
                                  char *parent,
                                  int length);
const char*
     pl_file_get_filename(const char *path);
const char*
     pl_file_get_extension(const char *path);
int  pl_file_get_file_size(const char *path);
int  pl_file_exists(const char *path);
int  pl_file_rm(const char *path);
int  pl_file_open_directory(const char *path,
                            const char *subdir,
                            char *result,
                            int  result_len);
int  pl_file_is_directory(const char *path);
int  pl_file_is_root_directory(const char *path);
int  pl_file_is_of_type(const char *path,
                        const char *extension);
int  pl_file_mkdir_recursive(const char *path);
/* Returns number of files successfully read; <0 if error */
int  pl_file_get_file_list(pl_file_list *list,
                           const char *path,
                           const char **filter);
void pl_file_destroy_file_list(pl_file_list *list);
int  pl_file_get_file_list_count(const pl_file_list *list);

#ifdef __cplusplus
}
#endif

#endif // _PL_FILE_H
