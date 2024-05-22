LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Injector/Headers

LOCAL_MODULE    := RevenyInjector
LOCAL_CPPFLAGS += -Werror -Wextra -Wpedantic -Wshadow -Wconversion -w -s -std=c++17 -fexceptions

LOCAL_SRC_FILES := Injector/Injector.cpp \
				   Injector/RevMemory.cpp \
				   Injector/RemoteProcess.cpp \
				   Injector/NativeBridge.cpp \
				   Injector/ELFUtil.cpp

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)

# Proxy
include $(CLEAR_VARS)

MAIN_LOCAL_PATH := $(call my-dir)/..
LOCAL_C_INCLUDES += $(MAIN_LOCAL_PATH)/Include \
                    $(LOCAL_PATH)/External/ \

LOCAL_MODULE    := RevenyProxy
LOCAL_CPPFLAGS += -Werror -Wextra -Wpedantic -Wshadow -Wconversion -w -s -std=c++17
LOCAL_STATIC_LIBRARIES := libxdl

LOCAL_SRC_FILES := Proxy/Proxy.cpp

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/external/Android.mk