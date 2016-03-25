#include "IInterface.h"
#include "Defines.h"
#include <WebRTCAPI.h>
#include <unistd.h>
#include "ISource.h"
#include <dlfcn.h>
#include <sys/mman.h>
#include <stdlib.h>

using namespace CCStone;
static	FILE *file;
class RTCClientNodeCallback : public RTCIPCNode {
    private:
        RTCRender *_surface;
        RTCColorConvert *_convert;
        int _dst_w;
        int _dst_h;
        int _dst_stride;
    public:
        RTCClientNodeCallback(int x = 0, int y = 0, int w = 0, int h = 0) {
            //_dst_w = 984;
            //_dst_h = 1749;
            _dst_w = w;
            _dst_h = h;
            _dst_stride = 0;
            _convert = NULL;
            RTCIPCNode::died = RTCClientNodeCallback::sDied;
            RTCIPCNode::transport = RTCClientNodeCallback::sTransport;
            RTCIPCNode::set_callback = RTCClientNodeCallback::sSet_callback;
            _surface = WebRTCAPI::Create()->GetRenderAPI()->create(x, y,_dst_w, _dst_h);
            //_convert = WebRTCAPI::Create()->GetColorConvertAPI()->create(w,h, 2304, _dst_h, kKeyYCbCr420SP, kKeyRGB565, 0, w);
            CHECK(_surface);
        }

        virtual int Transport(const RTCIPCData_t *p)
        {
            Parcel arg(p->data);
            CHECK(arg.readInt32() == kCallId_DeliverVideoBuffer);
            TestVideoBuffer frame_inst;
            frame_inst.w = arg.readInt32();
            frame_inst.h = arg.readInt32();
            frame_inst.color = arg.readInt32();
            frame_inst.size = arg.readInt32();
            frame_inst.gb = arg.readGraphicBuffer();
            //frame_inst.gb = WebRTCAPI::Create()->GetGraphicsAPI()->alloc(frame_inst.w, frame_inst.h, IOMX_HAL_PIXEL_FORMAT_YCrCb_420_SP, IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN);
            CHECK(frame_inst.w && frame_inst.h && frame_inst.color && frame_inst.gb);
            class ScopedIPCGbTset {
                graphics_handle *_gb;
             public:
                 ScopedIPCGbTset(graphics_handle *gb):_gb(gb){}
                 ~ScopedIPCGbTset() {
                     printf("~Dtor ScopedGb\n");
                     WebRTCAPI::Create()->GetGraphicsAPI()->destory(_gb);
                 }
            };

            TestVideoBuffer *frame = &frame_inst;
            ScopedIPCGbTset scoped_gb(const_cast<graphics_handle *>(frame->gb));

            printf("video buffer w %d h %d color %d  size %d gb %p\n", frame->w, frame->h, frame->color, frame->size, frame->gb);
            if (false && file) {
                printf("Write file ...\n");
                void *src_base = NULL;
                WebRTCAPI::Create()->GetGraphicsAPI()->lock(const_cast<graphics_handle *>(frame->gb), 0x100, &src_base);
                printf("Write file size %d src base %p\n", 1280 * 720 * 3 / 2, src_base);
                CHECK(src_base);
                fwrite(src_base,1, frame->size, file);
                fflush(file);
                WebRTCAPI::Create()->GetGraphicsAPI()->unlock(const_cast<graphics_handle *>(frame->gb));
            }

            graphics_handle *render_buffer = NULL;
            render_buffer = WebRTCAPI::Create()->GetRenderAPI()->dequeue(_surface);
            if (render_buffer == NULL ) {
                printf("Not found avalid render buffer and skip the frame.\n");
                return 0;
            }else {
                printf("Surface w %d h %d pixfmt %d stride %d\n", WebRTCAPI::Create()->GetGraphicsAPI()->width(render_buffer),
                        WebRTCAPI::Create()->GetGraphicsAPI()->height(render_buffer), WebRTCAPI::Create()->GetGraphicsAPI()->pixelFormat(render_buffer), WebRTCAPI::Create()->GetGraphicsAPI()->stride(render_buffer));
            }
            if (_convert == NULL) {
                _dst_stride = WebRTCAPI::Create()->GetGraphicsAPI()->stride(render_buffer);
                _convert = WebRTCAPI::Create()->GetColorConvertAPI()->create(frame->w,frame->h, _dst_stride, _dst_h, kKeyYCbCr420SP, kKeyRGB565, 0, frame->w);
                CHECK(_convert);
            }

            CHECK(WebRTCAPI::Create()->GetColorConvertAPI()->convert(_convert, const_cast<graphics_handle *>(frame->gb), render_buffer) == 0);
            if (file ) {
                void *dst_base = NULL;
                WebRTCAPI::Create()->GetGraphicsAPI()->lock(render_buffer, IOMX_GRALLOC_USAGE_SW_READ_OFTEN,&dst_base);
                CHECK(dst_base);
                //fwrite(dst_base,1, _dst_h * _dst_w * 2, file);
                //fflush(file);
                WebRTCAPI::Create()->GetGraphicsAPI()->unlock(render_buffer);
            }
            WebRTCAPI::Create()->GetRenderAPI()->queue(_surface,render_buffer);
            return 0;
        }

