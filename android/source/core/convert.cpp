#ifdef ANDROID_ON_QCOM
#include <C2DColorConverter.h>
#include "bug_helper.h"
#include "convert.h"
#include "graphics_type.h"
#include "core.cc"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>

#ifdef DEBUG_RTC
class ScopedDebug{
private:
  const char *string;
  int ln;

public:
  ScopedDebug(int line, const char *s){
    string = s;
    ln = line;
    DEBUG("ScopedDebug: [%d] --> %s", ln,s);
  }
  ~ScopedDebug(){
    DEBUG("ScopedDebug: [%d] <-- %s", ln,string);
  }
};

#define SCOPEDDEBUG() ScopedDebug __ScopedDebug(__LINE__,__FUNCTION__)
#else
#define SCOPEDDEBUG()
#endif

using namespace android;

class ConvertModule{
private:
  createC2DColorConverter_t *_open;
  destroyC2DColorConverter_t *_destory;
  void * _handle;
public:
  ConvertModule(){
    SCOPEDDEBUG();
    _open = NULL;
    _destory = NULL;

    const char *libname = "libc2dcolorconvert.so";
    const char *open_method = "createC2DColorConverter";
    const char *destroy_method = "destroyC2DColorConverter";

    _handle = dlopen(libname, RTLD_LAZY);
    if (_handle){
        _open = (createC2DColorConverter_t *)
            dlsym(_handle,open_method);
        DEBUG("Find method[%s] %s",open_method, _open ? "OK." : dlerror());

        _destory = (destroyC2DColorConverter_t *)
            dlsym(_handle,destroy_method);
        DEBUG("Find method[%s] %s",destroy_method, _destory ? "OK." : dlerror());

      }else{
        DEBUG("Open %s faild, %s", libname, dlerror());
      }
  }

  ~ConvertModule(){
    SCOPEDDEBUG();
    if (_handle){
        _open = NULL;
        _destory = NULL;
        dlclose(_handle);
      }
  }

  C2DColorConverterBase *Create(size_t srcWidth,
                                size_t srcHeight,
                                size_t dstWidth,
                                size_t dstHeight,
                                ColorConvertFormat srcFormat,
                                ColorConvertFormat dstFormat,
                                int32_t flags,
                                size_t srcStride)
  {
    SCOPEDDEBUG();
    C2DColorConverterBase *tmp = NULL;
    if (_open){
        tmp = (_open)(srcWidth,
                      srcHeight,
                      dstWidth,
                      dstHeight,
                      srcFormat,
                      dstFormat,
                      flags,
                      srcStride);
      }
    DEBUG("Create c2d module %p",tmp);
    return tmp;
  }

  void Destroy(C2DColorConverterBase *handle)
  {
    SCOPEDDEBUG();
    if (_destory && handle){
        (_destory)(handle);
      }
  }

};

static ConvertModule ctx;

static int32_t  ColorFmtToC2dFmt(int32_t color, ColorConvertFormat &target_color)
{
  int ret = 0;
  switch(color){
    case kKeyRGB565:
      target_color = RGB565;
      break;
    case kKeyYCbCr420Tile:
      target_color = YCbCr420Tile;
      break;
    case kKeyYCbCr420SP:
      target_color = YCbCr420SP;
      break;
    case kKeyYCbCr420P:
      target_color = YCbCr420P;
      break;
    case kKeyYCrCb420P:
      target_color = YCrCb420P;
      break;
    case kKeyRGBA8888:
      target_color = RGBA8888;
      break;
    case kKeyNV12_2K:
      target_color = NV12_2K;
      break;
    case kKeyNV12_128m:
      target_color = NV12_128m;
      break;
    default:
      DEBUG("Unsupported target color %d", color);
      ret = ERRNUM("");
      break;
    }
  return ret;
}

static RTCColorConvert *icreate(int32_t  src_w ,
                                int32_t  src_h ,
                                int32_t  dst_w ,
                                int32_t  dst_h ,
                                int32_t  src_color ,
                                int32_t  dst_color ,
                                int32_t  flags ,
                                int32_t  src_stride )
{
  SCOPEDDEBUG();
  ColorConvertFormat src_fmt;
  ColorConvertFormat dst_fmt;

  if (ColorFmtToC2dFmt(src_color,src_fmt) !=0 ||
      ColorFmtToC2dFmt(dst_color,dst_fmt) !=0){
      return NULL;
    }

  return (RTCColorConvert *)ctx.Create((size_t)src_w,
                                       (size_t)src_h,
                                       (size_t)dst_w,
                                       (size_t)dst_h,
                                       src_fmt,
                                       dst_fmt,
                                       flags,
                                       (size_t)src_stride);
}

