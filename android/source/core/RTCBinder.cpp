#include "RTCBinder.h"
#include "ipc.h"
#include "core.cc"
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>

#include "graphics_type.h"
#include "bug_helper.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define rtc_binder_data_2_RTCIPCData_t(rtc_data, ipc_data_name) \
     RTCIPCData_t ipc_data_name; \
     LocalParcel __parcel_data(rtc_data->data); \
     LocalParcel __parcel_reply(rtc_data->reply); \
     ipc_data_name.data = &__parcel_data; \
     ipc_data_name.reply = &__parcel_reply;

#define RTCIPCData_t_rtc_binder_data(ipc_data, rtc_bind_data_name) \
     rtc_binder_data_t rtc_bind_data_name; \
     rtc_bind_data_name.data = ((LocalParcel *)(ipc_data->data))->_parcel; \
     rtc_bind_data_name.reply = ((LocalParcel *)(ipc_data->reply))->_parcel; \
     rtc_bind_data_name.data->setDataPosition(0); \
     rtc_bind_data_name.reply->setDataPosition(0);

class VideoBufferImplIPC: public VideoBuffer {
    public:
        virtual void release() {
            const native_handle *hwd = graphicBuffer->handle;
            graphicBuffer = NULL;
            native_handle_close(hwd);
            native_handle_delete(const_cast<native_handle *>(hwd));
            DEBUG("~Dtor RTCIPC GB");delete this;
        }
        ~VideoBufferImplIPC() {DEBUG("");}
};

class LocalParcel {
    public:
        android::Parcel _parcel_inst;
        android::Parcel *_parcel;
        LocalParcel(const android::Parcel *p = NULL) {
         _parcel = p ? const_cast<android::Parcel *>(p) : &_parcel_inst;
        }

    static void sRelease(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        delete self;
    }

    static void *sCreate() {
        return new LocalParcel;
    }

    static int sWriteFd(void *me, int fd) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->writeFileDescriptor(fd);
    }

    static int sDataAvailSize(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->dataAvail();
    }

    static int sSetDataPosition(void *me, int pos) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        self->_parcel->setDataPosition(pos);
        return 0;
    }

    static int sDataPosition(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->dataPosition();
    }

    static int sReadFd(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->readFileDescriptor();
    }

    static int sWriteGraphicsHandle(void *me, graphics_handle *handle) {
        VideoBuffer *vb = static_cast<VideoBuffer *>(handle);
        android::sp<android::GraphicBuffer> gb = vb->graphicBuffer;
        LocalParcel *self = static_cast<LocalParcel *>(me);
        int ret = (int)self->_parcel->writeNativeHandle(gb->handle);
        if (ret == 0) {
         self->_parcel->writeInt32(gb->getWidth());
         self->_parcel->writeInt32(gb->getHeight());
         self->_parcel->writeInt32(gb->getStride());
         self->_parcel->writeInt32(gb->getUsage());
         self->_parcel->writeInt32(gb->getPixelFormat());
        }
        return ret;
    }

    static graphics_handle *sReadGraphicsHandle(void *me) {
       LocalParcel *self = static_cast<LocalParcel *>(me);
       native_handle* hwd = self->_parcel->readNativeHandle();

       VideoBufferImplIPC *buffer = NULL;
       if (hwd) {
         buffer = new VideoBufferImplIPC;
         unsigned int w = self->_parcel->readInt32();
         unsigned int h = self->_parcel->readInt32();
         unsigned int s = self->_parcel->readInt32();
         unsigned int u = self->_parcel->readInt32();
         unsigned int p = self->_parcel->readInt32();
         DEBUG("w %d h %d s %d u %d p %d hwd %p", w, h, s, u, p, hwd);
         buffer->graphicBuffer = new GraphicBuffer(w, h, (PixelFormat)p,u,s,hwd,false);
       }
       return buffer;
    }

    static int sWriteInt32(void *me, int v) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->writeInt32(v);
    }

    static int sWriteInt64(void *me, long long v) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->writeInt64(v);
    }

    static int sWriteFloat(void *me, float v) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->writeFloat(v);
    }

    static int sWriteDouble(void *me, double v) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->writeDouble(v);
    }

    static int sReadInt32(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->readInt32();
    }

    static long long sReadInt64(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->readInt64();
    }

    static float sReadFloat(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->readFloat();
    }

    static double sReadDouble(void *me) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->readDouble();
    }

    static int sWriteData(void *me,const void *data , unsigned int size) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
#if ANDROID_SDK_VERSION >= 21
        return (int)self->_parcel->writeByteArray(size, (const unsigned char *)data);
