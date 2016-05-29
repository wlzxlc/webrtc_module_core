#ifndef _RTC_CORE_API_H_
#define _RTC_CORE_API_H_

#include <stdlib.h>
#include <string.h>

#if 0
#include <gui/Surface.h>
#endif 

#include "bug_helper.h"

using namespace android;

class VideoBuffer{
    public:
        sp<GraphicBuffer> graphicBuffer;
        virtual void release() = 0;
        virtual ~VideoBuffer() { DEBUG("~Dtor VideoBuffer");}
};
#if 0
struct DummyConsumer;

struct LocalSurface{
    sp<Surface> surface;
    sp<IGraphicBufferProducer> producer;
    DummyConsumer *csr;
};
#endif

#endif //_RTC_CORE_API_H_
