//
//  CUDisplay.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a singleton providing display information about the device.
//  Originally, we had made this part of Application. However, as we have
//  have diversified the graphics options away from OpenGL to include Vulkan
//  and headless, we needed to factor this code out.
//
//  The details of the display class are delegated to the specific graphics API.
//  That means that the cpp for this header is not in the core library. Instead,
//  it appears in the appropriate graphics package. This header is the bridge
//  between the graphics package and the core library.
//
//  This module is a headless implementation of Display for non-graphical
//  simulations. This module was created because students did not want to have
//  to a graphics API onto Docker containers when running a CUGL server.
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
//  Version: 12/2/25 (SDL3 Integration)
//
#include <cugl/core/CUBase.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/core/util/CUDebug.h>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUTexture.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/** The display singleton */
Display* Display::_thedisplay = nullptr;

// Flags for the device initialization
/** Whether this display should use the fullscreen */
Uint32 Display::INIT_FULLSCREEN   = 0x001;
/** Whether this display should be centered (on windowed screens) */
Uint32 Display::INIT_CENTERED     = 0x002;
/** Whether this display resizable (incompatible with full screen) */
Uint32 Display::INIT_RESIZABLE    = 0x004;
/** Whether this display should support a High DPI screen */
Uint32 Display::INIT_HIGH_DPI     = 0x008;
/** Whether this display should support the camera (EXPERIMENTAL) */
Uint32 Display::INIT_CAMERA       = 0x010;
/** Whether this display should have VSync enabled. */
Uint32 Display::INIT_VSYNC        = 0x020;
/** Whether this display should be multisampled */
Uint32 Display::INIT_MULTISAMPLED = 0x040;
/** Whether to support points larger than one pixel (Vulkan only) */
Uint32 Display::INIT_LARGE_POINTS = 0x080;
/** Whether to support points lines wider than one pixel (Vulkan only) */
Uint32 Display::INIT_WIDE_LINES   = 0x100;

#pragma mark -
#pragma mark Display Singleton
/**
 * Creates a new, unitialized Display.
 *
 * All of the values are set to 0 or UNKNOWN, depending on their type. You
 * must initialize the Display to access its values.
 *
 * WARNING: This class is a singleton.  You should never access this
 * constructor directly.  Use the {@link start()} method instead.
 */
Display::Display() :
_window(nullptr),
_display(0),
_context(nullptr),
_fullscreen(false),
_configOrientation(Orientation::UNKNOWN),
_windowOrientation(Orientation::UNKNOWN),
_deviceOrientation(Orientation::UNKNOWN) {}

/**
 * Initializes the display with the current screen information.
 *
 * This method creates a display with the given title and bounds. As part
 * of this initialization, it will create the OpenGL context, using the
 * flags provided.  The bounds are ignored if the display is fullscreen.
 * In that case, it will use the bounds of the display.
 *
 * This method gathers the native resolution bounds, pixel density, and
 * orientation  using platform-specific tools.
 *
 * WARNING: This class is a singleton.  You should never access this
 * initializer directly.  Use the {@link start()} method instead.
 *
 * @param title     The window/display title
 * @param bounds    The window/display bounds
 * @param flags     The initialization flags
 *
 * @return true if initialization was successful.
 */
bool Display::init(std::string title, Rect bounds, Uint32 flags) {
    _display = 0;
    SDL_Rect native;
    
    // Still doing everything, but no window
    SDL_InitFlags systems = SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
    systems = systems | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC | SDL_INIT_EVENTS | SDL_INIT_SENSOR;
    if (flags & INIT_CAMERA) {
        systems |= SDL_INIT_CAMERA;
    }
    
    if (!SDL_Init(systems)) {
        CULogError("Could not initialize display: %s",SDL_GetError());
        return false;
    }
    
    // Initialize the TTF library
    if (!TTF_Init()) {
        CULogError("Could not initialize TTF: %s",SDL_GetError());
        return false;
    }
    
    // Assume native resolution is 1080p
    native.x = 0;
    native.y = 0;
    native.w = 1920;
    native.h = 1080;
    
    if (flags & INIT_FULLSCREEN) {
        _fullscreen = true;
        _frame.origin.x = native.x;
        _frame.origin.y = native.y;
        _frame.size.width  = native.w;
        _frame.size.height = native.h;
    } else {
        _fullscreen = false;
        if (flags & INIT_CENTERED) {
            _frame.origin.x = (native.w - bounds.size.width)/2;
            _frame.origin.y = (native.h - bounds.size.height)/2;
        }
        _frame.size = bounds.size;
    }
    resync();
    
    
    // Prevent this from happening in loading threads
    Texture::getBlank();
    
    if (_frame.size.width >= _frame.size.height) {
        _configOrientation = Display::Orientation::LANDSCAPE;
        _windowOrientation = Display::Orientation::LANDSCAPE;
    } else {
        _configOrientation = Display::Orientation::PORTRAIT;
        _windowOrientation = Display::Orientation::PORTRAIT;
    }
    _deviceOrientation = Display::Orientation::LANDSCAPE;
    
    _context = new GraphicsContext();
    return true;
}

