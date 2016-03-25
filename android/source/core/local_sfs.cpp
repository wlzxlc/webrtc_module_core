
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <binder/ProcessState.h>
#include "bug_helper.h"
#include "render.h"
#if defined(ANDROID_ON_QCOM) && (ANDROID_SDK_VERSION >= 21)
#include <gui/Surface.h>
#include <gui/GLConsumer.h>

#if defined(ANDROID_ON_QCOM)
#include <camera/ICameraRecordingProxyListener.h>
#endif
#include <camera/ICamera.h>
#include <camera/Camera.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include "graphics_type.h"
#include "core.cc"
using namespace android;

class VideoBufferImplLocalSurface: public VideoBuffer{
    public:
        int fence;
        virtual void release()
        {
            assert(!"Should not be here.");
            //delete this;
        }
};

typedef struct LocalSurfaceImpl{
 void * pri_data;
 sp<SurfaceComposerClient> mComposerClient;
 sp<SurfaceControl> mSurfaceControl;
 sp<Surface> mSurface;
 ANativeWindow *mSurface2;
}LocalSurfaceImpl;

static void *icreate(int x, int y, int w, int h)
{
    LocalSurfaceImpl *local_surface = new LocalSurfaceImpl;
    local_surface->mSurface2 = NULL;
    local_surface->mComposerClient = new SurfaceComposerClient;
    if (NO_ERROR != local_surface->mComposerClient->initCheck()){
        DEBUG("initCheck Faild.");
     delete local_surface;
     return NULL;
    }

    local_surface->mSurfaceControl = local_surface->mComposerClient->createSurface(
            String8("LocalSurfaceImpl"),
            w, h, PIXEL_FORMAT_RGB_565, 0);

    if (local_surface->mSurfaceControl != NULL && 
            local_surface->mSurfaceControl->isValid()) {
        local_surface->mSurfaceControl->setPosition(x, y);
        //local_surface->mSurfaceControl->setAlpha(0.5);
    } else {
        DEBUG("Create Surface control failed\n");
        delete local_surface;
        return NULL;
    }

    SurfaceComposerClient::openGlobalTransaction();
    local_surface->mSurfaceControl->setLayer(0x7FFFFFFF);
    local_surface->mSurfaceControl->show();
    SurfaceComposerClient::closeGlobalTransaction();
    local_surface->mSurface = local_surface->mSurfaceControl->getSurface();
    sp<ANativeWindow> mNativeWindow(local_surface->mSurface);
#if 0
    # Test YUV display code scoped on MSM8939@Adreno GPU 400MH
    # a way RGB + a way RGB  --GPU 0%
    # a way RGB + a way RGB + a way RGB  --GPU 50%
    # a way RGB + a way RGB + Camera preview --GPU 50%
    # a way RGB + Camera preview --GPU 0%
    # a way YUV + Camera preview --GPU 70%
    # a way RGB + a way YUV  --GPU 0%
    # Conclusion:
    # MDP = framebuffer(one channel of rgb) + overly(two channel of rgb) or
    # MDP = framebuffer(one channel of rgb) + overly(one channel of rgb) + overly(one channel of YUV)

    NV12@32M color format, for decoder to display
    int ret = native_window_set_buffers_geometry(mNativeWindow.get(), w, h, 2141391876);
    assert(ret == 0);
    int queuesToNativeWindow = 0;
    ret = mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER, &queuesToNativeWindow);
    assert(ret == 0);
    if (queuesToNativeWindow != 1) {
        printf("queuesToNativeWindow no permissions...\n");
    }
    native_window_set_usage(mNativeWindow.get(), GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);
#endif
    return local_surface;
}

static void *icreate_from_java(void *java_surface)
{
    LocalSurfaceImpl *local_surface = new LocalSurfaceImpl;
    local_surface->mSurface2 = static_cast<ANativeWindow *>(java_surface);
    return local_surface;
}

static graphics_handle *idequeuebuffer(void *surface)
{
    VideoBufferImplLocalSurface *lbuffer = new VideoBufferImplLocalSurface;
    LocalSurfaceImpl *lsurface = static_cast<LocalSurfaceImpl *>(surface);
    ANativeWindow *window = lsurface->mSurface2;
    if (window == NULL){
     window = lsurface->mSurface.get();
    }
    ANativeWindowBuffer * buffer = NULL;
    int ret = native_window_dequeue_buffer_and_wait(window, &buffer);
    // int ret = window->dequeueBuffer(window.get(), &buffer, &lbuffer->fence);
     if (ret < 0 ){
      DEBUG("dequeueBuffer faild. ret %#x", ret);
      delete lbuffer;
      return NULL;
     }
     lbuffer->graphicBuffer = new GraphicBuffer(buffer, false);
     return lbuffer;
}

static int iqueuebuffer(void *surface, graphics_handle *buffer)
{
    VideoBufferImplLocalSurface *lbuffer = static_cast<VideoBufferImplLocalSurface *>
        (buffer);
    LocalSurfaceImpl *lsurface = static_cast<LocalSurfaceImpl *>
        (surface);
     ANativeWindow *window = lsurface->mSurface2;
     if (window == NULL){
        window = lsurface->mSurface.get();
     }
     int ret = window->queueBuffer(window, 
               lbuffer->graphicBuffer.get(),
               -1);
     // Next version optmized.
     // queue->push(lbuffer)
     delete lbuffer;
     if (ret < 0 ){
      DEBUG("QueueBuffer faild. ret %#x", ret);
      return -__LINE__;
     }
     return 0;
}

static void idestory(void * surface)
{
   LocalSurfaceImpl * lsfs = (LocalSurfaceImpl *)(surface);
   delete lsfs;
}

static int iget_display_info(int32_t id, uint32_t *w,
                            uint32_t *h, float *fps) {
  int32_t disp_id;
  if (0 == id) {
    disp_id = ISurfaceComposer::eDisplayIdMain;
  } else if (1 == id) {
    disp_id = ISurfaceComposer::eDisplayIdHdmi;
  } else {
    DEBUG("Invalid display Id\n");
    return -1;
  }
  DisplayInfo dinfo;
  sp<IBinder> display(SurfaceComposerClient::getBuiltInDisplay(disp_id));
  status_t ret = SurfaceComposerClient::getDisplayInfo(display, &dinfo);
  if (ret != NO_ERROR) {
    DEBUG("getDisplayInfo failed!\n");
    return -1;
  }
  *w = dinfo.w;
  *h = dinfo.h;
  *fps = dinfo.fps;

  return 0;
}

extern "C" {
    int PREFIX(Android_CreateRender)(RTCRenderHandle_t *p)
    {
        if (p){
         memset(p, 0, sizeof(*p));
         p->create = icreate;
         p->create_from_java = icreate_from_java;
         p->queue = iqueuebuffer;
         p->dequeue = idequeuebuffer;
         p->destroy = idestory;
         p->get_display_info = iget_display_info;
        }
        return 0;
    }
}
#endif
