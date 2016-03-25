#include "WebRTCAPI.h"
#include "bug_helper.h"
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "render/Defines.h"

#ifndef CHECK
#define CHECK assert 
#endif
#define MAX_BUFFER_CNT 24

////////////////////////////////////////////////

#define QOMX_IMAGE_EXT_EXIF_NAME                  "OMX.QCOM.image.exttype.exif"
#define QOMX_IMAGE_EXT_THUMBNAIL_NAME        "OMX.QCOM.image.exttype.thumbnail"
#define QOMX_IMAGE_EXT_BUFFER_OFFSET_NAME "OMX.QCOM.image.exttype.bufferOffset"
#define QOMX_IMAGE_EXT_MOBICAT_NAME            "OMX.QCOM.image.exttype.mobicat"
#define QOMX_IMAGE_EXT_ENCODING_MODE_NAME        "OMX.QCOM.image.encoding.mode"
#define QOMX_IMAGE_EXT_WORK_BUFFER_NAME      "OMX.QCOM.image.exttype.workbuffer"
#define QOMX_IMAGE_EXT_METADATA_NAME      "OMX.QCOM.image.exttype.metadata"
#define QOMX_IMAGE_EXT_META_ENC_KEY_NAME      "OMX.QCOM.image.exttype.metaEncKey"
#define QOMX_IMAGE_EXT_MEM_OPS_NAME      "OMX.QCOM.image.exttype.mem_ops"
#define QOMX_IMAGE_EXT_JPEG_SPEED_NAME      "OMX.QCOM.image.exttype.jpeg.speed"

///////////////////////////////////////////////
using namespace CCStone;
using namespace std;