/**
 * Uninitializes this object, releasing all resources.
 *
 * This method quits the SDL video system and disposes the OpenGL context,
 * effectively exitting and shutting down the entire program.
 *
 * WARNING: This class is a singleton.  You should never access this
 * method directly.  Use the {@link stop()} method instead.
 */
void Display::dispose() {
    _fullscreen = false;
    _frame.size.set(0,0);
    _drawable.size.set(0,0);
    _usable.size.set(0,0);
    _scale = 0.0f;
    
    _configOrientation = Orientation::UNKNOWN;
    _windowOrientation = Orientation::UNKNOWN;
    _deviceOrientation  = Orientation::UNKNOWN;
    SDL_Quit();
}

#pragma mark -
#pragma mark Static Accessors
/**
 * Starts up the SDL display and video system.
 *
 * This static method needs to be the first line of any application, though
 * it is handled automatically in the {@link Application} class.
 *
 * This method creates the display with the given title and bounds. As part
 * of this initialization, it will create the OpenGL context, using
 * the flags provided.  The bounds are ignored if the display is fullscreen.
 * In that case, it will use the bounds of the display.
 *
 * Once this method is called, the {@link get()} method will no longer
 * return a null value.
 *
 * @param title     The window/display title
 * @param bounds    The window/display bounds
 * @param flags     The initialization flags
 *
 * @return true if the display was successfully initialized
 */
bool Display::start(std::string name, Rect bounds, Uint32 flags) {
    if (_thedisplay != nullptr) {
        CUAssertLog(false, "The display is already initialized");
        return false;
    }
    _thedisplay = new Display();
    return _thedisplay->init(name,bounds,flags);
}

/**
 * Shuts down the SDL display and video system.
 *
 * This static method needs to be the last line of any application, though
 * it is handled automatically in the {@link Application} class. It will
 * dipose of the display and the OpenGL context.
 *
 * Once this method is called, the {@link get()} method will return nullptr.
 * More importantly, no SDL function calls will work anymore.
 */
void Display::stop() {
    if (_thedisplay == nullptr) {
        CUAssertLog(false, "The display is not initialized");
    }
    delete _thedisplay;
    _thedisplay = nullptr;
}

#pragma mark -
#pragma mark Window Management
/**
 * Sets the title of this display
 *
 * On a desktop, the title will be displayed at the top of the window.
 *
 * @param title  The title of this display
 */
void Display::setTitle(const std::string title) {
    _title = title;
}

/**
 * Shows the window for this display (assuming it was hidden).
 *
 * This method does nothing if the window was not hidden.
 */
void Display::show() {
    // Does nothing
}

/**
 * Hides the window for this display (assuming it was visible).
 *
 * This method does nothing if the window was not visible.
 */
void Display::hide() {
    // Does nothing
}

#pragma mark -
#pragma mark Coordinate Mapping
/**
 * Returns the window position for the given mouse location.
 *
 * This method transforms the position to support Android large-display
 * behavior. A window position is always with respect to an origin in the
 * bottom-left corner.
 *
 * @param x The mouse x-coordinate
 * @param y The mouse y-coordinate
 *
 * @return the window position for the given mouse location.
 */
Vec2 Display::getMouseWindowPosition(float x, float y) {
    Vec2 result(x*_scale,y*_scale);
    result.y = _drawable.size.height-result.y;
    return result;
}

/**
 * Returns the screen position for the given mouse location.
 *
 * This method transforms the position to support Android large-display
 * behavior. A screen position is always with respect to an origin in the
 * top-left corner.
 *
 * @param x The mouse x-coordinate
 * @param y The mouse y-coordinate
 *
 * @return the screen position for the given mouse location.
 */
Vec2 Display::getMouseScreenPosition(float x, float y) {
    Vec2 result(x*_scale,y*_scale);
    return result;
}

