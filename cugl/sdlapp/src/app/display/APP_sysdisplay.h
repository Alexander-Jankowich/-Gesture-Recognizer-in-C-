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
#ifndef __APP_SYS_DISPLAY_H__
#define __APP_SYS_DISPLAY_H__
#include <SDL3/SDL.h>

/**
 *  \file APP_sysdisplay.h
 *
 *  \brief Include file for system-specific display functions
 *  \author Walker M. White
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * System dependent version of APP_GetWindowSafeAreaInPixels
 *
 * @param window        The window to query
 * @param rect          Rectangle to store the display bounds
 */
extern SDL_DECLSPEC bool SDLCALL APP_SYS_GetWindowSafeAreaInPixels(SDL_Window* window, SDL_Rect *rect);

/**
 * System dependent version of APP_CheckDisplayNotch
 *
 * @param displayId	The display to query
 *
 * @return true if this device has a notch
 */
extern SDL_DECLSPEC bool SDLCALL APP_SYS_CheckDisplayNotch(SDL_DisplayID displayId);

/**
 * System dependent version of APP_CheckAccelerometerOrientation
 *
 * @param displayId     The display to query
 *
 * @return true if the accelerometer axes have the standard orientation.
 */
extern SDL_DECLSPEC bool SDLCALL APP_SYS_CheckAccelerometerOrientation(SDL_DisplayID displayId);

/**
 * System dependent version of APP_GetDisplayConfiguration
 *
 * @param displayId	The display to query
 *
 * @return the configuration orientation of this display.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_SYS_GetDisplayConfiguration(SDL_DisplayID displayId);

/**
 * System dependent version of APP_GetDisplayOrientation
 *
 * @param displayId	The display to query
 *
 * @return the orientation of this display.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_SYS_GetDisplayOrientation(SDL_DisplayID displayId);

/**
 * System dependent version of APP_GetDeviceOrientation
 *
 * @return the current device orientation.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_SYS_GetDeviceOrientation(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_SYS_DISPLAY_H__ */