#else
        int ret = 0;
        if (size) {
            ret = (int)self->_parcel->writeInt32(size);
            if (ret == 0 ){
                void *rp = self->_parcel->writeInplace(size);
                memcpy(rp, data, size);
            }
        }
        return ret;
#endif
    }

    static int sReadData(void *me, unsigned int size, void *p) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return (int)self->_parcel->read(p, size);
    }

    static const void * sReadDataP(void *me, unsigned int size) {
        LocalParcel *self = static_cast<LocalParcel *>(me);
        return self->_parcel->readInplace(size);
    }
};

namespace android {
    int RTCBinder::Transport(const rtc_binder_data_t *p) {
        int ret = 0;
        if (_obs) {
         ret = _obs->Transport( p );
        }
        return ret;
    }

    int IRTCBinder::StartService(const char *name,IRTCBinderServerObserver *obs ,bool block)
    {
        sp<ProcessState> proc(ProcessState::self());
        sp<IServiceManager> sm = defaultServiceManager();
        sp<RTCBinder> temp = new RTCBinder(obs);
        int ret = (int) sm->addService( String16(name), temp);
        if (ret == 0){
            if (block){
                IPCThreadState::self()->joinThreadPool();
            }else {
                proc->startThreadPool();
            }
        }
        return ret;
    }

    sp<IRTCNode> RTCBinder::NewNode(int type)
    {
        IRTCNodeTransport *transport = NULL;

        if (type != kNodeIDDefaultCallBackTest) {
            transport = _obs->Create(type);
        }
        sp<RTCNode> node;

        if (type == kNodeIDDefaultCallBackTest || transport){
            node = new RTCNode(this, transport, type);
        }
        return node;
    }

    RTCBinder::~RTCBinder()
    {
        DEBUG("~Dtor RTCBinder");
    }

    int RTCNode::Transport(const rtc_binder_data_t *p)
    {
        if (_type == kNodeIDDefaultCallBackTest && _obs.get()) {
         return _obs->Transport(p);
        }
      return _transport->Transport(p);
    }

    RTCNode::RTCNode(RTCBinder *service, IRTCNodeTransport *transport, int type)
    {
      _server = service;
      _transport = transport;
      _internal_transport = NULL;
      _type = type;
    }

    RTCNode::~RTCNode()
    {
        if (_internal_transport) {
         delete _internal_transport;
        }

        if (_type == kNodeIDDefaultCallBackTest) {
            delete _transport;
        }else {
            if (_transport) {
                _transport->BinderDied();
                _transport = NULL;
            }
        }
        DEBUG("~Dtor RTCNode %p",this);
    }

    RTCNode::RTCNodeInternalTransport::RTCNodeInternalTransport(RTCNode *node):_node(node)
    {
    }

    RTCNode::RTCNodeInternalTransport::~RTCNodeInternalTransport()
    {
        DEBUG("~Dtor RTCNodeInternalTransport");
    }

    int RTCNode::RTCNodeInternalTransport::Transport(const rtc_binder_data_t *p)
    {
        if (_node && _node->_obs.get()) {
         return _node->_obs->Transport(p);
        }
        return -1;
    }

    void RTCNode::binderDied(const wp<IBinder> &who)
    {
       DEBUG("RTCNode dinderDied...");
      if (_transport) {
        _transport->SetCallback(NULL);
        _transport->BinderDied();
        _transport = NULL;
      }
    }

    int RTCNode::RegisterObserver(IRTCNodeObserver node)
    {
        _obs = node;

        if (_transport == NULL ){
            return 0;
        }

        _transport->SetCallback(NULL);

        if (_internal_transport) {
            delete _internal_transport;
            _internal_transport = NULL;
        }

        if (_obs.get()) {
            _internal_transport = new RTCNodeInternalTransport(this);
        }

        _transport->SetCallback(_internal_transport);
#if ANDROID_SDK_VERSION >= 23
        if (_obs.get())
            node->asBinder(node.get())->linkToDeath(this);
#else
        if (_obs.get())
            node->asBinder()->linkToDeath(this);
#endif
        return 0;
    }

    int IRTCBinder::StopService(const char * name)
    {
        return ERRNUM("Unsupported");;
    }

    sp<IRTCBinder> IRTCBinder::GetService(const char *name)
    {
        sp<IRTCBinder> pp;
        getService(String16(name),&pp);
        return pp;
    }
};

using namespace android;

