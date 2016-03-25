#include "render.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitRTCRender(RTCRenderHandle_t *render, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_CreateRender)(RTCRenderHandle_t *);
  if (render && handle){
   memset(render, 0 , sizeof(*render));
   RTC_FIND_SYM(Android_CreateRender, handle);
   ret = RTC_CALL(Android_CreateRender)(render);
  }
  return ret;
}
