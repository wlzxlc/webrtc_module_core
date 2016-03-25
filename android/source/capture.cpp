#include "capture.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitCapture(RTCCamera_t *capture, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_CreateCamera)(RTCCamera_t *);
  if (capture && handle){
   memset(capture, 0 , sizeof(*capture));
   RTC_FIND_SYM(Android_CreateCamera, handle);
   ret = RTC_CALL(Android_CreateCamera)(capture);
  }
  return ret;
}
