LOCAL_PATH := $(call my-dir)

# libxdl.a
include $(CLEAR_VARS)
LOCAL_MODULE := libxdl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/xDL/xdl/src/main/cpp/include
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_SRC_FILES := \
    xDL/xdl/src/main/cpp/xdl.c \
    xDL/xdl/src/main/cpp/xdl_iterate.c \
    xDL/xdl/src/main/cpp/xdl_linker.c \
    xDL/xdl/src/main/cpp/xdl_lzma.c \
    xDL/xdl/src/main/cpp/xdl_util.c

include $(BUILD_STATIC_LIBRARY)

# liblsplt.a
include $(CLEAR_VARS)
LOCAL_MODULE:= liblsplt
LOCAL_C_INCLUDES := $(LOCAL_PATH)/LSPlt/lsplt/src/main/jni/include
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_CFLAGS := -Wall -Wextra -Werror -fvisibility=hidden
LOCAL_CPPFLAGS := -std=c++20
LOCAL_SRC_FILES := \
    lsplt/lsplt/src/main/jni/elf_util.cc \
    lsplt/lsplt/src/main/jni/lsplt.cc
include $(BUILD_STATIC_LIBRARY)

CWD := $(LOCAL_PATH)