template<class T>
static void InitOMXParams(T *params) {
    memset(params, 0, sizeof(T));
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 0;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

typedef struct {
    OMX_U32 yOffset;
    OMX_U32 cbcrOffset[2];
    OMX_U32 cbcrStartOffset[2];
} QOMX_YUV_FRAME_INFO;

typedef struct {
    OMX_U32 fd;
    OMX_U32 offset;
} QOMX_BUFFER_INFO;

struct jpeg_name {
 const char *enc;
 const char *dec;
};

static jpeg_name comp_name = {
 "OMX.qcom.image.jpeg.encoder",
 "OMX.qcom.image.jpeg.decoder"
};

enum {
 kInputPort = 0,
 kOutputPort,
 kInputTmbPort,
 kMAXPort
};

typedef enum QOMX_IMG_COLOR_FORMATTYPE {
    OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar = OMX_COLOR_FormatVendorStartUnused + 0x300,
    OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar,
    OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar_h1v2,
    OMX_QCOM_IMG_COLOR_FormatYUV422SemiPlanar_h1v2,
    OMX_QCOM_IMG_COLOR_FormatYVU444SemiPlanar,
    OMX_QCOM_IMG_COLOR_FormatYUV444SemiPlanar,
    eMX_QCOM_IMG_COLOR_FormatYVU420Planar,
    OMX_QCOM_IMG_COLOR_FormatYVU422Planar,
    OMX_QCOM_IMG_COLOR_FormatYVU422Planar_h1v2,
    OMX_QCOM_IMG_COLOR_FormatYUV422Planar_h1v2,
    OMX_QCOM_IMG_COLOR_FormatYVU444Planar,
    OMX_QCOM_IMG_COLOR_FormatYUV444Planar
} QOMX_IMG_COLOR_FORMATTYPE;

typedef enum {
      OMX_Serial_Encoding,
        OMX_Parallel_Encoding
} QOMX_ENCODING_MODE;

typedef enum {
      QOMX_JPEG_SPEED_MODE_NORMAL,
        QOMX_JPEG_SPEED_MODE_HIGH
} QOMX_JPEG_SPEED_MODE;

typedef struct {
    void *handle;
    void *mem_hdl;
    int8_t isheap;
    size_t size; /*input*/
    void *vaddr;
    int fd;
} omx_jpeg_ouput_buf_t;

/** QOMX_MEM_OPS
 * * Structure holding the function pointers to
 * * buffer memory operations
 * * @get_memory - function to allocate buffer memory
 * **/
typedef struct {
    int (*get_memory)( omx_jpeg_ouput_buf_t *p_out_buf);
} QOMX_MEM_OPS;

/** QOMX_JPEG_SPEED
 * * Structure used to set the jpeg speed mode
 * * parameter
 * * @speedMode - jpeg speed mode
 * **/
typedef struct {
      QOMX_JPEG_SPEED_MODE speedMode;
} QOMX_JPEG_SPEED;


typedef struct jpeg_port_param_{
    int w;
    int h;
    int stride;
    int slice_height;
    int color;
    int buffer_size;
    int buffer_cnt_actual;
    struct {
       int rotate;
    }out;
}jpeg_port_param_t;

typedef struct jpeg_mem_buffer_ {
 OMX_U8 *vaddr;
 int fd;
 int offset;
 graphics_handle *gb;
}jpeg_mem_buffer_t;

typedef struct jpeg_param_{
 Mutex *lock;
 Condition *cond;
 void *hwd;
 OMXCore *omx;
 OmxGraphics_t *gops;
 OMX_PARAM_PORTDEFINITIONTYPE port_def[kMAXPort];

 OMX_BUFFERHEADERTYPE *buf[kMAXPort][MAX_BUFFER_CNT];
 jpeg_mem_buffer_t lbuffer[kMAXPort][MAX_BUFFER_CNT];

 jpeg_port_param_t user_port_param[kMAXPort];
 bool enableTmdPort;
 OMX_STATETYPE stat;
 bool stat_async;
}jpeg_param_t;

class ScopedDebug {
 const char *name;
 int line;
 public:
    ScopedDebug(const char *n, int line) {
        name = n;
        this->line = line;
     DEBUG("ScopedDebug: - > %s[%d]", n, line);
    }
    ~ScopedDebug() {
     DEBUG("ScopedDebug: < - %s[%d]", name, line);
    }
};

#define SCODEDEBUG() ScopedDebug ___scoped_debug(__FUNCTION__, __LINE__)

class OMXCallBack : public IOMXCallBack{
    private:
        void OnCmdCompleted(jpeg_param_t *ctx, OMX_COMMANDTYPE cmd, OMX_U32 data)
        {
            DEBUG("Command done cmd %d data %d", (int)cmd, (int)data);
            switch(cmd) {
                case OMX_CommandStateSet:
                    ctx->stat = (OMX_STATETYPE)data;
                    break;
                default:
                    break;
            }
        }

        virtual OMX_ERRORTYPE EventHandler(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_EVENTTYPE eEvent,
                OMX_U32 nData1,
                OMX_U32 nData2,
                OMX_PTR pEventData)
        {
            DEBUG("");
          jpeg_param_t *ctx = (jpeg_param_t *)pAppData;

          if (eEvent == OMX_EventError) {
              DEBUG("Component error nData1 %d nData2 %d",nData1, nData2);
              return OMX_ErrorNone;
           assert(!"Component error.");
          }else if (eEvent == OMX_EventCmdComplete) {
            OnCmdCompleted(ctx, (OMX_COMMANDTYPE)nData1, nData2);
          }

            return OMX_ErrorNone;
        }

        virtual OMX_ERRORTYPE EmptyBufferDone(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_BUFFERHEADERTYPE* pBuffer)
        {
            DEBUG("");
            return OMX_ErrorNone;
        }

        virtual OMX_ERRORTYPE FillBufferDone(
                OMX_HANDLETYPE hComponent,
                OMX_PTR pAppData,
                OMX_BUFFERHEADERTYPE* pBuffer)
        {
            DEBUG(" addr %p size %d offset %d",
                   pBuffer->pBuffer,(int) pBuffer->nFilledLen,(int) pBuffer->nOffset); 
        FILE *jpeg = fopen("720p.jpeg", "w");
        assert(jpeg);
        fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, jpeg);
        fclose(jpeg);return OMX_ErrorNone;
        }
        virtual ~OMXCallBack(){}
};

void DumpUserJpegPortParame(jpeg_port_param_t *p) {
  DEBUG("w %d", p->w); 
  DEBUG("h %d", p->h); 
  DEBUG("stride %d", p->stride); 
  DEBUG("slice_height %d", p->slice_height); 
  DEBUG("buffer_size %d", p->buffer_size); 
  DEBUG("buffer_cnt_actual %d", p->buffer_cnt_actual); 
  DEBUG("color %d", p->color); 
}

int EncoderStatChangeCondition(jpeg_param_t *p) {
    DEBUG("");
    return 0;
}

