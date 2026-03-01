LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ../..
SDL3_OFFSET := ../../../sdlapp
APP_OFFSET  := ../../../sdlapp

###########################
#
# SDL_app shared library
#
###########################
SDL3_PATH  := $(LOCAL_PATH)/$(CURR_DEPTH)/$(SDL3_OFFSET)
SDL3_APP_MAKE   := $(LOCAL_PATH)
SDL3_APP_PATH   := $(LOCAL_PATH)/$(CURR_DEPTH)/$(APP_OFFSET)
SDL3_APP_SOURCE := $(SDL3_APP_PATH)/src/app

# SDL Features
include $(SDL3_APP_MAKE)/sdl3/Android.mk
include $(SDL3_APP_MAKE)/sdl3atk/Android.mk
include $(SDL3_APP_MAKE)/sdl3image/Android.mk
include $(SDL3_APP_MAKE)/sdl3ttf/Android.mk

include $(CLEAR_VARS)

LOCAL_MODULE := SDL3_app

# Keep file mangling at a minimum
LOCAL_PATH := $(SDL3_APP_SOURCE)

LOCAL_C_INCLUDES := $(SDL3_PATH)/include
LOCAL_C_INCLUDES += $(SDL3_APP_PATH)/include

LOCAL_SRC_FILES :=  \
	$(subst $(LOCAL_PATH)/,, \
	$(LOCAL_PATH)/APP_version.c \
	$(LOCAL_PATH)/display/APP_display.c \
	$(LOCAL_PATH)/display/android/APP_sysdisplay.c \
	$(LOCAL_PATH)/device/APP_device.c \
	$(LOCAL_PATH)/device/android/APP_sysdevice.c)

LOCAL_SHARED_LIBRARIES := SDL3

LOCAL_EXPORT_C_INCLUDES += $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

###########################
#
# SDL3_app static library
#
###########################

LOCAL_MODULE := SDL3_app_static

LOCAL_MODULE_FILENAME := libSDL3_app

LOCAL_LDLIBS :=
LOCAL_LDFLAGS :=
LOCAL_EXPORT_LDLIBS :=

LOCAL_STATIC_LIBRARIES := SDL3_static
LOCAL_STATIC_LIBRARIES += SDL3_atk_static
LOCAL_STATIC_LIBRARIES += SDL3_image_static
LOCAL_STATIC_LIBRARIES += SDL3_ttf_static

include $(BUILD_STATIC_LIBRARY)

