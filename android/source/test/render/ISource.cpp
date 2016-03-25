#include "Defines.h"
#include <WebRTCAPI.h>
#include "test/cap_test/camera_parameters.h"
#include "ISource.h"
#include <IInterface.h>
#include <list>
#include <algorithm>
#include <dlfcn.h>
#include <sys/mman.h>
using namespace CCStone;

class MediaSourceImp: public ISource{
    protected:
        int _w;
        int _h;
        int _color;
        Mutex _mutex;
        typedef std::list<IDeliver *> DeliverList;
        DeliverList _delivers;
    public:
        MediaSourceImp (int w, int h, int color);
    public:
       void deliverVideBuffer(TestVideoBuffer *);
       virtual int registerDeliver(IDeliver *);
       virtual int deregisterDeliver(IDeliver *);
       virtual int width();
       virtual int height();
       virtual int color();
	   ~MediaSourceImp();
};

class CamMediaSourceImp : public MediaSourceImp{
    private:
        RTCSurfaceHandle_t *_sfs;
        RTCSurface *_surface;
        RTCCamera_t *_cap;
        OmxGraphics_t *_omxgb;
        RTCCameraHandle *_camera;
		bool _start;
    public:
        CamMediaSourceImp (int w, int h, int color);
       virtual int start();
       virtual int stop();
	   virtual int sourceType();
    public:
        void surfaceCallback(graphics_handle *gb);
        static void sRTCSurfaceCallback(RTCSurface *,
                graphics_handle *graphics,
                void *priv_data)
        {
            CamMediaSourceImp *me = static_cast<CamMediaSourceImp *>(priv_data);
            me->surfaceCallback(graphics);
        }
};

class FileMediaSourceImp : public MediaSourceImp, public Thread {
    private:
        graphics_handle *_gb;
    public:
        FileMediaSourceImp(int w, int h, int color):
            MediaSourceImp(w, h, color)
    {
        _gb = WebRTCAPI::Create()->GetGraphicsAPI()->alloc(_w,
                _h, IOMX_HAL_PIXEL_FORMAT_YCrCb_420_SP, IOMX_GRALLOC_USAGE_HW_TEXTURE);
        CHECK(_gb);
    }
        virtual bool run()
        {
            int fps = 30;
            TestVideoBuffer buffer;
            buffer.gb = _gb;
            buffer.size = ALIGN(ALIGN(_w, 128) * ALIGN(_h, 32), 4096);
            buffer.w = _w;
            buffer.h = _h;
            buffer.color = _color;
            deliverVideBuffer(&buffer);
            usleep (1000 * (1000 / fps));
            return true;
        }

        virtual int start()
        {
            return startLoop() ? 0 : ERRNUM("Thread not run ...");    
        }

        virtual int stop()
        {
            stopLoop(true);
			return 0;
        }

		virtual int sourceType()
		{
			return kSourceFile;
		}

        ~FileMediaSourceImp()
        {
            if (_gb) {
                WebRTCAPI::Create()->GetGraphicsAPI()->destory(_gb);
            }
        }
};

int MediaSourceImp::width() {return _w;}
int MediaSourceImp::height() {return _h;}
int MediaSourceImp::color() {return _color;}

void MediaSourceImp::deliverVideBuffer(TestVideoBuffer *buffer)
{
    AutoLock lock(&_mutex); 
    DeliverList::iterator it = _delivers.begin();
    while( it != _delivers.end()) {
     (*it)->deliver(buffer);
	 it++;
    }
}

int MediaSourceImp::registerDeliver(IDeliver *deliver)
{
    AutoLock lock(&_mutex); 
    DeliverList::iterator it = std::find(_delivers.begin(), _delivers.end(), deliver);
    if (it == _delivers.end()) {
     _delivers.push_back(deliver);
     DEBUG("register ideliver[%p]", deliver);
    }
	return 0;
}

