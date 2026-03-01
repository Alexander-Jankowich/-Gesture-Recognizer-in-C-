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
#include "../APP_sysdevice.h"
#if defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_TVOS)

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <string.h>

#define MAX_SIZE 1024

/**
 * System dependent version of APP_GetDeviceName
 *
 * @return the name of this device
 */
const char* APP_SYS_GetDeviceName(void) {
    static char device_name[MAX_SIZE];

    NSString* result = [[UIDevice currentDevice] name];
    size_t len = strlen(result.UTF8String);
    if (len >= MAX_SIZE) { len = MAX_SIZE-1; }

    strncat(device_name,result.UTF8String,len);
    return device_name;
}

/**
 * System dependent version of APP_GetDeviceModel
 *
 * @return the model of this device
 */
const char* APP_SYS_GetDeviceModel(void) {
    static char device_model[MAX_SIZE];
    
    NSString* result = [[UIDevice currentDevice] model];
    size_t len = strlen(result.UTF8String);
    if (len >= MAX_SIZE) { len = MAX_SIZE-1; }

    strncat(device_model,result.UTF8String,len);
    return device_model;
}

/**
 * System dependent version of APP_GetDeviceOS
 *
 * @return the operating system running this device
 */
const char* APP_SYS_GetDeviceOS(void) {
    return "iOS";
}

/**
 * System dependent version of APP_GetDeviceOSVersion
 *
 * @return the operating system version of this device
 */
const char* APP_SYS_GetDeviceOSVersion(void) {
    static char os_version[64];
    
    NSString* result = [[UIDevice currentDevice] systemVersion];
    size_t len = strlen(result.UTF8String);
    if (len >= 64) { len = 63; }
    
    strncat(os_version,result.UTF8String,len);
    return os_version;
}

/**
 * System dependent version of APP_GetDeviceID
 *
 * @return a unique identifier for this device
 */
const char* APP_SYS_GetDeviceID(void) {
    static char device_id[MAX_SIZE];
    
    NSUUID* uuid = [[UIDevice currentDevice] identifierForVendor];
    NSString* result = uuid.UUIDString;
    
    size_t len = strlen(result.UTF8String);
    if (len >= MAX_SIZE) { len = MAX_SIZE-1; }
    
    strncat(device_id,result.UTF8String,len);
    return device_id;
}

#endif
