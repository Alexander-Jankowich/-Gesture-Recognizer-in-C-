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
#if defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_TVOS)

#include "APP_sysinternals.h"
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

/**
 * System dependent version of APP_GetWindowSafeBoundsInPixels
 *
 * @param window        The window to query
 * @param rect          Rectangle to store the display bounds
 */
bool APP_SYS_GetWindowSafeAreaInPixels(SDL_Window* window, SDL_Rect *rect) {
@autoreleasepool {
    UIWindow* uiwin = APP_GetUIWindow(window);
    if (!uiwin || rect == NULL) {
        return false;
    }
    
    CGRect winRect = uiwin.bounds;
    CGFloat scale = uiwin.screen.scale;
    rect->x = winRect.origin.x*scale;
    rect->y = winRect.origin.y*scale;
    rect->w = winRect.size.width*scale;
    rect->h = winRect.size.height*scale;

    UIEdgeInsets insets = uiwin.safeAreaInsets;
    rect->x += insets.left*scale;
    rect->w -= (insets.left + insets.right)*scale;
    rect->y += insets.top*scale;
    rect->h -= (insets.top + insets.bottom)*scale;
    return true;
}
}

/**
 * System dependent version of SDL_CheckDisplayNotch
 *
 * @param displayId	The display to query
 *
 * @return true if this device has a notch
 */
bool APP_SYS_CheckDisplayNotch(SDL_DisplayID displayId) {
    UIScreen *screen = APP_GetUIScreen(displayId);
    if (!screen) {
        return false;
    }
    
    // Try scene-based windows first (iOS 13+ / Catalyst)
    if (@available(iOS 13.0, *)) {
        for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
            if (![scene isKindOfClass:[UIWindowScene class]]) {
                continue;
            }
            UIWindowScene *ws = (UIWindowScene *)scene;

            // Consider only active/foreground scenes
            if (ws.activationState != UISceneActivationStateForegroundActive) {
                continue;
            }

            for (UIWindow *w in ws.windows) {
                if (w.screen == screen) {
                    // Portrait: top ~44; Landscape: left/right > 0 on notched phones
                    UIEdgeInsets insets = w.safeAreaInsets;
                    if (insets.top >= 44.0 || insets.left > 0.0 || insets.right > 0.0) {
                        return true;
                    }
                }
            }
        }
    }
    
    // Fallback for iOS 11–12 (no scenes)
    if (@available(iOS 11.0, *)) {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        for (UIWindow *w in UIApplication.sharedApplication.windows) {
        #pragma clang diagnostic pop
            if (w.screen == screen) {
                UIEdgeInsets insets = w.safeAreaInsets;
                if (insets.top >= 44.0 || insets.left > 0.0 || insets.right > 0.0) {
                    return true;
                }
            }
        }
    }

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
	switch ([UIDevice currentDevice].orientation) {
		case UIDeviceOrientationUnknown:
			return SDL_ORIENTATION_UNKNOWN;
		case UIDeviceOrientationPortrait:
			return SDL_ORIENTATION_PORTRAIT;
		case UIDeviceOrientationPortraitUpsideDown:
			return SDL_ORIENTATION_PORTRAIT_FLIPPED;
		case UIDeviceOrientationLandscapeRight:
			return SDL_ORIENTATION_LANDSCAPE;
		case UIDeviceOrientationLandscapeLeft:
			return SDL_ORIENTATION_LANDSCAPE_FLIPPED;
		case UIDeviceOrientationFaceUp:
			return SDL_ORIENTATION_UNKNOWN;
		case UIDeviceOrientationFaceDown:
			return SDL_ORIENTATION_UNKNOWN;
	}
	return SDL_ORIENTATION_UNKNOWN;
}

#endif
