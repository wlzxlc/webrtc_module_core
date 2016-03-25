#include "render_jni.h"
#include <android/log.h>
#include "IInterface.h"
#include "Defines.h"
#include <WebRTCAPI.h>
#include <unistd.h>
#include "ISource.h"
#include <dlfcn.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <android/native_window_jni.h>


#define  LOG_TAG "render_java_test"
#define  DLOGI(fmt,...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,"%s[L%d]:"fmt,__FUNCTION__,__LINE__,## __VA_ARGS__)

using namespace CCStone;
class MediaStream {
	RTCIPCNode * _stream;
	int _id;
	const char *_servname;
public:
	MediaStream(int source_id):_stream(NULL),
		_servname("media_source_service")
	{
		_id = source_id;
	}
	const char *MediaSourceName()
	{
		return _servname;
	}
	int start()
	{
		_stream = WebRTCAPI::Create()->GetIPCInteraceAPI()->create(_servname, _id);
		CHECK(_stream);
        IPCArgs args;
        Parcel &in = *args.data();
        in.writeInt32(ISource::kCallId_Start);
		int ret = _stream->transport(_stream, args.args());
		return ret;
	}

	int stop()
	{
		CHECK(_stream);
        IPCArgs args;
        Parcel &in = *args.data();
        in.writeInt32(ISource::kCallId_Stop);
		int ret = _stream->transport(_stream, args.args());
		return ret;
	}
	int width()
	{
		CHECK(_stream);
        IPCArgs args;
        Parcel &in = *args.data();
        in.writeInt32(ISource::kCallId_Width);
		int ret = _stream->transport(_stream, args.args());
		return ret;
	}
	int height()
	{
		CHECK(_stream);
        IPCArgs args;
        Parcel &in = *args.data();
        in.writeInt32(ISource::kCallId_Height);
		int ret = _stream->transport(_stream, args.args());
		return ret;
	}
	int color()
	{
		CHECK(_stream);
        IPCArgs args;
        Parcel &in = *args.data();
        in.writeInt32(ISource::kCallId_Color);
		int ret = _stream->transport(_stream, args.args());
		return ret;
	}
};

class TestRender : public RTCIPCNode{
private:
	Mutex _lock;
	RTCRender *_render;
	RTCColorConvert *_c2d;
	int _w;
	int _h;
	int _color;
	int _dst_w;
	int _dst_h;
	int _new_dst_w;
	int _new_dst_h;
	MediaStream _source;
	ANativeWindow* _win;
	RTCIPCNode *_stream;
private:
	virtual void Died() {}
	virtual int setCallback(struct RTCIPCNode * /*cb*/) {return -1;}

	virtual int Transport(const RTCIPCData_t *p)
	{
        IPCArgs args(p);
		CHECK(args.data()->readInt32() == kCallId_DeliverVideoBuffer);
		TestVideoBuffer frame;
        frame.w = args.data()->readInt32();
        frame.h = args.data()->readInt32();
        frame.color = args.data()->readInt32();
        frame.size = args.data()->readInt32();
        frame.gb = args.data()->readGraphicBuffer();
        assert(frame.gb);
		return renderFrame(&frame);
	}

	static int sTransport(struct RTCIPCNode *thiz, const RTCIPCData_t *p)
	{
		TestRender *me = static_cast<TestRender *>(thiz);
		return me->Transport(p);
	}

	static void sDied(struct RTCIPCNode *thiz)
	{
		TestRender *me = static_cast<TestRender *>(thiz);
		me->Died();
	}

	static int sSet_callback(struct RTCIPCNode *thiz, struct RTCIPCNode * cb)
	{
		TestRender *me = static_cast<TestRender *>(thiz);
		return me->setCallback(cb);
	}

public:
	TestRender();
	int create(JNIEnv *env, jobject obj);
	int destroy(JNIEnv *env, jobject obj);
	int change(JNIEnv *env, jobject obj, int fmt, int w, int h);
	int renderFrame(const TestVideoBuffer *frame);
private:
	int initStream();
};

static TestRender _grender;
int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1create(JNIEnv *env,jobject thiz, jobject holader)
{
	DLOGI(" env %p, holder %p", env, holader);
	return _grender.create(env,holader);
	return 0;
}

int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1change(JNIEnv *env, jobject thiz,jobject holader, int format, int width, int height)
{
	DLOGI(" env %p, holder %p foramt %d width %d height %d", env, holader,format,width,height);
	return _grender.change(env, holader, format, width, height);
	return 0;
}

int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1destory(JNIEnv *env,jobject thiz,jobject holader)
{
	DLOGI("env %p holder %p", env, holader);
	return _grender.destroy(env, holader);
	return 0;
}

