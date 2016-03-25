#ifndef _WEBRTCAPI_H_
#define _WEBRTCAPI_H_
#include "OmxCore.h"
#include "videobuffer.h"
#include "surface.h"
#include "capture.h"
#include "convert.h"
#include "render.h"
#include "ipc.h"
#include "hw_jpeg.h"

typedef struct SysInternalAPI{
    int (*getprop)(const char *, char *);
    int (*setprop)(const char *, char *);
}SysInternalAPI_t;

namespace CCStone{
    class WebRTCAPI{
        private:
            void *_handle;
            RTCSurfaceHandle_t _sfs;
            RTCCamera_t _cap;
            OmxGraphics_t _gbs;
            OMXCore _core;
            SysInternalAPI_t _sapi;
            RTCColorConvertHandle_t _convert;
            RTCRenderHandle_t _render;
            RTCIPCHandle_t _ipc;
            RTCJpegCodec_t _jpeg;
            WebRTCAPI();
        public:
            static WebRTCAPI * Create();
            OMXCore *GetOMXCoreAPI();
            OmxGraphics_t *GetGraphicsAPI();
            RTCSurfaceHandle_t *GetSurfaceAPI();
            SysInternalAPI_t *GetSysInternalAPI();
            RTCCamera_t *GetCaptureAPI();
            RTCColorConvertHandle_t *GetColorConvertAPI();
            RTCRenderHandle_t *GetRenderAPI();
            RTCIPCHandle_t *GetIPCInteraceAPI();
            RTCJpegCodec_t *GetVendorJPEGAPI();
    };
}
#endif
