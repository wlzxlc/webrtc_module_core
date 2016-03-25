# ----------------------------------------------------
# Auto genarate templements
# Author : lichao@kedacom.com
# Time   :Fri Mar 25 14:15:18 CST 2016
# ----------------------------------------------------
# Always to point an absolute path of the this make.mk
 LOCAL_PATH := $(call my-dir)

# These variables by script auto genarate. In order 
# to reduce the current make writing burden. if you 
# are have any question and to see:
# $(sysbuild_root)/docs/A&Q.txt. 
# About to usage of the this make.mk, you are can 
# to see :
# $(sysbuild_root)/docs/make_mk.txt
 
 
# Include others make.mk
# $(call include-makefiles, /foo/make.mk /boo/make.mk)
 

include $(CLEAR_VARS)
# Declare module name
LOCAL_MODULE := rtc_ipc_render_client_test

LOCAL_SRC_FILES := render/client_main.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC -UNDEBUG
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -pie -fPIE
ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif

# Append any include's paths.
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/render

LOCAL_STATIC_LIBRARIES := librtc_module_api
include $(BUILD_TEST)

include $(CLEAR_VARS)
LOCAL_MODULE := rtc_ipc_render_service_test

LOCAL_SRC_FILES := render/service_main.cpp \
	               render/MediaSourceService.cpp \
				   render/ISource.cpp  \
				   render/IServiceNode.cpp \
				   cap_test/camera_parameters.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC -UNDEBUG
LOCAL_LDFLAGS := -pie -fPIE

ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif

# Append any include's paths.
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/render

LOCAL_STATIC_LIBRARIES := librtc_module_api
include $(BUILD_TEST)


include $(CLEAR_VARS)
LOCAL_MODULE := surface_test

LOCAL_SRC_FILES := render/apk/render_jni.cpp

LOCAL_STATIC_LIBRARIES := librtc_module_api

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC -UNDEBUG
LOCAL_CPPFLAGS :=
#LOCAL_LDFLAGS := -pie -fPIE
ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif
LOCAL_LDLIBS += -llog -landroid

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
   	$(LOCAL_PATH)/render \
   	$(LOCAL_PATH)/render/apk

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := qcom_jpeg_test

LOCAL_SRC_FILES := qcom_jpeg_test.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC -UNDEBUG
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -pie -fPIE
ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH) 

LOCAL_STATIC_LIBRARIES := librtc_module_api

# Ingore module
#include $(BUILD_TEST)

include $(CLEAR_VARS)
LOCAL_MODULE := rtc_gl_surface_test

LOCAL_SRC_FILES := gl/surface_test2.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC -UNDEBUG -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -pie -fPIE 
ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif

LOCAL_LDLIBS += -lGLESv2 -lGLESv1_CM -lEGL

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	$(LOCAL_PATH)/render

LOCAL_STATIC_LIBRARIES := librtc_module_api
include $(BUILD_TEST)

include $(CLEAR_VARS)
LOCAL_MODULE := rtc_gl_overlay_test

LOCAL_SRC_FILES := gl/overlay.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -DDEBUG_RTC \
	            -UNDEBUG \
				-DEGL_EGLEXT_PROTOTYPES \
				-DGL_GLEXT_PROTOTYPES

LOCAL_LDFLAGS := -pie -fPIE 
ifeq (arm64,$(TARGET_ARCH))
ifdef APP_TOOLCHAIN_SYSROOT
LOCAL_LDLIBS := $(APP_TOOLCHAIN_SYSROOT)/usr/lib/libc.a
else
LOCAL_LDLIBS := $(TOOLCHAIN_ROOT)/../sysroot/usr/lib/libc.a
endif
endif

LOCAL_LDLIBS += -lGLESv2 -lGLESv1_CM -lEGL

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
   	$(LOCAL_PATH)/render

LOCAL_STATIC_LIBRARIES := librtc_module_api
include $(BUILD_TEST)

include $(call all-subdir-makefiles)
