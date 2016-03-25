#ifndef _VIDEOBUFFER_H_
#define _VIDEOBUFFER_H_
#include "graphics_type.h"

typedef struct OmxGraphics_{
  graphics_handle *(*alloc)(int w, int h, graphics_pixformat pixfmt, int usage);
  void      (*destory)(graphics_handle *);
  uint32_t  (*width)(graphics_handle *);
  uint32_t  (*height)(graphics_handle *);
  uint32_t  (*stride)(graphics_handle *);
  uint32_t  (*usage)(graphics_handle *);
  int       (*pixelFormat)(graphics_handle *);
  ARect      (*rect)(graphics_handle *);
  graphics_native_handle (*handle)(graphics_handle *, int *size);
  void *(*winbuffer)(graphics_handle *);
  int (*reallocate)(graphics_handle *, uint32_t w, uint32_t h, graphics_pixformat f, uint32_t usage);
  int (*lock)(graphics_handle *, uint32_t usage, void** vaddr);
  int (*lockRect)(graphics_handle *, uint32_t usage, const ARect& rect, void** vaddr);
  int (*unlock)(graphics_handle *);
  int (*lockYCbCr)(graphics_handle *, uint32_t usage, graphics_ycbcr_t *ycbcr);
  int (*lockYCbCrRect)(graphics_handle *, uint32_t usage, const ARect& rect, graphics_ycbcr_t *ycbcr);
}OmxGraphics_t;

#ifdef __cplusplus
extern "C" {
#endif
 int InitVideoBuffer(OmxGraphics_t *gps, void *handle);
#ifdef __cplusplus
}
#endif

#endif
