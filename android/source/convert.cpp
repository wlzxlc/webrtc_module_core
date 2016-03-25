#include "../include/convert.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int InitColorConvert(RTCColorConvertHandle_t *convert, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_CreateColorConvert)(RTCColorConvertHandle_t *);
  if (convert && handle){
   memset(convert, 0 , sizeof(*convert));
   RTC_FIND_SYM(Android_CreateColorConvert, handle);
   ret = RTC_CALL(Android_CreateColorConvert)(convert);
  }
  return ret;
}
