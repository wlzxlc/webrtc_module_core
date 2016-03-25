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

// tag as surfaceflinger
#define LOG_TAG "RTCBinder"

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <ui/Point.h>
#include <ui/Rect.h>

#include <binder/MemoryDealer.h>

#include "IRTCBinder.h"
#include "bug_helper.h"
// ---------------------------------------------------------------------------
namespace android {

class BpRTCBinder : public BpInterface<IRTCBinder>
{
public:
    BpRTCBinder(const sp<IBinder>& impl)
        : BpInterface<IRTCBinder>(impl) {
    }

    virtual int Transport(const rtc_binder_data_t *data) {
        const Parcel &parcel = *data->data;
        Parcel &reply = *data->reply;
        remote()->transact(kDefault_CALL_ID, parcel, &reply);
        int ret = reply.readInt32();
        return ret;
    }

    virtual sp<IRTCNode> NewNode(int type) {
        Parcel data, reply;
        data.writeInterfaceToken(IRTCBinder::getInterfaceDescriptor());
        data.writeInt32(type);
        remote()->transact(kDefault_CreateNodeID, data, &reply);
        sp<IBinder> handle = reply.readStrongBinder();
        sp<IRTCNode> node = interface_cast<IRTCNode>(handle);
        return node;
    }
};

IMPLEMENT_META_INTERFACE(RTCBinder, "WebRTC.Binder");

class BpRTCNode : public BpInterface<IRTCNode>
{
public:
    BpRTCNode(const sp<IBinder>& impl)
        : BpInterface<IRTCNode>(impl) {
    }

    virtual int Transport(const rtc_binder_data_t *data) {
        const Parcel &parcel = *data->data;
        Parcel &reply = *data->reply;
        remote()->transact(kIDTransport, parcel, &reply);
        int ret = reply.readInt32();
        return ret;
    }

    virtual int RegisterObserver(sp<IRTCNode> node) {
        Parcel data, reply;
        data.writeInterfaceToken(IRTCNode::getInterfaceDescriptor());
#if ANDROID_SDK_VERSION >=23
        data.writeStrongBinder(node->asBinder(node.get()));
#else
        data.writeStrongBinder(node->asBinder());
#endif
        remote()->transact(kIDResigerObs, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(RTCNode, "WebRTC.Binder.node");

// ----------------------------------------------------------------------

status_t BnRTCNode::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
     switch(code) {
        case kIDTransport: {
            rtc_binder_data_t out;
            out.data = &data;
            out.reply = reply;
            // Reserved 4 bytes;
            reply->writeInt32(0);
            int ret = Transport(&out);
            int pos = reply->dataPosition();
            reply->setDataPosition(0);
            reply->writeInt32(ret);
            reply->setDataPosition(pos);
            return NO_ERROR;
        } break;
        case kIDResigerObs: {
            CHECK_INTERFACE(IRTCNode, data, reply);
            sp<IBinder> handle = data.readStrongBinder();
            sp<IRTCNode> node = interface_cast<IRTCNode>(handle);
            int ret = RegisterObserver(node);
            reply->writeInt32(ret);
            return NO_ERROR;
        }break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

status_t BnRTCBinder::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
     switch(code) {
        case kDefault_CALL_ID: {
            rtc_binder_data_t out;
            out.data = &data;
            out.reply = reply;
            // Reserved 4 bytes
            reply->writeInt32(0);
            int ret = Transport(&out);
            int pos = reply->dataPosition();
            reply->setDataPosition(0);
            // Rewrite 4 bytes
            reply->writeInt32(ret);
            reply->setDataPosition(pos);
            return NO_ERROR;
        } break;
       case kDefault_CreateNodeID: {
            CHECK_INTERFACE(IRTCBinder, data, reply);
            int type = 0;
            type = data.readInt32();
            sp<IRTCNode> node = NewNode(type);
#if ANDROID_SDK_VERSION >=23
            reply->writeStrongBinder(node->asBinder(node.get()));
#else
            reply->writeStrongBinder(node->asBinder());
#endif
            return NO_ERROR;
       } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android
