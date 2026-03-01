LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ..
CUGL_OFFSET := __CUGL_PATH__

########################
#
# Main Application Entry
#
########################
CUGL_PATH := $(LOCAL_PATH)/$(CURR_DEPTH)/$(CUGL_OFFSET)
SDL3_PATH := $(CUGL_PATH)/sdlapp
PROJ_PATH := $(LOCAL_PATH)/${CURR_DEPTH}/__SOURCE_PATH__

include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_C_INCLUDES := $(CUGL_PATH)/include
LOCAL_C_INCLUDES += $(SDL3_PATH)/include
LOCAL_C_INCLUDES += $(CUGL_PATH)/external/libdatachannel/include
LOCAL_C_INCLUDES += $(CUGL_PATH)/external/box2d/include
LOCAL_C_INCLUDES += $(CUGL_PATH)/external/poly2tri
__EXTRA_INCLUDES__

# Add your application source files here.
LOCAL_PATH = $(PROJ_PATH)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,__SOURCE_FILES__)

# Link in CUGL and SDL3
LOCAL_STATIC_LIBRARIES := CUGL

LOCAL_LDLIBS := -lEGL -lGLESv1_CM -lGLESv2 -lGLESv3 -lOpenSLES -llog -landroid
include $(BUILD_SHARED_LIBRARY)
