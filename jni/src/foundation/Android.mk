LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
	AHandler.cpp         \
	ALooper.cpp          \
	ALooperRoster.cpp    \
	AMessage.cpp         \
	AString.cpp          

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS += -Wno-multichar

LOCAL_MODULE:= libstagefright_foundation

#LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
