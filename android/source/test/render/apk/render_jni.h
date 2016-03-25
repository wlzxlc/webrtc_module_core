// libsurface_test.so
#ifndef _RENDER_JNI_H_
#define _RENDER_JNI_H_
#include <jni.h>
#ifdef __cplusplus
extern "C" {
#endif
int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1create(JNIEnv *,jobject thiz, jobject holader);
int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1change(JNIEnv *,jobject thiz, jobject holader, int format, int widht, int height);
int Java_srw_com_demo_1camera_1sametime_MainActivity_surface_1destory(JNIEnv *,jobject thiz,jobject holader);
#ifdef __cplusplus
};
#endif

#endif
