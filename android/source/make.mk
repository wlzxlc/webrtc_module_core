# ----------------------------------------------------
# Auto genarate templements
# Author : lichao@keacom.com
# Time   :Tue May 12 16:26:22 CST 2015
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
 
 
include $(CLEAR_VARS)
# Declare module name
LOCAL_MODULE := rtc_module_api
# LOCAL_MODULE_FILENAME :=

LOCAL_SRC_FILES := ./WebRTCAPI.cpp \
                   ./videobuffer.cpp \
                   ./surface.cpp \
				   ./OmxCore.cpp \
				   ./capture.cpp \
				   ./convert.cpp \
				   ./render.cpp \
				   ./ipc.cpp \
				   ./jpeg.cpp

LOCAL_LINK_MODE := c
LOCAL_CFLAGS := -Wno-reorder -Werror

ifdef DEBUG_RTC
 LOCAL_CFLAGS += -DDEBUG_RTC
endif
 
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS :=
LOCAL_LDLIBS :=

# Append any include's paths.
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	                $(LOCAL_PATH)/../include \
	                $(LOCAL_PATH)/../include/openMAX

 LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare module name
LOCAL_MODULE := rtc_module_api_test
# LOCAL_MODULE_FILENAME :=

LOCAL_SRC_FILES := test/test.cpp

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
LOCAL_C_INCLUDES := $(LOCAL_PATH) 

LOCAL_STATIC_LIBRARIES := librtc_module_api
include $(BUILD_EXECUTABLE)

include $(wildcard $(LOCAL_PATH)/*/make.mk) 
