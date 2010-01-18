LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	external/libshveu/include \

#LOCAL_CFLAGS := -DDEBUG

LOCAL_SRC_FILES := \
	veu_colorspace.c

LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_MODULE := libshveu
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

