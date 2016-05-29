
# Default using the sys-build compiled.
SYSBUILD_MK := $(wildcard $(SYSBUILD_ROOT)/sys-build.mk)

ifdef SYSBUILD_MK
 $(info )
 $(info #################Using SYS-BUILD building. TARGET=$(TARGET)################)
 $(info )
 include $(SYSBUILD_MK)
else

ndk_cmd := $(shell which ndk-build)
 ifndef ndk_cmd
  ndk_cmd := $(ANDROID_NDK)/ndk-build
 endif
 
ndk_cmd := $(wildcard $(ndk_cmd))
ifndef ndk_cmd
	$(error Not found ANDROID_NDK variable define.)
endif

.PHONY: all
all:
	@echo "###############Using NDK building. TARGET=$(TARGET)#################"
	@$(ndk_cmd) NDK_PROJECT_PATH=ndk APP_BUILD_SCRIPT=make.mk NDK_APPLICATION_MK=ndk/Application.mk $(TARGET)

endif
