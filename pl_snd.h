/* psplib/pl_snd.h: Simple sound playback library
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

#ifndef _PL_SND_H
#define _PL_SND_H

#ifdef __cplusplus
extern "C" {
#endif

#define PL_SND_ALIGN_SAMPLE(s) (((s) + 63) & ~63)
#define PL_SND_TRUNCATE_SAMPLE(s) ((s) & ~63)

typedef struct pl_snd_stereo_sample_t
{
  short l;
  short r;
} pl_snd_stereo_sample;

typedef struct pl_snd_mono_sample_t
{
  short ch;
} pl_snd_mono_sample;

typedef union pl_snd_sample_t
{
  pl_snd_stereo_sample stereo;
  pl_snd_mono_sample mono;
} pl_snd_sample;

typedef void (*pl_snd_callback)(pl_snd_sample *buffer,
                                unsigned int samples,
                                void *user_data);

int  pl_snd_init(int samples, 
                 int stereo);
int  pl_snd_set_callback(int channel,
                         pl_snd_callback callback,
                         void *userdata);
int  pl_snd_pause(int channel);
int  pl_snd_resume(int channel);
void pl_snd_shutdown();

#ifdef __cplusplus
}
#endif

#endif // _PL_SND_H

