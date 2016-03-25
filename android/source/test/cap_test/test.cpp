/*************************************************************************
  > File Name: test.cpp
  > Author:
  > Mail:
  > Created Time: Wed 13 May 2015 07:36:32 PM CST
 ************************************************************************/
#include <iostream>
#include <unistd.h>
#include <list>
#include <stdlib.h>
#include <getopt.h>
#include "camera_parameters.h"
#include "WebRTCAPI.h"
#include "graphics_type.h"
#include "bug_helper.h"

#define RtcCapApi CCStone::WebRTCAPI::Create()->GetCaptureAPI()
#define RtcSfsApi CCStone::WebRTCAPI::Create()->GetSurfaceAPI()
#define RtcGpbApi CCStone::WebRTCAPI::Create()->GetGraphicsAPI()

#define GBUSAGE IOMX_GRALLOC_USAGE_HW_RENDER | \
  IOMX_GRALLOC_USAGE_HW_VIDEO_ENCODER | \
  IOMX_GRALLOC_USAGE_HW_TEXTURE

#define GBRAW IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN | \
  IOMX_GRALLOC_USAGE_SW_READ_OFTEN

typedef struct Context {
  int id;
  int frame_cnt;
  void *ctx;
  bool can_takepic;
  bool previewed;
}TContext;

static void cameraCb(void *data, uint32_t size, void *context);

RTCSurface *g_surface;

// save yuv frames
static int gs_save_yuv_frame_cnt = 0;
static FILE *gs_fp = NULL;

static std::list<graphics_handle *> g_gbList;
static void SurfaceCallback(RTCSurface *, graphics_handle *graphics, void *) {
  static bool saved = false;
  static int frame_num = 0;
  static int reserve_num = 0;
  DEBUG("RTCSurface Callback: %p\n", graphics);
  DEBUG("Gb width: %d height: %d\n", RtcGpbApi->width(graphics), RtcGpbApi->height(graphics));
  graphics_ycbcr_t yuv;
  int ret = RtcGpbApi->lockYCbCr(graphics, GBRAW, &yuv);
  if (0 == ret)
    DEBUG("Lock Gb yuv.y: %p yuv.cb %p ret: %d\n", yuv.y, yuv.cb, ret);
  else
    DEBUG("Gb lock ret: %d\n", ret);

  frame_num++;

  int w = RtcGpbApi->width(graphics);
  int h = RtcGpbApi->height(graphics);

  if (10 == frame_num && saved == false) {
    int fmt = RtcGpbApi->pixelFormat(graphics);
    printf("Gp fmt: %d\n", fmt);
    FILE *fp = fopen("surface.yuv", "w");
    if (fp) {
      /*
      fwrite(yuv.y, w*h, 1, fp);
      fwrite(yuv.cb, w*h/2, 1, fp);*/
      fwrite(yuv.y, ((w + 127)/128*128)*((h + 31)/32*32)*3/2, 1, fp);
      fclose(fp);
      printf("Gb Save success!\n");
    }
    saved = true;
  }

  if (99 == frame_num && gs_save_yuv_frame_cnt > 0) {
    gs_fp = fopen("yuv_frames.yuv", "w");
  }

  int dummy = 0;
  if (gs_fp) {
    fwrite(yuv.y, (w*h), 1, gs_fp);
    fwrite(yuv.cr, (w*h)/2 - 1, 1, gs_fp);
    fwrite(&dummy, 1, 1, gs_fp);
  }

  if ((gs_save_yuv_frame_cnt > 0) && (frame_num == 99 + gs_save_yuv_frame_cnt) && gs_fp) {
    fclose(gs_fp);
    printf("yuv %d frames save success!\n", gs_save_yuv_frame_cnt);
  }

  g_gbList.insert(g_gbList.end(), graphics);
  ++reserve_num;
  if (reserve_num > 1) {
    reserve_num--;
    DEBUG("reserve_num: %d", reserve_num);
    graphics_handle *ptmp = g_gbList.front();
    RtcGpbApi->destory(ptmp);
    g_gbList.pop_front();
  }
}

using namespace std;
static struct option long_options[] = {
  {"camera_id",       1,    0,  'i'},
  {"width",           1,    0,  'w'},
  {"height",          1,    0,  'h'},
  {"framerate",       1,    0,  'p'},
  {"format",          1,    0,  'f'},
  {"save-frame-num",  1,    0,  'n'},
  {"enable_surface",  0,    0,  'e'},
  {"enable_show",     0,    0,  'E'},
  {"enable recording",0,    0,  'P'},
  {"enable take pic", 1,    0,  'T'},
  {"excute time",     1,    0,  't'}
};