/**
 * Returns the window position for the given touch location.
 *
 * This method converts (x,y) from normalized coordinates to pixel
 * coordinates and transforms the result to support Android large-display
 * behavior. A window position is always with respect to an origin in the
 * bottom-left corner.
 *
 * @param x The touch x-coordinate
 * @param y The touch y-coordinate
 *
 * @return the window position for the given touch location.
 */
Vec2 Display::getTouchWindowPosition(float x, float y) {
    Vec2 result(x*_drawable.size.width,y*_drawable.size.height);
    result.y = _drawable.size.height-result.y;
    return result;
}

/**
 * Returns the screen position for the given touch location.
 *
 * This method converts (x,y) from normalized coordinates to pixel
 * coordinates and transforms the result to support Android large-display
 * behavior. A screen position is always with respect to an origin in the
 * top-left corner.
 *
 * @param x The touch x-coordinate
 * @param y The touch y-coordinate
 *
 * @return the screen position for the given touch location.
 */
Vec2 Display::getTouchScreenPosition(float x, float y) {
    Vec2 result(x*_drawable.size.width,y*_drawable.size.height);
    return result;
}

/**
 * Returns the normalized window position for the given touch location.
 *
 * This method keeps (x,y) in normalized coordinate but transforms the
 * result to support Android large-display behavior. A window position is
 * always with respect to an origin in the bottom-left corner.
 *
 * @param x The touch x-coordinate
 * @param y The touch y-coordinate
 *
 * @return the window position for the given touch location.
 */
Vec2 Display::getTouchWindowValue(float x, float y) {
    return Vec2(x,1-y);
}

/**
 * Returns the normalized window position for the given touch location.
 *
 * This method keeps (x,y) in normalized coordinate but transforms the
 * result to support Android large-display behavior. A screen position is
 * always with respect to an origin in the top-left corner.
 *
 * @param x The touch x-coordinate
 * @param y The touch y-coordinate
 *
 * @return the window position for the given touch location.
 */
Vec2 Display::getTouchScreenValue(float x, float y) {
    return Vec2(x,y);
}

#pragma mark -
#pragma mark Attributes
/**
 * Returns a description of this graphics API for this display.
 *
 * For example, if this application is backed by OpenGL, this method
 * returns the OpenGL version. If it is a headless client, it returns
 * "Headless".
 *
 * @return a description of this graphics API for this display.
 */
const std::string Display::getGraphicsDescription() const {
    return "Headless";
}

/**
 * Returns the refresh rate of the display.
 *
 * If the rate is unspecified, this will return 0.
 *
 * @return the refresh rate of the display.
 */
float Display::getRefreshRate() const {
    return 0.0f;
}

/**
 * Resets the the bounds of the external frame of this display.
 *
 * This method will do nothing if the display is fullscreen. Otherwise,
 * it will attempt to resize the window to the given dimensions. Not that
 * the units used by this method are often in points, not pixels. So on
 * Retina display Apple devices, the dimensions of this frame could be
 * half, or even a third, of the {@link #getDrawableBounds}.
 *
 * In addition, the bounds are specified using the screen coordinate system.
 * In this coordinate system, the origin is often the top-right.
 *
 * @param bounds    The new frame bounds
 *
 * @return true if the bound were successfully updated.
 */
bool Display::setFrameBounds(const Rect& bounds) {
    if (_fullscreen) {
        return false;
    }
    _frame = bounds;
    resync();
    return true;
}

/**
 * Resizes the external frame of this display.
 *
 * This method will do nothing if the display is fullscreen. Otherwise,
 * it will attempt to resize the window to the given dimensions. Not that
 * the units used by this method are often in points, not pixels. So on
 * Retina display Apple devices, the dimensions of this frame could be
 * half, or even a third, of the {@link #getDrawableBounds}.
 *
 * @param width     The new frame width
 * @param height    The new frame height
 *
 * @return true if the window was successfully resized
 */
bool Display::setFrameSize(Uint32 width, Uint32 height) {
    if (_fullscreen) {
        return false;
    }
    _frame.size.set(width, height);
    resync();
    return true;
}

/**
 * Repositions the external frame of this display
 *
 * This method will do nothing if the display is fullscreen. Otherwise,
 * it will attempt to reposition the window to the given coordinates. Note
 * that the units used by this method are often in points, not pixels. So
 * on Retina display Apple devices, the dimensions of this frame could be
 * half, or even a third, of the {@link #getDrawableBounds}. In addition,
 * in the screen coordinate system, the origin is often the top-right.
 *
 * @param x The x-coordinate of the frame
 * @param y The x-coordinate of the frame
 *
 * @return true if the window was successfully repositioned
 */
