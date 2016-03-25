#ifndef HW_JPEG_QCOM_H_
#define HW_JPEG_QCOM_H_
#include "graphics_type.h"
#ifdef __cplusplus
extern "C" {
#endif
#define JPEG_MAX_BUFFER 3
#define JPEG_MAX_BUFFER_IN 1
#define JPEG_MAX_BUFFER_OUT 1
#define JPEG_MAX_BUFFER_TMD 1

typedef void RTCJpegCodecHandle;

enum {
 kJPEGPortInput = 0,
 kJPEGPortOutput,
 kJPEGPortThumbnail,
 kJPEGPortMAX
};

struct jpeg_buffer {
    // Buffer file description.
    // (vaddr) or (fd and vaddr) or (gb) must be non-null.
    int fd;
    void *vaddr;
    graphics_handle *gb;
    int offset;
    int len;
    int size;
};

enum {
 kJPEGColorFmtStart = 100,
 kJPEGYUV422,
 kJPEGNV12,
 kJPEGI420P,
 kJPEGRGB565,
 kJPEGColorFmtMAX
};

typedef struct jpeg_codec_parameters {
    int w;
    int h;
    int thumbnail_w;
    int thumbnail_h;
    int color;
    int thumbnail_color;
    struct jpeg_buffer *buffer[kJPEGPortMAX];
    int buffer_cnt[kJPEGPortMAX];
}jpeg_codec_parameters;

typedef struct RTCJpegCodec_{
    // Create a hw jpec codec on qcom.
    RTCJpegCodecHandle * (*open)(bool encoder);
    // Encode/decode a frame image.
    int (*process)(RTCJpegCodecHandle *, jpeg_codec_parameters *);
    // Close codec
    int (*close)(RTCJpegCodecHandle *);
}RTCJpegCodec_t;

int InitVendorJpegCodec(RTCJpegCodec_t *, void *);

#ifdef __cplusplus
}
#endif

#endif