static const char *short_options = "i:w:h:p:f:en:EPt:T:";

void usage() {
  printf("Usage:\n");
  printf("kedacom_icamera_test [options] [parameters] \n"
         "options: \n"
         "-i        camera_id which start idx=0,1,2...\n"
         "-w        video width\n"
         "-h        video height\n"
         "-p        video framerate\n"
         "-f        video format: yuv420sp,nv12,nv12-venus...\n"
         "-n        save video frame num\n"
         "-e        enable surface\n"
         "-E        enalbe show on screen\n"
         "-P        enalbe recording\n"
         "-T        enalbe takePic cycle time\n"
         "-t        excute time(second)\n"
        );
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    usage();
    return 0;
  }

  int camera_id = 0;
  int w = 355;
  int h = 288;
  int fps = -1;
  bool enable_preview = true;
  bool enable_recording = false;
  bool enable_takepic = false;
  bool enable_rtc_surface = false;
  bool enable_show = false;
  char preview_fmt[20] = "yuv420sp";
  int times = -1;
  int takepic_cycle_time = 2;//(s)

  int opt = 0;
  int option_index = 0;
  while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
        case 'i':
            camera_id = atoi(optarg);
            break;
        case 'w':
            w = atoi(optarg);
            break;
        case 'h':
            h = atoi(optarg);
            break;
        case 'p':
            fps = atoi(optarg);
            break;
        case 'n':
            gs_save_yuv_frame_cnt = atoi(optarg);
            break;
        case 'f':
            strcpy(preview_fmt, optarg);
            break;
        case 'e':
            enable_rtc_surface = true;
            break;
        case 'E':
            enable_show = true;
            break;
        case 'P':
            enable_recording = true;
            break;
        case 'T':
            enable_takepic = true;
            takepic_cycle_time = atoi(optarg);
            if (takepic_cycle_time < 1)
              takepic_cycle_time = 1;
            break;
        case 't':
            times = atoi(optarg);
            break;
        }
  }

  if (RtcCapApi->getNum == 0) {
    DEBUG("Init OmxCamera failed\n");
    return -1;
  } else {
    DEBUG("Camera cnt(%d)\n", RtcCapApi->getNum());
  }

  RTCCameraHandle *cameraHndl = RtcCapApi->create(camera_id);
  if (!cameraHndl) {
    DEBUG("Open cameraId failed\n");
    return -1;
  }

  if (enable_rtc_surface) {
    RTCSurface *surface;

    RtcSfsApi->create(&surface, w, h, GBUSAGE, SurfaceCallback, 10, NULL);
    if (0 != RtcCapApi->setRTCSurface(cameraHndl, surface)) {
      DEBUG("Set surface failed\n");
      RtcCapApi->destroy(cameraHndl);
      return -1;
    } else {
      g_surface = surface;
      DEBUG("Set surface success\n");
    }
  } else {
    if (0 != RtcCapApi->setSurface(cameraHndl, w, h, \
                                   3/*IOMX_HAL_PIXEL_FORMAT_RGB_888*/, enable_show ? 0x01 : 0x0)) {
      DEBUG("Set surface failed\n");
      RtcCapApi->destroy(cameraHndl);
      return -1;
    } else {
      DEBUG("Set surface success\n");
    }
  }

  char *szParameter = new char[10 * 1024];
  if (0 != RtcCapApi->getParameters(cameraHndl, szParameter, 10 * 1024)) {
    DEBUG("getParameters failed!\n");
  }

  DEBUG("CameraParam: %s\n", szParameter);
  CameraParameters camParam(szParameter);

  camParam.setPreviewSize(w,h);
  camParam.setPictureSize(w,h); //for takePicture
  camParam.setPictureFormat("yuv420sp");
  if (fps > 0) {
    camParam.setPreviewFpsRange(fps*1000, fps*1000);
    camParam.setPreviewFrameRate(fps);
  } else {
    printf("Waring ********skip configure camera framerate*******\n");
  }
  camParam.setPreviewFormat(preview_fmt);
  camParam.set("zsl", "off");

  camParam.dump();
  if (0 != RtcCapApi->setParameters(cameraHndl, camParam.flatten().c_str())) {
    DEBUG("setParameters failed!\n");
  }
  int prev_w, prev_h;
  camParam.getPreviewSize(&prev_w, &prev_h);
  DEBUG("preview size %dx%d-%dfps", prev_w, prev_h, fps);
  delete szParameter;

  TContext previewCtx = {0,0};
  if (enable_preview) {
    if (0 != RtcCapApi->setPreviewCallback(cameraHndl, cameraCb, &previewCtx)) {
      cout << "Set preview callback failed" << endl;
      RtcCapApi->destroy(cameraHndl);
      return -1;
    }
    if (0 != RtcCapApi->startPreview(cameraHndl)) {
      cout << "Start preview failed" <<endl;
      RtcCapApi->destroy(cameraHndl);
      return -1;
    }
    else {
      cout << "startPreview sucess " << endl;
    }
  }

  TContext recordingCtx = {1,0};
  if (enable_recording) {
    if (0 != RtcCapApi->setRecordingCallback(cameraHndl, cameraCb, &recordingCtx)) {
      cout << "Set preview callback failed" << endl;
      RtcCapApi->destroy(cameraHndl);
      return -1;
    }

    if (0 != RtcCapApi->startRecording(cameraHndl)) {
      cout << "Start recording failed" <<endl;
      RtcCapApi->destroy(cameraHndl);
      return -1;
    }
    else {
      cout << "startRecording sucess " << endl;
    }
  }

  TContext takePicCtx = {2, 0, cameraHndl, true, true};
  if (enable_takepic) {
    if (0 != RtcCapApi->setPictureCallback(cameraHndl, cameraCb, &takePicCtx)) {
      cout << "Set takePicture callback failed" << endl;
      RtcCapApi->destroy(cameraHndl);
      return -1;
    }
  }

  int last_frame_cnt[3] = {0};
  int secs = 0;
  while(times < 0 || ((times > 0) && (secs < times))) {

    if (enable_takepic) {
      if (takePicCtx.can_takepic == true && takePicCtx.previewed == false) {
        if (0 != RtcCapApi->startPreview(cameraHndl)) {
          cout << "Start preview failed" <<endl;
          RtcCapApi->destroy(cameraHndl);
          return -1;
        } else {
          takePicCtx.previewed = true;
        }
      }
      if (takePicCtx.previewed && (secs % takepic_cycle_time == 0) && secs != 0) {
        if (0 != RtcCapApi->takePicture(cameraHndl,
							TAKE_PICTRUE_USAGE_YUV /*| TAKE_PICTRUE_USAGE_RAW */| TAKE_PICTRUE_USAGE_AUTO_FOCUS
                                       | TAKE_PICTRUE_USAGE_ENABLE_SHUTTER)) {
          cout << "Take picture failed" << endl;
        } else {
          cout << "Take picture success" << endl;
          takePicCtx.can_takepic = false;
        }
      }
    }

    sleep(1);
    secs++;
    if (enable_preview) {
      if (last_frame_cnt[0] > 0) {
        cout << "Preview Chn: " << (previewCtx.frame_cnt - last_frame_cnt[0])/secs << "fps" << endl;
      } else
        last_frame_cnt[0] = previewCtx.frame_cnt;
    }

    if (enable_recording) {
      if (last_frame_cnt[1] > 0) {
        cout << "Recording Chn: " << (recordingCtx.frame_cnt - last_frame_cnt[1])/secs << "fps" << endl;
      } else
        last_frame_cnt[1] = recordingCtx.frame_cnt;
    }

    if (enable_takepic) {
      if (last_frame_cnt[2] > 0) {
          cout << "Takepic Chn: " << (takePicCtx.frame_cnt - last_frame_cnt[2])/secs << "fps" << endl;
      } else
        last_frame_cnt[2] = takePicCtx.frame_cnt;
    }
  }
  return 0;
}

static void cameraCb(void *data, uint32_t size, void *context) {
  TContext *pCtx = (TContext*)context;
  pCtx->frame_cnt++;
  DEBUG("FramCb addr: %p size: %d\n", data, size);
  DEBUG("Frame Cb [type: %d cnt: %d]\n", pCtx->id, pCtx->frame_cnt);
  if (pCtx->id == 2) {
    //#define SAVE_JPEG
    #ifdef SAVE_JPEG
    static int s_jpg_idx = 1;
    char szName[20] = {0};
    sprintf(szName, "pic%d.yuv", s_jpg_idx);
    FILE *fp = fopen(szName, "w+");
    if (fp) {
      uint8_t *heapBase = (uint8_t*)heap->base();

      if (heapBase != NULL) {
        const char* data = reinterpret_cast<const char*>(heapBase + offset);
        fwrite(data, size, 1, fp);
      }

      fclose(fp);
    } else {
      DEBUG("open failed\n");
    }
    s_jpg_idx++;
    #endif
    pCtx->can_takepic = true;
    pCtx->previewed = false;
  }
}
