#include "IRTCBinder.h"
#include <stdio.h>
#include <string>
#include <assert.h>
#ifndef CHECK
#define CHECK assert
#endif
using namespace android;

class TestNodeTransportOnServer: public IRTCNodeTransport {
    private:
        std::string _name;
        IRTCNodeTransport *_cb;
    public:
        TestNodeTransportOnServer(const char *name):_name(name),_cb(NULL){}

        ~TestNodeTransportOnServer() 
        {
            printf("[%s]~Dtor TestNodeTransportOnServer\n", _name.c_str());
        } 

        virtual void BinderDied () 
        {
            printf("Client died this[%p]\n", this);
            delete this;
        }
        // Client's node to Service's node invok.
        virtual int Transport(const void *p, int size)
        {
            printf("Node:[%s,tid:%d]\n receved data %d bytes[%s]\n", _name.c_str(), (int)getpid(), size, (const char *)p);
            if (_cb) {
                // Ack 
                _name = _name + " 1 ";
                _cb->Transport(_name.c_str(), strlen(_name.c_str()) + 1);
            }
            return 0;
        }
        // Service's node to Client's node invok.
        // Service's node implement.
        virtual int SetCallback( IRTCNodeTransport * cb) 
        {
            _cb = cb;
            return 0;
        }

};

class TestNodeTransportLoopOnClientCallback : public BnRTCNode{
    private:
        std::string _name;
        struct timeval _starttime;
    public:
        TestNodeTransportLoopOnClientCallback(const std::string &str):_name(str) {}

        void StartTimer() 
        {
            gettimeofday(&_starttime,NULL);
        }

        virtual int Transport(const void *p, int size)
        {
            int64_t us = 0;
            struct timeval currtime;
            gettimeofday(&currtime, NULL);
            us = 1000000*(currtime.tv_sec - _starttime.tv_sec) + 
                (currtime.tv_usec - _starttime.tv_usec);

            printf("%s[callback from service, cost Time[%0.2f(ms)]] receved data size %d bytes[%s]\n",
                    _name.c_str(),us / 1000.0, size, (const char *)p);
            return kNodeIDDefaultCallBackTest;
        }

        virtual int RegisterObserver(IRTCNodeObserver)
        {
            return 0;
        }
};

class TestNodeTransportOnClient {
    private:
        class TestNodeTransportOnClientCallback : public BnRTCNode{
            private:
                std::string _name;
            public:
                TestNodeTransportOnClientCallback(const std::string &str):_name(str) {}
                virtual int Transport(const void *p, int size)
                {
                    printf("%s[callback from service] receved data size %d bytes[%s]\n",
                            _name.c_str(), size, (const char *)p);
                    return 0;
                }

                virtual int RegisterObserver(IRTCNodeObserver)
                {
                    return 0;
                }
        };

    private:
        sp<IRTCNode> _node;
        std::string _name;
        sp<IRTCNode> _cb;

    public:
        TestNodeTransportOnClient(const char *str):_name(str) {}
        void SetNode(sp<IRTCNode> node)
        {
            _node = node;
        }

        int SetCallBack()
        {
            _cb = new TestNodeTransportOnClientCallback(_name);
            return _node->RegisterObserver(_cb);
        }

        int Call(const void *p ,int size)
        {
            return _node->Transport(p, size);
        }

        ~TestNodeTransportOnClient()
        {
        }
};

class TestBinderServerObserver: public IRTCBinderServerObserver{
    private:
        std::string _name;
    public:

        enum {
            kNodeType_1 = 1,
            kNodeType_2 = 2,
            kNodeType_3 = 3,
            kNodeType_4 = 4,
        };

        TestBinderServerObserver(const char *name):_name(name){}

        virtual int Transport(const void *p, int size) 
        {
            printf("Service:[%s,tid:%d]\n receved data %d bytes [%s] \n", 
                    _name.c_str(), (int)getpid(), size, (const char *)p);
            return 0;
        }

        virtual IRTCNodeTransport *Create(int type)
        {
            IRTCNodeTransport *node = NULL;
            switch(type) {
                case kNodeType_1:
                    node = new TestNodeTransportOnServer("NodeType1");
                    break;
                case kNodeType_2:
                    node = new TestNodeTransportOnServer("NodeType2");
                    break;
                case kNodeType_3:
                    node = new TestNodeTransportOnServer("NodeType3");
                    break;
                case kNodeType_4:
                    node = new TestNodeTransportOnServer("NodeType4");
                    break;
                default:
                    break;
            }
            return node;
        }
};


int main(int c, char **s)
{
    const char *server_name = "MyIPCBinder";
#ifdef SERVICE
    printf("Start Server...\n");
    IRTCBinder::StartService(server_name,new TestBinderServerObserver(server_name),true);
#else
    sp<IRTCBinder> service;
    service = IRTCBinder::GetService(server_name);
    CHECK(service.get());

    // Test service API
    const char *test_service_string = "Hello, My name is main thread.";
    printf("Sending '%s' to service\n", test_service_string);
    CHECK(service->Transport(test_service_string, strlen(test_service_string) + 1) == 0);

    // Test default binder loop.
    sp<IRTCNode> node_loop = service->NewNode(kNodeIDDefaultCallBackTest);
    CHECK(node_loop.get());
    sp<TestNodeTransportLoopOnClientCallback> loop_cb= new TestNodeTransportLoopOnClientCallback("BinderLoopTest");
    CHECK(node_loop->RegisterObserver(loop_cb) == 0 );
    const char *node_loop_str = "TestBinderLoop";
    loop_cb->StartTimer();
    CHECK(node_loop->Transport(node_loop_str, strlen(node_loop_str) + 1) == kNodeIDDefaultCallBackTest);

    sp<IRTCNode> node1 = service->NewNode(TestBinderServerObserver::kNodeType_1);
    CHECK(node1.get());
    TestNodeTransportOnClient client1("client1");
    client1.SetNode(node1);

    sp<IRTCNode> node2 = service->NewNode(TestBinderServerObserver::kNodeType_2);
    CHECK(node2.get());
    TestNodeTransportOnClient client2("client2");
    client2.SetNode(node2);

    sp<IRTCNode> node3 = service->NewNode(TestBinderServerObserver::kNodeType_3);
    CHECK(node3.get());
    TestNodeTransportOnClient client3("client3");
    client3.SetNode(node3);

    sp<IRTCNode> node4 = service->NewNode(TestBinderServerObserver::kNodeType_4);
    CHECK(node4.get());
    TestNodeTransportOnClient client4("client4");
    client4.SetNode(node4);

    CHECK(client1.SetCallBack() == 0 );
    CHECK(client2.SetCallBack() == 0 );
    CHECK(client3.SetCallBack() == 0 );
    CHECK(client4.SetCallBack() == 0 );


    std::string string1("Hello, This is Client 1");
    std::string string2("Hello, This is Client 2");
    std::string string3("Hello, This is Client 3");
    std::string string4("Hello, This is Client 4");

    CHECK(client1.Call(string1.c_str(), strlen(string1.c_str()) + 1) == 0);
    CHECK(client2.Call(string2.c_str(), strlen(string2.c_str()) + 1) == 0);
    CHECK(client3.Call(string3.c_str(), strlen(string3.c_str()) + 1) == 0);
    CHECK(client4.Call(string4.c_str(), strlen(string4.c_str()) + 1) == 0);
#endif
    return 0;
}
