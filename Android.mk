# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= src/b2g-dialer-daemon.c

LOCAL_SHARED_LIBRARIES := libcutils liblog

LOCAL_MODULE:= b2g-dialer-daemon

include $(BUILD_EXECUTABLE)
