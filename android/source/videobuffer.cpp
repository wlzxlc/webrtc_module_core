#include "videobuffer.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitVideoBuffer(OmxGraphics_t *gps, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_CreateOmxGraphics)(OmxGraphics_t *);
  if (gps && handle){
   memset(gps, 0 , sizeof(*gps));
   RTC_FIND_SYM(Android_CreateOmxGraphics, handle);
   ret = RTC_CALL(Android_CreateOmxGraphics)(gps);
  }
  return ret;
}

