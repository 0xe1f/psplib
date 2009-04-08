/* psplib/pl_snd.c: Simple sound playback library
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

#include "pl_snd.h"

#include <stdio.h>
#include <pspaudio.h>
#include <pspthreadman.h>
#include <string.h>
#include <malloc.h>

#define AUDIO_CHANNELS  1
#define DEFAULT_SAMPLES 512
#define VOLUME_MAX      0x8000

static int sound_ready;
static volatile int sound_stop;

typedef struct {
  int thread_handle;
  int sound_ch_handle;
  int left_vol;
  int right_vol;
  pl_snd_callback callback;
  void *user_data;
  short *sample_buffer[2];
  unsigned int samples[2];
  unsigned char paused;
  unsigned char stereo;
} pl_snd_channel_info;

static pl_snd_channel_info sound_stream[AUDIO_CHANNELS];

static int channel_thread(int args, void *argp);
static void free_buffers();
static unsigned int get_bytes_per_sample(int channel);

int pl_snd_init(int sample_count,
                int stereo)
{
  int i, j, failed;
  sound_stop = 0;
  sound_ready = 0;

  /* Check sample count */
  if (sample_count <= 0) sample_count = DEFAULT_SAMPLES;
  sample_count = PSP_AUDIO_SAMPLE_ALIGN(sample_count);

  pl_snd_channel_info *ch_info;
  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    ch_info = &sound_stream[i];
    ch_info->sound_ch_handle = -1;
    ch_info->thread_handle = -1;
    ch_info->left_vol = VOLUME_MAX;
    ch_info->right_vol = VOLUME_MAX;
    ch_info->callback = NULL;
    ch_info->user_data = NULL;
    ch_info->paused = 1;
    ch_info->stereo = stereo;

    for (j = 0; j < 2; j++)
    {
      ch_info->sample_buffer[j] = NULL;
      ch_info->samples[j] = 0;
    }
  }

  /* Initialize buffers */
  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    ch_info = &sound_stream[i];
    for (j = 0; j < 2; j++)
    {
      if (!(ch_info->sample_buffer[j] = 
              (short*)malloc(sample_count * get_bytes_per_sample(i))))
      {
        free_buffers();
        return 0;
      }

      ch_info->samples[j] = sample_count;
    }
  }

  /* Initialize channels */
  for (i = 0, failed = 0; i < AUDIO_CHANNELS; i++)
  {
    sound_stream[i].sound_ch_handle = 
      sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 
                        sample_count,
                        (stereo) 
                          ? PSP_AUDIO_FORMAT_STEREO
                          : PSP_AUDIO_FORMAT_MONO);

    if (sound_stream[i].sound_ch_handle < 0)
    { 
      failed = 1;
      break;
    }
  }

  if (failed)
  {
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (sound_stream[i].sound_ch_handle != -1)
      {
        sceAudioChRelease(sound_stream[i].sound_ch_handle);
        sound_stream[i].sound_ch_handle = -1;
      }
    }

    free_buffers();
    return 0;
  }

  sound_ready = 1;

  char label[16];
  strcpy(label, "audiotX");

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    label[6] = '0' + i;
    sound_stream[i].thread_handle = 
      sceKernelCreateThread(label, (void*)&channel_thread, 0x12, 0x10000, 
        0, NULL);

    if (sound_stream[i].thread_handle < 0)
    {
      sound_stream[i].thread_handle = -1;
      failed = 1;
      break;
    }

    if (sceKernelStartThread(sound_stream[i].thread_handle, sizeof(i), &i) != 0)
    {
      failed = 1;
      break;
    }
  }

  if (failed)
  {
    sound_stop = 1;
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (sound_stream[i].thread_handle != -1)
      {
        //sceKernelWaitThreadEnd(sound_stream[i].threadhandle,NULL);
        sceKernelDeleteThread(sound_stream[i].thread_handle);
      }

      sound_stream[i].thread_handle = -1;
    }

    sound_ready = 0;
    free_buffers();
    return 0;
  }

  return sample_count;
}

