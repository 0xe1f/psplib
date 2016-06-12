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

#ifndef _PL_REWIND_H
#define _PL_REWIND_H

#ifdef __cplusplus
extern "C" {
#endif

struct rewind_state;

typedef struct
{
  int state_data_size;
  int state_count;
  struct rewind_state *start;
  struct rewind_state *current;
  int (*save_state)(void *);
  int (*load_state)(void *);
  int (*get_state_size)();
} pl_rewind;

int  pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)());
void pl_rewind_realloc(pl_rewind *rewind);
void pl_rewind_destroy(pl_rewind *rewind);
void pl_rewind_reset(pl_rewind *rewind);
int  pl_rewind_save(pl_rewind *rewind);
int  pl_rewind_restore(pl_rewind *rewind);

#ifdef __cplusplus
}
#endif

#endif // _PL_REWIND_H
