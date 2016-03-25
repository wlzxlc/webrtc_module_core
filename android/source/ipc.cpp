#include "ipc.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitRTCIPC(RTCIPCHandle_t *ipc, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_CreateIPC)(RTCIPCHandle_t *);
  if (ipc && handle){
   memset(ipc, 0 , sizeof(*ipc));
   RTC_FIND_SYM(Android_CreateIPC, handle);
   ret = RTC_CALL(Android_CreateIPC)(ipc);
  }
  return ret;
}
