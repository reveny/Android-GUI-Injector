LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Injector/Headers
LOCAL_C_INCLUDES += $(LOCAL_PATH)/External

LOCAL_MODULE    := RevenyInjector
LOCAL_CPPFLAGS += -fexceptions -Werror -Wshadow -Wextra -Wpedantic -Wconversion -w -s -std=c++17
LOCAL_STATIC_LIBRARIES := libxdl

LOCAL_SRC_FILES := Injector/Injector.cpp \
				   Injector/RevMemory.cpp \
				   Injector/RemoteProcess.cpp \
				   Injector/NativeBridge.cpp \
				   Utility/ELFUtil.cpp \
				   Utility/RemapTools.cpp \
				   Utility/Utility.cpp \

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)

# Proxy
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Injector/Headers
LOCAL_C_INCLUDES += $(LOCAL_PATH)/External

LOCAL_MODULE    := RevenyProxy
LOCAL_CPPFLAGS += -fexceptions -Werror -Wextra -Wpedantic -Wshadow -Wconversion -w -s -std=c++17
LOCAL_STATIC_LIBRARIES := libxdl liblsplt

LOCAL_SRC_FILES := Proxy/Proxy.cpp \
                   Proxy/JNIProxy.cpp \
                   Utility/Utility.cpp \
                   Utility/RemapTools.cpp \

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/External/Android.mk