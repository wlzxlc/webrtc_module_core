#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <utils/Vector.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>

#include <binder/ProcessState.h>
#include "bug_helper.h"
#include "capture.h"
#include "surface.h"
#if defined(ANDROID_ON_QCOM) && (ANDROID_SDK_VERSION >= 21)
#include <gui/Surface.h>
#include <gui/GLConsumer.h>

#if (ANDROID_SDK_VERSION >=22)
#include <gui/BufferItem.h>
#endif

#if defined(ANDROID_ON_QCOM)
#include <camera/ICameraRecordingProxyListener.h>
#endif
#include <camera/ICamera.h>
#include <camera/Camera.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <gui/SurfaceComposerClient.h>

#include "graphics_type.h"
#include "core.cc"

#define GBUSAGE IOMX_GRALLOC_USAGE_HW_RENDER | \
            IOMX_GRALLOC_USAGE_HW_VIDEO_ENCODER | \
            IOMX_GRALLOC_USAGE_HW_TEXTURE

#if 0 //just for logcat debug
#include <android/log.h>

#ifdef DEBUG
#undef DEBUG
#define DEBUG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif
#endif

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

#define SURFACE_USAGE_FORMAT_MASK	(0xff)
#define SURFACE_USAGE_TEXTURE	    (0x0)
#define SURFACE_USAGE_COMPOSER	  (0x1)

#define frameCallback       RTCCamFrameCallback
#define camera_info_t       RTCCameraInfo_t
#define camera_handle       RTCCameraHandle
#define surface_pixelFmt    RTCCamSurfacePixelFmt
#define notifyCallback      RTCCamNotify

class CameraNotifyThread;
class OMXCamera{
  public:
  sp<android::Camera> mCamera;
  sp<CameraListener> mListener;
  #if defined(ANDROID_ON_QCOM)
  sp<ICameraRecordingProxyListener> mRecordListener;
  #endif

  //Surface
  sp<SurfaceComposerClient> mComposerClient;
  sp<SurfaceControl> mSurfaceControl;
  sp<Surface> mSurface;

  //SurfaceTexture
  sp<IGraphicBufferProducer> mProducer;
  sp<IGraphicBufferConsumer> mConsumer;

  frameCallback mPreviewFrameCb;
  void *mPreviewCbData;

  #if defined(ANDROID_ON_QCOM)
  frameCallback mRecordingFrameCb;
  void *mRecordingCbData;
  frameCallback mTakePicCb;
  void *mTakePicCbData;
  sp<CameraNotifyThread> mNotifyThread;

  Condition mAutoFocusCondition;
  Mutex mAutoFocusMutex;
  static void DummyCamNotity(int32_t msgType, int32_t arg1, int32_t arg2, void *context) {
    OMXCamera *me = static_cast<OMXCamera*>(context);
    if (me != NULL && msgType == CAMERA_MSG_FOCUS) {
      me->mAutoFocusMutex.lock();
      if (arg1 == 1) {
        me->mAutoFocusCondition.signal();
        me->mCamera->cancelAutoFocus();
      }
      me->mAutoFocusMutex.unlock();
    }
  }

  #endif
};

class CameraNotifyThread : public Thread {
  typedef struct notifyMsg {
    int32_t msgType;
    int32_t arg1;
    int32_t arg2;
  }notifyMsg;
  typedef struct notifyFun {
    notifyCallback cb;
    void *ctx;
  }notifyFun;
  public:
  CameraNotifyThread() {
  }
  virtual bool threadLoop() {
    Mutex::Autolock lock(mNotifyQueueMutex);
    if (mNotifyMsgQueue.size() == 0) {
      mNotifyMsgQueueCondition.wait(mNotifyQueueMutex);
    }

    Mutex::Autolock cb_lock(mNotifyCbListMutex);
    if (mNotifyCbList.size() == 0)
      return true;
    else {
      for (uint32_t i = 0; i < mNotifyCbList.size(); ++i) {
        notifyFun pFun = mNotifyCbList.itemAt(i);
        if (pFun.cb == NULL)
          continue;
        Vector<notifyMsg>::const_iterator it = mNotifyMsgQueue.begin();
        while(it != mNotifyMsgQueue.end()) {
          (*(pFun.cb))(it->msgType, it->arg1, it->arg2, pFun.ctx);
          ++it;
        }
      }
      mNotifyMsgQueue.clear();
    }
    return true;
  }
  int32_t QueueMsg(int32_t msgType, int32_t arg1, int32_t arg2) {
    Mutex::Autolock lock(mNotifyQueueMutex);
    notifyMsg msg;
    msg.msgType = msgType;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    mNotifyMsgQueue.push(msg);
    mNotifyMsgQueueCondition.broadcast();
    return 0;
  }

