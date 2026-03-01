/*
 * SDL_app:  An all-in-one library for packing SDL applications.
 * Copyright (C) 2022-2025 Walker M. White
 *
 * This library is a shim built on top of SDL and several extensions. This
 * library allows us to introduce several functions to expose functionality
 * that we feel to be missing on mobile devices. In particular,these functions
 * have to do with improving orientation detection. They also expose device 
 * information for data analytics. 
 *
 * In addition, this library provides us with a custom build system that makes
 * it easy to quickly to create apps on top of SDL.
 *
 * SDL License:
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "../APP_sysdisplay.h"
#include <SDL3/SDL_system.h>
#include <jni.h>

/**
 * System dependent version of APP_GetWindowSafeAreaInPixels
 *
 * @param window        The window to query
 * @param rect          Rectangle to store the display bounds
 */
bool APP_SYS_GetWindowSafeAreaInPixels(SDL_Window* window, SDL_Rect *rect) {
	if (SDL_GetWindowSafeArea(window,rect)) {
		// Do poor man's scaling for now
		int w1 = 1;
		int w2 = 1;
		SDL_GetWindowSize(window,&w1,NULL);
		SDL_GetWindowSizeInPixels(window,&w2,NULL);
		float scale = ((float)w2)/w1;
		rect->x *= scale;
		rect->y *= scale;
		rect->w *= scale;
		rect->h *= scale;
		return true;
	}
    return false;
}

/**
 * System dependent version of APP_CheckDisplayNotch
 *
 * @param displayId	The display to query
 *
 * @return true if this device has a notch
 */
bool APP_SYS_CheckDisplayNotch(SDL_DisplayID displayId) {
	// Retrieve the JNI environment (this one is not cached)
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetStaticMethodID(env, clazz, "hasNotch", "()Z");
    int result = (*env)->CallStaticBooleanMethod(env, clazz, method_id);

    // Clean up
	(*env)->DeleteLocalRef(env, activity);
    (*env)->DeleteLocalRef(env, clazz);

    return result;
}

/**
 * System dependent version of APP_CheckAccelerometerOrientation
 *
 * @param displayId     The display to query
 *
 * @return true if the accelerometer axes have the standard orientation.
 */
bool APP_SYS_CheckAccelerometerOrientation(SDL_DisplayID displayId) {
	// Retrieve the JNI environment (this one is not cached)
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetStaticMethodID(env, clazz, "isXYSwapped", "()Z");
    int result = (*env)->CallStaticBooleanMethod(env, clazz, method_id);

    // Clean up
	(*env)->DeleteLocalRef(env, activity);
    (*env)->DeleteLocalRef(env, clazz);

    return !result;
}

/** A cache variable storing the configuration orientation */
static int g_android_config_orientation = SDL_ORIENTATION_UNKNOWN;

/** 
 * Receives the configuration orientation from the SDL activity
 *
 * @param env			The JNI environment
 * @param clazz			The communicating class
 * @param orientation	The configuration orientation
 */
JNIEXPORT void JNICALL
Java_org_libsdl_app_DisplayOrientation_nativeSetConfigOrientation
    (JNIEnv *env, jclass clazz, jint orientation) {
	g_android_config_orientation = orientation;
}

/**
 * System dependent version of APP_GetDisplayConfiguration
 *
 * @param displayId	The display to query
 *
 * @return the configuration orientation of this display.
 */
SDL_DisplayOrientation APP_SYS_GetDisplayConfiguration(SDL_DisplayID displayId) {
    if (g_android_config_orientation >= 3) {
		return SDL_ORIENTATION_PORTRAIT;
	}
    
	return SDL_ORIENTATION_LANDSCAPE;
}

/** A cache variable storing the window orientation */
static int g_android_window_orientation = SDL_ORIENTATION_UNKNOWN;

/** 
 * Receives the window orientation from the SDL activity
 *
 * @param env			The JNI environment
 * @param clazz			The communicating class
 * @param orientation	The window orientation
 */
JNIEXPORT void JNICALL
Java_org_libsdl_app_DisplayOrientation_nativeSetWindowOrientation
    (JNIEnv *env, jclass clazz, jint orientation) {
    g_android_window_orientation = orientation;

    SDL_Event event;
    event.type = SDL_EVENT_DISPLAY_ORIENTATION;
    event.display.data1 = orientation;
    SDL_PushEvent(&event);
}

/**
 * System dependent version of APP_GetDisplayOrientation
 *
 * @param displayId	The display to query
 *
 * @return the orientation of this display.
 */
SDL_DisplayOrientation APP_SYS_GetDisplayOrientation(SDL_DisplayID displayId) {
	switch (g_android_window_orientation) {
		case 0:
			return SDL_ORIENTATION_UNKNOWN;
		case 1:
			return SDL_ORIENTATION_LANDSCAPE;
		case 2:
			return SDL_ORIENTATION_LANDSCAPE_FLIPPED;
		case 3:
			return SDL_ORIENTATION_PORTRAIT;
		case 4:
			return SDL_ORIENTATION_PORTRAIT_FLIPPED;
		default:
			return SDL_ORIENTATION_UNKNOWN;
	}
}

/** A cache variable storing the device orientation */
static int g_android_device_orientation = SDL_ORIENTATION_UNKNOWN;

/** 
 * Receives the device orientation from the SDL activity
 *
 * @param env			The JNI environment
 * @param clazz			The communicating class
 * @param orientation	The device orientation
 */
JNIEXPORT void JNICALL
Java_org_libsdl_app_DeviceOrientation_nativeSetDeviceOrientation
    (JNIEnv *env, jclass clazz, jint orientation) {
    g_android_device_orientation = orientation;
}


/**
 * System dependent version of SDL_GetDeviceOrientation
 *
 * @return the current device orientation.
 */
SDL_DisplayOrientation APP_SYS_GetDeviceOrientation(void) {
	switch (g_android_device_orientation) {
		case 0:
			return SDL_ORIENTATION_UNKNOWN;
		case 1:
			return SDL_ORIENTATION_LANDSCAPE;
		case 2:
			return SDL_ORIENTATION_LANDSCAPE_FLIPPED;
		case 3:
			return SDL_ORIENTATION_PORTRAIT;
		case 4:
			return SDL_ORIENTATION_PORTRAIT_FLIPPED;
		default:
			return SDL_ORIENTATION_UNKNOWN;
	}
}