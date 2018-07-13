LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../SDL2-2.0.7/include\
                    $(LOCAL_PATH)/../SDL2_image-2.0.2\
                    $(LOCAL_PATH)/../SDL2_mixer-2.0.2\
					$(LOCAL_PATH)/../../../../android-ndk-r16/sources/third_party/vulkan/src/libs/glm

LOCAL_CPP_FEATURES := rtti

LOCAL_CFLAGS += -O2
LOCAL_CPPFLAGS += -std=c++14

#LOCAL_SRC_FILES := ../../../src/main.cpp\
#                   ../../../src/gfx.cpp\
#                   ../../../src/map.cpp

LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/../../../src/*.cpp))

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