  int32_t RegisterCallback(notifyCallback cb, void *ctx) {
    Mutex::Autolock lock(mNotifyCbListMutex);
    notifyFun fun;
    fun.cb = cb;
    fun.ctx = ctx;
    mNotifyCbList.push(fun);
    return 0;
  }

  int32_t DeRegisterCallback(notifyCallback cb) {
    Mutex::Autolock lock(mNotifyCbListMutex);
    bool found = false;
    if (mNotifyCbList.size() == 0)
      return -1;
    for (uint32_t i = 0; i < mNotifyCbList.size(); ++i) {
      if (cb == mNotifyCbList.itemAt(i).cb) {
        mNotifyCbList.removeAt(i);
        found = true;
        break;
      }
    }
    return found == true ? 0 : -1;
  }
  private:
    Mutex mNotifyQueueMutex;
    Vector<notifyMsg> mNotifyMsgQueue;
    Condition mNotifyMsgQueueCondition;

  Mutex mNotifyCbListMutex;
  Vector<notifyFun> mNotifyCbList;
  };

class CameraListenerImpl: public CameraListener
{
  public:
  virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
  virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr,
                        camera_frame_metadata_t *metadata);
  virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

  CameraListenerImpl(OMXCamera* pOmxCamera);
  ~CameraListenerImpl();
  private:
  void copyAndPost(const sp<IMemory>& dataPtr, int msgType);
  OMXCamera* mOmxCamera;
};

#if defined(ANDROID_ON_QCOM)
class CameraRecordingProxyListenerImpl : public ICameraRecordingProxyListener {
  public:
  CameraRecordingProxyListenerImpl(OMXCamera* pOmxCamera);
  ~CameraRecordingProxyListenerImpl();

  virtual void dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                                     const sp<IMemory>& data);
  virtual IBinder*            onAsBinder() { return NULL;}

  private:
  OMXCamera* mOmxCamera;
};

CameraRecordingProxyListenerImpl::CameraRecordingProxyListenerImpl(OMXCamera* pCamera) {
  mOmxCamera = pCamera;
}
CameraRecordingProxyListenerImpl::~CameraRecordingProxyListenerImpl() {
}
void CameraRecordingProxyListenerImpl::dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                                                             const sp<IMemory>& data) {
  DEBUG("RecordingFrame: type(%d)", msgType);
  if (mOmxCamera->mRecordingFrameCb != NULL  && data != NULL) {
    ssize_t offset;
    size_t size;
    sp<IMemoryHeap> heap = data->getMemory(&offset, &size);
    DEBUG("RecordingFrame: off=%d, size=%d memSz: %d timestamp: %llu",
    offset, size, data->size(), timestamp);

    uint8_t *heapBase = (uint8_t*)heap->base();
    const char* dataPtr = reinterpret_cast<const char*>(heapBase + offset);
    mOmxCamera->mRecordingFrameCb((void*)dataPtr, size, mOmxCamera->mRecordingCbData);
    mOmxCamera->mCamera->releaseRecordingFrame(data);
  }
}
#endif

