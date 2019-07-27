LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../SDL2/include\
                    $(LOCAL_PATH)/../SDL2_image\
                    $(LOCAL_PATH)/../libsndfile\
					/home/twobit/Programming/c++/android-sdl/android-ndk-r16/sources/third_party/vulkan/src/libs/glm

LOCAL_CPPFLAGS += -std=c++17
LOCAL_CFLAGS += -O3
LOCAL_LDFLAGS += -s

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/../../src/main/cpp/*.cpp)

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image sndfile

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
