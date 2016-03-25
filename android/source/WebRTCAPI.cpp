#include <sys/system_properties.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include "bug_helper.h"
#include "WebRTCAPI.h"

static const char *vendor_name[] = {
#ifdef VENDOR_NAME
 VENDOR_NAME,
#endif
 "qcom",
 "stone"
};

static int _getprop(const char *, char *buffer)
{
  return ERRNUM("Unsupported");
}

static int _setprop(const char *, char *buffer)
{
  return ERRNUM("Unsupported");
}

namespace CCStone{
    WebRTCAPI * WebRTCAPI::Create()
    {
        static WebRTCAPI api;
        return &api;
    }
    WebRTCAPI::WebRTCAPI():_handle(NULL){
        memset(&_sfs, 0 , sizeof(_sfs));
        memset(&_cap, 0 , sizeof(_cap));
        memset(&_gbs, 0 , sizeof(_gbs));
        memset(&_sapi, 0 , sizeof(_sapi));
        memset(&_convert, 0, sizeof(_convert));
        memset(&_render, 0, sizeof(_render));
        memset(&_ipc, 0, sizeof(_ipc));
        memset(&_jpeg, 0, sizeof(_jpeg));
        char buff[1024] = {0};
        __system_property_get("ro.build.version.sdk",buff);
        int32_t sdk = atoi(buff);
        if (sdk){
            int32_t tsdk = sdk;
            void *hwd = NULL;
            while(!hwd && tsdk > 10 ){
                char buffer[256] = {0};
                uint32_t vendor = 0;
                while(vendor < sizeof(vendor_name) / sizeof(char *)){
                    sprintf(buffer,"lib%s_omx_%d.so",vendor_name[vendor], tsdk);
                    DEBUG("Loading '%s' library.", buffer);        
                    hwd = dlopen(buffer, RTLD_NOW);
                    if (hwd){
                        DEBUG("Dlopen '%s' library succeed!", buffer);

                        // Get VideoBuffer  interface
                        if (InitVideoBuffer(&_gbs, hwd) < 0 ){
                            DEBUG("Init VideoBuffer failed.");
                        }

                        // Get Surface interface
                        if (InitSurface(&_sfs, hwd) < 0 ){
                            DEBUG("Init RTCSurface failed.");
                        }

                        // Get Capture interface
                        if (InitCapture(&_cap, hwd) < 0 ){
                            DEBUG("Init RTCCapture failed.");
                        }

                        // Get OpenMAX interface 
                        if (!_core.OpenLibs(NULL, hwd)){
                            DEBUG("Init OMXCore failed.");
                        }   

                        // Get Color convert interface
                        if (InitColorConvert(&_convert, hwd) < 0){
                            DEBUG("Init ColorConvert failed.");
                        }

                        // Get Render interface
                        if (InitRTCRender(&_render, hwd) < 0){
                            DEBUG("Init Render api failed.");
                        }

                        // Get IPC interface
                        if (InitRTCIPC(&_ipc, hwd) < 0){
                            DEBUG("Init IPC api failed.");
                        }

                        // Get Vendor JPEG interface
                        if (InitVendorJpegCodec(&_jpeg, hwd) < 0){
                            DEBUG("Init Vendor JPEG api failed.");
                        }

                        {
                            // Init local internal api.
                            typedef int (*type_prop)(const char *,char *);
                            type_prop getprop = (type_prop)dlsym(hwd, "igetprop");
                            type_prop setprop = (type_prop)dlsym(hwd, "isetprop");
                            _sapi.getprop = getprop ? getprop : _getprop;
                            _sapi.setprop = setprop ? setprop : _setprop;
                        }

                        _handle = hwd;
                        break;
                    }else{
                        DEBUG("%s", dlerror());
                        vendor++;
                    }
                }
                tsdk--;
            }

            if (!_handle){
             DEBUG("Not found a valid WebRTCAPI library.");
            }
        }
    }

    OMXCore * WebRTCAPI::GetOMXCoreAPI()
    {
        return &_core;
    }

    OmxGraphics_t *WebRTCAPI::GetGraphicsAPI()
    {
        return &_gbs;
    }

    RTCSurfaceHandle_t *WebRTCAPI::GetSurfaceAPI()
    {
        return &_sfs;
    }

    SysInternalAPI_t * WebRTCAPI::GetSysInternalAPI()
    {
        return &_sapi;
    }

    RTCCamera_t *WebRTCAPI::GetCaptureAPI()
    {
        return &_cap;
    }

    RTCColorConvertHandle_t *WebRTCAPI::GetColorConvertAPI()
    {
      return &_convert;
    }

    RTCRenderHandle_t *WebRTCAPI::GetRenderAPI()
    {
      return &_render;
    }

    RTCIPCHandle_t *WebRTCAPI::GetIPCInteraceAPI()
    {
      return &_ipc;
    }

    RTCJpegCodec_t *WebRTCAPI::GetVendorJPEGAPI()
    {
      return &_jpeg;
    }
}