CameraListenerImpl::CameraListenerImpl(OMXCamera* pCamera) {
  mOmxCamera = pCamera;
}
CameraListenerImpl::~CameraListenerImpl() {
}
void CameraListenerImpl::notify(int32_t msgType, int32_t ext1, int32_t ext2) {
  DEBUG("listener notify (Msgtype: %d)", msgType);
  if (mOmxCamera != NULL && \
      mOmxCamera->mNotifyThread != NULL) {
        mOmxCamera->mNotifyThread->QueueMsg(msgType, ext1, ext2);
  }
  // VM pointer will be NULL if object is released
}
void CameraListenerImpl::postData(int32_t msgType, const sp<IMemory>& dataPtr,
                                  camera_frame_metadata_t *metadata) {
  DEBUG("PostData msgType: %#x\n", msgType);
  int32_t dataMsgType = msgType & ~CAMERA_MSG_PREVIEW_METADATA;

  // return data based on callback type
  switch (dataMsgType) {
    case CAMERA_MSG_VIDEO_FRAME:
    // should never happen
    DEBUG("dataMsgType: video frame\n");
    break;
    // For backward-compatibility purpose, if there is no callback
    // buffer for raw image, the callback returns null.
    case CAMERA_MSG_RAW_IMAGE:
    DEBUG("dataMsgType: raw image\n");
    copyAndPost(dataPtr, dataMsgType);
    break;
    case CAMERA_MSG_PREVIEW_FRAME:
    DEBUG("dataMsgType: preview frame\n");
    copyAndPost(dataPtr, dataMsgType);
    break;
    case CAMERA_MSG_COMPRESSED_IMAGE:
    DEBUG("dataMsgType: compressed image\n");
    copyAndPost(dataPtr, dataMsgType);
    break;
    case 0:
    break;
    default:
    DEBUG("dataCallback(%d, %p)", dataMsgType, dataPtr.get());
    copyAndPost(dataPtr, dataMsgType);
    break;
  }
}
void CameraListenerImpl::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {
  // TODO: plumb up to Java. For now, just drop the timestamp
  DEBUG("Timestamp : %d, msgType: %d\n", (uint32_t)timestamp, msgType);
  postData(msgType, dataPtr, NULL);
}
void CameraListenerImpl::copyAndPost(const sp<IMemory>& data, int msgType)
{
  if (mOmxCamera != NULL && \
      mOmxCamera->mPreviewFrameCb != NULL && \
      data != NULL) {
    ssize_t offset;
    size_t size;
    sp<IMemoryHeap> heap = data->getMemory(&offset, &size);
    DEBUG("copyAndPost: off=%d, size=%d memSz: %d\n", offset, size, data->size());

    uint8_t *heapBase = (uint8_t*)heap->base();
    const char* dataPtr = reinterpret_cast<const char*>(heapBase + offset);

    if (msgType == CAMERA_MSG_PREVIEW_FRAME) {
      DEBUG("mPreviewFrameCb: %p\n", mOmxCamera->mPreviewFrameCb);
      if (mOmxCamera->mPreviewFrameCb)
      mOmxCamera->mPreviewFrameCb((void*)dataPtr, size, mOmxCamera->mPreviewCbData);
    } else if (msgType == CAMERA_MSG_RAW_IMAGE) {
      #if defined(ANDROID_ON_QCOM)
      DEBUG("mRawImageCb: %p\n", mOmxCamera->mTakePicCb);
      if (mOmxCamera->mTakePicCb)
      mOmxCamera->mTakePicCb((void*)dataPtr, size, mOmxCamera->mTakePicCbData);
      #endif
    } else if (msgType == CAMERA_MSG_COMPRESSED_IMAGE) {
      #if defined(ANDROID_ON_QCOM)
      DEBUG("mJpegCompressImageCb: %p\n", mOmxCamera->mTakePicCb);
      if (mOmxCamera->mTakePicCb)
        mOmxCamera->mTakePicCb((void*)dataPtr, size, mOmxCamera->mTakePicCbData);
      #endif
    }
  }
}

