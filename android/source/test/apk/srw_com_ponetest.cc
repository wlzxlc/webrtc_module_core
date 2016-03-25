#include <jni.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "WebRTCAPI.h"
#include "bug_helper.h"
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <android/native_window_jni.h>
#ifndef LOG_TAG
#define LOG_TAG "PONE_SURFACETEST"
#endif
using namespace CCStone;
using namespace std;
jint srw_com_ponetest_InitNative(JNIEnv* env, jobject thiz,
        jobject context, jobject local_java_surface)
{
    // local_java_surface 创建方法:
    // SurfaceView view = new SurfaceView
    // SurfaceHold hold = view.getHold()
    // Surface local_java_surface = hold.getSurface()
    // 调用该方法最好在SurfaceCreate中调用，如果非Ui
    // 线程调用需要等到该Surface创建完毕后才调用
    // 当surface被重新create后还需重新调用该方法，
    // native中必须调用标准的ANativeWindow_release方法
    // 释放上一次的surface。
    
    // 确保该surface被创建后调用
    usleep(1000 * 1000 * 5);
    jobject java_surface = local_java_surface;
    WebRTCAPI &api = *WebRTCAPI::Create();
    OmxGraphics_t *gb = api.GetGraphicsAPI();
    RTCRenderHandle_t *render =  api.GetRenderAPI();
    ANativeWindow* win = ANativeWindow_fromSurface(env, java_surface);
    assert(win);
    RTCRender * surface = render->create_from_java((void *)win);
    assert(surface);
    while(1){
        graphics_handle *buffer = render->dequeue(surface);
        if (buffer == NULL){
            usleep(1000 * 1000 * 1);
            continue;
        }
        ALOGD("dequeue buffer %p\n", buffer);
        void *data = NULL;
        gb->lock(buffer, IOMX_GRALLOC_USAGE_SW_READ_OFTEN, &data);
        ALOGD("w %d h %d buffer addr %p\n",gb->width(buffer), gb->height(buffer), data);
        memset(data, rand(), gb->width(buffer) * gb->height(buffer));
        gb->unlock(buffer);
        render->queue(surface, buffer);
        usleep(1000*33);
    }

    while(1){
        ALOGD("%s kedacom", __FUNCTION__);
        usleep(1000* 300);
    }
    return 0;
}

jint srw_com_ponetest_ReleaseNative(JNIEnv* env, jobject thiz)
{
    return 0;
}

//** audio test **//
jint srw_com_ponetest_startAudioRecord(JNIEnv* env, jobject thiz, jint Type)
{
    return 0;
}

jint srw_com_ponetest_stopAudioRecord(JNIEnv* env, jobject thiz)
{
    return 0;
}

// this will block, do it in another thread
jint srw_com_ponetest_PlayAudioFile(JNIEnv* env, jobject thisz,
        jstring file, jstring sizename, jint type)
{
    return 0;

}

jint srw_com_ponetest_setAudioLoop(JNIEnv* env, jobject thiz, jboolean on)
{
    return 0;
}

jint srw_com_ponetest_setAudioAEC(JNIEnv* env, jobject thiz, jboolean on)
{
    return 0;
}

jint srw_com_ponetest_setAudioAGC(JNIEnv* env, jobject thiz, jboolean on)
{
    return 0;
}

jint srw_com_ponetest_setAudioNS(JNIEnv* env, jobject thiz, jboolean on)
{
    return 0;
}

jint srw_com_ponetest_setAudioHPF(JNIEnv* env, jobject thiz, jboolean on)
{
    return 0;
}

//** video test **//
jint srw_com_ponetest_startCapture(JNIEnv* env, jobject thiz, jint index)
{
    return 0;
}

jint srw_com_ponetest_stopCapture(JNIEnv* env, jobject thiz, jint index)
{
    return 0;
}

jint srw_com_ponetest_startRecord(JNIEnv* env, jobject thiz)
{
    return 0;
}

jint srw_com_ponetest_stopRecord(JNIEnv* env, jobject thiz)
{
    return 0;
}

jint srw_com_ponetest_AdjustBitrate(JNIEnv* env, jobject thiz, jint bitrate)
{
    return 0;
}

jint srw_com_ponetest_AdjustFramerate(JNIEnv* env, jobject thiz, jint fps)
{
    return 0;
}

jint srw_com_ponetest_OpenFlashLight(JNIEnv* env, jobject thiz,
        jint index, jboolean on)
{
    return 0;
}

jint srw_com_ponetest_SetCameraParam(JNIEnv* env, jobject thiz,
        jint index, jint function, jint value)
{
    return 0;
}

static JNINativeMethod gMethods[] = {
    {"InitNative","(Ljava/lang/Object;Ljava/lang/Object;)I",(void*)srw_com_ponetest_InitNative},
    {"ReleaseNative","()I",(void*)srw_com_ponetest_ReleaseNative},

    //** audio test **//
    {"StartAudioRecord","(I)I",(void*)srw_com_ponetest_startAudioRecord},
    {"StopAudioRecord","()I",(void*)srw_com_ponetest_stopAudioRecord},

    {"PlayAudioFile","(Ljava/lang/String;Ljava/lang/String;I)I",(void*)srw_com_ponetest_PlayAudioFile},

    {"SetAudioLoop", "(Z)I", (void*)srw_com_ponetest_setAudioLoop},
    {"SetAudioAEC", "(Z)I", (void*)srw_com_ponetest_setAudioAEC},
    {"SetAudioAGC", "(Z)I", (void*)srw_com_ponetest_setAudioAGC},
    {"SetAudioNS", "(Z)I", (void*)srw_com_ponetest_setAudioNS},
    {"SetAudioHPF", "(Z)I", (void*)srw_com_ponetest_setAudioHPF},

    //** video test **//
    {"StartCapture","(I)I",(void*)srw_com_ponetest_startCapture},
    {"StopCapture","(I)I",(void*)srw_com_ponetest_stopCapture},

    {"StartRecord","()I",(void*)srw_com_ponetest_startRecord},
    {"StopRecord","()I",(void*)srw_com_ponetest_stopRecord},

    {"AdjustBitrate","(I)I",(void*)srw_com_ponetest_AdjustBitrate},
    {"AdjustFramerate","(I)I",(void*)srw_com_ponetest_AdjustFramerate},

    {"OpenFlashLight","(IZ)I",(void*)srw_com_ponetest_OpenFlashLight},

    {"SetCameraParam","(III)I",(void*)srw_com_ponetest_SetCameraParam},

};

int register_pone_media_test(JNIEnv* env)
{
    jclass test_class;
    test_class = env->FindClass("srw/com/ponetest/PoneTest");
    return env->RegisterNatives(test_class, gMethods, 18);
}
