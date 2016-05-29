#include "WebRTCAPI.h"
#include "bug_helper.h"
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#ifndef CHECK
#define CHECK assert
#endif
using namespace CCStone;
using namespace std;
#if 0
class OMXCallBack : public IOMXCallBack{
    public:
        virtual OMX_ERRORTYPE EventHandler(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_EVENTTYPE eEvent,
                OMX_U32 nData1,
                OMX_U32 nData2,
                OMX_PTR pEventData){return OMX_ErrorNone;}

        virtual OMX_ERRORTYPE EmptyBufferDone(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_BUFFERHEADERTYPE* pBuffer){return OMX_ErrorNone;}

        virtual OMX_ERRORTYPE FillBufferDone(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_BUFFERHEADERTYPE* pBuffer){return OMX_ErrorNone;}
        virtual ~OMXCallBack(){}
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}
void testOMX(OMXCore &core)
{
    assert(core.OMX_Init() == 0);
    int index = 0;
    string targetcomp;
    const char *avcro = "video_encoder.avc";
    while ( true ){
        char comp[256] = {0};
        if (core.OMX_ComponentNameEnum(comp,sizeof(comp),index++) != OMX_ErrorNone){
            break;
        }
        cout << "Components: " << (const char *)comp << endl;
        OMX_U32 size = 3;
        char *roles[3] = {0};
        char r[3][256] = { 0 };
        roles[0] = r[0];
        roles[1] = r[1];
        roles[2] = r[2];
        core.OMX_GetRolesOfComponent(comp,&size, (unsigned char **)roles);
        printf("  %s\n",roles[0]);
        cout << "--------------------------------"<< endl;
        if (strcmp(avcro, roles[0]) == 0 && strncmp(comp, "OMX.qcom.", 9) == 0 && targetcomp.size() == 0){
            targetcomp = string(comp);
        }

    }
    cout << "Total " << index-- << " components." << endl;
    cout <<" Test " << targetcomp << endl;
    core.JoinBinderListing();
    OMX_HANDLETYPE handle = NULL;
    int sdk = 0;
    cout << core.OMX_GetHandle(&handle ,const_cast<char *>(targetcomp.c_str()), &sdk, new OMXCallBack) << "handle " << handle << endl;

    if (handle == 0){
        assert(core.OMX_Deinit() == 0);
        return;
    }
    OMX_VIDEO_PARAM_PORTFORMATTYPE portFormat;
    InitOMXParams(&portFormat);
    portFormat.nPortIndex = 0;
    OMX_U32 indexu = 0;
    portFormat.nIndex = indexu;
    while (true) {
        if (OMX_ErrorNone != OMX_GetParameter(
                    handle, OMX_IndexParamVideoPortFormat,
                    &portFormat)) {
            break;
        }
        cout <<"Color " << portFormat.eColorFormat << endl;
        ++indexu;
        portFormat.nIndex = indexu;
    }
    assert(core.OMX_FreeHandle(handle) == 0);
    assert(core.OMX_Deinit() == 0);
}

