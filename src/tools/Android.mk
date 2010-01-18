LOCAL_PATH:= $(call my-dir)

# shveu-convert
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := external/libshveu/include
LOCAL_CFLAGS := -DVERSION=\"1.0.0\"
LOCAL_SRC_FILES := shveu-convert.c
LOCAL_SHARED_LIBRARIES := libshveu
LOCAL_MODULE := shveu-convert
include $(BUILD_EXECUTABLE)
