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
ANDROID_MIN_TARGET_SDK ?= 14 

include $(wildcard $(LOCAL_PATH)/*/make.mk)
 
