# ----------------------------------------------------
# Auto genarate templements
# Author : lichao@keacom.com
# Time   :Thu Mar 12 18:55:18 CST 2015
# ----------------------------------------------------
# Always to point a absolute path of the this make.mk
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
 
###################Module 'android_codec' begain####################
include $(CLEAR_VARS)
# Declare module name
LOCAL_MODULE := kedacom_icamera_test 
# LOCAL_MODULE_FILENAME :=

LOCAL_SRC_FILES := ./test.cpp \
					./camera_parameters.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -Werror -Wno-multichar \
				-Wno-unused-local-typedefs \
				-Wno-delete-non-virtual-dtor
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -pie -fPIE
#LOCAL_LDLIBS := 

# Append any include's paths.
LOCAL_C_INCLUDES := $(LOCAL_PATH) \

#LOCAL_SHARED_LIBRARIES := capture_codec

# If need to them, the comments can be removed
# LOCAL_CPP_EXTENSION :=
LOCAL_DEPS_MODULES := rtc_module_api
LOCAL_STATIC_LIBRARIES := rtc_module_api

#LOCAL_WHOLE_STATIC_LIBRARIES := 
 LOCAL_EXPORT_CFLAGS := $(LOCAL_CFLAGS)
# LOCAL_EXPORT_CPPFLAGS :=
# LOCAL_EXPORT_LDFLAGS :=
# LOCAL_EXPORT_C_INCLUDES := 
ifdef TARGET_RELEASE_DIR
# LOCAL_RELEASE_PATH := $(TARGET_RELEASE_DIR)/....
endif
include $(BUILD_TEST)
###################Module 'android_codec' end####################
