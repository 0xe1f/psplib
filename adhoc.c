/* psplib/adhoc.c: Adhoc Wi-fi matching and communication
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

#include "adhoc.h"

#include <pspnet_adhoc.h>
#include <pspnet_adhocmatching.h>
#include <pspnet_adhocctl.h>
#include <string.h>
#include <stdio.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <psputility_netmodules.h>
#include <psputility_sysparam.h>
#include <psputilsforkernel.h>
#include <malloc.h>

#define ADHOC_TIMEOUT    (10 * 1000000)
#define ADHOC_BLOCKSIZE   0x400

int _net_adhoc_ctl_connect = 0;
int _net_adhoc_pdp_create = 0;
int _net_adhoc_matching_start = 0;
int _net_adhoc_matching_create = 0;
int _net_adhoc_matching_init = 0;
int _net_adhoc_ctl_init = 0;
int _net_adhoc_init = 0;
int _net_init = 0;
char _matching_data[128];
int _pdp_id, _matching_id;
PspMAC _own_mac;

int pspAdhocRecvBlockingMAC(void *buffer, int length, PspMAC mac);

int pspAdhocInit(const char *product_name, 
                PspMatchingCallback callback)
{
  /* Shut down adhoc, if currently enabled */
  pspAdhocShutdown();

  struct productStruct product;
  char mac[20];
  int err, state = 0;

  strncpy(product.product, product_name, 9);
  product.unknown = 0;
  if (sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, 
                                     _matching_data, 128) != 0)
    return 0;

  if ((err = sceNetInit(0x20000, 0x2A, 0x1000, 0x2A, 0x1000)) == 0)
  {
    _net_init = 1;
    if ((err = sceNetAdhocInit()) == 0)
    {
      _net_adhoc_init = 1;
      if ((err = sceNetAdhocctlInit(0x2000, 0x20, &product)) == 0)
      {
        _net_adhoc_ctl_init = 1;
        if ((err = sceNetAdhocctlConnect((void*)"")) == 0)
        {
          _net_adhoc_ctl_connect = 1;
          do
          {
            if ((err = sceNetAdhocctlGetState(&state)) != 0) break;
            sceKernelDelayThread(1000000/60);
          } while (state != 1);

          /* Get WLAN MAC */
          unsigned char own_mac[8];
          sceWlanGetEtherAddr(own_mac);
          memcpy(_own_mac, own_mac, sizeof(unsigned char) * 6);

          if (err == 0)
          {
            sceWlanGetEtherAddr((unsigned char*)mac);
            if ((_pdp_id = sceNetAdhocPdpCreate((unsigned char*)mac, 0x309, 0x400, 0)) > 0)
            {
              _net_adhoc_pdp_create = 1;
              if ((err = sceNetAdhocMatchingInit(0x20000)) == 0)
              {
                _net_adhoc_matching_init = 1;
                _matching_id = sceNetAdhocMatchingCreate(3,
                                                        0xa,
                                                        0x22b,
                                                        0x800,
                                                        0x2dc6c0,
                                                        0x5b8d80,
                                                        3,
                                                        0x7a120,
                                                        callback);
                if (_matching_id >= 0)
                {
                  _net_adhoc_matching_create = 1;
                  err = sceNetAdhocMatchingStart(_matching_id,
                                                0x10,
                                                0x2000,
                                                0x10,
                                                0x2000,
                                                strlen(_matching_data) + 1,
                                                _matching_data);
                  if (err == 0)
                  {
                    _net_adhoc_matching_start = 1;
                    /* Everything checked out */
                    return 1;
                  }
                  sceNetAdhocMatchingDelete(_matching_id);
                  _net_adhoc_matching_create = 0;
                }
                sceNetAdhocMatchingTerm();
                _net_adhoc_matching_init = 0;
              }
              sceNetAdhocPdpDelete(_pdp_id, 0);
              _net_adhoc_pdp_create = 0;
            }
          }
          sceNetAdhocctlDisconnect();
          _net_adhoc_ctl_connect = 0;
        }
        sceNetAdhocctlTerm();
        _net_adhoc_ctl_init = 0;
      }
      sceNetAdhocTerm();
      _net_adhoc_init = 0;
    }
    sceNetTerm();
    _net_init = 0;
  }
  
  return 0;
}

void pspAdhocCancelTarget(const PspMAC mac)
{
  sceNetAdhocMatchingCancelTarget(_matching_id, (unsigned char*)mac);
}

void pspAdhocSelectTarget(const PspMAC mac)
{
  sceNetAdhocMatchingSelectTarget(_matching_id, (unsigned char*)mac, 0, 0);
}

int pspAdhocIsMACEqual(const PspMAC mac1, const PspMAC mac2)
{
  int i;
  for (i = 0; i < 6; i++)
    if (mac1[i] != mac2[i])
      return 0;
  return 1;
}

