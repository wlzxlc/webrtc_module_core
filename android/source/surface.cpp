#include "surface.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitSurface(RTCSurfaceHandle_t *sfs, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_SurfaceHandle)(RTCSurfaceHandle_t *);
  if (sfs && handle){
   memset(sfs, 0 , sizeof(*sfs));
   RTC_FIND_SYM(Android_SurfaceHandle, handle);
   ret = RTC_CALL(Android_SurfaceHandle)(sfs);
  }
  return ret;
}
