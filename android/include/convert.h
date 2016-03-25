#ifndef WEBRTC_KEDACOM_CONVERT_H
#define WEBRTC_KEDACOM_CONVERT_H
#include "graphics_type.h"

typedef void RTCColorConvert;
enum{
  kKeyRGB565 = 1,
  // Width align to 128
  kKeyYCbCr420Tile,
  // Width align to 16
  kKeyYCbCr420SP,
  kKeyYCbCr420P,
  kKeyYCrCb420P,
  kKeyRGBA8888,
  // By used QCOM platform and ...
  kKeyNV12_2K,
  // By used QCOM platform and ...
  kKeyNV12_128m
};

typedef struct RTCColorConvertHandle{
  RTCColorConvert *(*create)(int32_t /* src_w */,
                             int32_t /* src_h */,
                             int32_t /* dst_w */,
                             int32_t /* dst_h */,
                             int32_t /* src_color */,
                             int32_t /* dst_color */,
                             int32_t /* flags */,
                             int32_t /* src_stride */);

  int (*convert)(RTCColorConvert * /* this */,
                 graphics_handle * /* src */,
                 graphics_handle * /* dst */);
  int (*convert_fd)(RTCColorConvert * /* this */,
                 int /*src_fd*/,
                 void * /*src_addr*/,
                 int /*dst_fd*/,
                 void * /*dst_addr*/);

  void (*destroy)(RTCColorConvert *);
}RTCColorConvertHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
    int InitColorConvert(RTCColorConvertHandle_t *convert, void *handle);
#ifdef __cplusplus
}
#endif
#endif // CONVERT_H
