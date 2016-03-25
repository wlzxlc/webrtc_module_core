#ifndef RTC_RTCBINDER_H
#define RTC_RTCBINDER_H

#include "IRTCBinder.h"

namespace android {
    class RTCBinder: public BnRTCBinder{
        private:
            IRTCBinderServerObserver *_obs;
        public:
            RTCBinder( IRTCBinderServerObserver *obs):_obs(obs){}
            virtual int Transport(const rtc_binder_data_t *);
            virtual sp<IRTCNode> NewNode(int type);
            IRTCBinderServerObserver *Observer();
            ~RTCBinder();
    };

    class RTCNode: public BnRTCNode, public IBinder::DeathRecipient{
        private:
            class RTCNodeInternalTransport: public IRTCNodeTransport {
                private:
                    RTCNode *_node;
                public:
                    RTCNodeInternalTransport(RTCNode *node);
                    virtual int Transport(const rtc_binder_data_t *);
                    ~RTCNodeInternalTransport();
            };
            friend class RTCNodeInternalTransport;

            // Callback binder client observer
            IRTCNodeObserver _obs;

            // callback application client observer;
            RTCNodeInternalTransport *_internal_transport;
            RTCBinder *_server;
            // Send to applicaton.
            IRTCNodeTransport *_transport;
            int _type;
        public:
            RTCNode(RTCBinder *service, IRTCNodeTransport *transport, int type);
            ~RTCNode();
            virtual int Transport(const rtc_binder_data_t *);
            virtual int RegisterObserver(IRTCNodeObserver node);
            virtual void binderDied(const wp<IBinder> &who);
    };

}; // end of the android
#endif
