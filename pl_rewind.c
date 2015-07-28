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

#include <stdlib.h>
#include <stdio.h>

#include "pl_rewind.h"

typedef struct rewind_state
{
  void *data;
  struct rewind_state *prev;
  struct rewind_state *next;
} rewind_state_t;

static int get_free_memory();

int pl_rewind_init(pl_rewind *rewind,
  int (*save_state)(void *),
  int (*load_state)(void *),
  int (*get_state_size)())
{
	float memory_needed = (float)get_free_memory() * 0.85;
  int state_data_size = get_state_size();
  int state_count = (int)(memory_needed 
                    / (float)(state_data_size + sizeof(rewind_state_t)));

	if (state_count < 1)
    return 0;

  /* First state */
  rewind_state_t *prev, *curr = NULL;
  if (!(rewind->start = (rewind_state_t*)malloc(sizeof(rewind_state_t))))
    return 0;

  if (!(rewind->start->data = malloc(state_data_size)))
  {
    free(rewind->start);
    rewind->start = NULL;
    return 0;
  }

  prev = rewind->start;

  /* The rest */
  int i;
  for (i = 1; i < state_count; i++)
  {
    /* If allocation fails, compose a shorter state chain */
    if (!(curr = (rewind_state_t*)malloc(sizeof(rewind_state_t))))
    {
      state_count = i + 1;
      break;
    }

    /* If allocation fails, compose a shorter state chain */
    if (!(curr->data = malloc(state_data_size)))
    {
      state_count = i + 1;
      free(curr);
      break;
    }

    prev->next = curr;
    curr->prev = prev;
    prev = curr;
  }

  /* Make circular */
  rewind->start->prev = prev;
  curr->next = rewind->start;

  /* Init structure */
  rewind->current = NULL;
  rewind->save_state = save_state;
  rewind->load_state = load_state;
  rewind->get_state_size = get_state_size;
  rewind->state_data_size = state_data_size;
  rewind->state_count = state_count;

  return 1;
}

void pl_rewind_realloc(pl_rewind *rewind)
{
  pl_rewind_destroy(rewind);
  pl_rewind_init(rewind,
    rewind->save_state,
    rewind->load_state,
    rewind->get_state_size);
}

void pl_rewind_destroy(pl_rewind *rewind)
{
  rewind->start->prev->next = NULL; /* Prevent infinite loop */
	rewind_state_t *curr, *next;

  for (curr = rewind->start; curr; curr = next)
  {
    next = curr->next;
    free(curr->data);
    free(curr);
  }

  rewind->start = NULL;
  rewind->current = NULL;
}

void pl_rewind_reset(pl_rewind *rewind)
{
  rewind->current = NULL;
}

int pl_rewind_save(pl_rewind *rewind)
{
  rewind_state_t *save_slot = 
    (rewind->current) ? rewind->current->next
                      : rewind->start;

	if (!rewind->save_state(save_slot->data))
    return 0;

	rewind->current = save_slot;

  /* Move starting point forward, if we've reached the start node */
  if (save_slot->next == rewind->start)
    rewind->start = rewind->start->next;

  return 1;
}

int pl_rewind_restore(pl_rewind *rewind)
{
  rewind_state_t *load_slot = rewind->current;
  if (!(load_slot && rewind->load_state(load_slot->data)))
    return 0;

  /* Can't go past the starting point */
  if (load_slot != rewind->start)
    rewind->current = rewind->current->prev;

  return 1;
}

static int get_free_memory()
{
  const int 
    chunk_size = 65536, // 64 kB
    chunks = 1024; // 65536 * 1024 = 64 MB
  void *mem_reserv[chunks];
  int total_mem = 0, i;

  /* Initialize */	
  for (i = 0; i < chunks; i++)
    mem_reserv[i] = NULL;

  /* Allocate */
  for (i = 0; i < chunks; i++)
  {
    if (!(mem_reserv[i] = malloc(chunk_size)))
      break;

    total_mem += chunk_size;
  }

  /* Free */
  for (i = 0; i < chunks; i++)
  {
    if (!mem_reserv[i])
      break;
    free(mem_reserv[i]);
  }
	
	return total_mem;
}