bool Display::setFrameOrigin(Uint32 x, Uint32 y) {
    if (_fullscreen) {
        return false;
    }
    // No need to resync here
    _frame.origin.set(x,y);
    return true;
}

#pragma mark -
#pragma mark Orientation
/**
 * Returns the natural orientation of this device.
 *
 * The natural orientation corresponds to the intended orientiation
 * that this mobile device should be held. For devices with home
 * buttons, this home button is always expected at the bottom. For
 * the vast majority of devices, this means the intended orientation
 * is Portrait.  However, some Samsung tablets have the home button
 * oriented for Landscape.
 *
 * This is important because the accelerometer axis is oriented relative
 * to the default orientation. So a default landscape device will have a
 * different accelerometer orientation than a portrait device.
 *
 * Note that this method can have unexpected results on Android 15+ devices
 * with large-screen behavior.
 *
 * @return the natural orientation of this device.
 */
cugl::Display::Orientation Display::getNaturalOrientation() const {
    return Display::Orientation::LANDSCAPE;
}

/**
 * Returns true if the accelerometer axes are swapped.
 *
 * By default, accelerometer axes assume that the user is holding the
 * device in portrait mode. So the x-axis is the short edge, while the
 * y-axis is the long edge. If this function returns true, these axes are
 * swapped so that the x-axis is the long edge, while the y-axis is the
 * short edge.
 *
 * @return true if the accelerometer axes as swapped.
 */
bool Display::hasSwappedAxes() const {
    return false;
}

/**
 * Returns true if this display (not device) has a landscape orientation
 *
 * @return true if this display (not device)  has a landscape orientation
 */
bool Display::isLandscape() const {
    return true;
}

/**
 * Returns true if this display (not device)  has a portrait orientation
 *
 * @return true if this display (not device)  has a portrait orientation
 */
bool Display::isPortrait() const {
    return false;
}

/**
 * Removes the display orientation listener for this display.
 *
 * This listener handles changes in either the device orientation (see
 * {@link getDeviceOrientation()} or the display orientation (see
 * {@link getDeviceOrientation()}. Since the device orientation will always
 * change when the display orientation does, this callback can easily safely
 * handle both. The boolean parameter in the callback indicates whether or
 * not a display orientation change has happened as well.
 *
 * Unlike other events, this listener will be invoked at the end of an
 * animation frame, after the screen has been drawn.  So it will be
 * processed before any input events waiting for the next frame.
 *
 * A display may only have one orientation listener at a time.  If this
 * display does not have an orientation listener, this method will fail.
 *
 * @return true if the listener was succesfully removed
 */
bool Display::removeOrientationListener() {
    bool result = _orientationListener != nullptr;
    _orientationListener = nullptr;
    return result;
}

#pragma mark -
#pragma mark Virtual Keyboard
/**
 * Enable text events on the given display.
 *
 * The method is used by {@link TextInput} to activate text input events.
 * On devices without keyboards, calling this method will typically create
 * a virtual keyboard on the display. You can dismiss it by calling
 * {@link #stopText}
 *
 * @return true if events were successfully enabled
 */
bool Display::startTextEvents() {
    return false;
}

/**
 * Disables text events on the given display.
 *
 * The method is used by {@link TextInput} to deactivate text input events.
 * If {@link #startText} created a virtual keyboard, this method will
 * dismiss it.
 *
 * @return true if events were successfully disabled
 */
bool Display::stopTextEvents() {
    return false;
}

#pragma mark -
#pragma mark Drawing Support
/**
 * Locks the display, preventing any drawing commands.
 *
 * This method provides some safety with regard to shader usage.
 */
void Display::lock() {
    // No-op in headless
}

/**
 * Unlocks the display, allowing drawing commands to proceed
 *
 * This method provides some safety with regard to shader usage.
 */
void Display::unlock() {
    // No-op in headless
}

/**
 * Clears the screen to the given clear color.
 *
 * This method should be called before any user drawing happens.
 *
 * @param color The clear color
 */
void Display::clear(Color4f color) {
    // Does nothing
}

/**
 * Refreshes the display.
 *
 * This method will swap the framebuffers, drawing the screen. This
 * method should be called after any user drawing happens.
 *
 * It will also reassess the orientation state and call the listener as
 * necessary
 */
void Display::refresh() {
    // Does nothing
}

/**
 * Resynchronizes this object with the physical display.
 *
 * This method is called in response to a resizing event for windows that
 * can be resized. This may also adjust the configuration orientation.
 */
void Display::resync() {
    _drawable.origin.setZero();
    _drawable.size = _frame.size;
    _usable   = _drawable;
    _scale = 1.0f;
}
