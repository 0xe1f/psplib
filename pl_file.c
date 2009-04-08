/* psplib/pl_file.c: File and directory query routines
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

#include <pspkernel.h>
#include <psptypes.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pl_file.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static void
  sort_file_list(pl_file_list *list,
                 int count);
static int
  compare_files_by_name(const void *s1, 
                        const void *s2);
static int 
  mkdir_recursive(const char *path);

void pl_file_get_parent_directory(const char *path,
                                  char *parent,
                                  int length)
{
  int pos = strlen(path) - 1,
      len;

  for (; pos >= 0 && path[pos] == '/'; pos--);
  for (; pos >= 0 && path[pos] != '/'; pos--);

  if (pos < 0)
  {
    len = MIN(strlen(path), length - 1);
    strncpy(parent, path, len);
    parent[len] = '\0';
    return;
  }

  len = MIN(pos + 1, length - 1);
  strncpy(parent, path, len);
  parent[len] = '\0';
}

const char* pl_file_get_filename(const char *path)
{
  const char *filename;
  if (!(filename = strrchr(path, '/')))
    return path;
  return filename + 1;
}

const char* pl_file_get_extension(const char *path)
{
  const char *filename = pl_file_get_filename(path);
  const char *ext;
  if (!(ext = strrchr(filename, '.')))
    return filename + strlen(filename);
  return ext + 1;
}

int pl_file_rm(const char *path)
{
  return sceIoRemove(path) >= 0;
}

/* Returns size of file in bytes or <0 if error */
int pl_file_get_file_size(const char *path)
{
  SceIoStat stat;
  memset(&stat, 0, sizeof(stat));
  if (sceIoGetstat(path, &stat) < 0)
    return -1;

  return (int)stat.st_size;
}

int pl_file_is_root_directory(const char *path)
{
  const char *pos = strchr(path, '/');
  return !pos || !(*(pos + 1));
}

int pl_file_exists(const char *path)
{
  SceIoStat stat;
  return sceIoGetstat(path, &stat) == 0;
}

int pl_file_is_of_type(const char *path,
                       const char *extension)
{
  int fn_len, ext_len;
  const char *file_ext;

  fn_len = strlen(path);
  ext_len = strlen(extension);

  /* Filename must be at least 2 chars longer (period + a char) */
  if (fn_len < ext_len + 2)
    return 0;

  file_ext = path + (fn_len - ext_len);
  if (*(file_ext - 1) == '.' && strcasecmp(file_ext, extension) == 0)
    return 1;

  return 0;
}

void pl_file_destroy_file_list(pl_file_list *list)
{
  pl_file *file, *next;

  for (file = list->files; file; file = next)
  {
    next = file->next;
    free(file->name);
    free(file);
  }
}

static int mkdir_recursive(const char *path)
{
  int exit_status = 1;
  SceIoStat stat;

  if (sceIoGetstat(path, &stat) == 0)
    /* If not a directory, cannot continue; otherwise success */
    return (stat.st_attr & FIO_SO_IFDIR);

  /* First, try creating its parent directory */
  char *slash_pos = strrchr(path, '/');
  if (!slash_pos); /* Top level */
  else if (slash_pos != path && slash_pos[-1] == ':'); /* Top level */
  else
  {
    char *parent = strdup(path);
    parent[slash_pos - path] = '\0';
    exit_status = mkdir_recursive(parent);

    free(parent);
  }

  if (exit_status && slash_pos[1] != '\0')
  {
    if (sceIoMkdir(path, 0777) != 0)
      exit_status = 0;
  }

  return exit_status;
}

int pl_file_mkdir_recursive(const char *path)
{
  return mkdir_recursive(path);
}

int pl_file_get_file_list_count(const pl_file_list *list)
{
  int count = 0;
  pl_file *file;

  for (file = list->files; file; file = file->next)
    count++;
  return count;
}

/* Returns number of files successfully read; negative number if error */
int pl_file_get_file_list(pl_file_list *list,
                          const char *path,
                          const char **filter)
{
  SceUID fd = sceIoDopen(path);
  if (fd < 0) return -1;

  SceIoDirent dir;
  memset(&dir, 0, sizeof(dir));

  pl_file *file, *last = NULL;
  list->files = NULL;

  const char **pext;
  int loop;
  int count = 0;

  while (sceIoDread(fd, &dir) > 0)
  {
    if (filter && !(dir.d_stat.st_attr & FIO_SO_IFDIR))
    {
      /* Loop through the list of allowed extensions and compare */
      for (pext = filter, loop = 1; *pext; pext++)
      {
        if (pl_file_is_of_type(dir.d_name, *pext))
        {
          loop = 0;
          break;
        }
      }

      if (loop) continue;
    }

    /* Create a new file entry */
    if (!(file = (pl_file*)malloc(sizeof(pl_file))))
    {
      pl_file_destroy_file_list(list);
      return -1;
    }

    file->name = strdup(dir.d_name);
    file->next = NULL;
    file->attrs = (dir.d_stat.st_attr & FIO_SO_IFDIR) 
                  ? PL_FILE_DIRECTORY : 0;

    /* Update preceding element */
    if (last) last->next = file;
    else list->files = file;

    last = file;
    count++;
  }

  sceIoDclose(fd);

  /* Sort the files by name */
  sort_file_list(list, count);
  return count;
}

static void sort_file_list(pl_file_list *list,
                           int count)
{
  pl_file **files, *file, **fp;
  int i;

  if (count < 1)
    return;

  /* Copy the file entries to an array */
  files = (pl_file**)malloc(sizeof(pl_file*) * count);
  for (file = list->files, fp = files; file; file = file->next, i++, fp++)
    *fp = file;

  /* Sort the array */
  qsort((void*)files, count, sizeof(pl_file*), compare_files_by_name);

  /* Rearrange the file entries in the list */
  list->files = files[0];
  list->files->next = NULL;

  for (i = 1; i < count; i++)
    files[i - 1]->next = files[i];

  pl_file *last = files[count - 1];
  last->next = NULL;
  free(files);
}

static int compare_files_by_name(const void *s1, const void *s2)
{
  pl_file *f1 = *(pl_file**)s1, *f2 = *(pl_file**)s2;
  if ((f1->attrs & PL_FILE_DIRECTORY) == (f2->attrs & PL_FILE_DIRECTORY))
    return strcasecmp(f1->name, f2->name);
  else if (f1->attrs & PL_FILE_DIRECTORY)
    return -1;
  else return 1;
}

int pl_file_open_directory(const char *path,
                           const char *subdir,
                           char *result,
                           int  result_len)
{
  /* This routine should be made more robust */
  /* to accept subdirs like ../../ etc... */
  int path_len = strlen(path);
  int copy_len;

  /* Ascend one level */
  if (strcmp(subdir, "..") == 0)
  {
    pl_file_get_parent_directory(path,
                                 result,
                                 result_len);
    return 1;
  }
  else
  {
    copy_len = MIN(result_len - 1, strlen(subdir) + path_len + 2);
    snprintf(result, copy_len, "%s%s/", path, subdir);
    result[copy_len] = '\0';
    return 1;
  }

  /* If we're here, then we couldn't figure out final path */
  /* Just copy the original path */
  copy_len = MIN(result_len - 1, path_len);
  strncpy(result, path, copy_len);
  result[copy_len] = '\0';

  return (strcmp(subdir, ".") == 0);
}

int pl_file_is_directory(const char *path)
{
  int len;
  if ((len = strlen(path)) < 1)
    return 0;
  return (path[len - 1] == '/');
}
