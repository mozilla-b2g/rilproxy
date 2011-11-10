# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= src/b2g-dialer-daemon.c
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_MODULE:= b2g-dialer-daemon
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE)
# include $(CLEAR_VARS)
# LOCAL_SRC_FILES:= src/b2g-dialer-forward.cpp
# LOCAL_SHARED_LIBRARIES := libcutils liblog libmedia libutils libbinder
# LOCAL_MODULE:= b2g-dialer-forward
# LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
# LOCAL_MODULE_TAGS := eng
# include $(BUILD_EXECUTABLE)