static int createOnScreenSurface(OMXCamera* pCamera, uint32_t w, uint32_t h,
                                 PixelFormat fmt, uint32_t usage) {
  if (pCamera == NULL)
    return -1;

  pCamera->mComposerClient = new SurfaceComposerClient;
  assert(NO_ERROR == pCamera->mComposerClient->initCheck());

  pCamera->mSurfaceControl = pCamera->mComposerClient->createSurface(
    String8("KedacomICamera Surface"),
    w/*getSurfaceWidth()*/, h/*getSurfaceHeight()*/,
    fmt/*PIXEL_FORMAT_RGB_888*/, usage/*0*/);

  if (pCamera->mSurfaceControl != NULL)
  pCamera->mSurfaceControl->setPosition(0, 0);
  else {
    DEBUG("Create Surface control failed\n");
    return -1;
  }

  assert(pCamera->mSurfaceControl != NULL);
  assert(pCamera->mSurfaceControl->isValid());

  SurfaceComposerClient::openGlobalTransaction();
  pCamera->mSurfaceControl->setLayer(0x7FFFFFFF);
  pCamera->mSurfaceControl->show();
  SurfaceComposerClient::closeGlobalTransaction();

  sp<ANativeWindow> window = pCamera->mSurfaceControl->getSurface();
  pCamera->mSurface = pCamera->mSurfaceControl->getSurface();

  assert(pCamera->mSurface.get() != NULL);

  return 0;
}

class VideoBufferImplCap : public VideoBuffer {
  public:
  void release() {
    assert(!"Should not be here.");
  }
};

static void createOnScreenSurfaceTexture(OMXCamera* pCamera, uint32_t w, uint32_t h,
                                         PixelFormat fmt, uint32_t usage) {
  if (pCamera == NULL)
    return;

  bool singleBufferMode = true;
  DEBUG("CreateSurfaceTexture....\n");
  sp<IGraphicBufferProducer> producer;
  sp<IGraphicBufferConsumer> consumer;
  BufferQueue::createBufferQueue(&producer, &consumer);

  if (singleBufferMode) {
    consumer->disableAsyncBuffer();
    consumer->setDefaultMaxBufferCount(20);
    consumer->setConsumerUsageBits(GBUSAGE);
  }
  producer->setBufferCount(15);
  class DummyConsumer : public BnConsumerListener {
    private:
    VideoBufferImplCap m_GBufslot[BufferQueue::NUM_BUFFER_SLOTS];
    sp<IGraphicBufferConsumer> m_consumer;
    public:
    DummyConsumer(sp<IGraphicBufferConsumer> &consumer) {
      m_consumer = consumer;
    }
    ~DummyConsumer() {}
#if ANDROID_SDK_VERSION >= 22
    virtual void onFrameAvailable(const android::BufferItem& /*item*/) {
#if ANDROID_SDK_VERSION == 22
			IGraphicBufferConsumer::BufferItem item;
#elif ANDROID_SDK_VERSION == 23
      android::BufferItem item;
#endif
      int err = (int) m_consumer->acquireBuffer(&item, static_cast<nsecs_t>(0));
      if (err != 0) {
        printf("acquireBuffer failed!\n");
        return;
      }
      m_consumer->releaseBuffer(item.mBuf, item.mFrameNumber, \
                                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
    }
#else
    virtual void onFrameAvailable() {
      BufferQueue::BufferItem item;
      item.mBuf = -1;
      //DEBUG("Create Item Slot %d item %p\n", item.mBuf, &item);
      int err = (int) m_consumer->acquireBuffer(&item, 0);
      if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
        // shouldn't happen
        DEBUG("fillCodecBuffer_l: frame was not available\n");
        return ;
      } else if (err != 0) {
        // now what? fake end-of-stream?
        DEBUG("fillCodecBuffer_l: acquireBuffer returned err=%d\n", err);
        return;
      }else{
        DEBUG("Get BufferSlot %d item %p\n", item.mBuf, &item);
      }

      sp<GraphicBuffer> buffer = item.mGraphicBuffer;
      if (buffer.get()){
        m_GBufslot[item.mBuf].graphicBuffer = buffer;
      }else{
        buffer = m_GBufslot[item.mBuf].graphicBuffer;
      }

      DEBUG("Get a openGlES  colorformat %#x usage %#x slot %d\n",
            (int) buffer->getPixelFormat(),buffer->getUsage(),
            item.mBuf);

      m_consumer->releaseBuffer(item.mBuf, item.mFrameNumber,
                                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR, Fence::NO_FENCE);
      //DEBUG("releaseBuffer idx: %#x status_t : %d\n", item.mFrameNumber, ret_s);
      DEBUG("Release Buffer slot %d\n", item.mBuf);
    }
#endif
    virtual void onBuffersReleased() {
      DEBUG("OnBufferReleased.................................\n");
    }
    virtual void onSidebandStreamChanged() {}
  };
  consumer->consumerConnect(new DummyConsumer(consumer), false);

  pCamera->mProducer = producer;
  pCamera->mConsumer = consumer;
  DEBUG("CreateSurfaceTexture....\n");
}

