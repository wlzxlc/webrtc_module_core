#ifndef RTC_RENDER_H
#define RTC_RENDER_H
#include <android/native_window.h>
#include "graphics_type.h"

typedef void RTCRender;
typedef struct RTCRenderHandle{
    RTCRender *(*create)(int32_t /*x*/,
            int32_t /*y*/,
            int32_t /*w*/,
            int32_t /*h*/);

    RTCRender *(*create_from_java)(void * /* ANativeWindow */ );

    int (*queue)(RTCRender * /* this */, graphics_handle * /* buff*/);

    graphics_handle * (*dequeue)(RTCRender * /* this */);

    void (*destroy)(RTCRender *);

    int (*get_display_info)(int32_t /*id*/, uint32_t * /*w*/,
                            uint32_t * /*h*/, float * /*fps*/);

}RTCRenderHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
    int InitRTCRender(RTCRenderHandle_t *render, void *handle);
#ifdef __cplusplus
}
#endif
#endif // RTC_RENDER_H
