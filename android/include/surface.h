#ifndef _RTCSURFACEHANDLE_H_
#define _RTCSURFACEHANDLE_H_
#include "graphics_type.h"

typedef void RTCSurface;

typedef void (*RTCSurfaceCallback)(RTCSurface *,
                                   graphics_handle *graphics,
                                   void *priv_data);

typedef struct RTCSurfaceHandle{
    int (*create)(RTCSurface **sfs, int w, int h, uint32_t usage,
                  RTCSurfaceCallback , int buff_counts, void *priv_data);
    int (*destroy)(RTCSurface *sfs);
    int (*window)(RTCSurface *sfs, void **wind);
    int (*releaseBuffer)(RTCSurface *, graphics_handle *);   
}RTCSurfaceHandle_t;

#ifdef __cplusplus
extern "C" {
#endif
    int InitSurface(RTCSurfaceHandle_t *, void *handle);
#ifdef __cplusplus
}
#endif
#endif