int EncoderSetupOffset(jpeg_param_t *ctx)
{
    OMX_ERRORTYPE rc ;
    OMX_INDEXTYPE buffer_index;
    QOMX_YUV_FRAME_INFO frame_info;
    int port = kInputPort;
    memset(&frame_info, 0x0, sizeof(QOMX_YUV_FRAME_INFO));

    int w = ctx->port_def[port].format.image.nFrameWidth;
    int h = ctx->port_def[port].format.image.nFrameHeight;

    frame_info.cbcrStartOffset[0] = w * h;
    frame_info.cbcrStartOffset[1] = w * h;
    frame_info.yOffset = 0;
    frame_info.cbcrOffset[0] = 0;
    frame_info.cbcrOffset[1] = 0;

    rc = OMX_GetExtensionIndex(ctx->hwd,
            (OMX_STRING)QOMX_IMAGE_EXT_BUFFER_OFFSET_NAME, &buffer_index);
    if (rc != OMX_ErrorNone) {
        DEBUG("GetExtensionIndex Failed ret %d", (int)rc);
        return ERRNUM("");
    }

    DEBUG("%s:%d] yOffset = %d, cbcrOffset = (%d %d),"
            "cbcrStartOffset = (%d %d)", __func__, __LINE__,
            (int)frame_info.yOffset,
            (int)frame_info.cbcrOffset[0],
            (int)frame_info.cbcrOffset[1],
            (int)frame_info.cbcrStartOffset[0],
            (int)frame_info.cbcrStartOffset[1]);

    rc = OMX_SetParameter(ctx->hwd, buffer_index, &frame_info);
    if (rc != OMX_ErrorNone) {
        DEBUG("SetParameter Failed ret %d", (int)rc);
        return ERRNUM("");;
    }
    return 0;
}

int EncoderSetupSpeedMode(jpeg_param_t *ctx)
{
    DEBUG("");
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    OMX_INDEXTYPE indextype;
    QOMX_JPEG_SPEED jpeg_speed;

    rc = OMX_GetExtensionIndex(ctx->hwd,
           (OMX_STRING) QOMX_IMAGE_EXT_JPEG_SPEED_NAME, &indextype);
    if (rc != OMX_ErrorNone) {
        DEBUG("GetExtensionIndex '%s' failed.", QOMX_IMAGE_EXT_JPEG_SPEED_NAME);
        return ERRNUM("");;
    }

    jpeg_speed.speedMode = QOMX_JPEG_SPEED_MODE_HIGH;

    rc = OMX_SetParameter(ctx->hwd, indextype, &jpeg_speed);
    if (rc != OMX_ErrorNone) {
        DEBUG("SetParameter failed ret %d", rc);
        return ERRNUM("");
    }
    return 0;
}

int EncoderSetupMode(jpeg_param_t *ctx)
{
    DEBUG("");
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    OMX_INDEXTYPE indextype;
    QOMX_ENCODING_MODE encoding_mode;

    rc = OMX_GetExtensionIndex(ctx->hwd,
           (OMX_STRING) QOMX_IMAGE_EXT_ENCODING_MODE_NAME, &indextype);
    if (rc != OMX_ErrorNone) {
        DEBUG("GetExtensionIndex '%s' failed.", QOMX_IMAGE_EXT_ENCODING_MODE_NAME);
        return ERRNUM("");
    }

    encoding_mode = OMX_Serial_Encoding;
    DEBUG("encoding mode = %d ", (int)encoding_mode);
    rc = OMX_SetParameter(ctx->hwd, indextype, &encoding_mode);
    if (rc != OMX_ErrorNone) {
        DEBUG("SetParameter faild. ret %d", (int)rc);
        return ERRNUM("");
    }
    return 0;
}

int GetGrallocBuffer(jpeg_param_t *ctx, int port,int idx, bool enablegb) 
{
    SCODEDEBUG();
    jpeg_mem_buffer_t * buffer = &ctx->lbuffer[port][idx];
    memset(buffer, 0, sizeof(*buffer));
    int w = ctx->port_def[port].format.image.nFrameWidth;
    int h = ctx->port_def[port].format.image.nFrameHeight;
    if (enablegb) {
        DEBUG("Alloc buffer on GraphicBuffer port %d idx %d w %d h %d",
                port, idx, w, h);
        assert(ctx->gops->alloc);
        buffer->gb = ctx->gops->alloc(w, h, IOMX_HAL_PIXEL_FORMAT_YCrCb_420_SP,
                IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN | IOMX_GRALLOC_USAGE_SW_READ_OFTEN);
        assert(buffer->gb);
        struct MockNativeHandle {
            int version;
            int numFds;
            int numInts;
            int data[0];
        };
        int size = 0;
        struct MockNativeHandle * handle = (struct MockNativeHandle *)(
                ctx->gops->handle(buffer->gb, &size));
        assert(handle->data[0]);
        buffer->fd = handle->data[0];
        ctx->gops->lock(buffer->gb, IOMX_GRALLOC_USAGE_SW_READ_OFTEN, (void **)&buffer->vaddr);
        ctx->gops->unlock(buffer->gb);
    }else {
        DEBUG("Alloc buffer on Heap port %d idx %d w %d h %d",
                port, idx, w, h);
        buffer->vaddr =(OMX_U8 *) malloc(ctx->port_def[port].nBufferSize);
    }
    return buffer->vaddr ?  0: ERRNUM("");
}