void testGb(OmxGraphics_t *fun)
{
    if (!fun) return;
    printf("======== Dump OmxGraphics_t start =========\n");
    printf("Dump OmxGraphics_t:\n");
    printf("  alloc:%p\n",fun->alloc);
    printf("  destory:%p\n",fun->destory);
    printf("  width:%p\n",fun->width);
    printf("  height:%p\n",fun->height);
    printf("  stride:%p\n",fun->stride);
    printf("  usage:%p\n",fun->usage);
    printf("  pixelFormat:%p\n",fun->pixelFormat);
    printf("  rect:%p\n",fun->rect);
    printf("  reallocate:%p\n",fun->reallocate);
    printf("  lock:%p\n",fun->lock);
    printf("  lockRect:%p\n",fun->lockRect);
    printf("  unlock:%p\n",fun->unlock);
    printf("  lockYCbCr:%p\n",fun->lockYCbCr);
    printf("  lockYCbCrRect:%p\n",fun->lockYCbCrRect);
    printf("  handle:%p\n",fun->handle);
    printf("======== Dump OmxGraphics_t end =========\n"); 
    assert(fun->alloc != NULL);
    graphics_handle *graphicBuffer = fun->alloc(1280, 720,
            IOMX_QCOM_HAL_PIXEL_FORMAT_NV12_ENCODEABLE,
            IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN | IOMX_GRALLOC_USAGE_SW_READ_OFTEN);
    assert(graphicBuffer);
    assert(fun->width(graphicBuffer) == 1280);
    assert(fun->height(graphicBuffer) == 720);
    graphics_ycbcr_t yuv;
    int size = 0;
    printf("native_handle %p\n", fun->handle(graphicBuffer, &size));
    assert(fun->lockYCbCr(graphicBuffer,IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN,&yuv) == 0);
    printf("Map memory address for yuv\n");
    int hand_len = 0;//
    fun->handle(graphicBuffer, &hand_len);
    printf("y %p cb %p cr %p ystride %d cstride %d chroma_step %d native handle %p len %d\n",
            yuv.y, yuv.cb, yuv.cr, yuv.ystride, yuv.cstride, yuv.chroma_step,
            fun->handle(graphicBuffer, &hand_len),hand_len);

    memset(yuv.y, 'y', 1280);
    for (int i = 0 ; i < (int)yuv.cstride; i+= yuv.chroma_step ){
        ((char *)(yuv.cb))[i] = 'u';
        ((char *)(yuv.cr))[i] = 'v';
    }
    assert(((char *)(yuv.y))[1275] == 'y');
    assert(((char *)(yuv.cb))[yuv.chroma_step * 44] == 'u');
    assert(((char *)(yuv.cr))[yuv.chroma_step * 55] == 'v');
    fun->unlock(graphicBuffer);
    memset(yuv.y, 'y', 1280);
    ARect rect;
    rect.left = 500;
    rect.top = 500;
    rect.right = 100;
    rect.bottom = 100;
    assert(fun->lockYCbCrRect(graphicBuffer,IOMX_GRALLOC_USAGE_SW_READ_OFTEN,rect,&yuv) == 0);
    printf("Map memory address for yuv\n");
    fun->handle(graphicBuffer, &hand_len);
    printf("y %p cb %p cr %p ystride %d cstride %d chroma_step %d native handle %p len %d\n",
            yuv.y, yuv.cb, yuv.cr, yuv.ystride, yuv.cstride, yuv.chroma_step,fun->handle(graphicBuffer, &hand_len),hand_len);
    memset(yuv.y, 'y', 1280);
    assert(((char *)(yuv.y))[1275] == 'y');
    assert(((char *)(yuv.cb))[yuv.chroma_step * 33] == 'u');
    assert(((char *)(yuv.cr))[yuv.chroma_step * 22] == 'v');
    fun->unlock(graphicBuffer);

    void * buffer = NULL;
    assert(fun->lockRect(graphicBuffer,IOMX_GRALLOC_USAGE_SW_READ_OFTEN,rect,&buffer) == 0 && buffer);
    printf("lock addr %p\n",buffer);
    for (int i = 0 ; i < 10 ; i ++){
        printf("%c",((char *)(buffer))[i]);
    }
    printf("\n");
    buffer = NULL;
    assert(fun->lock(graphicBuffer,IOMX_GRALLOC_USAGE_SW_READ_OFTEN,&buffer) == 0 && buffer);
    printf("lock addr %p\n",buffer);
    for (int i = 0 ; i < 10 ; i ++){
        printf("%c",((char *)(buffer))[i]);
    }
    printf("\n");

    fun->destory(graphicBuffer);
}

void testSurface(RTCSurfaceHandle_t *sfs)
{
    if (!sfs) return;
    printf("======== Dump RTCSurfaceHandle_t start =========\n"); 
    printf("  create:%p\n",sfs->create);
    printf("  destory:%p\n",sfs->destroy);
    printf("  window:%p\n",sfs->window);
    printf("  releaseBuffer:%p\n",sfs->releaseBuffer);
    printf("======== Dump RTCSurfaceHandle_t end =========\n"); 
}