static int iconvert(RTCColorConvert * handle,
                    graphics_handle * src ,
                    graphics_handle * dst )
{
  SCOPEDDEBUG();
  int err = 0;
  C2DColorConverterBase *c2d = (C2DColorConverterBase *)handle;
  VideoBuffer *srcbuffer = static_cast<VideoBuffer *>(src);
  VideoBuffer *dstbuffer = static_cast<VideoBuffer *>(dst);
  void *src_base = NULL;
  void *dst_base = NULL;
  if (c2d && srcbuffer && dstbuffer &&
      srcbuffer->graphicBuffer.get() && dstbuffer->graphicBuffer.get()){
      const native_handle_t* src_handle = srcbuffer->graphicBuffer->getNativeBuffer()->handle;
      const native_handle_t* dst_handle = dstbuffer->graphicBuffer->getNativeBuffer()->handle;

      bool src_base_mmap = false;
      bool dst_base_mmap = false;
      int fd_size_idx = 3;
      int fd_offset_idx = 4;
#ifdef ANDROID_ON_QCOM
      fd_size_idx = 4;
      fd_offset_idx = 5;
#endif
      if (srcbuffer->graphicBuffer->lock(
            GraphicBuffer::USAGE_SW_WRITE_OFTEN |
            GraphicBuffer::USAGE_SW_READ_OFTEN, &src_base) ) {
       src_base = mmap(NULL,src_handle->data[fd_size_idx],PROT_READ,MAP_SHARED, src_handle->data[0],src_handle->data[fd_offset_idx]);
       src_base_mmap = true;
      }

      if(dstbuffer->graphicBuffer->lock(
            GraphicBuffer::USAGE_SW_WRITE_OFTEN |
            GraphicBuffer::USAGE_SW_READ_OFTEN, &dst_base) ) {
       dst_base = mmap(NULL,src_handle->data[fd_size_idx],PROT_READ,MAP_SHARED, dst_handle->data[0],dst_handle->data[fd_offset_idx]);
       dst_base_mmap = true;
      }


      if (int result __attribute__((unused))= c2d->convertC2D(src_handle->data[0],
                                       src_base,src_base,
                                       dst_handle->data[0],
                                       dst_base,dst_base)){
          DEBUG("Convert color faild err[%d].",result);
          err = ERRNUM("Convert faild.");
        }

      if (src_base_mmap) {
         munmap(src_base, src_handle->data[fd_size_idx]);
      }else {
          srcbuffer->graphicBuffer->unlock();
      }
      if (dst_base_mmap) {
          munmap(dst_base, dst_handle->data[fd_size_idx]);
      }else {
          dstbuffer->graphicBuffer->unlock();
      }
    }else{
      err = ERRNUM("Null pointer.");
    }
  return err;
}

static int iconvert_fd(RTCColorConvert * handle, int src_fd, void *src_base, int dst_fd, void *dst_base)
{
  SCOPEDDEBUG();
  int err = 0;
  C2DColorConverterBase *c2d = (C2DColorConverterBase *)handle;

  if (c2d && src_base && dst_base && src_fd > -1 && dst_fd > -1){
      if (int result __attribute__((unused))= c2d->convertC2D(src_fd,
                                       src_base,src_base,
                                       dst_fd,
                                       dst_base,dst_base)){
          DEBUG("Convert color faild err[%d].",result);
          err = ERRNUM("Convert faild.");
        }
    }else{
      err = ERRNUM("Null pointer.");
    }
  return err;
}

static void idestroy(RTCColorConvert *handle)
{
  SCOPEDDEBUG();
  if (handle){
      ctx.Destroy((C2DColorConverterBase *)handle);
    }
}

extern "C" {
int PREFIX(Android_CreateColorConvert)(RTCColorConvertHandle_t *handle)
{
  SCOPEDDEBUG();
  handle->convert = iconvert;
  handle->convert_fd = iconvert_fd;
  handle->create = icreate;
  handle->destroy = idestroy;
  return 0;
}
}
#endif