int pspAdhocShutdown()
{
  if (_net_init)
  {
    if (_net_adhoc_init)
    {
      if (_net_adhoc_ctl_init)
      {
        if (_net_adhoc_ctl_connect)
        {
          if (_net_adhoc_pdp_create)
          {
            if (_net_adhoc_matching_init)
            {
              if (_net_adhoc_matching_create)
              {
                if (_net_adhoc_matching_start)
                {
                  sceNetAdhocMatchingStop(_matching_id);
                  _net_adhoc_matching_start = 0;
                }
                sceNetAdhocMatchingDelete(_matching_id);
                _net_adhoc_matching_create = 0;
              }
              sceNetAdhocMatchingTerm();
              _net_adhoc_matching_init = 0;
            }
            sceNetAdhocPdpDelete(_pdp_id, 0);
            _net_adhoc_pdp_create = 0;
          }
          sceNetAdhocctlDisconnect();
          _net_adhoc_ctl_connect = 0;
        }
        sceNetAdhocctlTerm();
        _net_adhoc_ctl_init = 0;
      }
      sceNetAdhocTerm();
      _net_adhoc_init = 0;
    }
    sceNetTerm();
    _net_init = 0;
  }

  return 1;
}

/* Must be called from KERNEL thread */
int pspAdhocLoadDrivers()
{
  _net_adhoc_matching_start = 0;
  _net_adhoc_matching_create = 0;
  _net_adhoc_matching_init = 0;
  _net_adhoc_pdp_create = 0;
  _net_adhoc_ctl_connect = 0;
  _net_adhoc_ctl_init = 0;
  _net_adhoc_init = 0;
  _net_init = 0;

#if (_PSP_FW_VERSION < 200)
	int modID;

	modID = pspSdkLoadStartModule("flash0:/kd/ifhandle.prx", 
                                PSP_MEMORY_PARTITION_KERNEL);
	if (modID < 0) return modID;

	modID = pspSdkLoadStartModule("flash0:/kd/memab.prx", 
                                PSP_MEMORY_PARTITION_KERNEL);
	if (modID < 0) return modID;

	modID = pspSdkLoadStartModule("flash0:/kd/pspnet_adhoc_auth.prx", 
                                PSP_MEMORY_PARTITION_KERNEL);
	if (modID < 0) return modID;

	modID = pspSdkLoadStartModule("flash0:/kd/pspnet.prx", 
                                PSP_MEMORY_PARTITION_USER);
	if (modID < 0) return modID;
	else pspSdkFixupImports(modID);

	modID = pspSdkLoadStartModule("flash0:/kd/pspnet_adhoc.prx", 
                                PSP_MEMORY_PARTITION_USER);
	if (modID < 0) return modID;
	else pspSdkFixupImports(modID);

	modID = pspSdkLoadStartModule("flash0:/kd/pspnet_adhocctl.prx", 
                                PSP_MEMORY_PARTITION_USER);
	if (modID < 0) return modID;
	else pspSdkFixupImports(modID);

	modID = pspSdkLoadStartModule("flash0:/kd/pspnet_adhoc_matching.prx", 
                                PSP_MEMORY_PARTITION_USER);
	if (modID < 0) return modID;
	else pspSdkFixupImports(modID);

	sceKernelDcacheWritebackAll();
	sceKernelIcacheInvalidateAll();
#else
  sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
  sceUtilityLoadNetModule(PSP_NET_MODULE_ADHOC);
#endif

  return 1;
}

int pspAdhocConnect(const PspMAC mac)
{
  int err, state = 0;
  char temp[64];
  char ssid[10];
  sceNetEtherNtostr((unsigned char*)mac, temp);

  ssid[0] = temp[9];
  ssid[1] = temp[10];
  ssid[2] = temp[12];
  ssid[3] = temp[13];
  ssid[4] = temp[15];
  ssid[5] = temp[16];
  ssid[6] = '\0';

  if (_net_adhoc_ctl_connect)
  {
    if (_net_adhoc_pdp_create)
    {
      if (_net_adhoc_matching_init)
      {
        if (_net_adhoc_matching_create)
        {
          if (_net_adhoc_matching_start)
          {
            sceNetAdhocMatchingStop(_matching_id);
            _net_adhoc_matching_start = 0;
          }
          sceNetAdhocMatchingDelete(_matching_id);
          _net_adhoc_matching_create = 0;
        }
        sceNetAdhocMatchingTerm();
        _net_adhoc_matching_init = 0;
      }
      sceNetAdhocPdpDelete(_pdp_id, 0);
      _net_adhoc_pdp_create = 0;
    }
    sceNetAdhocctlDisconnect();
    _net_adhoc_ctl_connect = 0;
  }

  do
  {
    if ((err = sceNetAdhocctlGetState(&state)) != 0) break;
    sceKernelDelayThread(1000000/60);
  } while (state == 1);

  if ((err = sceNetAdhocctlConnect((void*)ssid)) == 0)
  {
    do
    {
      if ((err = sceNetAdhocctlGetState(&state)) != 0) break;
      sceKernelDelayThread(1000000/60);
    } while (state != 1);

    if (!err)
    {
      if ((_pdp_id = sceNetAdhocPdpCreate(_own_mac, 0x309, 0x800, 0)) > 0)
      {
        if (pspAdhocIsMACEqual(mac, _own_mac))
          sceKernelDelayThread(1000000);
        return 1;
      }
    }

    if (_net_adhoc_ctl_connect)
    {
      sceNetAdhocctlDisconnect();
      _net_adhoc_ctl_connect = 0;
    }

    if (state == 1)
    {
      do
      {
        if ((err = sceNetAdhocctlGetState(&state)) != 0) 
          break;
        sceKernelDelayThread(1000000/60);
      } while (state == 1);
    }
  }

  if (_net_init)
  {
    if (_net_adhoc_init)
    {
      if (_net_adhoc_ctl_init)
      {
        sceNetAdhocctlTerm();
        _net_adhoc_ctl_init = 0;
      }
      sceNetAdhocTerm();
      _net_adhoc_init = 0;
    }
    sceNetTerm();
    _net_init = 0;
  }

  return 0;
}