void testColorConvert(RTCColorConvertHandle_t *module)
{
  if (!module) return;
  printf("======== Dump RTCColorConvertHandle_t start =========\n");
  printf("  create:%p\n",module->create);
  printf("  destory:%p\n",module->destroy);
  printf("  convert:%p\n",module->convert);
  if (module->create){
      RTCColorConvert *convert = module->create(
            1280, 720, 640, 480, kKeyYCbCr420SP, kKeyNV12_128m, 0, 1280);
      assert(convert);
      WebRTCAPI &api = *WebRTCAPI::Create();
      OmxGraphics_t *gbm = api.GetGraphicsAPI();
      if(gbm->alloc == NULL){
          return;
        }
      graphics_handle *src_graphicBuffer = gbm->alloc(1280, 720,
              IOMX_QCOM_HAL_PIXEL_FORMAT_NV12_ENCODEABLE,
              IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN | IOMX_GRALLOC_USAGE_SW_READ_OFTEN);
      graphics_handle *dst_graphicBuffer = gbm->alloc(640, 480,
              IOMX_QCOM_HAL_PIXEL_FORMAT_NV12_ENCODEABLE,
              IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN | IOMX_GRALLOC_USAGE_SW_READ_OFTEN);
      if (src_graphicBuffer && dst_graphicBuffer){
          void *src_base  = NULL;
          void *dst_base = NULL;
          gbm->lock(src_graphicBuffer,IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN |
                    IOMX_GRALLOC_USAGE_SW_READ_OFTEN, &src_base);
          gbm->lock(dst_graphicBuffer,IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN |
                    IOMX_GRALLOC_USAGE_SW_READ_OFTEN, &dst_base);
          if (src_base && dst_base){
              memset(src_base,'y',1280 * 720 * 3  / 2);
              memset(dst_base,0,640 * 480 * 3  / 2);\
              printf("Convert 720p to 480p\n");
              assert(module->convert(convert, src_graphicBuffer, dst_graphicBuffer) == 0);
              uint8_t *dst_buffer = (uint8_t *)(dst_base);
              assert(dst_buffer[0] == 'y');
              assert(dst_buffer[640 * 480] == 'y');
              if (dst_buffer[640 * 420] != 'y'){
                  printf("Convert 720p to 480p failed...\n");
                }else{
                  printf("Convert 720p to 480p ok...\n");
                }
              gbm->unlock(src_graphicBuffer);
              gbm->unlock(dst_graphicBuffer);
              gbm->destory(src_graphicBuffer);
              gbm->destory(dst_graphicBuffer);
            }
        }
      module->destroy(convert);
    }
  printf("======== Dump RTCColorConvertHandle_t end =========\n");
}
char **gs = NULL;
void testRender(RTCRenderHandle_t *render)
{
  if (render == NULL){
   printf("Render api is null.\n");
   return;
  }

  printf("======== Dump RTCRenderHandle_t start =========\n");
  printf("  create:%p\n",render->create);
  printf("  create_from_java:%p\n",render->create_from_java);
  printf("  queue:%p\n",render->queue);
  printf("  dequeue:%p\n",render->dequeue);
  printf("  destory:%p\n",render->destroy);
  printf("======== Dump RTCRenderHandle_t end =========\n");

  int x = atoi(gs[0]);
  int y = atoi(gs[1]);
  int w = atoi(gs[2]);
  int h = atoi(gs[3]);
  RTCRender *surface = render->create(x,y,w,h);
  assert(surface);
  printf("creaet surface %p\n", surface);
    WebRTCAPI &api = *WebRTCAPI::Create();
    OmxGraphics_t *gb = api.GetGraphicsAPI();
  int count = 25;
  printf("Draw %d frames to display...\n", count);
  while(count--){
  graphics_handle *buffer = render->dequeue(surface);
  if (buffer == NULL){
   usleep(1000 * 1000 * 1);
   continue;
  }
  printf("dequeue buffer %p\n", buffer);
  void *data = NULL;
  gb->lock(buffer, IOMX_GRALLOC_USAGE_SW_READ_OFTEN, &data);
  printf("w %d h %d buffer addr %p\n",gb->width(buffer), gb->height(buffer), data);
  // Default is RGB565
  memset(data, rand(), w * h * 2);
  gb->unlock(buffer);
  render->queue(surface, buffer);
  usleep(1000*33);
  }
  printf("End of the draw.\n");
  render->destroy(surface);
//  assert(buffer);
}