int EncoderAllocBuffer(jpeg_param_t *ctx, int port)
{
    DEBUG("AllocBuffer on %d", port);
    int bufsize = ctx->port_def[port].nBufferSize;
    int bufcnt = ctx->port_def[port].nBufferCountActual;
    for (int i = 0;i < bufcnt; i++) {
        assert(GetGrallocBuffer(ctx, port, i, false) == 0);
        OMX_U8 *buffer = (OMX_U8 *)(ctx->lbuffer[port][i].vaddr);
        // Output Used Heap
        if (  port == kOutputPort) {
            int ret = OMX_UseBuffer(ctx->hwd, &ctx->buf[port][i], port, NULL, bufsize, buffer);
            DEBUG("#%d AllocBuffer index %d size %d addr %p %s[%d]", port, i, bufsize, buffer, ret ? "Error..." : "OK...", ret);
            CHECK(ret == 0 );
            CHECK(ctx->buf[port][i]->pBuffer);
        }else {
            // Input used ION
         QOMX_BUFFER_INFO info;
         memset(&info, 0, sizeof(info));
         info.fd = ctx->lbuffer[port][i].fd;
         //assert(info.fd);
         int ret = OMX_UseBuffer(ctx->hwd, &ctx->buf[port][i], port,  info.fd ? &info : NULL, bufsize, buffer);
         DEBUG("#%d AllocBuffer index %d size %d addr %p %s[%d]", port, i, bufsize, buffer, ret ? "Error..." : "OK...", ret);
         CHECK(ret == 0 );
         CHECK(ctx->buf[port][i]->pBuffer);
        }
    }
    return 0;
}

int EncoderFillBuffer(jpeg_param_t *p, int port)
{
    if (port == kInputPort) {
     for (int i = 0; i< MAX_BUFFER_CNT; i++) {
       if (p->buf[port][i] == NULL) {
         break;
        }
       FILE *file = fopen("nv12_720p.yuv", "r");
       assert(file);
       fread(p->buf[port][i]->pBuffer, 1, p->buf[port][i]->nAllocLen, file);
       fclose(file);
       p->buf[port][i]->nFilledLen = p->buf[port][i]->nAllocLen;
       p->buf[port][i]->nTimeStamp = 0;
       DEBUG("#%d Submit buffer addr %p", port, p->buf[port][i]);
       assert(OMX_EmptyThisBuffer(p->hwd, p->buf[port][i]) == 0 );
     }
    }else {
        for (int i = 0; i< MAX_BUFFER_CNT; i++) {
            if (p->buf[port][i] == NULL) {
                break;
            }
            p->buf[port][i]->nFilledLen = 0;
            DEBUG("#%d Submit buffer addr %p", port, p->buf[port][i]);
            assert(OMX_FillThisBuffer(p->hwd, p->buf[port][i]) == 0 );
        }
    }
    return 0;
}

int EncoderAllocBuffer(jpeg_param_t *p)
{
  int ret = EncoderAllocBuffer(p, kInputPort);

  if (ret == 0 && p->enableTmdPort) {
   ret = EncoderAllocBuffer(p, kInputTmbPort);
  }

  if (ret) {
   DEBUG("Alloc buffer error ret %d", ret);
   return ret;
  }

  if (ret == 0){
   ret = EncoderAllocBuffer(p, kOutputPort);
  }
  return ret;
}

static int sGetMemory( omx_jpeg_ouput_buf_t *)
{
    DEBUG("");
    return 0;
}

