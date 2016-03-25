#include <binder/MemoryDealer.h>
#include <OMX_Component.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include "rtc_binder.h"

using namespace android;
class BpWebRTCIPC: public BpInterface<IWebRTCIPC>
{
public:
    BpWebRTCIPC(const sp<IBinder>& impl)
        : BpInterface<IWebRTCIPC>(impl)
    {
    }
    virtual void rtc_transport(void *p, int32_t size)
    {
      Parcel data, reply;
      data.writeInterfaceToken(ITestService::getInterfaceDescriptor());
      MemoryDealer dear(size);
      sp<IMemory> mem = dear.allocate(size);
      memcpy(mem->pointer(),p, size);
      data.writeStrongBinder(mem->asBinder());
      remote()->transact(kDefaultCallID, data, &reply);
    }
};

//��ӦITestService�еĺ�����������������������õ���Bpxxx�࣬���Զ�����Bpxx������档
IMPLEMENT_META_INTERFACE(WebRTCIPC, "WebRTC.android.IPC")

class BnWebRTCIPC: public BnInterface<IWebRTCIPC>
{
public:
  int bn;
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0)
  {
    switch(code) {
        case kDefaultCallID:{
        CHECK_INTERFACE(IWebRTCIPC, data, reply);
          sp<IMemory> mem = interface_cast<IMemory>(data.readStrongBinder());
         rtc_transport(mem->pointer(), mem->size());
         return NO_ERROR;
          default:
           break;
        }
//������ǲ��ܽ���������ɸ��ദ��
         return BBinder::onTransact(code,data,reply,flags);
  }
}
};

class WebRTCIPC: public BnWebRTCIPC
{//ʵ�������Ľӿ�
public:
    static  void  instantiate()
    {
      sp<WebRTCIPC> *temp = &(new WebRTCIPC);

      defaultServiceManager()->addService(
      String16("WebRTC.android.IPC"), temp);
    }
    WebRTCIPC(){}
    virtual   ~WebRTCIPC(){}
    virtual void rtc_transport(void *p, int32_t size){
      printf("Server recv mem, size %d data:%#x \n", size,
             (int *)p);
    }
};

IWebRTCIPC::StartServer()
{
  sp<ProcessState> proc(ProcessState::self());
  sp<IServiceManager> sm = defaultServiceManager();
  LOGI("WebRTCIPC: %p", sm.get());
  WebRTCIPC::instantiate();
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}
