/* psplib/adhoc.h: Adhoc Wi-fi matching and communication
   Copyright (C) 2008-2009 Akop Karapetyan

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

#ifndef _PSP_ADHOC_H
#define _PSP_ADHOC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char PspMAC[6];
typedef void(*PspMatchingCallback)(int unk1, 
              int event, 
              unsigned char *mac2, 
              int opt_len, 
              void *opt_data);

int pspAdhocLoadDrivers();
int pspAdhocInit(const char *product_name, 
                 PspMatchingCallback callback);
int pspAdhocConnect(const PspMAC mac);
int pspAdhocShutdown();

void pspAdhocSelectTarget(const PspMAC mac);
void pspAdhocCancelTarget(const PspMAC mac);

int pspAdhocGetOwnMAC(PspMAC mac);
int pspAdhocIsMACEqual(const PspMAC mac1, const PspMAC mac2);
int pspAdhocIsWLANEnabled();

int pspAdhocSend(const PspMAC mac, const void *buffer, int length);
int pspAdhocRecv(void *buffer, int length);
int pspAdhocSendBlocking(const PspMAC mac, const void *buffer, int length);
int pspAdhocRecvBlocking(void *buffer, int length);
int pspAdhocSendWithAck(const PspMAC mac, const void *buffer, int length);
int pspAdhocRecvWithAck(void *buffer, int length);
int pspAdhocRecvBlob(void **buffer, int *length);
int pspAdhocSendBlob(const PspMAC mac, const void *buffer, int length);

#ifdef __cplusplus
}
#endif


#endif /* _PSP_ADHOC_H */
