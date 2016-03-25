/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RTC_IRTCBINDER_H
#define RTC_IRTCBINDER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rtc_binder_data {
    const android::Parcel *data;
    android::Parcel *reply;
}rtc_binder_data_t;
#ifdef __cplusplus
}
#endif
namespace android {


enum {
 kNodeIDDefaultCallBackTest = -20151103,
};

class IRTCNodeTransport {
 public:
     virtual int Transport(const rtc_binder_data_t *) = 0;
     // Died notify.
     virtual void BinderDied() {}
     // Service's node implement.
     // Ingore by client.
     virtual int SetCallback( IRTCNodeTransport *) {return -1;}
     virtual ~IRTCNodeTransport() {}
};

// Service implmemt
class IRTCBinderServerObserver {
 public:
     // Process message of the service.
     virtual int Transport(const rtc_binder_data_t *) = 0;
     // By service invok when create a new node.
     virtual IRTCNodeTransport *Create(int type) = 0;
     virtual ~IRTCBinderServerObserver() {}
};

class IRTCNode : public IInterface
{
public:
    DECLARE_META_INTERFACE(RTCNode);

    enum { // (keep in sync with Surface.java)
        kIDTransport = 1,
        kIDResigerObs
    };

    typedef sp<IRTCNode> IRTCNodeObserver;
    virtual int Transport(const rtc_binder_data_t *) = 0;
    virtual void BinderDied() {}
    virtual int RegisterObserver(IRTCNodeObserver node) = 0;
};

class IRTCBinder : public IInterface
{
public:
    DECLARE_META_INTERFACE(RTCBinder);

    enum { // (keep in sync with Surface.java)
        kDefault_CALL_ID = 1,
        kDefault_CreateNodeID
    };


    virtual int Transport(const rtc_binder_data_t *) = 0;
    virtual sp<IRTCNode> NewNode(int type) = 0;
    static int StartService(const char *name, IRTCBinderServerObserver *obs ,bool block = true);
    static int StopService(const char *name);
    static sp<IRTCBinder> GetService(const char *name);
};

// ----------------------------------------------------------------------------

class BnRTCNode: public BnInterface<IRTCNode> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

class BnRTCBinder: public BnInterface<IRTCBinder> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_GUI_ISURFACE_COMPOSER_CLIENT_H
