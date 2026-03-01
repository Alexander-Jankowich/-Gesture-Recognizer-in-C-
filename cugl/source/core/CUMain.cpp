//
//  CUMain.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the glue that connects SDL3 to the Application class.
//  it is separated from that file to solve linking issues that occur in
//  Windows.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 11/23/25 (SDL3 Integration)
//
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <cugl/core/CUApplication.h>

using namespace cugl;

/**
 * Initialize the application object and calls the start-up method.
 *
 * To work properly, this method should be invoked after CU_ROOTCLASS has been
 * used to register the proper subclass.
 *
 * @param appstate  Pointer to store the appstate (e.g. the Application object)
 * @param argc      The number of command line arguments (unused)
 * @param argv      The command line arguments (unused)
 *
 * @return SDL_APP_CONTINUE on successful initialization
 */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    return app_init(appstate,argc,argv);
}

/**
 * Processes an input event for the application
 *
 * This may be called zero or more times before each invocation of
 * SDL_AppIterate.
 *
 * @param appstate  A reference to the application object
 * @param event     The event to process
 *
 * @return the app running state
 */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    return app_event(appstate, event);
}

/**
 * Processes a single update/draw loop step of the application.
 *
 * @param appstate  A reference to the application object
 *
 * @return the app running state
 */
SDL_AppResult SDL_AppIterate(void *appstate) {
    return app_step(appstate);
}

/**
 * Cleanly shuts down the application
 *
 * @param appstate  A reference to the application object
 * @param result    The shutdown category (unused)
 */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    app_quit(appstate, result);
}
