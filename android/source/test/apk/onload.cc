#include <android/log.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include <jni.h>

int register_pone_media_test(JNIEnv* env);

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }

    register_pone_media_test(env);

    return JNI_VERSION_1_4;
}