class IPCClientCallback: public RTCIPCNode
{
    private:
        std::string _tag;
        RTCIPCNode *_node;
    public:
        IPCClientCallback(const std::string &str, RTCIPCNode *node):_tag(str),_node(node)
    {
      RTCIPCNode::transport= IPCClientCallback::test_ipc_receved;
      RTCIPCNode::died = IPCClientCallback::test_ipc_died;
      RTCIPCNode::set_callback = IPCClientCallback::test_ipc_set_callback;
    }
        ~IPCClientCallback() {printf("~Dtor IPCClientCallback\n");}
        static int test_ipc_receved(RTCIPCNode_t *node, const RTCIPCData_t *p)
        {
           IPCClientCallback *self = static_cast<IPCClientCallback *>(node);
           printf("[%s:%p] send ...\n", self->_tag.c_str(),self);
           return 456;
        }

        static void test_ipc_died(RTCIPCNode_t * node)
        {
           IPCClientCallback *self = static_cast<IPCClientCallback *>(node);
           printf("[%s:%p] died, detached service is crash ...\n", self->_tag.c_str(),self);
        }

        static int test_ipc_set_callback(RTCIPCNode_t *node, RTCIPCNode_t *cb)
        {
           IPCClientCallback *self = static_cast<IPCClientCallback *>(node);
           printf("[%s:%p] set_callback ...\n", self->_tag.c_str(),self);
           return 0;
        }
};

class IPCSericeNode: public RTCIPCNode
{
    private:
        std::string _tag;
        RTCIPCNode *_cb;
    public:
        IPCSericeNode(const std::string &str):_tag(str)
    {
      RTCIPCNode::transport= IPCSericeNode::test_ipc_send;
      RTCIPCNode::died = IPCSericeNode::test_ipc_died;
      RTCIPCNode::set_callback = IPCSericeNode::test_ipc_set_callback;
    }
        ~IPCSericeNode(){printf("~Dtor IPCSericeNode\n");}
        static int test_ipc_send(RTCIPCNode_t *node, const RTCIPCData_t *p)
        {
            WebRTCAPI &api = *WebRTCAPI::Create();
            RTCIPCHandle_t *ipc = api.GetIPCInteraceAPI();
            int size = ipc->parcel.read32(p->data);
            assert(size);
            const char *data = (const char *)ipc->parcel.read_array_pointer(p->data, size);
           IPCSericeNode *self = static_cast<IPCSericeNode *>(node);
           printf("[%s:%p][%s] send with ack ... \n", self->_tag.c_str(),self,data);
           if (self->_cb) {
               RTCIPCData_t ack_data;
               ack_data.data = ipc->parcel.create();
               ack_data.reply = ipc->parcel.create();
            int ret = self->_cb->transport(self->_cb, &ack_data);
            printf("ret = %d\n", ret);
            ipc->parcel.release(ack_data.data);
            ipc->parcel.release(ack_data.reply);
           }
           return 123;
        }

        static void test_ipc_died(RTCIPCNode_t * node)
        {
           IPCSericeNode *self = static_cast<IPCSericeNode *>(node);
           printf("[%s:%p] died with clear memory ...\n", self->_tag.c_str(),self);
           delete self;
        }

        static int test_ipc_set_callback(RTCIPCNode_t *node, RTCIPCNode_t *cb)
        {
           IPCSericeNode *self = static_cast<IPCSericeNode *>(node);
           self->_cb = cb;
           printf("[%s:%p] set_callback [cb:%p]...\n", self->_tag.c_str(),self, cb);
           return 0;
        }
};

static int test_ipc_process(RTCIPCHandleInterface_t *me, const RTCIPCData_t *p)
{
    int ret = 1111;
    WebRTCAPI &api = *WebRTCAPI::Create();
    RTCIPCHandle_t *ipc = api.GetIPCInteraceAPI();
    int size = 0;
    size = ipc->parcel.read32(p->data);
    assert(size);
    const char *data = (const char *)ipc->parcel.read_array_pointer(p->data, size);
  printf("Service: Recv string '%s' private addr %p with ret %d pid %u\n", data, me, ret, (unsigned int)gettid());
  usleep(1000 * 1000 * 3);
  return 1111;
}

static RTCIPCNode_t * test_ipc_create(RTCIPCHandleInterface_t *interface, int type)
{
  printf("Service: create type %d\n", type);
  char buffer[128] = {0};
  sprintf(buffer, "Service[type:%d]", type);
  IPCSericeNode *node = new IPCSericeNode(buffer);
  return node;
}


