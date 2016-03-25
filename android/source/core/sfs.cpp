#include "graphics_type.h"
#include "surface.h"
#include "bug_helper.h"
#include <stdlib.h>
#include <string.h>

#if ANDROID_SDK_VERSION >= 21
#include <binder/ProcessState.h>
#include <gui/Surface.h>
#include "core.cc"

using namespace android;

class LocalBuffer :public VideoBuffer{
public:
        LocalBuffer(sp<IGraphicBufferConsumer> &cons)
            :mConsumer(cons){
            slot = 0;
            frameCount = 0;
        }
        ~LocalBuffer() {}

        int slot;
        int64_t frameCount;

        //Implement VideoBuffer
        void release() { 
            if (mConsumer.get()) {
                mConsumer->releaseBuffer(slot, frameCount,
                    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
                DEBUG("VideoBuffer Release Buffer slot %d\n", slot);
            }
        }
        sp<IGraphicBufferConsumer> mConsumer;
};

struct DummyConsumer : public BnConsumerListener {
    public:
        sp<IGraphicBufferConsumer> mConsumer;
        LocalBuffer *_gslot[BufferQueue::NUM_BUFFER_SLOTS];
        int slot;
        RTCSurfaceCallback _cb;
        void * _cb_data;
        RTCSurface *_target_sfs;
        DummyConsumer(sp<IGraphicBufferConsumer> &cons, RTCSurfaceCallback cb,
                      void *priv_data)
            :mConsumer(cons){
            slot = 0; 
            _cb = cb;
            _cb_data = priv_data;
            _target_sfs = NULL;
            for (int i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; ++i) {
                _gslot[i] = new LocalBuffer(cons);
            }
        }

        ~DummyConsumer() {
             for (int i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; ++i) {
                 if (_gslot[i]) {
                    delete _gslot[i];
                    _gslot[i] = NULL;
                 }
             }
        }
#if ANDROID_SDK_VERSION >= 22
       virtual void onFrameAvailable(const android::BufferItem& /*item*/) {
#if ANDROID_SDK_VERSION == 22
          IGraphicBufferConsumer::BufferItem item;
#elif ANDROID_SDK_VERSION == 23
          android::BufferItem item;
#endif
          int err = (int) mConsumer->acquireBuffer(&item, static_cast<nsecs_t>(0));
          if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
            // shouldn't happen
            DEBUG("fillCodecBuffer_l: frame was not available\n");
            return ;
          } else if (err != 0) {
            // now what? fake end-of-stream?
            DEBUG("fillCodecBuffer_l: acquireBuffer returned err=%d\n", err);
            return;
          }else{
            DEBUG("Get BufferSlot %d item %p timestamp: %llu\n", item.mBuf, &item, item.mTimestamp);
          }
          if (item.mFence.get() != NULL) {
            err = item.mFence->waitForever("waitForever");
            if (err != 0) {
              DEBUG("failed to wait for buffer fence: %d", err);
            }
          }
          sp<GraphicBuffer> buffer = item.mGraphicBuffer;
          if (buffer.get()){
            _gslot[item.mBuf]->graphicBuffer = buffer;
          }else {
            buffer = _gslot[item.mBuf]->graphicBuffer;
          }

          _gslot[(int)item.mBuf]->slot = item.mBuf;
          _gslot[(int)item.mBuf]->frameCount = item.mFrameNumber;
          DEBUG("Get a openGlES  colorformat %#x usage %#x slot %d\n", \
                (int) buffer->getPixelFormat(),buffer->getUsage(), item.mBuf);

          if (_cb)
            _cb(_target_sfs ,(graphics_handle*)_gslot[item.mBuf], _cb_data);
          #if 0
            mConsumer->releaseBuffer(item.mBuf, item.mFrameNumber,
                    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
            DEBUG("Release Buffer slot %d\n", item.mBuf);
          #endif
        }
#else
        virtual void onFrameAvailable() { 
            BufferQueue::BufferItem item; 
            item.mBuf = -1;
            DEBUG("Create Item Slot %d item %p\n", item.mBuf, &item);
            int err = (int) mConsumer->acquireBuffer(&item, 0);
            if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
                // shouldn't happen      
                DEBUG("fillCodecBuffer_l: frame was not available\n");
                return ;
            } else if (err != 0) {  
                // now what? fake end-of-stream?
                DEBUG("fillCodecBuffer_l: acquireBuffer returned err=%d\n", err);
                return;
            }else{
                DEBUG("Get BufferSlot %d item %p timestamp: %llu\n", item.mBuf, &item, item.mTimestamp);
            }
            err = item.mFence->waitForever("waitForever");
            if (err != 0) {    
                DEBUG("failed to wait for buffer fence: %d", err);
            }
            sp<GraphicBuffer> buffer = item.mGraphicBuffer;
            if (buffer.get()){
                _gslot[item.mBuf]->graphicBuffer = buffer;
            }else{
                buffer = _gslot[item.mBuf]->graphicBuffer;
            }
            _gslot[item.mBuf]->slot = item.mBuf;
            _gslot[item.mBuf]->frameCount = item.mFrameNumber;
            DEBUG("Get a openGlES  colorformat %#x usage %#x slot %d\n", 
                    (int) buffer->getPixelFormat(),buffer->getUsage(),
                    item.mBuf);
            
