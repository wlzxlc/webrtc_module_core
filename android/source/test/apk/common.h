#include <android/log.h>
#ifndef MEDIA_TEST_COMMON_H
#define MEDIA_TEST_COMMON_H

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#ifdef ALOGD
#undef ALOGD
#endif
#ifdef ALOGI
#undef ALOGI
#endif
#ifdef ALOGE
#undef ALOGE
#endif
#ifdef ALOGW
#undef ALOGW
#endif
#ifdef ALOGV
#undef ALOGV
#endif

#define LOG_TAG "poneTest"

#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)

#endif // MEDIA_TEST_COMMON_H