void testIPC(RTCIPCHandle_t *handle)
{
  printf("======== Dump RTCIPCHandle_t start =========\n");
  printf("  handle:%p\n",handle);
  printf("  start:%p\n",handle->start);
  printf("  stop:%p\n",handle->stop);
  printf("  create:%p\n",handle->create);
  printf("  release:%p\n",handle->release);
  printf("======== Dump RTCIPCHandle_t end =========\n");

  const char *sername = "rtc_module_api_test";
  printf("Start services '%s'...\n", sername);
  RTCIPCHandleInterface_t api;
  api.transport= test_ipc_process;
  api.create = test_ipc_create;
  CHECK(handle->start(sername, &api, false) == 0);
  RTCIPCData_t msg;
  msg.data = handle->parcel.create();
  msg.reply = handle->parcel.create();
  const char *buffer = "Hello, My name is Client.";
  int buffer_len = strlen(buffer) + 1;
  {
      RTCIPCNode_t *node = handle->create(sername, 20151106);
      CHECK(node);
      printf("Client: send string '%s' to Service ..., private addr %p pid %u \n", buffer, handle,(unsigned int)gettid());

      handle->parcel.write_array(msg.data, buffer, buffer_len);
      CHECK(node->transport(node,&msg) == 123);
      printf("Setup callback ...\n");
      IPCClientCallback cb("ClientCallback", node);
      CHECK(node->set_callback(node, &cb) == 0);
      printf("Send 123 ...\n");

      char data2[] = "123";
      handle->parcel.set_position(msg.data, 0);
      handle->parcel.write_array(msg.data, data2, 3);

      CHECK(node->transport(node, &msg) == 123);
      CHECK(handle->release(node) == 0);
      node = handle->create(sername, 20151106);
      CHECK(handle->release(node) == 0);
      node = handle->create(sername, 20151106);
      CHECK(handle->release(node) == 0);
      node = handle->create(sername, 20151106);
      CHECK(handle->release(node) == 0);
      node = handle->create(sername, 20151106);
      CHECK(handle->release(node) == 0);
      node = handle->create(sername, 20151106);
      CHECK(handle->release(node) == 0);
      handle->stop(sername);
      handle->parcel.release(msg.data);
      handle->parcel.release(msg.reply);
  }
  printf("Sleep 1 seconds ...\n");
  usleep(1000 * 1000 * 1);
}

void testParcel (RTCIPCHandle_t *handle)
{
  RTCIPCParcel_t &api = handle->parcel;
  RTCIPCData_t msg;
  msg.data = api.create();
  msg.reply = api.create();
  assert(msg.data);
  assert(msg.reply);

  int v = 2222;
  int v1 = 0;
  assert(api.write32(msg.data, v)==0);
  const char *buffer = "hello. world.";
  int buffer_len = strlen(buffer) + 1;
  api.write_array(msg.data,buffer, buffer_len);
  api.write32(msg.data, 33333);
  printf("data vail %d pos %d\n", api.data_avail_size(msg.data), api.position(msg.data));
  assert(api.set_position(msg.data, 0) == 0);
  v1 = api.read32(msg.data);
  printf("v1 = %d\n", v1);
  assert(v == v1);
  assert(api.read32(msg.data) == buffer_len );
  printf("%s \n", (const char *)api.read_array_pointer(msg.data, buffer_len));
  assert(api.read32(msg.data) == 33333);
}

