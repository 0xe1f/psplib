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

#include <pspkernel.h>
#include <psppower.h>
#include <malloc.h>
#include <string.h>

#include "pl_psp.h"

typedef struct pl_psp_callback_t
{
  void (*handler)(void *param);
  void *param;
} pl_psp_callback;

static char _app_directory[1024];
static pl_psp_callback _exit_callback;
int ExitPSP;

static int _callback_thread(SceSize args, void* argp);
static int _callback(int arg1, int arg2, void* common);

int pl_psp_init(const char *app_path)
{
  ExitPSP = 0;

  char *pos;
  if (!(pos = strrchr(app_path, '/')))
    _app_directory[0] = '\0';
  else
  {
    strncpy(_app_directory, app_path, pos - app_path + 1);
    _app_directory[pos - app_path + 1] = '\0';
  }

  _exit_callback.handler = NULL;
  _exit_callback.param = NULL;

  return 1;
}

const char* pl_psp_get_app_directory()
{
  return _app_directory;
}

void pl_psp_shutdown()
{
  sceKernelExitGame();
}

void pl_psp_set_clock_freq(int freq)
{
  if (freq < 222) freq = 222;
  else if (freq > 333) freq = 333;
  scePowerSetClockFrequency(freq, freq, freq/2);
}

static int _callback(int arg1, int arg2, void* common)
{
  pl_psp_callback *callback = (pl_psp_callback*)common;
  callback->handler(callback->param);

  return 0;
}

static int _callback_thread(SceSize args, void* argp)
{
  int cbid;

  if (_exit_callback.handler)
  {
    cbid = sceKernelCreateCallback("Exit Callback", _callback, &_exit_callback);
    sceKernelRegisterExitCallback(cbid);
  }

  sceKernelSleepThreadCB();
  return 0;
}

int pl_psp_register_callback(pl_callback_type type,
                             void (*func)(void *param),
                             void *param)
{
  switch (type)
  {
  case PSP_EXIT_CALLBACK:
    _exit_callback.handler = func;
    _exit_callback.param = param;
    break;
  default:
    return 0;
  }

  return 1;
}

int pl_psp_start_callback_thread()
{
  int thid;

  if ((thid = sceKernelCreateThread("update_thread", _callback_thread, 0x11, 0xFA0, 0, 0)) < 0)
    return 0;

  sceKernelStartThread(thid, 0, NULL);

  return thid;
}