int pspAdhocGetOwnMAC(PspMAC mac)
{
  memcpy(mac, _own_mac, sizeof(unsigned char) * 6);
  return 1;
}

int pspAdhocIsWLANEnabled()
{
  return sceWlanGetSwitchState();
}

int pspAdhocSendBlocking(const PspMAC mac, const void *buffer, int length)
{
  if (sceNetAdhocPdpSend(_pdp_id, (unsigned char*)mac, 0x309, 
    (void*)buffer, length, ADHOC_TIMEOUT, 0) < 0)
      return 0;
  return length;
}

int pspAdhocRecvBlockingMAC(void *buffer, int length, PspMAC mac)
{
  unsigned short port = 0;
  if (sceNetAdhocPdpRecv(_pdp_id, mac, &port, buffer, &length, ADHOC_TIMEOUT, 0) < 0)
    return 0;
  return length;
}

int pspAdhocRecvBlocking(void *buffer, int length)
{
  PspMAC mac;
  return pspAdhocRecvBlockingMAC(buffer, length, mac);
}

int pspAdhocSendWithAck(const PspMAC mac, const void *buffer, int length)
{
  int ack_data   = 0;
  int chunk_size = length;
  int bytes_sent = 0;

  do
  {
    if (chunk_size > ADHOC_BLOCKSIZE) 
      chunk_size = ADHOC_BLOCKSIZE;

    pspAdhocSendBlocking(mac, buffer, chunk_size);

    if (pspAdhocRecvBlocking(&ack_data, sizeof(int)) == 0)
      return 0;

    if (ack_data != chunk_size)
      return 0;

    buffer += ADHOC_BLOCKSIZE;
    bytes_sent += ADHOC_BLOCKSIZE;
    chunk_size = length - bytes_sent;
  } while (bytes_sent < length);

  return length;
}

int pspAdhocRecvWithAck(void *buffer, int length)
{
  int chunk_size = length;
  int bytes_rcvd = 0;
  PspMAC source_mac;

  do
  {
    if (chunk_size > ADHOC_BLOCKSIZE) 
      chunk_size = ADHOC_BLOCKSIZE;

    if (pspAdhocRecvBlockingMAC(buffer, chunk_size, source_mac) == 0)
      return 0;

    pspAdhocSendBlocking(source_mac, &chunk_size, sizeof(int));

    buffer += ADHOC_BLOCKSIZE;
    bytes_rcvd += ADHOC_BLOCKSIZE;
    chunk_size = length - bytes_rcvd;
  } while (bytes_rcvd < length);

  return length;
}

int pspAdhocSendBlob(const PspMAC mac, const void *buffer, int length)
{
  if (!pspAdhocSendWithAck(mac, &length, sizeof(int)))
    return 0;
  if (!pspAdhocSendWithAck(mac, buffer, length))
    return 0;
  return 1;
}

int pspAdhocRecvBlob(void **buffer, int *length)
{
  if (!pspAdhocRecvWithAck(length, sizeof(int)))
    return 0;
  if (!(*buffer = malloc(*length)))
    return 0;
  if (!pspAdhocRecvWithAck(*buffer, *length))
  {
    free(*buffer);
    return 0;
  }
  return 1;
}

int pspAdhocSend(const PspMAC mac, const void *buffer, int length)
{
  if (sceNetAdhocPdpSend(_pdp_id, (unsigned char*)mac, 0x309, (void*)buffer, length, 0, 1) < 0)
    return 0;
  return length;
}

int pspAdhocRecv(void *buffer, int length)
{
  unsigned short port = 0;
  unsigned char mac[6];

  if (sceNetAdhocPdpRecv(_pdp_id, mac, &port, buffer, &length, 0, 1) < 0)
    return 0;
  return length;
}
