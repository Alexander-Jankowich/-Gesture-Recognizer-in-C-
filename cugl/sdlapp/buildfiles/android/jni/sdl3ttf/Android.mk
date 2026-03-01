LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ..
SDL3_OFFSET := ../../..
TTF_OFFSET  := ../../..

###########################
#
# SDL_ttf shared library
#
###########################
SDL3_PATH  := $(LOCAL_PATH)/$(CURR_DEPTH)/$(SDL3_OFFSET)
SDL3_TTF_MAKE    := $(LOCAL_PATH)
SDL3_TTF_PATH    := $(LOCAL_PATH)/$(CURR_DEPTH)/$(TTF_OFFSET)
SDL3_TTF_SOURCE  := $(SDL3_TTF_PATH)/src
SDL3_TTF_INCLUDE := $(SDL3_TTF_PATH)/include

# Enable this if you want to use PlutoSVG for emoji support
SUPPORT_PLUTOSVG ?= true
PLUTOSVG_LIBRARY_PATH := external/plutosvg

# Enable this if you want to use HarfBuzz
SUPPORT_HARFBUZZ ?= true
HARFBUZZ_LIBRARY_PATH := external/harfbuzz

FREETYPE_LIBRARY_PATH := external/freetype

# Build freetype library
ifneq ($(FREETYPE_LIBRARY_PATH),)
    include $(SDL3_TTF_MAKE)/$(FREETYPE_LIBRARY_PATH)/Android.mk
endif

# Build the harfbuzz library
ifeq ($(SUPPORT_HARFBUZZ),true)
    include $(SDL3_TTF_MAKE)/$(HARFBUZZ_LIBRARY_PATH)/Android.mk
endif

# Build the plutosvg library
ifeq ($(SUPPORT_PLUTOSVG),true)
    include $(SDL3_TTF_MAKE)/$(PLUTOSVG_LIBRARY_PATH)/Android.mk
endif

# Restore local path
LOCAL_PATH := $(SDL3_TTF_SOURCE)
include $(CLEAR_VARS)

LOCAL_MODULE := SDL3_ttf

LOCAL_C_INCLUDES := $(SDL3_PATH)/include
LOCAL_C_INCLUDES += $(SDL3_TTF_PATH)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(LOCAL_PATH)/SDL_ttf.c.neon \
	$(LOCAL_PATH)/SDL_hashtable.c \
	$(LOCAL_PATH)/SDL_hashtable_ttf.c \
	$(LOCAL_PATH)/SDL_gpu_textengine.c \
	$(LOCAL_PATH)/SDL_renderer_textengine.c \
	$(LOCAL_PATH)/SDL_surface_textengine.c)

LOCAL_CFLAGS += -O2

LOCAL_LDFLAGS := -Wl,--no-undefined -Wl,--version-script=$(SDL3_TTF_PATH)/src/SDL_ttf.sym

ifneq ($(FREETYPE_LIBRARY_PATH),)
    LOCAL_C_INCLUDES += $(SDL3_TTF_PATH)/$(FREETYPE_LIBRARY_PATH)/include
    LOCAL_STATIC_LIBRARIES += freetype
endif

ifeq ($(SUPPORT_HARFBUZZ),true)
    LOCAL_C_INCLUDES += $(SDL3_TTF_PATH)/$(HARFBUZZ_LIBRARY_PATH)/src
    LOCAL_CFLAGS += -DTTF_USE_HARFBUZZ
    LOCAL_STATIC_LIBRARIES += harfbuzz
endif

ifeq ($(SUPPORT_PLUTOSVG),true)
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(PLUTOVG_LIBRARY_PATH)/include
    LOCAL_C_FLAGS += -DTTF_USE_PLUTOSVG -DPLUTOSVG_HAS_FREETYPE
    LOCAL_STATIC_LIBRARIES += plutosvg
endif

LOCAL_SHARED_LIBRARIES := SDL3

LOCAL_EXPORT_C_INCLUDES += $(SDL3_TTF_PATH)/include

include $(BUILD_SHARED_LIBRARY)

###########################
#
# SDL3_ttf static library
#
###########################

LOCAL_MODULE := SDL3_ttf_static

LOCAL_MODULE_FILENAME := libSDL3_ttf

LOCAL_LDLIBS :=
LOCAL_LDFLAGS :=
LOCAL_EXPORT_LDLIBS :=

include $(BUILD_STATIC_LIBRARY)

