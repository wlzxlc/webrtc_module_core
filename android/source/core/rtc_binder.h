#ifndef RTC_BINDER_H
#define RTC_BINDER_H
#include <utils/String16.h>
#include <binder/IInterface.h>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>


namespace android {
class IWebRTCIPC: public IInterface
{
 android::String16 str;
public:
enum {
  kDefaultCallID = 1
}
    DECLARE_META_INTERFACE(WebRTCIPC)
    virtual void rtc_transport(void *p, int32_t size) = 0;
    static void StartServer();
};
}
#endif // RTC_BINDER_H

