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
# $(call include-makefiles, /foo/make.mk /boo/make.mk)
 
all_omx_libs := binder \
	            cutils \
				hardware \
				media \
				stagefright \
				ui \
				utils \
				gui \
				stagefright_omx \
				camera_client

$(foreach lib, $(all_omx_libs),\
	$(eval include $(CLEAR_VARS))\
	$(eval LOCAL_MODULE := $(lib))\
	$(eval LOCAL_SRC_FILES := ./lib$(lib).c)\
	$(eval LOCAL_CFLAGS := -fpic -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes )\
	$(eval include $(BUILD_SHARED_LIBRARY)))

