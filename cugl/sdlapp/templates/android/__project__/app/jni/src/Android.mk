LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ..
SDL3_OFFSET := __SDL3_PATH__

########################
#
# Main Application Entry
#
########################
SDL3_PATH := $(LOCAL_PATH)/$(CURR_DEPTH)/$(SDL3_OFFSET)
PROJ_PATH := $(LOCAL_PATH)/${CURR_DEPTH}/__SOURCE_PATH__

include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_C_INCLUDES := $(SDL3_PATH)/include
__EXTRA_INCLUDES__

# Add your application source files here.
LOCAL_PATH = $(PROJ_PATH)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,__SOURCE_FILES__)

# Link in SDL3
LOCAL_STATIC_LIBRARIES := SDL3_static
LOCAL_STATIC_LIBRARIES += SDL3_app_static
LOCAL_STATIC_LIBRARIES += SDL3_atk_static
LOCAL_STATIC_LIBRARIES += SDL3_image_static
LOCAL_STATIC_LIBRARIES += SDL3_ttf_static

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lGLESv3 -lOpenSLES -llog -landroid
include $(BUILD_SHARED_LIBRARY)
