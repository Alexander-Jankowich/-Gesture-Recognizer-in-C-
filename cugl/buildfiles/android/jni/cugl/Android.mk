LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ..
CUGL_OFFSET := ../../..
SDL3_OFFSET := sdlapp

###########################
#
# CUGL static library
#
###########################
CUGL_MAKE := $(LOCAL_PATH)
CUGL_PATH := $(LOCAL_PATH)/$(CURR_DEPTH)/$(CUGL_OFFSET)

CUGL_INCLUDES := $(CUGL_PATH)/include
CUGL_INCLUDES += $(CUGL_PATH)/src/graphics/opengl
CUGL_INCLUDES += $(CUGL_PATH)/$(SDL3_OFFSET)/include
CUGL_INCLUDES += $(CUGL_PATH)/$(ATK_OFFSET)/include
CUGL_INCLUDES += $(CUGL_PATH)/$(APP_OFFSET)/include
CUGL_INCLUDES += $(CUGL_PATH)/$(IMG_OFFSET)/include
CUGL_INCLUDES += $(CUGL_PATH)/$(TTF_OFFSET)/include
CUGL_INCLUDES += $(CUGL_PATH)/external/poly2tri
CUGL_INCLUDES += $(CUGL_PATH)/external/box2d/include
CUGL_INCLUDES += $(CUGL_PATH)/external/libdatachannel/include

# CUGL Core
include $(CUGL_MAKE)/external/poly2tri/Android.mk
include $(CUGL_MAKE)/sdl3app/Android.mk

include $(CLEAR_VARS)

LOCAL_MODULE := CUGL-Core
LOCAL_C_INCLUDES := $(CUGL_INCLUDES)

LOCAL_PATH := $(CUGL_PATH)/source

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/core/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/actions/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/assets/*.c) \
	$(wildcard $(LOCAL_PATH)/core/assets/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/input/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/io/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/math/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/math/polygon/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/util/*.cpp))

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
LOCAL_CFLAGS += \
	-Wno-missing-prototypes \
	-Wno-implicit-const-int-float-conversion \

LOCAL_STATIC_LIBRARIES := poly2tri
LOCAL_STATIC_LIBRARIES += SDL3_app_static

include $(BUILD_STATIC_LIBRARY)


# CUGL GRAPHICS
include $(CLEAR_VARS)

LOCAL_MODULE := CUGL-Graphics
LOCAL_C_INCLUDES := $(CUGL_INCLUDES)

LOCAL_PATH := $(CUGL_PATH)/source

SUPPORT_OPENGL ?= true
LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/graphics/*.cpp) \
	$(wildcard $(LOCAL_PATH)/graphics/loaders/*.cpp))
ifeq ($(SUPPORT_OPENGL),true)
	LOCAL_SRC_FILES += \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/graphics/opengl/*.cpp))
	LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
	LOCAL_CFLAGS += -DCU_USE_OPENGL
endif

LOCAL_CFLAGS += \
	-Wno-missing-prototypes \
	-Wno-implicit-const-int-float-conversion \

include $(BUILD_STATIC_LIBRARY)


# CUGL Audio
SUPPORT_AUDIO ?= true
ifeq ($(SUPPORT_AUDIO),true)
	include $(CLEAR_VARS)
	LOCAL_MODULE := CUGL-Audio
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/audio/*.cpp) \
		$(wildcard $(LOCAL_PATH)/audio/graph/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL SCENE2
SUPPORT_SCENE2 ?= true
ifeq ($(SUPPORT_SCENE2),true)
	include $(CLEAR_VARS)
	LOCAL_MODULE := CUGL-Scene2
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/scene2/*.cpp) \
		$(wildcard $(LOCAL_PATH)/scene2/layout/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL SCENE3
SUPPORT_SCENE3 ?= true
ifeq ($(SUPPORT_SCENE3),true)
	include $(CLEAR_VARS)
	LOCAL_MODULE := CUGL-Scene3
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/scene3/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL PHYSICS2
SUPPORT_PHYSICS2 ?= true
ifeq ($(SUPPORT_PHYSICS2),true)
	include $(CUGL_MAKE)/external/box2d/Android.mk

	include $(CLEAR_VARS)
	
	LOCAL_MODULE := CUGL-Physics2
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/physics2/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	LOCAL_STATIC_LIBRARIES := box2d
	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL NETCODE
SUPPORT_NETCODE ?= true
ifeq ($(SUPPORT_NETCODE),true)
	include $(CUGL_MAKE)/external/libdatachannel/Android.mk

	include $(CLEAR_VARS)
	
	LOCAL_MODULE := CUGL-Netcode
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/netcode/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	LOCAL_STATIC_LIBRARIES := datachannel
	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL PHYSICS2 DISTRIB
SUPPORT_PHYSICS2_DISTRIB ?= true
ifeq ($(SUPPORT_PHYSICS2_DISTRIB),true)
	include $(CLEAR_VARS)
	LOCAL_MODULE := CUGL-Physics2-Distrib
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/physics2/distrib/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	include $(BUILD_STATIC_LIBRARY)
endif

# CUGL PHYSICS2 LIGHTS
SUPPORT_PHYSICS2_LIGHTS ?= true
ifeq ($(SUPPORT_PHYSICS2_LIGHTS),true)
	include $(CLEAR_VARS)
	LOCAL_MODULE := CUGL-Physics2-Lights
	LOCAL_C_INCLUDES := $(CUGL_INCLUDES)
	
	LOCAL_PATH := $(CUGL_PATH)/source

	LOCAL_SRC_FILES := \
		$(subst $(LOCAL_PATH)/,, \
		$(wildcard $(LOCAL_PATH)/physics2/lights/*.cpp))

	LOCAL_CFLAGS += \
		-Wno-missing-prototypes \
		-Wno-implicit-const-int-float-conversion \

	include $(BUILD_STATIC_LIBRARY)
endif


# CUGL
include $(CLEAR_VARS)

LOCAL_MODULE := CUGL

LOCAL_STATIC_LIBRARIES := CUGL-Core
LOCAL_STATIC_LIBRARIES += CUGL-Graphics
ifeq ($(SUPPORT_AUDIO),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Audio
endif
ifeq ($(SUPPORT_SCENE2),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Scene2
endif
ifeq ($(SUPPORT_SCENE3),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Scene3
endif
ifeq ($(SUPPORT_PHYSICS2),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Physics2
endif
ifeq ($(SUPPORT_NETCODE),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Netcode
endif
ifeq ($(SUPPORT_PHYSICS2_DISTRIB),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Physics2-Lights
endif
ifeq ($(SUPPORT_PHYSICS2_LIGHTS),true)
	LOCAL_STATIC_LIBRARIES += CUGL-Physics2-Lights
endif

include $(BUILD_STATIC_LIBRARY)