int MediaSourceImp::deregisterDeliver(IDeliver *deliver)
{
    AutoLock lock(&_mutex); 
    DeliverList::iterator it = std::find(_delivers.begin(), _delivers.end(), deliver);
    if (it != _delivers.end()) {
     _delivers.erase(it);
     DEBUG("deregister ideliver[%p]", *it);
    }
	return 0;
}

MediaSourceImp::MediaSourceImp(int w, int h, int color):
    _w(w),
    _h(h),
    _color(color)
{
    _delivers.clear();
    DEBUG("Ctor MediaSourceImp %p",this );
}

	MediaSourceImp::~MediaSourceImp()
	{
		 DEBUG("~Dtor MediaSourceImp %p",this );
	}




CamMediaSourceImp::CamMediaSourceImp(int w, int h, int color):
    MediaSourceImp(w, h, color)
{
    _sfs = NULL;
	_surface = NULL;
	_cap = NULL;
	_camera = NULL;
    DEBUG("Ctor MediaSourceImp %p",this );
	_start = false;
}

void CamMediaSourceImp::surfaceCallback(graphics_handle *gb)
{
    int gb_w = _omxgb->width(gb);
    int gb_h = _omxgb->height(gb);
    CHECK(gb_w == _w && gb_h == _h);
    TestVideoBuffer buffer;
    buffer.gb = gb;
    buffer.size =  _w * _h * 3 / 2;
    buffer.w = _w;
    buffer.h = _h;
    buffer.color = _color;
    deliverVideBuffer(&buffer);
	_omxgb->destory(gb);
}

int CamMediaSourceImp::start()
{
	if (_start) {
	 return 0;
	}
 _sfs = WebRTCAPI::Create()->GetSurfaceAPI(); 
 _cap = WebRTCAPI::Create()->GetCaptureAPI();
 _omxgb = WebRTCAPI::Create()->GetGraphicsAPI();

 if (!_sfs || !_cap || !_omxgb) {
  return ERRNUM("Get WebRTCAPI faild.");
 }

 _sfs->create(&_surface, _w, _h, IOMX_GRALLOC_USAGE_HW_TEXTURE,
         CamMediaSourceImp::sRTCSurfaceCallback,10, this);

 if (!_surface) {
  return ERRNUM("Create surface faild.");
 }

 _camera = _cap->create(0);
 if (!_camera) {
  return ERRNUM("Create camera[0] faild.");
 }

 CHECK( _cap->setRTCSurface(_camera ,_surface) == 0);

 char *szParameter = new char[10 * 1024];
 if (0 != _cap->getParameters(_camera, szParameter, 10 * 1024)) {
     DEBUG("getParameters failed!");
 }

 int fps = 30;
 CameraParameters camParam(szParameter);
 camParam.setPreviewSize(_w, _h); 
 camParam.setPictureSize(_w,_h); //for takePicture
 camParam.setPreviewFpsRange(fps*1000, fps*1000);
 camParam.setPreviewFrameRate(fps);
 std::string colorfmt;
 if (_color == kTestColorNV12) {
  colorfmt = "yuv420sp";
 }
 camParam.setPreviewFormat(colorfmt.c_str());
 camParam.set("zsl", "off");
 _cap->setParameters(_camera, camParam.flatten().c_str());
 delete []szParameter;
 CHECK( _cap->startPreview(_camera) == 0);
 _start = true;
 return 0;
}

int CamMediaSourceImp::stop()
{
  if (_cap && _camera) {
   _cap->stopPreview(_camera);
   _cap->destroy(_camera); 
   _cap = NULL;
   _camera = NULL;
  }
  if (_surface) {
   _sfs->destroy(_surface);
  }
  _start = false;
  return 0;
}

int CamMediaSourceImp::sourceType()
{
	return kSourceCamera;
}




ISource * ISource::Create(int type, int w, int h, int color)
{
  ISource *source = NULL;
  if (type == kNodeTypeSourceCam){
   source = new CamMediaSourceImp(w, h, color);
  }else if (type == kNodeTypeSourceFile) {
   source = new FileMediaSourceImp(w, h, color);
  }
  return source;
}
