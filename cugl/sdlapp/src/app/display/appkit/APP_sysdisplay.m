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
#ifdef SDL_PLATFORM_MACOS

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <AppKit/AppKit.h>
#include "APP_sysinternals.h"

/**
 * System dependent version of APP_GetWindowSafeAreaInPixels
 *
 * @param window        The window to query
 * @param rect          Rectangle to store the display bounds
 */
bool APP_SYS_GetWindowSafeAreaInPixels(SDL_Window* window, SDL_Rect *rect) {
@autoreleasepool {
    // TODO: Starting in Tahoe, Window insets are supported.
    // Leave this here now as a stub
    if (SDL_GetWindowSafeArea(window,rect)) {
    	NSWindow* nswin = APP_GetNSWindow(window);
    	float scale = nswin.screen.backingScaleFactor;
    	rect->x *= scale;
    	rect->y *= scale;
    	rect->w *= scale;
    	rect->h *= scale;
    	return true;
    }
	return false;
}
}

/**
 * System dependent version of APP_CheckDisplayNotch
 *
 * @param displayId	The display to query
 *
 * @return true if this device has a notch
 */
bool APP_SYS_CheckDisplayNotch(SDL_DisplayID displayId) {
    return false;
}

/**
 * System dependent version of APP_CheckAccelerometerOrientation
 *
 * @param displayId     The display to query
 *
 * @return true if the accelerometer axes have the standard orientation.
 */
bool APP_SYS_CheckAccelerometerOrientation(SDL_DisplayID displayId) {
	return true;
}

/**
 * System dependent version of APP_GetDisplayConfiguration
 *
 * @param displayId	The display to query
 *
 * @return the configuration orientation of this display.
 */
SDL_DisplayOrientation APP_SYS_GetDisplayConfiguration(SDL_DisplayID displayId) {
	SDL_DisplayOrientation result = SDL_GetCurrentDisplayOrientation(displayId);
	if (result == SDL_ORIENTATION_PORTRAIT || result == SDL_ORIENTATION_PORTRAIT_FLIPPED) {
		return SDL_ORIENTATION_PORTRAIT;
	}
	return SDL_ORIENTATION_LANDSCAPE;
}

/**
 * System dependent version of APP_GetDisplayOrientation
 *
 * @param displayId	The display to query
 *
 * @return the orientation of this display.
 */
SDL_DisplayOrientation APP_SYS_GetDisplayOrientation(SDL_DisplayID displayId) {
	return SDL_GetCurrentDisplayOrientation(displayId);
}

/**
 * System dependent version of APP_GetDeviceOrientation
 *
 * @return the current device orientation.
 */
SDL_DisplayOrientation APP_SYS_GetDeviceOrientation(void) {
    return SDL_ORIENTATION_UNKNOWN;
}

#endif
