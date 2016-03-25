#ifndef _RTCCAMERA_H_
#define _RTCCAMERA_H_

#include "surface.h"

typedef void RTCCameraHandle;

typedef struct {
    int facing;
    int orientation;
}RTCCameraInfo_t;

typedef void (*RTCCamFrameCallback)(void *data, unsigned int size, void *context);
typedef void (*RTCCamNotify)(int32_t msgType, int32_t arg1, int32_t arg2, void *context);

typedef unsigned int RTCCamSurfacePixelFmt;

#define TAKE_PICTRUE_USAGE_JPEG           (0x1 << 0)
#define TAKE_PICTRUE_USAGE_YUV            (0x1 << 1)
#define TAKE_PICTRUE_USAGE_RAW            (0x1 << 2)
#define TAKE_PICTRUE_USAGE_AUTO_FOCUS     (0x1 << 3)
#define TAKE_PICTRUE_USAGE_ENABLE_SHUTTER (0x1 << 4)

typedef struct RTCCamera_{
  int (*getNum)();
  int (*getInfo)(int idx, RTCCameraInfo_t * info);

  RTCCameraHandle *(*create)(int idx);
  void (*destroy)(RTCCameraHandle *);

  int (*setPreviewCallback)(RTCCameraHandle*, RTCCamFrameCallback cb, void *context);
  int (*setRecordingCallback)(RTCCameraHandle*, RTCCamFrameCallback cb, void *context);
  int (*setSurface)(RTCCameraHandle*, unsigned int w, unsigned int h, \
                    RTCCamSurfacePixelFmt fmt, unsigned int usage);
  int (*setRTCSurface)(RTCCameraHandle *, const RTCSurface *sfs);
  int (*startPreview)(RTCCameraHandle*);
  int (*stopPreview)(RTCCameraHandle*);
  int (*startRecording)(RTCCameraHandle*);
  int (*stopRecording)(RTCCameraHandle*);
  int (*setParameters)(RTCCameraHandle*, const char *);
  int (*getParameters)(RTCCameraHandle*, char *buf, unsigned int size);

  int (*setPictureCallback)(RTCCameraHandle*, RTCCamFrameCallback cb, void *context);
  int (*takePicture)(RTCCameraHandle*, int type);
  int (*autoFocus)(RTCCameraHandle*);
  int (*cancelAutoFocus)(RTCCameraHandle*);
  int (*setNotifyCallback)(RTCCameraHandle*, RTCCamNotify cb, void *context);
}RTCCamera_t;

#ifdef __cplusplus
extern "C" {
#endif
    int InitCapture(RTCCamera_t *, void *handle);
#ifdef __cplusplus
}
#endif
#endif //_RTCCAMERA_H_
