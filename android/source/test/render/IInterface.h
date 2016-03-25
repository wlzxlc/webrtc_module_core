#ifndef _IINTERFACE_H_
#define _IINTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
	kNodeTypeSourceCam = 0,
	kNodeTypeSourceFile,
	kNodeTypeStream
};

enum {
    kTestColorI420 = 100,
    kTestColorNV12,
    kTestColorNV21
};

typedef struct TestVideoBuffer {
    int size;
    int w;
    int h;
    int color;
	const void *gb;
}VideoBuffer_t;

enum {
	kCallId_DeliverVideoBuffer = 1001,
	kCallId_SetRender,
	kCallId_ConnectSource,
	kCallId_DisConnectSource
};

#ifdef __cplusplus
}
#endif

class IDeliver {
 public:
     virtual int deliver(TestVideoBuffer *) = 0;
     virtual ~IDeliver(){}
};

// by IRenderClient inherit
class IRender : public IDeliver{
    public:
        virtual int SurfaceCreate(void *rtc_surface) = 0;
		virtual int SurfaceDestory(void *rtc_source) = 0;
		virtual int SurfaceChange(void *rtc_source) = 0;
        virtual int StartRender() = 0;
        virtual int PauseRender() = 0;
        virtual int StopRender() = 0;
		~IRender(){}
};

// by InterfaceStreamClient inherit
class IInterfaceStream: public IDeliver
{
public:
	virtual int SetRender(IRender *) = 0;
	virtual int ConnectSource(int source) = 0;
	virtual int DisConnectSource(int source) = 0;
	virtual ~IInterfaceStream(){}
};

#endif
