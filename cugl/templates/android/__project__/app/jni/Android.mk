# Top level NDK Makefile for Android application
# Module settings for CUGL
SUPPORT_OPENGL := __CUGL_OPENGL__
SUPPORT_AUDIO  := __CUGL_AUDIO__
SUPPORT_SCENE2 := __CUGL_SCENE2__
SUPPORT_SCENE3 := __CUGL_SCENE3__
SUPPORT_PHYSICS2 := __CUGL_PHYSICS2__
SUPPORT_NETCODE  := __CUGL_NETCODE__
SUPPORT_PHYSICS2_DISTRIB := __CUGL_DISTRIB_PHYSICS2__
SUPPORT_PHYSICS2_LIGHTS  := __CUGL_DISTRIB_LIGHTS__

# We just invoke the subdirectories
include $(call all-subdir-makefiles)