            if (_cb)
                _cb(_target_sfs ,(graphics_handle*)_gslot[item.mBuf], _cb_data);
            #if 0
            mConsumer->releaseBuffer(item.mBuf, item.mFrameNumber,
                    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
            DEBUG("Release Buffer slot %d\n", item.mBuf);
            #endif
        }
#endif
        void releaseBuffer(VideoBuffer *buffer){
            LocalBuffer *buf = static_cast<LocalBuffer *>(buffer);
            mConsumer->releaseBuffer(buf->slot, buf->frameCount,
                    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
            DEBUG("Release Buffer slot %d\n", buf->slot);
        }
        virtual void onBuffersReleased() {DEBUG("%s\n", __func__);}
        virtual void onSidebandStreamChanged() {DEBUG("%s\n", __func__);}
}; 

extern "C" {
    static int isurface_destory(RTCSurface *surface)
    {
        if (surface){
            LocalSurface *ret = (LocalSurface *)surface;
            DummyConsumer *csr = ret->csr;
            delete ret;
            if (csr)
                delete csr;
        }
        return 0;
    }

    static int isurface_create(RTCSurface **psurface, int w, int h, 
            uint32_t usage, RTCSurfaceCallback cb, int buff_counts,
                               void *priva_data)
    {
        if (psurface && w > 1 && h > 1 && usage != 0 && cb && buff_counts){
            sp<IGraphicBufferProducer> producer;
            sp<IGraphicBufferConsumer> consumer;
            BufferQueue::createBufferQueue(&producer, &consumer);
            String8 name("LocalSurface");
            consumer->setConsumerName(name);
            consumer->setDefaultBufferSize(w, h);
            consumer->setConsumerUsageBits(usage);
            consumer->setDefaultMaxBufferCount(BufferQueue::NUM_BUFFER_SLOTS);
            consumer->setMaxAcquiredBufferCount(buff_counts);
            DummyConsumer * csr = new DummyConsumer(consumer, cb, priva_data);
            consumer->consumerConnect(csr, false);
            sp<Surface> mSTC = new Surface(producer);
            LocalSurface *ret = new LocalSurface;
            ret->surface = mSTC;
            ret->producer = producer;
            ret->csr = csr;
            csr->_target_sfs = ret; 
            *psurface = ret;
            return 0;
        }else{
            return ERRNUM("Error parameters");
        }
        return ERRNUM("Unknow error");
    }

    static int isurface_window(RTCSurface *surface,void **pwindow)
    {
        if (surface && pwindow){
            LocalSurface *ret = (LocalSurface *)surface;
            ANativeWindow *window = ret->surface.get();
            *pwindow = window;
            return 0;
        }
        return ERRNUM("Unknow error");
    }

    static int irelease_buffer(RTCSurface *surface, graphics_handle *buffer)
    {
        if (surface && buffer){
            LocalSurface *ret = (LocalSurface *)surface;
            ret->csr->releaseBuffer((VideoBuffer *)buffer);
            return 0;
        }
        return ERRNUM("Unknow error");
    }

}
#endif // end of the ANDROID_SDK_VERSION >= 21

extern "C" {
    int PREFIX(Android_SurfaceHandle)(RTCSurfaceHandle_t *handle)
    {
        if (handle){
            memset(handle, 0, sizeof(*handle));
#if ANDROID_SDK_VERSION >= 21
            handle->create = isurface_create;
            handle->destroy = isurface_destory;
            handle->releaseBuffer = irelease_buffer;
            handle->window = isurface_window;
#endif
        }
        return 0;
    }
}
