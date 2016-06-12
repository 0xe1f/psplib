## Copyright 2007-2015 Akop Karapetyan
## 
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
## 
##    http://www.apache.org/licenses/LICENSE-2.0
## 
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

ifndef PSP_FW_VERSION
PSP_FW_VERSION=200
endif

CC=psp-gcc
AR=psp-ar
RANLIB=psp-ranlib
RM=rm -rf
PSPSDK=$(shell psp-config --pspsdk-path)
CFLAGS=-G0 -Wall -I$(PSPSDK)/include
DEFINES=-DPSP -D_PSP_FW_VERSION=$(PSP_FW_VERSION)

all: libpsplib.a

libpsplib.a: \
  adhoc.o font.o image.o ctrl.o video.o ui.o \
  pl_ini.o pl_perf.o pl_vk.o pl_util.o \
  pl_psp.o pl_menu.o pl_file.o pl_snd.o pl_gfx.o \
  pl_rewind.o pl_ctl.o pl_ui.o

	$(AR) cru $@ $?
	$(RANLIB) $@
	@echo Compiled for firmware $(PSP_FW_VERSION)

clean:
	$(RM) *.o genfont stockfont.h *.a

font.o: font.c font.h stockfont.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

video.o: video.c video.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

ctrl.o: ctrl.c ctrl.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

adhoc.o: adhoc.c adhoc.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

ui.o: ui.c ui.h pl_file.o pl_psp.o \
      ctrl.o font.o pl_menu.o video.o \
      adhoc.o
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_snd.o: pl_snd.c pl_snd.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_psp.o: pl_psp.c pl_psp.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_ini.o: pl_ini.c pl_ini.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_vk.o: pl_vk.c pl_vk.h video.o font.o \
         ctrl.o
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_util.o: pl_util.c pl_util.h image.o \
           pl_file.o video.o
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_perf.o: pl_perf.c pl_perf.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_menu.o: pl_menu.c pl_menu.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_file.o: pl_file.c pl_file.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_image.o: pl_image.c pl_image.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_gfx.o: pl_gfx.c pl_gfx.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_rewind.o: pl_rewind.c pl_rewind.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_ctl.o: pl_ctl.c pl_ctl.h
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

pl_ui.o: pl_ui.c pl_ui.h pl_file.o pl_psp.o \
         ctrl.o font.o pl_menu.o video.o adhoc.o
	$(CC) $(DEFINES) $(CFLAGS) -O2 -c -o $@ $<

stockfont.h: stockfont.fd genfont
	./genfont < $< > $@

genfont: genfont.c
	cc $< -o $@
