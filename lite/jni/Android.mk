LOCAL_PATH := $(call my-dir)
BUILD_API_VERSION := 14

include $(CLEAR_VARS)
LOCAL_MODULE := libsnail_graphic

LOCAL_SRC_FILES := \
	jni4android/gb/src/GraphicOnNative.cpp \
	jni4android/gb/src/j4a_GraphicOnNative.c \
	jni4android/gb/src/j4a_Parcel.c \
	jni4android/gb/src/j4a_SystemGraphicBuffer.c \
	jni4android/src/j4a_base.c \
    ../../android/source/videobuffer.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/jni4android \
	$(LOCAL_PATH)/jni4android/gb/ \
	$(LOCAL_PATH)/jni4android/gb/src \
	$(LOCAL_PATH)/../../android/include

LOCAL_LDLIBS := -llog -lGLESv2 -lEGL -landroid
LOCAL_CFLAGS := -DEGL_EGLEXT_PROTOTYPES

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := snail_graph_test

LOCAL_SRC_FILES := \
	../../android/source/videobuffer.cpp \
	jni4android/gb/src/nativeTest.cpp
	

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/jni4android \
	$(LOCAL_PATH)/jni4android/gb/ \
	$(LOCAL_PATH)/jni4android/gb/src \
	$(LOCAL_PATH)/../../android/include

LOCAL_LDLIBS := -llog 
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_CFLASG += -fPIE -pie
include $(BUILD_EXECUTABLE)

include ../android/source/core/symbols/make.mk
include ../android/source/core/GraphicBuffer.mk