class RTCBinderServerObserverImpl: public IRTCBinderServerObserver {
    private:
        class RTCBinderServerNodeTransport: public IRTCNodeTransport, public RTCIPCNode {
            private:
                RTCIPCNode_t *_api;
                IRTCNodeTransport *_cb;
            public:
                RTCBinderServerNodeTransport(RTCIPCNode_t *api):
                    _api(api),
                    _cb(NULL)
            {
                    RTCIPCNode::transport= RTCBinderServerNodeTransport::send;
                    RTCIPCNode::died = RTCBinderServerNodeTransport::died;
                    RTCIPCNode::set_callback = RTCBinderServerNodeTransport::set_callback;
                DEBUG("Ctor %s, %p", __FUNCTION__, this);
            }

                virtual int Transport(const rtc_binder_data_t *p)
                {
                    int ret = -1;
                        DEBUG("");
                    if (_api && _api->transport) {
                        DEBUG("");
                        rtc_binder_data_2_RTCIPCData_t(p, ipc_p);
                        DEBUG("api %p transport %p ipc_p %p", _api, _api->transport, &ipc_p);
                        ret = _api->transport(_api, &ipc_p);
                        DEBUG("");
                    }
                        DEBUG("");
                    return ret;
                }
                // Died notify.
                virtual void BinderDied()
                {
                    SetCallback(NULL);
                    if (_api && _api->died) {
                        _api->died(_api);
                    }
                    delete this;
                }
                // Service's node implement.
                // Ingore by client.
                virtual int SetCallback( IRTCNodeTransport *cb)
                {
                    _cb = cb;
                    if (_api && _api->set_callback) {
                     _api->set_callback(_api, _cb ? this : NULL);
                    }
                     return 0;
                }

                static int send(RTCIPCNode_t *me, const RTCIPCData_t *p)
                {
                    // callback to client
                    int ret = ERRNUM("Not found callback.");
                     RTCBinderServerNodeTransport *self = static_cast<RTCBinderServerNodeTransport *>(me);
                     if (self->_cb) {
                      RTCIPCData_t_rtc_binder_data(p, rtc_p);
                      ret = self->_cb->Transport(&rtc_p);
                     }
                     return ret;
                }

                static void died(RTCIPCNode_t *me)
                {
                    return;
                }

                static int set_callback(RTCIPCNode_t *me, RTCIPCNode_t *cb)
                {
                   return ERRNUM("Invalid operation.");
                }

                virtual ~RTCBinderServerNodeTransport() {DEBUG("~Dtor %s[%p]", __FUNCTION__, this);}
        };
    private:
        RTCIPCHandleInterface_t *_api;
 public:
     RTCBinderServerObserverImpl(RTCIPCHandleInterface_t *api):_api(api)
    {
        DEBUG("Ctor %s", __FUNCTION__);
    }

     // Process message of the service.
     virtual int Transport(const rtc_binder_data_t *p)
     {
         int ret = -1;
         if (_api && _api->transport) {
             rtc_binder_data_2_RTCIPCData_t(p, ipc_p);
          ret = _api->transport(_api, &ipc_p);
         }
         return ret;
     }
     // By service invok when create a new node.
     virtual IRTCNodeTransport *Create(int type)
     {
          RTCIPCNode_t* node = NULL;
         if (_api && _api->create) {
          node = _api->create(_api, type);
         }
         IRTCNodeTransport * rtcnode = NULL;
         if (node) {
           rtcnode = new RTCBinderServerNodeTransport(node);
         }
         return rtcnode;
     }

     virtual ~RTCBinderServerObserverImpl() {DEBUG("~Dtor %s", __FUNCTION__);}
};

struct LocalRTCIPCNode : public RTCIPCNode{
    private:
        class LocalBnRTCNodeObserver: public BnRTCNode, public IBinder::DeathRecipient{
            private:
                RTCIPCNode_t *_cb;
            public:
                LocalBnRTCNodeObserver():_cb(NULL)
            {
                DEBUG("Ctor %s", __FUNCTION__);
            }
                ~LocalBnRTCNodeObserver()
                {
                    DEBUG("~Dtor %s",__FUNCTION__);
                }

                virtual void binderDied(const wp<IBinder> &who)
                {
                    if (_cb && _cb->died) {
                        _cb->died(_cb);
                    }
                }

                virtual int Transport(const rtc_binder_data_t *p)
                {
                    int ret = 0;
                    if (_cb && _cb->transport){
                        rtc_binder_data_2_RTCIPCData_t(p, ipc_p);
                        ret = _cb->transport(_cb, &ipc_p);
                    }
                    return ret;
                }

                void SetCallback(RTCIPCNode_t *cb)
                {
                    _cb = cb;
                }

