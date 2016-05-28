
#=================================Sys-build=================================#
# Version: 1.0.0.1df057
# Author:  lichao@snailgame.net
# Date:    2016-05-28 11:07:05
#===========================================================================#

#   To point a make.mk scritp, it's will be contains by sys-build at first time.
# if the variable undefine then using $(APP_PROJECT_PATH)/make.mk as default.
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/make.mk

#   The option description that library or binary version is 'release' or 'debug'
# mode, the debug mode should be with -O0 -UNDEBUG options.
#   It is must be either debug or release.
APP_OPTIM := release

#   The variable description this code project whether append STL module, these
# modules by sysbuild contains in $(SYSBUILD_ROOT)/sources drectory. you can 
# refer thes modules by LOCAL_STATIC_LIBRARIES or LOCAL_SHARED_LIBRARIES if
# you already append these.
APP_STL := $(empty)

#   The variable declare the temporary file and target file generate drecotry,
# and normal is out/.
APP_OUTPUT_DIR := out

#   Enable or disable sysbuild log print, either '1' or '$(empty)', 1--enable
# $(empty)--disable.
APP_DEBUG_MODULES := $(empty)

#   Define NDK_LOG=1 in your environment or runing 'make NDK_LOG=1' to display 
# log traces when using the build scripts. See also the definition of 
# ndk_log below.
NDK_LOG := $(empty)

# Declare only by compile modules, if it's empty then compile all modules.
# For example:
# APP_MODULES := libgnu_stl libstl, or using command option
# 'sys-build --modules "libgnu_stl libstl" [options ...]'
APP_MODULES := $(empty)

#   Declare current architecture one of the arm | arm64 | x86 | x86_64, the value
# must not be empty.
APP_ARCH := arm

#   Declare current ABI(Application binary interface) one of the armeabi | 
# armeabi-v7a | arm64-v8a | x86 | x86_64.
APP_ABI := armeabi-v7a

#   Declare current platform, the current only supporte 'android', you can custom
# it if you are like.
# If the variable not empty, sysbuild should be include 
# $(SYSBUILD_ROOT)/configs/$(APP_PLATFORM).mk to build system scripts.
APP_PLATFORM := android 

# Declare current board name, only have one word.
APP_BOARD := unknow

# Declare board alias, not a must, only have one word.
APP_ALIAS_BOARD := unknow

#   The variable must not be empty, it is declare current cross_compile prefix.
# For example:
#   APP_TOOLCHAIN := /opt/arm-linux-, or using the command option:
#   'sysbuild --toolchain /opt/arm-linux-'
#APP_TOOLCHAIN := /home/wlzx/tools/android-ndk-r10/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi-
APP_TOOLCHAIN := /home/wlzx/tools/android-ndk-r10e/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86/bin/arm-linux-androideabi-

#   The variable declare toolchain's --sysroot option, in ordinary not declare
# it, but if you are to use the android-ndk toolchain then you are maby specify
# it, the value look the same $(NDK_ROOT)/platforms/android-<sdk>/arch-<arch>
APP_TOOLCHAIN_SYSROOT := /home/wlzx/tools/android-ndk-r10e/platforms/android-21/arch-arm

#   Declare the target library or bilary or third_part_binary releaase path, for
# example:
#  APP_RELEASE_DIR := /opt/version/release;/tmp/backup, or using command option
# 'sys-build --releasepath "/opt/version/release;/tmp/backup"'
APP_RELEASE_DIR := out/release

#   Declare the build system will be include makefile file, if it's avalid then 
# the to point's file should be include at config.mk by contains after, after
# the all make.mk can to refer these variable that by define in the config.mk
# or $(APP_USER_CONFIG).
#   Special, the config.mk and $(APP_USER_CONFIG) should be depended all modules.
APP_USER_CONFIG := $(empty)