extern "C" {
  static String16 GetPackageName() {
    String16 name("com.kedacom.camera");

    uint32_t pid = getpid();
    DEBUG("Cur process pid: %d\n", pid);
    char szBuf[50] = {0};
    sprintf(szBuf, "/proc/%d/cmdline", pid);

    DEBUG("Path: %s\n", szBuf);

    char szPkgName[100] = {0};
    int fd = open(szBuf, O_RDONLY);
    if (fd) {
      read(fd, szPkgName, 100);
      close(fd);
      DEBUG("GetPkgName : %s\n", szPkgName);
      name = String16(szPkgName);
    }

    return name;
  }

  static bool icJoinBinderListing(bool noBlock)
  {
    SCOPEDDEBUG();
    if (noBlock)
      android::ProcessState::self()->startThreadPool();
    return noBlock;
  }

  static int icgetNum()
  {
    SCOPEDDEBUG();
    return Camera::getNumberOfCameras();
  }

  static int icgetInfo(int idx, camera_info_t * info)
  {
    SCOPEDDEBUG();
    CameraInfo cameraInfo;
    status_t rc = Camera::getCameraInfo(idx, &cameraInfo);
    if (rc != NO_ERROR) {
      return -1;
    } else {
      info->facing = cameraInfo.facing;
      info->orientation = cameraInfo.orientation;
    }
    return 0;
  }

    static camera_handle * iccreate(int idx)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = new OMXCamera;
      String16 pkgName = GetPackageName();
      pcamera->mCamera = android::Camera::connect(idx, pkgName,
                                                  android::Camera::USE_CALLING_UID);
      if (pcamera->mCamera == NULL) {
        DEBUG("Connect cameraserver failed");
        delete pcamera;
        return NULL;
      }

      pcamera->mListener = new CameraListenerImpl(pcamera);
      pcamera->mCamera->setListener(pcamera->mListener);
      pcamera->mPreviewFrameCb = NULL;
      #if defined(ANDROID_ON_QCOM)
      pcamera->mRecordListener = new CameraRecordingProxyListenerImpl(pcamera);
      pcamera->mCamera->setRecordingProxyListener(pcamera->mRecordListener);
      pcamera->mRecordingFrameCb = NULL;

      pcamera->mTakePicCb = NULL;
      pcamera->mTakePicCbData = NULL;
      pcamera->mNotifyThread = new CameraNotifyThread();
      pcamera->mNotifyThread->run("KedacomICameraNotifyThread");
      #endif
      icJoinBinderListing(true);
      return (camera_handle *)pcamera;
    }

    static void icdestroy(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      pcamera->mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
      pcamera->mCamera->disconnect();
      delete pcamera;
    }

    static int icsetSurface(camera_handle * hndl, unsigned int w, unsigned int h, \
                            surface_pixelFmt fmt, unsigned int usage)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);

      if(pcamera == NULL)
        return -1;

      sp<IGraphicBufferProducer> gbp  = NULL;
      if ((usage & SURFACE_USAGE_FORMAT_MASK) == SURFACE_USAGE_COMPOSER) {
        if (0 != createOnScreenSurface(pcamera, w, h, fmt, usage)) {
          DEBUG("Create Surface failed!\n");
          return -1;
        }
        gbp = pcamera->mSurface->getIGraphicBufferProducer();
      } else if ((usage & SURFACE_USAGE_FORMAT_MASK) == SURFACE_USAGE_TEXTURE) {
        createOnScreenSurfaceTexture(pcamera, w, h, fmt, usage);
        gbp = pcamera->mProducer;
      }

      if (gbp == NULL || pcamera->mCamera->setPreviewTarget(gbp) != NO_ERROR) {
        DEBUG("Set preview target failed");
        return -1;
      } else {
        DEBUG("Set preview target success");
      }
      // clear flag
      pcamera->mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);

      // set flag
      pcamera->mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_CAMERA);
      //mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_BARCODE_SCANNER);

      return 0;
    }
    static int icsetRTCSurface(camera_handle * hndl, const RTCSurface *sfs)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);

      if(pcamera == NULL || sfs == NULL)
        return -1;

      LocalSurface *surface = (LocalSurface *)sfs;
      sp<IGraphicBufferProducer> gbp = surface->producer;

      if (gbp == NULL || pcamera->mCamera->setPreviewTarget(gbp) != NO_ERROR) {
        DEBUG("Set preview target failed");
        return -1;
      } else {
        DEBUG("Set preview target success");
      }
      // clear flag
      pcamera->mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_NOOP);

      // set flag
      pcamera->mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_CAMERA);
      //mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_BARCODE_SCANNER);

      return 0;
    }
    static int icsetPreviewCallback(camera_handle * hndl, frameCallback cb, void *context)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL)
        return -1;

      pcamera->mPreviewFrameCb = cb;
      pcamera->mPreviewCbData = context;

      return 0;
    }
    static int icsetRecordingCallback(camera_handle * hndl, frameCallback cb, void *context)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL)
        return -1;

      pcamera->mRecordingFrameCb = cb;
      pcamera->mRecordingCbData = context;
      #endif
      return 0;
    }
    static int icstartPreview(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL ||
         pcamera->mCamera->previewEnabled() == true)
        return -1;

      if(pcamera->mCamera->previewEnabled())
        return 0;
      #if 0
      String8 param;
      param = pcamera->mCamera->getParameters();
      DEBUG("Parameter :%s\n", param.string());
      DEBUG("***********************************************\n");
      #endif
      status_t ret= pcamera->mCamera->startPreview();
      if (ret != NO_ERROR) {
        DEBUG("Camera start preview failed, ret: %d\n", ret);
        return -1;
      } else {
        DEBUG("Camera start preview success");
      }
      return 0;
    }
    static int icstopPreview(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      pcamera->mCamera->stopPreview();

      return 0;
    }
    static int icstartRecording(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      if (pcamera->mCamera->startRecording() != NO_ERROR) {
        DEBUG("Camera start recording failed!");
        return -1;
      } else {
        DEBUG("Camera start recording success");
      }
      #endif
      return 0;
    }
    static int icstopRecording(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      pcamera->mCamera->stopRecording();
      #endif
      return 0;
    }
    static int icsetParameters(camera_handle * hndl, const char * strParameter)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      if (strParameter == NULL)
        return -1;

      String8 param(strParameter);
      if (NO_ERROR != pcamera->mCamera->setParameters(param)) {
        DEBUG("setParameter failed\n");
        return -1;
      } else {
        DEBUG("setParameter sucess size: %d\n", param.size());
      }
      return 0;
    }
    static int icgetParameters(camera_handle * hndl, char * buf, unsigned int size)
    {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      if (buf == NULL || size < 0)
        return -1;

      String8 param;
      param = pcamera->mCamera->getParameters();
      //printf("***********************************\n");
      //printf("Prameter: %s\n", param.string());
      DEBUG("actual size: %d\n", param.size());
      strncpy(buf, param.string(), param.size() < size ? param.size() : size - 1);
      return 0;
    }

    static int sAutoFocus(camera_handle * hndl, int32_t time_ms) {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;
      pcamera->mCamera->autoFocus();
      pcamera->mNotifyThread->RegisterCallback(OMXCamera::DummyCamNotity, hndl);
      Mutex::Autolock lock(pcamera->mAutoFocusMutex);
      status_t stat = pcamera->mAutoFocusCondition.waitRelative(pcamera->mAutoFocusMutex, time_ms*1000000);
      pcamera->mNotifyThread->DeRegisterCallback(OMXCamera::DummyCamNotity);
      if (OK != stat)
      pcamera->mCamera->cancelAutoFocus();
      #endif
      return 0;
    }
    static int ictakePicture(camera_handle * hndl, int type)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      int take_picture_type = 0x0;
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;
      if (type & TAKE_PICTRUE_USAGE_AUTO_FOCUS) {
        sAutoFocus(hndl, 5000);
      }
      if (type & TAKE_PICTRUE_USAGE_YUV || type & TAKE_PICTRUE_USAGE_JPEG) {
        take_picture_type |= 0x100;
      } else if (type & TAKE_PICTRUE_USAGE_RAW) {
        take_picture_type |= 0x080;
      }

      if (type & TAKE_PICTRUE_USAGE_ENABLE_SHUTTER)
      take_picture_type |= 0x002;

      status_t stat = pcamera->mCamera->takePicture(take_picture_type);
      if (stat != NO_ERROR) {
        DEBUG("Camera takePicture failed[%d]!--->type[%d]", stat, type);
        return -1;
      } else {
        DEBUG("Camera takePicture success!--->type[%d]", type);
      }
      #endif
      return 0;
    }
    static int icsetPictureCallback(camera_handle * hndl, frameCallback cb, void *context)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;

      pcamera->mTakePicCb = cb;
      pcamera->mTakePicCbData = context;
      #endif
      return 0;
    }
    static int icAutoFocus(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;
      status_t stat = pcamera->mCamera->autoFocus();
      if (stat != NO_ERROR) {
        DEBUG("Camera autoFocus failed[%d]!", stat);
        return -1;
      } else {
        DEBUG("Camera autoFocus success!");
      }
      #endif
      return 0;
    }
    static int icCancelAutoFocus(camera_handle * hndl)
    {
      SCOPEDDEBUG();
      #if defined(ANDROID_ON_QCOM)
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;
      status_t stat = pcamera->mCamera->cancelAutoFocus();
      if (stat != NO_ERROR) {
        DEBUG("Camera cancelAutoFocus failed[%d]!", stat);
        return -1;
      } else {
        DEBUG("Camera cancleAutoFocus success!");
      }
      #endif
      return 0;
    }

    static int icSetNotifyCallback(RTCCameraHandle* hndl, RTCCamNotify cb, void *context) {
      SCOPEDDEBUG();
      OMXCamera *pcamera = static_cast<OMXCamera*>(hndl);
      if(pcamera == NULL || pcamera->mCamera == NULL)
        return -1;
      pcamera->mNotifyThread->RegisterCallback(cb, context);
      return 0;
    }
}
#endif //ANDROID_SDK_VERSION

extern "C" {
  int PREFIX(Android_CreateCamera)(RTCCamera_t *p)
  {
    if (p){
      memset(p, 0, sizeof(*p));
      #if defined(ANDROID_ON_QCOM) && (ANDROID_SDK_VERSION >= 21)
      p->getNum = icgetNum;
      p->getInfo = icgetInfo;
      p->create = iccreate;
      p->destroy = icdestroy;
      p->setSurface = icsetSurface;
      p->setRTCSurface = icsetRTCSurface;
      p->setPreviewCallback = icsetPreviewCallback;
      p->startPreview = icstartPreview;
      p->stopPreview = icstopPreview;
      #if defined(ANDROID_ON_QCOM)
      p->setRecordingCallback = icsetRecordingCallback;
      p->startRecording = icstartRecording;
      p->stopRecording = icstopRecording;
      p->setPictureCallback = icsetPictureCallback;
      p->takePicture = ictakePicture;
      p->autoFocus = icAutoFocus;
      p->cancelAutoFocus = icCancelAutoFocus;
      p->setNotifyCallback = icSetNotifyCallback;
      #endif
      p->setParameters = icsetParameters;
      p->getParameters = icgetParameters;
      #endif
      return 0;
    }
    return -1;
  }
} // end of the extern "C"