int TestRender::initStream()
{
	AutoLock lock(&_lock);
	CHECK(_source.start() == 0);
	_w = _source.width();
	_h = _source.height();
    _dst_w = _w;
    _dst_h = _h;
    _new_dst_w = _w;
    _new_dst_h = _h;
	_color = _source.color();
	_stream = WebRTCAPI::Create()->GetIPCInteraceAPI()->create(
		_source.MediaSourceName(), kNodeTypeStream);
	CHECK(_stream );
	CHECK(_stream->set_callback(_stream, this) == 0);

	// Connect to Cam Source
    IPCArgs args;
    args.data()->writeInt32(kCallId_ConnectSource);
    args.data()->writeInt32(kNodeTypeSourceCam);
	CHECK(_stream->transport(_stream, args.args()) == 0);
	return 0;
}

TestRender::TestRender():
_render(NULL),
_source(kNodeTypeSourceCam)
{
	_w = _h = _color = _dst_h = _dst_w = _new_dst_w = _new_dst_h = 0;
	_c2d = NULL;
	_render = NULL;
	_win = NULL;
	RTCIPCNode::transport = TestRender::sTransport;
	RTCIPCNode::died = TestRender::sDied;
	RTCIPCNode::set_callback = TestRender::sSet_callback;
	initStream();
}

int TestRender::create(JNIEnv *env, jobject obj)
{
	AutoLock lock(&_lock);
	if (_render) {
		WebRTCAPI::Create()->GetRenderAPI()->destroy(_render);
		ANativeWindow_release(_win);
		_render = NULL;
	}
	CHECK(obj && env);
	 _win = ANativeWindow_fromSurface(env, obj);
	CHECK(_win);
	_render = WebRTCAPI::Create()->GetRenderAPI()->create_from_java(_win);
	CHECK(_render);
	return 0;
}

int TestRender::destroy(JNIEnv *env, jobject obj)
{
	AutoLock lock(&_lock);
	if (_render) {
		WebRTCAPI::Create()->GetRenderAPI()->destroy(_render);
		ANativeWindow_release(_win);
		_render = NULL;
	}
	return 0;
}

int TestRender::change(JNIEnv *env, jobject obj, int fmt, int w, int h)
{
	AutoLock lock(&_lock);
	_new_dst_w = w;
	_new_dst_h = h;
	return 0;
}

class GbHelper {
    public:
        int _fd;
        void *_p;
        int _size;
        GbHelper(graphics_handle *gb, int size) {
            _size = size;
            struct test_native_hwd {
                int version;
                int fds;
                int numInt;
                int data[0];
            };
            int tt = 0;
            const test_native_hwd * lhwd = (const test_native_hwd *) WebRTCAPI::Create()->GetGraphicsAPI()->handle(gb, &tt);

            _fd = lhwd->data[0];
            _size = size;
            _p = mmap(NULL,_size,PROT_READ,MAP_SHARED, _fd, 0);

            CHECK(_p);
        }
        ~GbHelper() {
            munmap(_p, _size);
        }
};

int TestRender::renderFrame(const TestVideoBuffer *frame)
{
	AutoLock lock(&_lock);
    class ScopedIPCGbTset {
        graphics_handle *_gb;
        public:
        ScopedIPCGbTset(graphics_handle *gb):_gb(gb){}
        ~ScopedIPCGbTset() {
            printf("~Dtor ScopedGb\n");
            WebRTCAPI::Create()->GetGraphicsAPI()->destory(_gb);
        }
    };
    ScopedIPCGbTset scoped_gb(const_cast<graphics_handle *>(frame->gb));
    GbHelper src_g(const_cast<graphics_handle *>(frame->gb), frame->size);
	if (_render == NULL) {
		DLOGI("Not found avalid render and skip this frame\n");
		return 0;
	}
    DLOGI("w = %d fw = %d h = %d fh = %d", _w, frame->w, _h, frame->h);
	CHECK(_w == frame->w && _h == frame->h);
	graphics_handle *buffer = WebRTCAPI::Create()->GetRenderAPI()->dequeue(_render);

	if (buffer == NULL ){
		DLOGI("Not found buffer slot and skip this frame");
		return 0;
	}

#if 1
	if (_new_dst_w != _dst_w || _new_dst_h != _dst_h) {
        _dst_w = _new_dst_w;
        _dst_h = _new_dst_h;
		if (_c2d) {
			WebRTCAPI::Create()->GetColorConvertAPI()->destroy(_c2d);
		}
		_c2d = WebRTCAPI::Create()->GetColorConvertAPI()->create(_w, _h, _dst_w, _dst_h, kKeyYCbCr420SP, kKeyRGB565, 0, _w);
		CHECK(_c2d);
	}

    {
        GbHelper dst_g(buffer, _dst_w * _dst_h * 2);
        CHECK(WebRTCAPI::Create()->GetColorConvertAPI()->convert_fd(_c2d,src_g._fd, src_g._p, dst_g._fd, dst_g._p) == 0);
    }
#endif
	WebRTCAPI::Create()->GetRenderAPI()->queue(_render,buffer);
	return 0;
}
