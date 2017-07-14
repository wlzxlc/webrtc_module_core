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

LIBIOMX_INCLUDES_COMMON := $(LOCAL_PATH)/../../include/openMAX \
	                       $(LOCAL_PATH)/../../include

ANDROID_SYS_HEADERS := $(LOCAL_PATH)/../../headers

LIBIOMX_INCLUDES_10 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/10 \
	$(ANDROID_SYS_HEADERS)/10/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/10/system/core/include \
	$(ANDROID_SYS_HEADERS)/10/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/10/frameworks/base/include/media/stagefright

LIBIOMX_INCLUDES_13 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/13 \
	$(ANDROID_SYS_HEADERS)/13/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/13/frameworks/base/native/include \
	$(ANDROID_SYS_HEADERS)/13/system/core/include \
	$(ANDROID_SYS_HEADERS)/13/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/13/frameworks/base/include/media/stagefright

LIBIOMX_INCLUDES_14 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/14 \
	$(ANDROID_SYS_HEADERS)/14/frameworks/base/include \
	$(ANDROID_SYS_HEADERS)/14/frameworks/base/native/include \
	$(ANDROID_SYS_HEADERS)/14/system/core/include \
	$(ANDROID_SYS_HEADERS)/14/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/14/frameworks/base/include/media/stagefright

LIBIOMX_INCLUDES_18 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/18 \
	$(ANDROID_SYS_HEADERS)/18/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/18/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/18/system/core/include \
	$(ANDROID_SYS_HEADERS)/18/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/18/frameworks/native/include/media/hardware

LIBIOMX_INCLUDES_19 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/19 \
	$(ANDROID_SYS_HEADERS)/19/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/19/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/19/system/core/include \
	$(ANDROID_SYS_HEADERS)/19/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/19/frameworks/native/include/media/hardware

LIBIOMX_INCLUDES_21 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/21 \
	$(ANDROID_SYS_HEADERS)/21/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/21/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/21/system/core/include \
	$(ANDROID_SYS_HEADERS)/21/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/21/frameworks/native/include/media/hardware

LIBIOMX_INCLUDES_23 := $(LIBIOMX_INCLUDES_COMMON) \
	$(ANDROID_SYS_HEADERS)/23 \
	$(ANDROID_SYS_HEADERS)/23/frameworks/native/include \
	$(ANDROID_SYS_HEADERS)/23/frameworks/av/include \
	$(ANDROID_SYS_HEADERS)/23/system/core/include \
	$(ANDROID_SYS_HEADERS)/23/hardware/libhardware/include \
	$(ANDROID_SYS_HEADERS)/23/frameworks/native/include/media/hardware
# $1 vendor_name
# $2 android sdk version
get-iomx-inc-path = $(if $(1),$(patsubst $(ANDROID_SYS_HEADERS)/%, \
$(ANDROID_SYS_HEADERS)/$(1)/%,$(LIBIOMX_INCLUDES_$(2))),$(LIBIOMX_INCLUDES_$(1)))

###################Module 'omxcore' begain####################
define build_iomx_context
include $(CLEAR_VARS)
# Declare module name
LOCAL_MODULE := libandroid_$(2)_$(1)

LOCAL_SRC_FILES := \
	gbuffer.cpp 

LOCAL_LINK_MODE := c++
LOCAL_CFLAGS := -Wno-reorder -DBUG_CHECK -DANDROID_SDK_VERSION=$(1) \
	         -Wno-psabi -fpic -ffunction-sections -funwind-tables \
             -fstack-protector -no-canonical-prefixes  \
             -fno-exceptions -fno-rtti \
             -DHAVE_PTHREADS \
			 -Wno-multichar \
			 -DCHROMIUM_AVAILABLE=1 \
			 -DHAVE_SYS_UIO_H \
			 -DHAVE_ANDROID_OS \
			 -DHAVE_POSIX_CLOCKS \
			 -UNDEBUG

ifdef DEBUG_RTC
 LOCAL_CFLAGS += -DDEBUG_RTC
endif

ifeq (qcom,$(2))
 LOCAL_CFLAGS += -DANDROID_ON_QCOM
 LOCAL_SRC_FILES += convert.cpp
 LOCAL_SRC_FILES += hw_jpeg.cpp
endif
 LOCAL_CFLAGS += -DBUILD_INFO="$(call rtc-build-info,[,])"

ifneq ($(filter $(1), 21 22 23 24 25),)
 LOCAL_CPPFLAGS += -std=gnu++11
endif

LOCAL_LDFLAGS := -no-canonical-prefixes  -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now
# LOCAL_LDLIBS := -Llibs -lutils -lcutils -lmedia -lstagefright -lbinder -lgui -lui -lstagefright_omx

# Append any include's paths.
LOCAL_C_INCLUDES := $(call get-iomx-inc-path,$(2),$(1))

LOCAL_SHARED_LIBRARIES := $(all_omx_libs)
# LOCAL_STATIC_LIBRARIES :=
# LOCAL_EXPORT_CFLAGS :=
# LOCAL_EXPORT_CPPFLAGS :=
# LOCAL_EXPORT_LDFLAGS :=
#LOCAL_RELEASE_PATH := $(TARGET_RELEASE_DIR)/
-include $(ANDROID_SYS_HEADERS)/$(2)/$(1)/vendor.mk
LOCAL_EXPORT_C_INCLUDES :=  $(LOCAL_C_INCLUDES)
include $(BUILD_SHARED_LIBRARY)
endef

BUILD_VENDOR := qcom stone

#ifeq (arm64,$(TARGET_ARCH))
#BUILD_API_VERSION := 21
#else
#BUILD_API_VERSION := 18 19
#ifneq ($(filter 4.8% 4.9%, $(TOOLCHAIN_VERSION)),)
#BUILD_API_VERSION += 21 23
#endif
#endif
BUILD_API_VERSION := $(ANDROID_MIN_TARGET_SDK)

$(foreach sdk,$(BUILD_API_VERSION), \
	$(foreach vendor, $(BUILD_VENDOR), \
	$(if $(wildcard $(ANDROID_SYS_HEADERS)/$(vendor)/$(sdk)), \
    $(eval $(call build_iomx_context,$(sdk),$(vendor))),$(info $(ANDROID_SYS_HEADERS)/$(vendor)/$(sdk)))))

ifneq ($(wildcard $(LOCAL_PATH)/test/make.mk),)
 #$(call include-makefile, $(LOCAL_PATH)/test/make.mk)
endif

