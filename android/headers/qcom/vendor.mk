MY_PATH := $(call my-dir)

# Convert(C2D hardware acceleration) module support on qcom
LOCAL_C_INCLUDES += \
 $(MY_PATH)/$(MY_BUILD_SDK)/hardware/qcom/media/libc2dcolorconvert \
 $(MY_PATH)/$(MY_BUILD_SDK)/hardware/qcom/display/libcopybit

# HW/SW JPEG module support on qcom
LOCAL_C_INCLUDES += \
 $(MY_PATH)/$(MY_BUILD_SDK)/hardware/qcom/camera/mm-image-codec/qomx_core \
 $(MY_PATH)/$(MY_BUILD_SDK)/hardware/qcom/camera/mm-image-codec/qexif
# Using '../../include/OpenMAX' header as default.
# LOCAL_C_INCLUDES += \
   $(MY_PATH)/$(MY_BUILD_SDK)/hardware/qcom/media/media/mm-core/inc