                virtual int RegisterObserver(IRTCNodeObserver node)
                {
                    return ERRNUM("Invalid operation.");
                }
        };
    private:
        sp<IRTCNode> _node;
        sp<LocalBnRTCNodeObserver> _observer;
    public:
       LocalRTCIPCNode(sp<IRTCNode> node):
           _node(node),
           _observer(new LocalBnRTCNodeObserver)
    {
        RTCIPCNode::transport= LocalRTCIPCNode::send;
        RTCIPCNode::died = LocalRTCIPCNode::died;
        RTCIPCNode::set_callback = LocalRTCIPCNode::set_callback;
        _node->RegisterObserver(_observer);
        DEBUG("Ctor %s", __FUNCTION__);
    }
      ~LocalRTCIPCNode()
      {
          DEBUG("~Dtor %s",__FUNCTION__);
      }

       void LinkToDeath(sp<IRTCBinder> service)
       {
#if ANDROID_SDK_VERSION >= 23
           service->asBinder(service.get())->linkToDeath(_observer);
#else
           service->asBinder()->linkToDeath(_observer);
#endif
       }

       static int send(RTCIPCNode_t *me, const RTCIPCData_t *p)
       {
         LocalRTCIPCNode * self = static_cast<LocalRTCIPCNode *>(me);
         RTCIPCData_t_rtc_binder_data(p, rtc_p);
         return self->_node->Transport(&rtc_p);
       }

       static void died(RTCIPCNode_t *me)
       {

       }

       static int set_callback(RTCIPCNode_t *me, RTCIPCNode_t *cb)
       {
         LocalRTCIPCNode * self = static_cast<LocalRTCIPCNode *>(me);
         self->_observer->SetCallback(cb);
         return 0;
       }
};


static int ibind_start(const char *server_name, RTCIPCHandleInterface_t *cb , bool block)
{
  if (cb == NULL || cb->create == NULL) {
   return ERRNUM();
  }

  RTCBinderServerObserverImpl *lcb = new RTCBinderServerObserverImpl(cb);
  if (android::IRTCBinder::StartService(server_name, lcb, block) != 0 ){
      delete lcb;
      return ERRNUM();
  }
  return 0;
}

static int ibind_stop(const char *name)
{
    return android::IRTCBinder::StopService(name);
}

static RTCIPCNode_t * ibind_create(const char *name, int type)
{
    sp<IRTCBinder> service;
    RTCIPCNode_t *tnode = NULL;
    service = IRTCBinder::GetService(name);
    if (service.get()) {
        sp<IRTCNode> node = service->NewNode(type);
        if (node.get()) {
         LocalRTCIPCNode * lnode = new LocalRTCIPCNode(node);
         lnode->LinkToDeath(service);
         tnode = lnode;
        }
    }

   return tnode;
}

static int ibind_release(RTCIPCNode_t *node)
{
  LocalRTCIPCNode *impl = static_cast<LocalRTCIPCNode *>(node);
  delete impl;
  return 0;
}

extern "C" {
int PREFIX(Android_CreateIPC)(RTCIPCHandle_t *ipc)
{
  ipc->start = ibind_start;
  ipc->stop = ibind_stop;
  ipc->create = ibind_create;
  ipc->release = ibind_release;
  ipc->parcel.release = LocalParcel::sRelease;
  ipc->parcel.create = LocalParcel::sCreate;
  ipc->parcel.write_fd = LocalParcel::sWriteFd;
  ipc->parcel.read_fd = LocalParcel::sReadFd;
  ipc->parcel.write_graphics_handle = LocalParcel::sWriteGraphicsHandle;
  ipc->parcel.read_graphics_handle = LocalParcel::sReadGraphicsHandle;

  ipc->parcel.write32 = LocalParcel::sWriteInt32;
  ipc->parcel.write64 = LocalParcel::sWriteInt64;
  ipc->parcel.writef = LocalParcel::sWriteFloat;
  ipc->parcel.writed = LocalParcel::sWriteDouble;

  ipc->parcel.read32 = LocalParcel::sReadInt32;
  ipc->parcel.read64 = LocalParcel::sReadInt64;
  ipc->parcel.readf = LocalParcel::sReadFloat;
  ipc->parcel.readd = LocalParcel::sReadDouble;

  ipc->parcel.write_array = LocalParcel::sWriteData;
  ipc->parcel.read_array = LocalParcel::sReadData;
  ipc->parcel.read_array_pointer = LocalParcel::sReadDataP;
  ipc->parcel.data_avail_size = LocalParcel::sDataAvailSize;
  ipc->parcel.set_position = LocalParcel::sSetDataPosition;
  ipc->parcel.position = LocalParcel::sDataPosition;
  return 0;
}
}
