# ----------------------------------------------------
# Auto genarate templements
# Author : lichao@keacom.com
# Time   :Wed Mar 11 14:44:04 CST 2015
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
#
include $(CLEAR_VARS)
LOCAL_MODULE := kedacom_rtc_qcom_jpeg_test

LOCAL_SRC_FILES := jpeg_test.cpp

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -UNDEBUG

LOCAL_SHARED_LIBRARIES := qcom_omx_21
LOCAL_LDFLAGS += -pie -fPIE

LOCAL_C_INCLUDES := $(call get-iomx-inc-path,qcom,21)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(LOCAL_PATH)/..
include $(BUILD_TEST)
