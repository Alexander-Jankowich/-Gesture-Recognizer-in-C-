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

/**
 *  \file SDL_app.h
 *
 *  \brief Include file for SDL App extensions used in CUGL
 *  \author Walker M. White
 */
#ifndef __SDL_APP_H__
#define __SDL_APP_H__
#include <SDL3/SDL.h>
#include <SDL3/SDL_begin_code.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


#pragma mark Mobile Display Utilities

/**
 * Acquires the safe area for for this window in pixels.
 *
 * While `SDL_GetWindowSafeArea` is a welcome addition to SDL3, it only gives
 * us units in points (particularly on Apple devices). This function is a 
 * a variation that is consistent with `SDL_GetWindowSizeInPixels`.
 *
 * Note that if {@link APP_GetPixelScale} is 1.0, then this function will agree
 * with `SDL_GetWindowSafeArea`. Like SDL_GetWindowSafeArea, this function 
 * assumes that the display origin is in the top left.
 *
 * @param window        The window to query
 * @param rect          Rectangle to store the window bounds
 *
 * @return true on success; false if window is invalid
 */
extern SDL_DECLSPEC bool SDLCALL APP_GetWindowSafeAreaInPixels(SDL_Window* window, SDL_Rect *rect);

/**
 * Returns true if this device has a notch
 *
 * Notched devices are edgeless smartphones or tablets that include a dedicated 
 * area in the screen for a camera. Examples include modern iPhones
 *
 * If a device is notched, then you should absolutely call the function
 * {@link APP_GetWindowSafeBoundsInPixels()} before laying out UI elements. It 
 * is acceptable to animate and draw backgrounds behind the notch, but it is 
 * not acceptable to place UI elements outside of these bounds.
 * 
 * @param displayId     The display to query
 *
 * @return true if this device has a notch
 */
extern SDL_DECLSPEC bool SDLCALL APP_CheckDisplayNotch(SDL_DisplayID displayId);

/**
 * Returns true if the accelerometer axes have the standard orientation.
 *
 * The vast majority of mobile devices have their accelerometer axes set up
 * relative to a portrait orientation. However, this is not required and some
 * older Android devices (like the Samsung Galaxy Tab S) have them oriented 
 * with respect to landscape orientation, meaning that the x and y axes are 
 * swapped. This function returns false in that case.
 *
 * This function returns true on non-mobile devices.
 * 
 * @param displayId     The display to query
 *
 * @return true if the accelerometer axes have the standard orientation.
 */
extern SDL_DECLSPEC bool SDLCALL APP_CheckAccelerometerOrientation(SDL_DisplayID displayId);

/**
 * Returns the configuration orientation of this display.
 *
 * For most devices this is the same as {@link SDL_GetCurrentDisplayOrientation}.
 * However, on more recent versions of Android, the configuration orientation
 * no longer necessarily matches the display orientation. The display 
 * orientation is the orientation of the window, while the configuration
 * orientation is the orientation of the screen. It is possible to have a
 * letter-boxed landscape display in a portrait configuration, and vice-versa.
 *
 * Note that the configuration orientation is always either 
 * {@link SDL_ORIENTATION_LANDSCAPE} or {@link SDL_ORIENTATION_PORTRAIT}. It
 * is never unknown and it is never flipped.
 * 
 * @param displayId	The display to query
 *
 * @return the configuration orientation of this display.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_GetDisplayConfiguration(SDL_DisplayID displayId);

/**
 * Returns the orientation of this display.
 *
 * This function is the same as {@link SDL_GetCurrentDisplayOrientation}.
 * It exists because that function reports incorrect orientations on Android
 * devices, particularly Android 15+ devices using large-screen behavior.
 * It is a patch fix and nothing more.
 * 
 * @param displayId	The display to query
 *
 * @return the orientation of this display.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_GetDisplayOrientation(SDL_DisplayID displayId);

/**
 * Returns the current device orientation.
 *
 * The device orientation is the orientation of a mobile device, as held by 
 * the user. This is not necessarily the same as the display orientation (as
 * returned by {@link SDL_GetDisplayOrientation}, as some applications may have
 * locked their display into a fixed orientation. Indeed, it is generally a bad
 * idea to let an OpenGL/Vulkan context auto-rotate when the device orientation
 * changes.
 *
 * The purpose of this function is to use device orientation as a (discrete)
 * control input while still permitting the graphics context to be locked. 
 *
 * If this display is not a mobile device, this function will always return
 * {@link SDL_ORIENTATION_UNKNOWN}. Note that this is different from
 * {@link SDL_GetDisplayOrientation}, which always has an orientation.
 *
 * @return the current device orientation.
 */
