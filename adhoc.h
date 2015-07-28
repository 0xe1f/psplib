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