int EncoderSetupOps(jpeg_param_t *ctx)
{
    DEBUG("");
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    OMX_INDEXTYPE indextype;
    QOMX_MEM_OPS mem_ops;

    mem_ops.get_memory = sGetMemory;

    rc = OMX_GetExtensionIndex(ctx->hwd,
           (OMX_STRING) QOMX_IMAGE_EXT_MEM_OPS_NAME, &indextype);
    if (rc != OMX_ErrorNone) {
        DEBUG("GetExtensionIndex '%s' failed.", QOMX_IMAGE_EXT_MEM_OPS_NAME);
        return ERRNUM("");
    }

    rc = OMX_SetParameter(ctx->hwd, indextype, &mem_ops);
    if (rc != OMX_ErrorNone) {
        DEBUG("SetParameter faild. ret %d", (int)rc);
        return ERRNUM("");;;
    }
    return 0;
}

int EncoderSetStat(jpeg_param_t *p, OMX_STATETYPE new_stat, int (*fun)(jpeg_param_t *)) {
 OMX_STATETYPE curr;
 OMX_ERRORTYPE ret = OMX_GetState(p->hwd, &curr);
 if (ret) {
  return ERRNUM("");
 }

 if (curr != new_stat) {
     DEBUG("Change stat %d -> %d", curr, new_stat);
     if(OMX_SendCommand(p->hwd, OMX_CommandStateSet,
                 new_stat, NULL) ) {
      return ERRNUM("");
     }
 }else {
     p->stat = curr;
 }

 if (fun) {
     if (int ret = fun(p)) {
         DEBUG("Call EncoderStatChangeCondition faild. ret %d", ret);
         return ERRNUM("");
     }
 }

 DEBUG("Wait stat %d", new_stat);

 while (p->stat != new_stat) {
     usleep(1000 * 100);
 }
 return 0;
}

int EncoderPortConfig (jpeg_param_t *args) {

    for (int i = 0; i < kMAXPort; i++) {
      InitOMXParams(&args->port_def[i]);
      int ret = OMX_GetParameter(args->hwd, OMX_IndexParamPortDefinition, &args->port_def[i]);

      if (ret) {
       DEBUG("OMX_GetParameter error idx %d\n", i);
       return ERRNUM("");
      }
          args->port_def[i].nPortIndex = i;
          args->port_def[i].nBufferSize = args->user_port_param[i].buffer_size;
          args->port_def[i].nBufferCountActual = args->user_port_param[i].buffer_cnt_actual;

      if (i != kOutputPort ) {

          if (i == kInputTmbPort && args->enableTmdPort == false) {
            DEBUG("Disable Tmd port");
            ret = OMX_SendCommand(args->hwd, OMX_CommandPortDisable, i, NULL);
            if (ret) {
                DEBUG("Disable Tmd port error ret %d", ret);
                return ERRNUM("");
            }
            continue;
          }

          args->port_def[i].format.image.nFrameWidth = args->user_port_param[i].w;
          args->port_def[i].format.image.nFrameHeight = args->user_port_param[i].h;
          args->port_def[i].format.image.nStride = args->user_port_param[i].stride;
          args->port_def[i].format.image.nSliceHeight = args->user_port_param[i].slice_height;
          args->port_def[i].format.image.eColorFormat = (OMX_COLOR_FORMATTYPE)args->user_port_param[i].color;

      }

      ret = OMX_SetParameter(args->hwd, OMX_IndexParamPortDefinition,
              &args->port_def[i]);

      if (ret) {
          DumpUserJpegPortParame(&args->user_port_param[i]);
          DEBUG("OMX_SetParameter error ret = %d idx %d", ret, i);
          return ERRNUM("");
      }else {
          OMX_PARAM_PORTDEFINITIONTYPE def;
          InitOMXParams(&def);
          def.nPortIndex = i;
          OMX_GetParameter(args->hwd, OMX_IndexParamPortDefinition, &def); 
          DEBUG("Config: idx %d", i);
          DEBUG("w %d h %d color %d bufferSize %d bufferCnt %d", 
                  (int)def.format.image.nFrameWidth ,
                  (int)def.format.image.nFrameHeight,
                  (int)def.format.image.eColorFormat, 
                  (int)def.nBufferSize,
                  (int)def.nBufferCountActual);
      }

      if (i == kInputTmbPort && args->enableTmdPort) {
           // Enable thumbnail port
          ret = OMX_SendCommand(args->hwd, OMX_CommandPortEnable,
                  i, NULL); 
          if (ret) {
           DEBUG("Enable Tmd port error ret %d", ret);
           return ERRNUM("");
          }
      }
    }

    return 0;
}

