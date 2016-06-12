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

#ifndef _PL_PSP_H
#define _PL_PSP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  PSP_EXIT_CALLBACK
} pl_callback_type;

extern int ExitPSP;

int  pl_psp_init(const char *app_path);
void pl_psp_shutdown();
void pl_psp_set_clock_freq(int freq);
const char* pl_psp_get_app_directory();

int pl_psp_register_callback(pl_callback_type type,
                             void (*func)(void *param),
                             void *param);
int pl_psp_start_callback_thread();

#ifdef __cplusplus
}
#endif

#endif // _PL_PSP_H