int testJPEG(RTCJpegCodec_t *hwd)
{
  printf("======== Dump RTCJpegCodec_t start =========\n");
  printf("  create:%p\n",hwd->open);
  printf("  process:%p\n",hwd->process);
  printf("  close:%p\n",hwd->close);
  printf("======== Dump RTCJpegCodec_t end =========\n");

  if (hwd->open == NULL || hwd->process == NULL || hwd->close == NULL) {
   return -1;
  }

    int w = 1920;
    int h = 1080;
    jpeg_buffer in;
    jpeg_buffer out;
    in.fd = 0;
    in.vaddr = new char[w * h * 3 / 2];
    in.offset = 0;
    in.size = w * h * 3 / 2;
    in.len = in.size;
   
    for (int row = 0; row < h; row++) { 
        memset((char *)in.vaddr + row * w, row % 255, w);
    }


    out.fd = 0;
    out.vaddr = new char[w * h * 3 / 2];
    out.offset = 0;
    out.size = w * h * 3 / 2;
    out.len = 0;

    jpeg_codec_parameters params;
    memset(&params, 0, sizeof(params));
    params.w = w;
    params.h = h;
    params.color = kJPEGI420P;
    params.buffer_cnt[kJPEGPortInput] = 1;
    params.buffer_cnt[kJPEGPortOutput] = 1;
    params.buffer[kJPEGPortInput] = &in;
    params.buffer[kJPEGPortOutput] = &out;

    RTCJpegCodecHandle *jpeg = hwd->open(true);
    assert(jpeg);
    {
        SCOPEDCOSTTIME_FUN();
        int ret = hwd->process(jpeg, &params); 
        assert(ret == 0 && out.len > 0);
    }
    hwd->close(jpeg);

    const char *savejpeg = "rtc_module_api_test_jpeg.jpeg";
    printf("Try write Jpeg image '%s' %d bytes\n", savejpeg, out.len);

    FILE *sfile = fopen(savejpeg,"w");
    
    if (sfile) {
     fwrite(out.vaddr, out.len, 1, sfile);
     fclose(sfile);
    }

    delete [] (char *)in.vaddr;
    delete [] (char *)out.vaddr;
  return 0;
}

int main(int c, char **s)
{
    if (c < 4) {
     printf("%s <x> <y> <w> <h>\n",s[0]);
     return 0;
    }
    gs = &s[1];
    WebRTCAPI &api = *WebRTCAPI::Create();
    OMXCore *omx = api.GetOMXCoreAPI();
    OmxGraphics_t *gb = api.GetGraphicsAPI();
    RTCSurfaceHandle_t *sfs = api.GetSurfaceAPI();
    RTCColorConvertHandle_t *colorcvt = api.GetColorConvertAPI();
    RTCRenderHandle_t *render =  api.GetRenderAPI();
    RTCIPCHandle_t *ipc = api.GetIPCInteraceAPI();
    RTCJpegCodec_t *jpeg = api.GetVendorJPEGAPI();
    testParcel(ipc);
    testOMX(*omx);
    testGb(gb);
    testSurface(sfs);
    testColorConvert(colorcvt);
    testRender(render);
    testIPC(ipc);
    testJPEG(jpeg);
    return 0;
}
#endif // #if 0
#define PRINT(exper)  printf("API[%s]: %s = %p \n",__FUNCTION__, #exper, exper)
#define CALL(exper, ret_type)  (printf("%s[L%d] Call %s\n",__FUNCTION__, __LINE__, #exper) && false ) || \
	exper == NULL ? (ret_type)0 : exper

int TestGraphicBufferModule(WebRTCAPI &rtc)
{
	OmxGraphics_t &omx = *rtc.GetGraphicsAPI();
	PRINT(omx.alloc);
	PRINT(omx.destory);
	PRINT(omx.handle);
	PRINT(omx.height);
	PRINT(omx.lock);
	PRINT(omx.lockRect);
	PRINT(omx.lockYCbCr);
	PRINT(omx.pixelFormat);
	PRINT(omx.reallocate);
	PRINT(omx.rect);
	PRINT(omx.stride);
	PRINT(omx.unlock);
	PRINT(omx.usage);
	PRINT(omx.width);
	PRINT(omx.winbuffer);

	graphics_handle *gb = CALL(omx.alloc, graphics_handle *)(1280, 720, 0x11, 0);
	if (gb) {
		DEBUG("gb %p", gb);
		CALL(omx.destory, void)(gb);
	}

	return 0;
}

int TestRenderModule(WebRTCAPI &rtc)
{
	RTCRenderHandle_t &render = *rtc.GetRenderAPI();
	PRINT(render.create);
	PRINT(render.create_from_java);
	PRINT(render.dequeue);
	PRINT(render.destroy);
	PRINT(render.get_display_info);
	PRINT(render.queue);
	return 0;
}

int main(int argv, char *args[])
{
	WebRTCAPI &rtc = *WebRTCAPI::Create();
	TestGraphicBufferModule(rtc);

}
