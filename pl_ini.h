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

#ifndef _PL_INI_H
#define _PL_INI_H

#ifdef __cplusplus
extern "C" {
#endif

struct pl_ini_section_t;

typedef struct pl_ini_file_t
{
  struct pl_ini_section_t *head;
} pl_ini_file;

int  pl_ini_create(pl_ini_file *file);
void pl_ini_destroy(pl_ini_file *file);
int  pl_ini_load(pl_ini_file *file,
                 const char *path);
int  pl_ini_save(const pl_ini_file *file,
                 const char *path);
int  pl_ini_get_int(const pl_ini_file *file,
                    const char *section,
                    const char *key,
                    const int default_value);
void pl_ini_set_int(pl_ini_file *file,
                    const char *section,
                    const char *key,
                    int value);
int  pl_ini_get_string(const pl_ini_file *file,
                       const char *section,
                       const char *key,
                       const char *default_value,
                       char *copy_to,
                       int dest_len);
void pl_ini_set_string(pl_ini_file *file,
                       const char *section,
                       const char *key,
                       const char *string);

#ifdef __cplusplus
}
#endif

#endif // _PL_INI_H
