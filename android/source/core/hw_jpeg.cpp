#ifdef ANDROID_ON_QCOM
#include "hw_jpeg.h"
#include "core.cc"
#include <bug_helper.h>
#include <OMX_Component.h>
#include <QOMX_JpegExtensions.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#define ENC_NAME "OMX.qcom.image.jpeg.encoder"
#define DEC_NAME "OMX.qcom.image.jpeg.decoder"
#define OMX_LIBS_NAME "libqomx_core.so"

#define CHECK_OMX(expression) do { \
    OMX_ERRORTYPE ret = expression; \
    if (ret != OMX_ErrorNone) {    \
        DEBUG("Expression " #expression "assert failure (%d)", (int)ret); \
        assert(!"Abort at expression " #expression ); \
    } \
}while(0)

#ifdef DEBUG_RTC

#ifdef ERRNUM
#undef ERRNUM
#define ERRNUM(fmt,...) sPrint(-__LINE__, fmt "\n", ##__VA_ARGS__)
#endif

class ScodedDebug {
    public:
const char *name;
int line;
ScodedDebug(int l, const char *n):name(n),line(l) {
 DEBUG("ScopedDebug:%s[L%d] --> %s", name, line, name);
}
~ScodedDebug() {
 DEBUG("ScopedDebug:%s[L%d] <-- %s", name, line, name);
}
};
#define SCODEDEBUG() ScodedDebug __scoped_debug(__LINE__, __FUNCTION__)
static int sPrint(int ret, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return ret;
}

#else
#define SCODEDEBUG()
#ifndef ERRNUM
#define ERRNUM(fmt,...) -__LINE__
#endif
#endif

template<class T>
static void InitOMXParams(T *params) {
    memset(params, 0, sizeof(T));
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

static OMX_COLOR_FORMATTYPE GetOmxColor(int color)
{
    //return (OMX_COLOR_FORMATTYPE) (OMX_COLOR_FormatVendorStartUnused + 0x300);
    OMX_COLOR_FORMATTYPE type = OMX_COLOR_FormatYUV420Planar;
    switch(color) {
        case kJPEGYUV422:
            type = OMX_COLOR_FormatYUV422Planar;
            break;
        case kJPEGRGB565:
            type = OMX_COLOR_Format16bitRGB565;
            break;
        case kJPEGNV12:
            type = (OMX_COLOR_FORMATTYPE)OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar;
            //type = OMX_COLOR_FormatYUV420SemiPlanar;
            break;
        case kJPEGI420P:
            type = (OMX_COLOR_FORMATTYPE)OMX_QCOM_IMG_COLOR_FormatYVU420Planar;
            break;
        default:
            break;
    }
    return type;
}

static int CalcRawImageSize(int w, int h, int color)
{
    int size = 0;
    switch(color) {
        case kJPEGYUV422:
        case kJPEGRGB565:
            size = w * h *2;
            break;
        case kJPEGNV12:
        case kJPEGI420P:
            size = w * h * 3 / 2;
            break;
        default:
            size = 0;
            break;
    }
    return size;
}

static void GetFdAndVaddrFromGraphicBuffer(jpeg_buffer *buffer)
{
    VideoBuffer *gbuffer = (VideoBuffer *)buffer->gb;
    const native_handle_t* native_handle = gbuffer->graphicBuffer->getNativeBuffer()->handle;
    int fd = native_handle->data[0];

    if (fd > 0 && buffer->size > 0) {
        void *vaddr = mmap(NULL,buffer->size,PROT_READ,MAP_SHARED, fd, 0);
        if (vaddr != MAP_FAILED) {
            buffer->fd = fd;
            buffer->vaddr = vaddr;
        }
    }
}

static void FreeFdAndVaddr(jpeg_buffer *buffer)
{
    if (buffer->gb && buffer->vaddr && buffer->size > 0){
        munmap(buffer->vaddr, buffer->size);
        buffer->vaddr = NULL;
        buffer->fd = 0;
    }
}

namespace RTCInternalJPEGClass {
class Condition;
class Mutex {
    private:
        friend class Condition;
        pthread_mutex_t _mutex;
        pthread_mutexattr_t _attr;
    public:
        Mutex(){
            pthread_mutexattr_init(&_attr);
            pthread_mutex_init(&_mutex, &_attr);
        }

        void lock()
        {
            pthread_mutex_lock(&_mutex);
        }
        void unlock()
        {
            pthread_mutex_unlock(&_mutex);
        }
};

class AutoLock {
    private:
        Mutex *_mutex;
    public:
        AutoLock(Mutex *lock):_mutex(lock)
    {
        _mutex->lock();
    }
        ~AutoLock()
        {
            _mutex->unlock();
        }
};

class Condition {
    private:
        pthread_condattr_t _attr;
        pthread_cond_t _cond;
    public:
        Condition() {
            pthread_condattr_init(&_attr);
            pthread_cond_init(&_cond, &_attr);
        }

        int wait(Mutex *mutex) {
            return pthread_cond_wait(&_cond, &mutex->_mutex);
        }

        int signal() {
            return pthread_cond_signal(&_cond);
        }

        int broadcast() {
            return pthread_cond_broadcast(&_cond);
        }

        int timedWait(Mutex *mutex, long long ms) {
            struct timespec spec;
            spec.tv_sec = ms / 1000;
            spec.tv_nsec = (ms % 1000) * 1E9;
            return pthread_cond_timedwait(&_cond,&mutex->_mutex, &spec);
        }

        ~Condition() {
        }
};
} // end of RTCInternalJPEGClass
class JpegInstence {
    public:
        virtual OMX_ERRORTYPE EventDone(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventData) = 0;
        virtual OMX_ERRORTYPE EmptyDone(OMX_BUFFERHEADERTYPE *pBuffer) = 0;
        virtual OMX_ERRORTYPE FillDone(OMX_BUFFERHEADERTYPE *pBuffer) = 0;
        virtual int Process(jpeg_codec_parameters *){return -1;}
        virtual ~JpegInstence(){}
};

class OmxCtx {
    private:
        void *_hwd;
        OMX_ERRORTYPE (*_omx_init)();
        OMX_ERRORTYPE (*_omx_deinit)();
        OMX_ERRORTYPE (*_omx_get_handle)(OMX_HANDLETYPE *, OMX_STRING , OMX_PTR , OMX_CALLBACKTYPE *);
        OMX_ERRORTYPE (*_omx_free_handle)(OMX_HANDLETYPE );

       static OMX_ERRORTYPE sEventHandler(
                OMX_IN OMX_HANDLETYPE hComponent,
                OMX_IN OMX_PTR pAppData,
                OMX_IN OMX_EVENTTYPE eEvent,
                OMX_IN OMX_U32 nData1,
                OMX_IN OMX_U32 nData2,
                OMX_IN OMX_PTR pEventData)
        {
            JpegInstence *inst = static_cast<JpegInstence *>(pAppData);
            return inst->EventDone(eEvent, nData1, nData2, pEventData);
        }

       static OMX_ERRORTYPE sEmptyBufferDone(
                OMX_IN OMX_HANDLETYPE hComponent,
                OMX_IN OMX_PTR pAppData,
                OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
        {
            JpegInstence *inst = static_cast<JpegInstence *>(pAppData);
            return inst->EmptyDone(pBuffer);
        }

       static OMX_ERRORTYPE sFillBufferDone(
                OMX_OUT OMX_HANDLETYPE hComponent,
                OMX_OUT OMX_PTR pAppData,
                OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
        {
            JpegInstence *inst = static_cast<JpegInstence *>(pAppData);
            return inst->FillDone(pBuffer);
        }

    public:
        OmxCtx() {
            _hwd = NULL;
            _omx_init = NULL;
            _omx_deinit = NULL;
            _omx_get_handle = NULL;
            _omx_free_handle = NULL;
            _hwd = dlopen(OMX_LIBS_NAME,RTLD_NOW);
            if (_hwd) {
                _omx_init = (typeof (_omx_init) )dlsym(_hwd, "OMX_Init");
                _omx_deinit = (typeof (_omx_deinit) )dlsym(_hwd, "OMX_Deinit");
                _omx_get_handle = (typeof (_omx_get_handle) )dlsym(_hwd, "OMX_GetHandle");
                _omx_free_handle = (typeof (_omx_free_handle) )dlsym(_hwd, "OMX_FreeHandle");
                if (!(_omx_init && _omx_deinit &&
                            _omx_get_handle && _omx_free_handle)){
                    DEBUG("Find symbol faiure in the library %s", OMX_LIBS_NAME);
                    dlclose(_hwd);
                    _hwd = NULL;
                    return;
                }else {
                 _omx_init();
                }
            }else {
                DEBUG("Dlopen %s faiure.", OMX_LIBS_NAME);
            }
        }

        OMX_HANDLETYPE GetHandle(bool encoder, JpegInstence *appdata)
        {
            OMX_STRING name = (OMX_STRING) (encoder ? ENC_NAME : DEC_NAME);

            if (!_omx_get_handle) {
                return NULL;
            }
            OMX_CALLBACKTYPE *callback = new OMX_CALLBACKTYPE;
            callback->EmptyBufferDone = sEmptyBufferDone;
            callback->FillBufferDone = sFillBufferDone;
            callback->EventHandler = sEventHandler;
            OMX_HANDLETYPE hwd = NULL;
            _omx_get_handle(&hwd, name, appdata, callback);
            return hwd;
        }

        int FreeHandle (OMX_HANDLETYPE p) {
            if (p && _omx_free_handle) {
                _omx_free_handle(p);
            }
            return 0;
        }

        ~OmxCtx() {
            if (_omx_deinit) {
                _omx_deinit();
                if (_hwd) {
                    dlclose(_hwd);
                }
            }
        }
};

static OmxCtx ctx;

class JpegInstenceQCom : public JpegInstence{
    private:
    enum {
        kEventNothing = 101,
        kEventFBD,
        kEventEBD,
        kEventComponent
    };
    private:
    OMX_HANDLETYPE _hwd;
    bool _encoder;
    RTCInternalJPEGClass::Mutex _lock;
    RTCInternalJPEGClass::Condition _cond;
    OMX_STATETYPE _stat;
    int _event;
    bool _no_error;
    bool _enable_thumbail;
    OMX_BUFFERHEADERTYPE *_omxbuffer[kJPEGPortMAX][JPEG_MAX_BUFFER];
    private:

    int WaitEvent(int event) {
        SCODEDEBUG();
        RTCInternalJPEGClass::AutoLock lock(&_lock);
        while (_event != event && _no_error) {
            _cond.timedWait(&_lock, 30);
        }
        return _no_error ? 0 : ERRNUM("");
    }

    int SetEvent(int event) {
        SCODEDEBUG();
        {
        //AutoLock lock(&_lock);
        _event = event;
        }
        _cond.broadcast();
        return 0;
    }

    int WaitStat(OMX_STATETYPE new_stat)
    {
        SCODEDEBUG();
        RTCInternalJPEGClass::AutoLock lock(&_lock);
        if (new_stat == _stat) return 0;
        while(_stat != new_stat && _no_error) {
            _cond.timedWait(&_lock, 30);
        }
        return _no_error ? 0 : ERRNUM("");
    }

    int SetStat(OMX_STATETYPE new_stat) {
        SCODEDEBUG();
        {
        //AutoLock lock(&_lock);
        DEBUG("Component stat %d - > %d", _stat, new_stat);
        _stat = new_stat;
        }
        _cond.broadcast();
        return 0;
    }

    int ConfigPort(jpeg_codec_parameters *args)
    {
        int err = 0;
        if (_encoder) {
            // Check input port
            if (args->w < 1 || args->h < 1 ||
                    args->color <= kJPEGColorFmtStart || args->color >= kJPEGColorFmtMAX) {
                return ERRNUM("Parameters w/h/color is invalid.");
            }

            if (args->buffer_cnt[kJPEGPortInput] > 1 ||
                    args->buffer_cnt[kJPEGPortInput] < 1) {
                return ERRNUM("Input port only to support one buffer on QCOM");
            }

            int size = CalcRawImageSize(args->w, args->h, args->color);
            if (args->buffer[kJPEGPortInput]->size < size) {
                return ERRNUM("Input buffer too small.");
            }

            jpeg_buffer *buf = args->buffer[kJPEGPortInput];
            for (int i = 0; i < args->buffer_cnt[kJPEGPortInput]; i++, buf++) {
                if (buf == NULL || (buf->vaddr == NULL && buf->gb == NULL)) {
                    return ERRNUM("Input buffer(idx:%d) address invalid.", i);
                }
            }


            // Check outout port
            if (args->buffer_cnt[kJPEGPortOutput] > 1 ||
                    args->buffer_cnt[kJPEGPortOutput] < 1) {
                return ERRNUM("Output port only to support one buffer on QCOM");
            }

            buf = args->buffer[kJPEGPortOutput];
            for (int i = 0; i < args->buffer_cnt[kJPEGPortOutput]; i++, buf++) {
                if (buf == NULL || (buf->vaddr == NULL && buf->gb == NULL)) {
                    return ERRNUM("Output buffer(idx:%d) address invalid.", i);
                }
            }

            // Check thumbail port
            bool enable_thumbail = args->thumbnail_w | args->thumbnail_h;
            _enable_thumbail = enable_thumbail;
            if (enable_thumbail) {
                DEBUG("Enable thumbail port.");
                if (args->thumbnail_w < 1 || 
                        args->thumbnail_h < 1 || 
                        args->thumbnail_color <= kJPEGColorFmtStart ||
                        args->thumbnail_color >= kJPEGColorFmtMAX || 
                        args->buffer_cnt[kJPEGPortThumbnail] > 1 || 
                        args->buffer_cnt[kJPEGPortThumbnail] < 1) {
                    return ERRNUM("Thumbail port parameter invalid.");
                }
                buf = args->buffer[kJPEGPortThumbnail];
                for (int i = 0; i < args->buffer_cnt[kJPEGPortThumbnail]; i++) {
                    if (buf == NULL || buf->vaddr == NULL) {
                        return ERRNUM("Thumbail buffer(idx:%d) address invalid.", i);
                    }
                }
            }

            for (int port = 0; port < kJPEGPortMAX ; port++) {
                if (port == kJPEGPortThumbnail && !enable_thumbail) {
                    CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandPortDisable, port, NULL));
                    continue;
                }
                OMX_PARAM_PORTDEFINITIONTYPE def;
                InitOMXParams(&def);
                def.nPortIndex = port;
                CHECK_OMX(OMX_GetParameter(_hwd, OMX_IndexParamPortDefinition, &def));

                def.nPortIndex = port;
                def.nBufferCountActual = args->buffer_cnt[port];
                def.nBufferSize = args->buffer[port]->size;

                DEBUG("Port #%d Setup ... ", (int)def.nPortIndex);
                DEBUG(" BufferCountActual = %d", (int)def.nBufferCountActual);
                DEBUG(" BufferSize = %d", (int)def.nBufferSize);

                if (port == kJPEGPortInput || port == kJPEGPortThumbnail) {
                    def.format.image.nFrameWidth = args->w;
                    def.format.image.nFrameHeight = args->h;
                    def.format.image.nStride = args->w;
                    def.format.image.nSliceHeight = args->h;
                    def.format.image.eColorFormat = GetOmxColor(args->color);
                    DEBUG(" W = %d", (int)def.format.image.nFrameWidth);
                    DEBUG(" H = %d", (int)def.format.image.nFrameHeight);
                    DEBUG(" Stride = %d", (int)def.format.image.nStride);
                    DEBUG(" Slice = %d", (int)def.format.image.nSliceHeight);
                    DEBUG(" Color = %d", (int)def.format.image.eColorFormat);
                }
                CHECK_OMX(OMX_SetParameter(_hwd, OMX_IndexParamPortDefinition, &def));

                for (OMX_U32 i = 0; i < def.nBufferCountActual; i++) {
                    _omxbuffer[port][i] = NULL;
                }

                if (port == kJPEGPortThumbnail && enable_thumbail) {
                    CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandPortEnable, port, NULL));
                }
            }
            err = 0;

        }else {
            // Decoder
            err = ERRNUM("not implement.");
        }
        return err;
    }

    int AllocBufferOnPort(jpeg_codec_parameters *p, int port)
    {
        SCODEDEBUG();
        if (port == kJPEGPortThumbnail && !_enable_thumbail) {
            return ERRNUM("Not alloc buffer at #%d", port);
        }
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = port;
        CHECK_OMX(OMX_GetParameter(_hwd, OMX_IndexParamPortDefinition, &def));
        DEBUG("Port #%d alloc buffers %d size %d ", (int)port, def.nBufferCountActual, def.nBufferSize);

        jpeg_buffer *buffer = p->buffer[port];
        for (OMX_U32 idx = 0; idx < def.nBufferCountActual; idx++, buffer++) {
            QOMX_BUFFER_INFO info;
            memset(&info, 0, sizeof(info));
            if (buffer->gb) {
                GetFdAndVaddrFromGraphicBuffer(buffer);
            }
            if (buffer->fd > 0) {
                DEBUG(" Used ION buffer at #%d idx %d", port, idx);
                info.fd = buffer->fd;
                info.offset = buffer->offset;
            }
            CHECK_OMX(OMX_UseBuffer(_hwd, &_omxbuffer[port][idx], port, info.fd ? (&info) : NULL,
                        def.nBufferSize,(OMX_U8 *) buffer->vaddr));
            assert(_omxbuffer[port][idx]->pBuffer == buffer->vaddr);
            DEBUG(" AllocBuffer idx %d size %d VS %d memory addr %p omx_buffer %p", (int)idx, buffer->size, (int)def.nBufferSize, buffer->vaddr, _omxbuffer[port][idx]);
        }
        return 0;
    }

    int AllocBuffer(jpeg_codec_parameters *p)
    {
        SCODEDEBUG();
        int ret = -1;
        CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandStateSet, OMX_StateIdle, NULL));
        for (int idx = 0; idx < kJPEGPortMAX; idx++) {
            if (idx == kJPEGPortThumbnail && !_enable_thumbail) {
                continue;
            }
            ret = AllocBufferOnPort(p, idx);
            if (ret) {
                return ret;
            }
        }
        DEBUG("Wait Idle statue...");
        ret = WaitStat(OMX_StateIdle);
        if (!ret) {
            CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandStateSet, OMX_StateExecuting, NULL));
        }
        DEBUG("Wait Executing statue...");
        return WaitStat(OMX_StateExecuting);
    }

    int FreeBuffer(jpeg_codec_parameters *p)
    {
        SCODEDEBUG();
        SCOPEDCOSTTIME_FUN();
        for (int port = 0; port < kJPEGPortMAX; port++) {
            if (port == kJPEGPortThumbnail && !_enable_thumbail) {
                continue;
            }
            for (int idx = 0; idx < p->buffer_cnt[port]; idx++){
                OMX_BUFFERHEADERTYPE *buffer = _omxbuffer[port][idx];
                CHECK_OMX(OMX_FreeBuffer(_hwd, port, buffer));
                _omxbuffer[port][idx] = NULL;

                jpeg_buffer *user_buffer = p->buffer[port] + idx;
                if (user_buffer->gb) {
                 FreeFdAndVaddr(user_buffer);
                }
            }
        }
        return 0;
    }

    int DoIt(jpeg_codec_parameters *args)
    {
        SCODEDEBUG();
        SCOPEDCOSTTIME_FUN();
        for (int port = kJPEGPortMAX - 1; port >= 0; port--) {
            if (port == kJPEGPortThumbnail && !_enable_thumbail) {
                continue;
            }
            jpeg_buffer *jbuff = args->buffer[port];
            for (int i = 0; i < args->buffer_cnt[port]; i++, jbuff++){
                OMX_BUFFERHEADERTYPE *buffer = _omxbuffer[port][i];
                if (port == kJPEGPortOutput) {
                    buffer->nFilledLen = 0;
                    DEBUG("#%d idx %d FillThisBuffer omx_buffer %p memory addr %p",
                            port, (int)i, buffer, buffer->pBuffer);
                    CHECK_OMX(OMX_FillThisBuffer(_hwd, buffer));
                }else {
                    DEBUG("#%d idx %d EmptyThisBuffer omx_buffer %p memory addr %p size %d",
                            port, (int)i, buffer, buffer->pBuffer, jbuff->len);
                    buffer->nFilledLen = jbuff->len;
                    CHECK_OMX(OMX_EmptyThisBuffer(_hwd, buffer));
                }
            }
        }

        int ret = WaitEvent(kEventFBD);

        for (int i = 0; i < args->buffer_cnt[kJPEGPortOutput]; i++) {
            if (_omxbuffer[kJPEGPortOutput][i]->nFilledLen) {
             args->buffer[kJPEGPortOutput]->len = (int)_omxbuffer[kJPEGPortOutput][i]->nFilledLen;
             break;
            }
        }


        CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandStateSet, OMX_StateIdle, NULL));
        WaitStat(OMX_StateIdle);
        CHECK_OMX(OMX_SendCommand(_hwd, OMX_CommandStateSet, OMX_StateLoaded, NULL));
        FreeBuffer(args);
        WaitStat(OMX_StateLoaded);
        return ret;
    }

    public:
            JpegInstenceQCom(bool encoder):_encoder(encoder) {
                _hwd = NULL;
                _enable_thumbail = false;
                _no_error = true;
                _event = kEventNothing;
            }

            bool CheckHandle() {
                RTCInternalJPEGClass::AutoLock lock(&_lock);
                _hwd = ctx.GetHandle(_encoder, this);
                _stat = OMX_StateLoaded;
                return _hwd ? true : false;
            }


            virtual OMX_ERRORTYPE EventDone(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventData)
            {
                SetEvent(kEventComponent);
                if(event == OMX_EventError) {
                    _no_error = false;
                    SetStat(OMX_StateInvalid);
                } else if (event == OMX_EventCmdComplete) {
                    OMX_COMMANDTYPE cmd = (OMX_COMMANDTYPE)data1;
                    switch(cmd) {
                        case OMX_CommandStateSet:
                            SetStat((OMX_STATETYPE)data2);
                            break;
                        default:
                            DEBUG("Ingore command cmd %d data1 %d data2 %d", (int)cmd,(int)data1, (int)data2);
                            break; 
                    } 
                }else {
                    DEBUG("Ingore event %d data1 %d data2 %d", (int)event,(int)data1, (int)data2);
                }
                return OMX_ErrorNone;
            }

            virtual OMX_ERRORTYPE EmptyDone(OMX_BUFFERHEADERTYPE *pBuffer)
            {
                SetEvent(kEventEBD);
                return OMX_ErrorNone;
            }

            virtual OMX_ERRORTYPE FillDone(OMX_BUFFERHEADERTYPE *pBuffer)
            {
                SCODEDEBUG();
                SetEvent(kEventFBD);
                return OMX_ErrorNone;
            }

            virtual int Process(jpeg_codec_parameters *p)
            {
                int ret = ConfigPort(p);
                if (ret == 0 && !(ret = AllocBuffer(p))) {
                    ret = DoIt(p);
                }
                return ret;
            }

            ~JpegInstenceQCom() {
                RTCInternalJPEGClass::AutoLock lock(&_lock);

                if (_hwd) {
                    ctx.FreeHandle(_hwd);
                }
            }
};

static int jpeg_close(void *hwd) {
    JpegInstenceQCom *inst = (JpegInstenceQCom *)hwd;
    delete inst;
    return 0;
}

static void *jpeg_open(bool encoder) {
    SCOPEDCOSTTIME_FUN();
    JpegInstenceQCom *inst = new JpegInstenceQCom(encoder);
    if (!inst->CheckHandle()) {
        delete inst;
        inst = NULL;
    }
    return inst;
}

static int jpeg_process(void *hwd, jpeg_codec_parameters *args) {
    SCOPEDCOSTTIME_FUN();
    JpegInstence *inst = static_cast<JpegInstence *>(hwd);
    if (!(inst && args)) {
        return ERRNUM("");
    }
    return inst->Process(args);
}

extern "C" {
 int PREFIX(Android_HWJpeg)(RTCJpegCodec_t *p)
 {
     SCODEDEBUG();
     if (p) {
         memset(p, 0, sizeof(*p));
         p->open = jpeg_open;
         p->process = jpeg_process;
         p->close = jpeg_close;
     }
     return 0;
 }
}
#endif