extern SDL_DECLSPEC SDL_DisplayOrientation SDLCALL APP_GetDeviceOrientation(void);

#pragma mark -
#pragma mark Device Information
/**
 * Returns the name of this device
 *
 * Typically this resolves to the user-visible name of this host (e.g. phone
 * or computer name). For Unix devices, this is the same as HOSTNAME. If the
 * value cannot be determined, this returns the empty string.
 *
 * @return the name of this device
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetDeviceName(void);

/**
 * Returns the model of this device
 *
 * This is device specific representation of the hardware model running this
 * application. On Unix systems, this is the equivalent of HW_MODEL passed
 * to sysctl. For mobile devices it is the vendor model. Note that the result
 * may be a code internal to the vendor (e.g. iPhone15,3).
 *
 * If the value cannot be determined, this returns "UNKNOWN"
 *
 * @return the model of this device
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetDeviceModel(void);

/**
 * Returns the operating system running this device
 *
 * This value returned will be a generic name like "Windows" or "macOS". For
 * the version, use {@link APP_GetDeviceOSVersion}. If the value cannot be 
 * determined, this returns "UNKNOWN"
 *
 * @return the operating system running this device
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetDeviceOS(void);

/**
 * Returns the operating system version of this device
 *
 * The value returned will typically be an identification number (in string
 * form) such as "13.6.5". However, the exact representation is platform
 * dependent. It the version cannot be determined, this function will return
 * "UNKNOWN".
 *
 * @return the operating system version of this device
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetDeviceOSVersion(void);

/**
 * Returns a unique identifier for this device
 *
 * The value returned will be what is known as a "vendor id". This means that
 * it is a pseudo-serial number that can be used to identify the device across
 * across multiple sessions. However, it is not (necessarily) a true serial
 * number. On some platforms (e.g. iOS) it is an identifier limited to
 * applications deployed by your developer account, and cannot be used to
 * identify a device cross-vendor. Furthermore, on some platforms it can be
 * changed by reseting the device to factory settings.
 *
 * There is no specific format for the string returned, as it varies from
 * platform to platform. For example, on iOS it is formal Version 4 UUID.
 * However, on Android this value is a 16 digit hexadecimal string. If it is
 * not possible to obtain a unique identifier, this function returns the empty
 * string.
 *
 * @return a unique identifier for this device
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetDeviceID(void);


#pragma mark -
#pragma mark Version Information

/** An enumeration of the libraries used by SDL_App */
typedef enum APP_Depedency {
    /** The core SDL library */
    APP_DEPENDENCY_SDL,
    /** The extension SDL_image */
    APP_DEPENDENCY_IMG,
    /** The extension SDL_ttf */
    APP_DEPENDENCY_TTF,
    /** The extension SDL_atk */
    APP_DEPENDENCY_ATK,
    /** The extension SDL_app */
    APP_DEPENDENCY_APP,
} APP_Depedency;

/**
 * Returns the version of the given dependency
 *
 * This allows the program to query the versions of the various libaries that
 * SDL_app depends on.
 *
 * @param dep   The library dependency
 *
 * @return the version of the given dependency
 */
extern SDL_DECLSPEC const char* SDLCALL APP_GetVersion(APP_Depedency dep);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include <SDL3/SDL_close_code.h>

#endif /* __SDL_APP_H__ */
