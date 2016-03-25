#include <hw_jpeg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <videobuffer.h>
extern "C" {
extern int IAndroid_CreateOmxGraphics(OmxGraphics_t *p);
extern int IAndroid_HWJpeg(RTCJpegCodec_t *);
}

struct test_native_buffer {
    int version;
    int fds;
    int intNums;
    int data[0];
};

static OmxGraphics_t *gapi;

struct test_jpeg_buffer {
 jpeg_buffer buffer;
 void *p;
};

int AllocGbBuffer(int w, int h, test_jpeg_buffer *out)
{
  if (!gapi || !out) {
   return -__LINE__;
  }
  //Using IOMX_GRALLOC_USAGE_SW_WRITE_RARELY flag and the tell libgralloc module
  // don't ion cache.
  graphics_handle *graphicBuffer = gapi->alloc(w, h,
          IOMX_HAL_PIXEL_FORMAT_YCrCb_420_SP,
          IOMX_GRALLOC_USAGE_SW_WRITE_RARELY);

  assert(graphicBuffer);

  int size = 0;
  graphics_native_handle hwd = gapi->handle(graphicBuffer,
          &size);

  const struct test_native_buffer *nhwd =
      (const struct test_native_buffer *)(hwd);

  out->buffer.fd = nhwd->data[0];
  gapi->lock(graphicBuffer,
          IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN, &out->buffer.vaddr);
  assert(out->buffer.vaddr);
  out->p = graphicBuffer;
  return 0;
}

int FreeBuffer(test_jpeg_buffer *in)
{
   if (in && gapi && in->p) {
       graphics_handle *hwd = (graphics_handle *)(in->p);
    gapi->unlock(hwd);
    gapi->destory(hwd);
    in->buffer.fd = 0;
    in->buffer.vaddr = NULL;
    in->p = NULL;
   }
   return 0;
}

int main(int c, char **s)
{
    if (c < 3) {
     printf("%s input_yuv1080_420p out_jpeg", s[0]);
     return 0;
    }
    OmxGraphics_t api;
    assert(IAndroid_CreateOmxGraphics(&api) == 0);
    RTCJpegCodec_t jcodec;
    assert(IAndroid_HWJpeg(&jcodec) == 0 && jcodec.open);
    gapi = &api;
    int w = 1920;
    int h = 1080;
    void *hwd = jcodec.open(true);
    test_jpeg_buffer tin;
    test_jpeg_buffer tout;

    jpeg_buffer &in = tin.buffer;
    jpeg_buffer &out = tout.buffer;

    AllocGbBuffer(w, h, &tin);
    AllocGbBuffer(w, h, &tout);

    //in.fd = 0;
    //in.vaddr = new char[w * h * 3 / 2];
    in.offset = 0;
    in.size = w * h * 3 / 2;
    in.len = in.size;

    //memset(in.vaddr, 0x88, w * h);
    //memset((char *)in.vaddr + w * h, 0x50, w * h / 4);
    //memset((char *)in.vaddr + w * h + w * h / 4, 0x60, w * h / 4);

    FILE *src_file = fopen(s[1], "r");
    assert(src_file);
    fread(in.vaddr, in.size, 1, src_file);
    fclose(src_file);

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

    assert(hwd);

    int ret = jcodec.process(hwd, &params);
    FILE *jpeg_test_file = fopen(s[2], "w");
    assert(jpeg_test_file);
    printf("ret %d outsize %d\n", ret, out.len);
    if (out.len) {
     fwrite(out.vaddr, out.len, 1, jpeg_test_file);
    }
    fclose(jpeg_test_file);
    jcodec.close(hwd);

    FreeBuffer(&tin);
    FreeBuffer(&tout);
    return 0;
}
