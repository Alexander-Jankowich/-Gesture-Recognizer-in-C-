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
// Separate this out as a shim with no dependencies (to prevent conflicts)
#include "APP_sysinternals.h"

// This is a brittle shim, but is okay when we merge in release
#include "../../video/SDL_sysvideo.h"
#include "../../video/cocoa/SDL_cocoamodes.h"
#include "../../video/cocoa/SDL_cocoawindow.h"

/**
 * Returns the NSScreen backing the given display.
 *
 * This function is a shim allowing us to access internals. We need this in
 * order to detailed information about the device display
 *
 * @param displayId     The display to query
 *
 * @return the UIScreen backing the given display.
 */
NSScreen* APP_GetNSScreen(uint32_t displayId) {
    SDL_VideoDisplay *disp = SDL_GetVideoDisplay(displayId);
    if (!disp || !disp->internal) {
        return NULL;
    }
    
    SDL_DisplayData *cocoa = (SDL_DisplayData *)disp->internal;
    CGDirectDisplayID cgdid = cocoa->display;
    for (NSScreen *screen in [NSScreen screens]) {
        NSNumber *num = [screen.deviceDescription objectForKey:@"NSScreenNumber"];
        if (num && [num unsignedIntValue] == cgdid) {
            return screen;
        }
    }
    return NULL;
}

/**
 * Returns the NSWindow backing the given window.
 *
 * This function is a shim allowing us to access internals. We need this in
 * order to detailed information about the backing window.
 *
 * @param window    The window to query
 *
 * @return the NSWindow backing the given window.
 */
NSWindow* APP_GetNSWindow(void* window) {
    if (!window) {
        return NULL;
    }
    SDL_CocoaWindowData *data = (__bridge SDL_CocoaWindowData *)((SDL_Window*)window)->internal;
    if (!data || !data.nswindow) {
        return NULL;
    }
    return data.nswindow;
}

