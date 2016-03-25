#include "hw_jpeg.h"
#include "bug_helper.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
int InitVendorJpegCodec(RTCJpegCodec_t *jpeg, void *handle)
{
  int ret = -__LINE__;
  int (*_Android_HWJpeg)(RTCJpegCodec_t *);
  if (jpeg && handle){
   memset(jpeg, 0 , sizeof(*jpeg));
   RTC_FIND_SYM(Android_HWJpeg, handle);
   ret = RTC_CALL(Android_HWJpeg)(jpeg);
  }
  return ret;
}
