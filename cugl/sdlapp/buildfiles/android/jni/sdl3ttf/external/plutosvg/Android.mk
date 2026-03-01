LOCAL_PATH  := $(call my-dir)
CURR_DEPTH  := ../../..
TTF_OFFSET  := ../../../components/SDL_ttf

###########################
#
# harfbuzz static library
#
###########################
SDL3_TTF_PATH := $(LOCAL_PATH)/$(CURR_DEPTH)/$(TTF_OFFSET)
PLUTOSVG_PATH := $(SDL3_TTF_PATH)/external/plutosvg
PLUTOVG_PATH  := $(PLUTOSVG_PATH)/plutovg

include $(CLEAR_VARS)

LOCAL_PATH := $(PLUTOSVG_PATH)

LOCAL_C_INCLUDES += $(PLUTOVG_PATH)/include
LOCAL_C_FLAGS += -DTTF_USE_PLUTOSVG -DPLUTOSVG_HAS_FREETYPE
LOCAL_PATH := $(SDL3_TTF_PATH)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(LOCAL_PATH)/$(PLUTOSVG_PATH)/source/plutosvg.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-blend.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-canvas.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-font.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-ft-math.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-ft-raster.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-ft-stroker.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-matrix.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-paint.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-path.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-rasterize.c \
	$(LOCAL_PATH)/$(PLUTOVG_PATH)/source/plutovg-surface.c)

# -DHAVE_ICU -DHAVE_ICU_BUILTIN
LOCAL_MODULE := plutosvg

include $(BUILD_STATIC_LIBRARY)