        virtual void Died() {}
        virtual int setCallback(struct RTCIPCNode * /*cb*/) {return -1;}

        static int sTransport(struct RTCIPCNode *thiz,const RTCIPCData_t *p)
        {
            RTCClientNodeCallback *me = static_cast<RTCClientNodeCallback *>(thiz);
            return me->Transport(p);
        }

        static void sDied(struct RTCIPCNode *thiz)
        {
            RTCClientNodeCallback *me = static_cast<RTCClientNodeCallback *>(thiz);
            me->Died();
        }

        static int sSet_callback(struct RTCIPCNode *thiz, struct RTCIPCNode * cb)
        {
            RTCClientNodeCallback *me = static_cast<RTCClientNodeCallback *>(thiz);
            return me->setCallback(cb);
        }
};

int main(int c, char **s)
{
    if (c < 6) {
        printf("Usage:%s x y w h save_file\n",s[0]);
        return 0;
    }
    file = fopen(s[5],"w");
    CHECK(file);
    const char *name = "media_source_service";
    RTCIPCHandle_t * ipc = WebRTCAPI::Create()->GetIPCInteraceAPI();
    /////////////////////////////////////
    RTCIPCNode_t *source = ipc->create(name, kNodeTypeSourceCam);
    CHECK(source);

    IPCArgs args;
    Parcel *in = args.data();
    
    // Start capture
    in->writeInt32(ISource::kCallId_Start);
    int ret = source->transport(source,args.args());

    int w = ISource::kCallId_Width;
    int h = ISource::kCallId_Height;
    int color = ISource::kCallId_Color;

    // Get width
    in->setPosition(0);
    in->writeInt32(ISource::kCallId_Width);
    w = source->transport(source, args.args());

    // Get height
    in->setPosition(0);
    in->writeInt32(ISource::kCallId_Height);
    h = source->transport(source, args.args());

    // Get color format
    in->setPosition(0);
    in->writeInt32(ISource::kCallId_Color);
    color = source->transport(source, args.args());

    printf("Call start ret %d w %d h %d color %d\n", ret, w, h, color);
    CHECK(ret == 0);

    /////////////////////////////////////
    RTCIPCNode_t *stream = ipc->create(name, kNodeTypeStream);
    if (stream == NULL) {
        printf("Get Stream faild...\n");
        return 0;
    }

    RTCClientNodeCallback * stream_callback = new RTCClientNodeCallback(atoi(s[1]) , atoi(s[2]), atoi(s[3]), atoi(s[4]));
    CHECK(stream->set_callback(stream, stream_callback) == 0 );
    
    in->setPosition(0);
    in->writeInt32(kCallId_ConnectSource);
    in->writeInt32(kNodeTypeSourceCam);

    ret = stream->transport(stream, args.args());
    printf("Connect stream is %d == 0\n", ret);
    CHECK( ret == 0 );
    printf("sleep ...\n");
    while (1) usleep (100000);
}