void pl_snd_shutdown()
{
  int i;
  sound_ready = 0;
  sound_stop = 1;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (sound_stream[i].thread_handle != -1)
    {
      //sceKernelWaitThreadEnd(sound_stream[i].threadhandle,NULL);
      sceKernelDeleteThread(sound_stream[i].thread_handle);
    }

    sound_stream[i].thread_handle = -1;
  }

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (sound_stream[i].sound_ch_handle != -1)
    {
      sceAudioChRelease(sound_stream[i].sound_ch_handle);
      sound_stream[i].sound_ch_handle = -1;
    }
  }

  free_buffers();
}

static inline int play_blocking(unsigned int channel,
                                unsigned int vol1,
                                unsigned int vol2,
                                void *buf)
{
  if (!sound_ready) return -1;
  if (channel >= AUDIO_CHANNELS) return -1;

  return sceAudioOutputPannedBlocking(sound_stream[channel].sound_ch_handle,
    vol1, vol2, buf);
}

static int channel_thread(int args, void *argp)
{
  volatile int bufidx = 0;
  int channel = *(int*)argp;
  int i, j;
  unsigned short *ptr_m;
  unsigned int *ptr_s;
  void *bufptr;
  unsigned int samples;
  pl_snd_callback callback;
  pl_snd_channel_info *ch_info;

  ch_info = &sound_stream[channel];
  for (j = 0; j < 2; j++)
    memset(ch_info->sample_buffer[j], 0,
           ch_info->samples[j] * get_bytes_per_sample(channel));

  while (!sound_stop)
  {
    callback = ch_info->callback;
    bufptr = ch_info->sample_buffer[bufidx];
    samples = ch_info->samples[bufidx];

    if (callback && !ch_info->paused)
      /* Use callback to fill buffer */
      callback(bufptr, samples, ch_info->user_data);
    else
    {
      /* Fill buffer with silence */
      if (ch_info->stereo)
        for (i = 0, ptr_s = bufptr; i < samples; i++) *(ptr_s++) = 0;
      else 
        for (i = 0, ptr_m = bufptr; i < samples; i++) *(ptr_m++) = 0;
    }

    /* Play sound */
	  play_blocking(channel,
                  ch_info->left_vol,
                  ch_info->right_vol,
                  bufptr);

    /* Switch active buffer */
    bufidx = (bufidx ? 0 : 1);
  }

  sceKernelExitThread(0);
  return 0;
}

static void free_buffers()
{
  int i, j;

  pl_snd_channel_info *ch_info;
  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    ch_info = &sound_stream[i];
    for (j = 0; j < 2; j++)
    {
      if (ch_info->sample_buffer[j])
      {
        free(ch_info->sample_buffer[j]);
        ch_info->sample_buffer[j] = NULL;
      }
    }
  }
}

int pl_snd_set_callback(int channel,
                        pl_snd_callback callback,
                        void *userdata)
{
  if (channel < 0 || channel > AUDIO_CHANNELS)
    return 0;
  volatile pl_snd_channel_info *pci = &sound_stream[channel];
  pci->callback = NULL;
  pci->user_data = userdata;
  pci->callback = callback;

  return 1;
}

static unsigned int get_bytes_per_sample(int channel)
{
  return (sound_stream[channel].stereo)
    ? sizeof(pl_snd_stereo_sample)
    : sizeof(pl_snd_mono_sample);
}

int pl_snd_pause(int channel)
{
  if (channel < 0 || channel > AUDIO_CHANNELS)
    return 0;
  sound_stream[channel].paused = 1;
  return 1;
}

int pl_snd_resume(int channel)
{
  if (channel < 0 || channel > AUDIO_CHANNELS)
    return 0;
  sound_stream[channel].paused = 0;
  return 1;
}
