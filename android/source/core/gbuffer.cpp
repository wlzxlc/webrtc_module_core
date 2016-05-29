#include <android/api-level.h>
#if __ANDROID_API__ >= 11
#include <stdint.h>
#include <binder/ProcessState.h>
#include <ui/GraphicBuffer.h>
#include "bug_helper.h"
#include "graphics_type.h"
#include "videobuffer.h"
#include "core.cc"

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

class VideoBufferImplGbuffer: public VideoBuffer{
public:
   virtual void release() {
        delete this;
    }
    ~VideoBufferImplGbuffer() {}
};

extern "C" {
    static graphics_handle *igalloc(int w, int h, graphics_pixformat pix, int usage)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = new VideoBufferImplGbuffer;
        buffer->graphicBuffer = new GraphicBuffer(w, h, (PixelFormat)pix, usage);
        return buffer;
    }

    static void igdestory(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        buffer->release();
    }
    static uint32_t  igwidth(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return buffer->graphicBuffer->getWidth();
    }
    static uint32_t  igheight(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return buffer->graphicBuffer->getHeight();
    }
    static uint32_t  igstride(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return buffer->graphicBuffer->getStride();
    }

    static uint32_t  igusage(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return buffer->graphicBuffer->getUsage();
    }

    static int       igpixelFormat(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return (int)buffer->graphicBuffer->getPixelFormat();
    }
    static ARect     igrect(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        Rect rect = buffer->graphicBuffer->getBounds();
        ARect r;
        r.left = rect.leftTop().x;
        r.top = rect.leftTop().y;
        r.bottom = buffer->graphicBuffer->getHeight() - rect.rightBottom().y;
        r.right = buffer->graphicBuffer->getWidth() - rect.rightBottom().x; 
        return r;
    }
    static int igreallocate(graphics_handle *gpc, uint32_t w, uint32_t h,  graphics_pixformat f, uint32_t usage)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return (int)buffer->graphicBuffer->reallocate(w,h,(PixelFormat)f,usage);
    }
    static int iglock(graphics_handle *gpc, uint32_t usage, void** vaddr)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return (int)buffer->graphicBuffer->lock(usage, vaddr);
    }
    static int iglockRect(graphics_handle *gpc, uint32_t usage, const ARect& rect, void** vaddr)
    {
        SCOPEDDEBUG();
        Rect r(rect.left, rect.top, rect.right, rect.bottom);
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return (int)buffer->graphicBuffer->lock(usage, r,  vaddr);
    }
    static int igunlock(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        return (int)buffer->graphicBuffer->unlock();
    }
    static const void * ighandle(graphics_handle *gpc, int * len)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        *len = sizeof(buffer_handle_t);
        return buffer->graphicBuffer->handle;
    }
    static void * igwinbuffer(graphics_handle *gpc)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        ANativeWindowBuffer *wbuffer = buffer->graphicBuffer.get();
        return wbuffer;
    }
#if __ANDROID_API__ >=18
    // For HAL_PIXEL_FORMAT_YCbCr_420_888^M
    static int iglockYCbCr(graphics_handle *gpc, uint32_t usage, graphics_ycbcr_t *ycbcr)
    {
        SCOPEDDEBUG();
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        android_ycbcr yuv;
        if (ycbcr && buffer->graphicBuffer->lockYCbCr(usage, &yuv)>=0){
            ycbcr->y = yuv.y;
            ycbcr->cb = yuv.cb;
            ycbcr->cr = yuv.cr;
            ycbcr->ystride = yuv.ystride;
            ycbcr->cstride = yuv.cstride;
            ycbcr->chroma_step = yuv.chroma_step;
            return 0;
        }
        return -__LINE__;
    }
    static int iglockYCbCrRect(graphics_handle *gpc, uint32_t usage, const ARect& rect, graphics_ycbcr_t *ycbcr)
    {
        VideoBuffer *buffer = static_cast<VideoBuffer *>(gpc);
        android_ycbcr yuv;
        Rect r(rect.left, rect.top, rect.right, rect.bottom);
        if (ycbcr && buffer->graphicBuffer->lockYCbCr(usage, r, &yuv)>=0){
            ycbcr->y = yuv.y;
            ycbcr->cb = yuv.cb;
            ycbcr->cr = yuv.cr;
            ycbcr->ystride = yuv.ystride;
            ycbcr->cstride = yuv.cstride;
            ycbcr->chroma_step = yuv.chroma_step;
            return 0;
        }
        return -__LINE__;
    }
#endif
    int PREFIX(Android_CreateOmxGraphics)(OmxGraphics_t *p)
    {
        if (p){
            memset(p, 0, sizeof(*p));
            p->alloc = igalloc;
            p->destory = igdestory;
            p->width = igwidth;
            p->height = igheight;
            p->stride = igstride;
            p->usage = igusage;
            p->pixelFormat = igpixelFormat;
            p->rect = igrect;
            p->reallocate = igreallocate;
            p->lock = iglock;
            p->lockRect = iglockRect;
            p->unlock = igunlock;
            p->handle = ighandle;
            p->winbuffer = igwinbuffer;
#if __ANDROID_API__ >= 18
            p->lockYCbCr = iglockYCbCr;
            p->lockYCbCrRect = iglockYCbCrRect;
#endif
            return 0;
        }
        return ERRNUM("Error parameters.");
    }
}
#endif // end of the ANDROID_SDK_VERSION
