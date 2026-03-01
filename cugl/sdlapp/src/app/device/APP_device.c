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
#include <SDL3/SDL_app.h>
#include "APP_sysdevice.h"

/**
 * Returns the name of this device
 *
 * Typically this resolves to the user-visible name of this host (e.g. phone
 * or computer name). However, this is not guaranteed. In some cases it may
 * resolve to the name of the volume on which this application is being run.
 *
 * @return the name of this device
 */
const char* APP_GetDeviceName(void) {
    return APP_SYS_GetDeviceName();
}

/**
 * Returns the model of this device
 *
 * This is device specific representation of the hardware model running this
 * application. On Unix systems, this is the equivalent of HW_MODEL passed
 * to sysctl. For mobile devices it is the vender model.
 *
 * @return the model of this device
 */
const char* APP_GetDeviceModel(void) {
    return APP_SYS_GetDeviceModel();
}

/**
 * Returns the operating system running this device
 *
 * This value returned will be a generic name like "Windows" or "macOS". For
 * the version, use {@link APP_GetDeviceOSVersion}.
 *
 * @return the operating system running this device
 */
const char* APP_GetDeviceOS(void) {
    return APP_SYS_GetDeviceOS();
}

/**
 * Returns the operating system version of this device
 *
 * The value returned will simply be an identification number (in string form)
 * such as "13.6.5". It the version cannot be determined, this function will
 * return "UNKNOWN".
 *
 * @return the operating system version of this device
 */
const char* APP_GetDeviceOSVersion(void) {
    return APP_SYS_GetDeviceOSVersion();
}

/**
 * Returns a unique identifier for this device
 *
 * Whenever possible, this function will return the serial number for this
 * device. When that is not possible (e.g. Linux without root access), it
 * will attempt to return the serial number of the volume running this
 * application instead. If this is not possible, this function will return
 * the empty string.
 *
 * @return a unique identifier for this device
 */
const char* APP_GetDeviceID(void)  {
    return APP_SYS_GetDeviceID();
}
