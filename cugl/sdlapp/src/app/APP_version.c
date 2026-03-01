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
const char* APP_GetVersion(APP_Depedency dep) {
    switch (dep) {
        case APP_DEPENDENCY_SDL:
            return "3.28.0";
        case APP_DEPENDENCY_IMG:
            return "3.2.5";
        case APP_DEPENDENCY_TTF:
            return "3.2.3";
        case APP_DEPENDENCY_ATK:
            return "3.2.0";
        case APP_DEPENDENCY_APP:
            return "3.2.0";
        default:
            break;
    }
    return "";
}