int EncoderSetupRotate(jpeg_param_t *ctx)
{
    OMX_CONFIG_ROTATIONTYPE rotate;
    InitOMXParams(&rotate);
      rotate.nPortIndex = kOutputPort;
      rotate.nRotation = ctx->user_port_param[kOutputPort].out.rotate;
     int ret =(int) OMX_SetConfig(ctx->hwd, OMX_IndexConfigCommonRotate,
              &rotate);

     if (ret) {
      DEBUG("Config rotate error in out port ret %d", ret);
      return ERRNUM("");
     }
     return 0;
}

void testOMX(jpeg_param_t *ctx)
{
    OMXCore &core = *ctx->omx;
    assert(core.OMX_Init() == 0);
    string targetcomp = comp_name.enc;
    DEBUG("Open component %s", comp_name.enc);
    int ret = core.OMX_GetHandle(&ctx->hwd ,const_cast<char *>(targetcomp.c_str()), ctx, new OMXCallBack);

    if (ret) {
     printf("Get handle error %d\n", ret);
    }
    if (ctx->hwd == 0){
        assert(core.OMX_Deinit() == 0);
        return;
    }

    CHECK(EncoderPortConfig(ctx) == 0);
    //Set offset
    //CHECK(EncoderSetupOffset(ctx) == 0);
    //CHECK(EncoderSetupMode(ctx) == 0);
    //Set encrypt key
    //CHECK(EncoderSetupOps(ctx) == 0);
    //CHECK(EncoderSetupSpeedMode(ctx) == 0);
    CHECK(EncoderSetStat(ctx, OMX_StateLoaded, NULL) == 0);
    CHECK(EncoderSetStat(ctx, OMX_StateIdle, EncoderAllocBuffer) == 0);
    CHECK(EncoderSetStat(ctx, OMX_StateExecuting, NULL) == 0);
    CHECK(EncoderSetupRotate(ctx) == 0);
    EncoderFillBuffer(ctx, kOutputPort);
    EncoderFillBuffer(ctx, kInputPort);
    DEBUG("Test done.");
    while(1) usleep(1000 * 1000);
    //core.OMX_Deinit();
}

int main(int c, char **s) {
  static  OMXCore omx;
    printf("Open '%s'\n", s[1]);
 if(omx.OpenLibs(s[1], NULL)) {
     jpeg_param_t ctx;
     memset(&ctx, 0, sizeof(ctx));

     ctx.omx = &omx;
     ctx.gops = WebRTCAPI::Create()->GetGraphicsAPI();
     assert(ctx.gops);
     ctx.lock = new Mutex;
     ctx.cond = new Condition;
     ctx.user_port_param[kInputPort].w = 1280;
     ctx.user_port_param[kInputPort].h = 720;
     ctx.user_port_param[kInputPort].stride = 1280;
     ctx.user_port_param[kInputPort].slice_height = 720;
     ctx.user_port_param[kInputPort].color = OMX_QCOM_IMG_COLOR_FormatYVU420Planar;
     ctx.user_port_param[kInputPort].buffer_size = 1280 * 720 * 3 / 2;
     ctx.user_port_param[kInputPort].buffer_cnt_actual = 1;

     ctx.enableTmdPort = false;
     ctx.user_port_param[kInputTmbPort].w = 640;
     ctx.user_port_param[kInputTmbPort].h = 480;
     ctx.user_port_param[kInputTmbPort].stride = 640;
     ctx.user_port_param[kInputTmbPort].slice_height = 480;
     ctx.user_port_param[kInputTmbPort].color = OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar;
     //ctx.user_port_param[kInputTmbPort].color = OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar;
     ctx.user_port_param[kInputTmbPort].buffer_size = 640 * 480 * 3 / 2;
     ctx.user_port_param[kInputTmbPort].buffer_cnt_actual = 2;

     // By calculate out buffer size
     ctx.user_port_param[kOutputPort].w = 1280;
     ctx.user_port_param[kOutputPort].h = 720;
     ctx.user_port_param[kOutputPort].buffer_size = 1280 * 720 * 3 / 2;
     ctx.user_port_param[kOutputPort].buffer_cnt_actual = 1;
     ctx.user_port_param[kOutputPort].out.rotate = 0;

     testOMX(&ctx);
     printf("Exit ...\n");
     while(1) usleep(11111);

 } else {
     printf("Open error, %s\n", dlerror());
 }

 return 0;
}
