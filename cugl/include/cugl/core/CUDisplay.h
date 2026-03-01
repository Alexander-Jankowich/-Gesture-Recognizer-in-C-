//
//  CUDisplay.h
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
//  Because this is a singleton, there are no publicly accessible constructors
//  or intializers.  Use the static methods instead.
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
#ifndef __CU_DISPLAY_H__
#define __CU_DISPLAY_H__
#include <cugl/core/math/CUMat4.h>
#include <cugl/core/math/CURect.h>
#include <cugl/core/math/CUColor4.h>

namespace cugl {
    
/** The opaque graphics context for this display */   
class GraphicsContext;

/**
 * This class is a singleton representing the native display.
 *
 * The static methods of this class {@link start()} and {@link stop()} the
 * SDL video system.  Without it, you cannot draw anything. This should be
 * the first and last methods called in any application. The {@link Application}
 * class does this for you automatically.
 *
 * The primary purpose of the display object is to initialize (and dispose)
 * the graphics context. Any start-up features for your graphics system (e.g.
 * OpenGL or Vulkan) should go in this class. They are set via the flags sent
 * to the method {@link #start()}. 
 *
 * The singleton display object also has several methods to get the (current)
 * screen resolution and aspect ratio. The most important of these two is the
 * the aspect ratio. Aspect ratio is one of the most unportable parts of
 * cross-platform development. Everything else can be scaled to the screen,
 * but the aspect ratio is fixed from the very beginning. With that said, 
 * applications on desktop platforms may support window resizing.
 *
 * The singleton display also has information about the display and device
 * orientation for mobile devices. In fact, is is possible to assign a
 * listener to the object to respond to changes in device orientation.
 *
 * If the device has multiple displays, this singleton will only refer to the
 * main display.
 */
class Display {
public:
    /**
     * The possible device/display orientations.
     *
     * We use the same orientations for device and display even though these
     * may not always agree (such as when the user has locked the display)
     */
    enum class Orientation : unsigned int {
        /**
         * The orientation is unknown.
         *
         * This is rarely ever reported, and may mean an issue with the
         * accelerometer in the case of a mobile device.
         */
        UNKNOWN = 0,
        /**
         * Landscape orientation with the right side up.
         *
         * One notched devices, this will put the notch to the left.  On devices
         * with a home button, the button will be to the right.
         */
        LANDSCAPE = 1,
        /**
         * Standard portrait orientation
         *
         * One notched devices, this will put the notch to the top.  On devices
         * with a home button, the button will be to the bottom.
         */
        PORTRAIT  = 2,
        /**
         * Landscape orientation with the left side up.
         *
         * One notched devices, this will put the notch to the right.  On devices
         * with a home button, the button will be to the left.
         */
        LANDSCAPE_REVERSED = 3,
        /**
         * Reversed portrait orientation
         *
         * One notched devices, this will put the notch to the bottom.  On devices
         * with a home button, the button will be to the top.
         *
         * Many devices (e.g. iPhones) do not allow this mode as it interferes
         * with the camera and incoming calls.
         */
        UPSIDE_DOWN = 4
    };
    
    /**
     * @typedef Listener
     *
     * This type represents a listener for an orientation change.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not as
     * objects. For simplicity, Displays only have a single listener that handles
     * both display and device changes (see {@link getDisplayOrientation()} and
     * {@link getDeviceOrientation()}.) If you wish for more than one listener,
     * then your listener should handle its own dispatch.
     *
     * Unlike other events, this callback will be invoked at the end of an
     * animation frame, after the screen has been drawn. So it will be
     * processed before any input events waiting for the next frame. If you
     * wish for more than one listener, then your listener should handle its
     * own dispatch.
     *
     * The function type is equivalent to
     *
     *      std::function<void(Orientation previous, Orientation current)>
     *
     * @param previous  The previous device orientation (before the change)
     * @param current   The current device orientation (after the change)
     */
    typedef std::function<void(Orientation previous, Orientation current)> Listener;
    
    // Flags for the device initialization
    /** Whether this display should use the fullscreen */
    static Uint32 INIT_FULLSCREEN;
    /** Whether this display should be centered (on windowed screens) */
    static Uint32 INIT_CENTERED;
    /** Whether this display resizable (incompatible with full screen) */
    static Uint32 INIT_RESIZABLE;
    /** Whether this display should support a High DPI screen */
    static Uint32 INIT_HIGH_DPI;
    /** Whether this display should support the camera (EXPERIMENTAL) */
    static Uint32 INIT_CAMERA;
    /** Whether this display should have VSync enabled */
    static Uint32 INIT_VSYNC;
    /** Whether this display should be multisampled */
    static Uint32 INIT_MULTISAMPLED;
    /** Whether to support points larger than one pixel (Vulkan only) */
    static Uint32 INIT_LARGE_POINTS;
    /** Whether to support points lines wider than one pixel (Vulkan only) */
    static Uint32 INIT_WIDE_LINES;
    
#pragma mark Values
protected:
    /** The display singleton */
    static Display* _thedisplay;

    /** The title (Window name) of the display */
    std::string _title;
    
    /** The SDL window, which provides the OpenGL drawing context */
    SDL_Window*   _window;
    /** The display index (for multiscreen displays) */
    SDL_DisplayID _display;
    /** The associated graphics context */
    GraphicsContext* _context;

    /** Whether we are using full screen */
    bool _fullscreen;
    /** The  bounds for this display with respect to the device */
    Rect _frame;
    /** The drawable bounds in pixels */
    Rect _drawable;
    /** 
     * The safe area bounds in pixels
     * 
     * The safe area consist of the drawing area minus regions that should not
     * be drawn over, like notches, menu bars, and home regions.
     */
    Rect _usable;
    /**
     * The pixel density of the device
     *
     * This value is the ratio of the display size to the drawable size. 
     */
    float _scale;
    
    /** Whether this device has a notch in it */
    bool _notched;
    
    /** A listener for the orientation */
    Listener _orientationListener;
    /** The value of the initial orientation */
    Orientation _initialOrientation;
    /** The value of the configuration orientation */
    Orientation _configOrientation;
    /** The value of the display orientation */
    Orientation _windowOrientation;
    /** The value of the device orientation */
    Orientation _deviceOrientation;


#pragma mark -
#pragma mark Constructors
    /**
     * Creates a new, unitialized Display.
     *
     * All of the values are set to 0 or UNKNOWN, depending on their type. You
     * must initialize the Display to access its values.
     *
     * WARNING: This class is a singleton.  You should never access this
     * constructor directly.  Use the {@link start()} method instead.
     */
    Display();
    
    /**
     * Initializes the display with the current screen information.
     *
     * This method creates a display with the given title and bounds. As part
     * of this initialization, it will create the graphics context, using the 
     * flags provided.  The bounds are ignored if the display is fullscreen.
     * In that case, it will use the bounds of the display.
     *
     * This method gathers the native resolution bounds, pixel density, and
     * orientation using platform-specific tools.
     *
     * WARNING: This class is a singleton. You should never access this
     * initializer directly.  Use the {@link start()} method instead.
     *
     * @param title     The window/display title
     * @param bounds    The window/display bounds
     * @param flags     The initialization flags
     *
     * @return true if initialization was successful.
     */
    bool init(std::string title, Rect bounds, Uint32 flags);
    
    /**
     * Uninitializes this object, releasing all resources.
     *
     * This method quits the SDL video system and disposes the graphics context,
     * effectively exitting and shutting down the entire program.
     *
     * WARNING: This class is a singleton. You should never access this
     * method directly.  Use the {@link stop()} method instead.
     */
    void dispose();
    
    /**
     * Deletes this object, releasing all resources.
     *
     * This method quits the SDL video system and disposes the graphics context,
     * effectively exitting and shutting down the entire program.
     *
     * WARNING: This class is a singleton. You should never access this
     * destructor directly.  Use the {@link stop()} method instead.
     */
    ~Display() { dispose(); }
    
#pragma mark -
#pragma mark Static Accessors
public:
    /**
     * Starts up the SDL display and video system.
     *
     * This static method needs to be the first line of any application, though
     * it is handled automatically in the {@link Application} class.
     *
     * This method creates the display with the given title and bounds. As part
     * of this initialization, it will create the graphics context, using the
     * flags provided. The bounds are ignored if the display is fullscreen.
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
    static bool start(std::string title, Rect bounds, Uint32 flags);

    /**
     * Shuts down the SDL display and video system.
     *
     * This static method needs to be the last line of any application, though
     * it is handled automatically in the {@link Application} class. It will
     * dipose of the display and the graphics context.
     *
     * Once this method is called, the {@link get()} method will return nullptr.
     * More importantly, no SDL function calls will work anymore.
     */
    static void stop();
    
    /**
     * Returns the singleton instance for the display
     *
     * You must call this static method first to get information about your
     * specific display. This method will return nullptr until {@link start()}
     * is called first.
     *
     * @return the singleton instance for the display
     */
    static Display* get() { return _thedisplay; }
    
#pragma mark -
#pragma mark Window Management
    /**
     * Returns the title of this display
     *
     * On a desktop, this title will be displayed at the top of the window.
     *
     * @return the title of this display
     */
    std::string getTitle() const { return _title; }
    
    /**
     * Sets the title of this display
     *
     * On a desktop, the title will be displayed at the top of the window.
     *
     * @param title  The title of this display
     */
    void setTitle(const std::string title);
    
    /**
     * Shows the window for this display (assuming it was hidden).
     *
     * This method does nothing if the window was not hidden.
     */
    void show();

    /**
     * Hides the window for this display (assuming it was visible).
     *
     * This method does nothing if the window was not visible.
     */
    void hide();

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
    Vec2 getMouseWindowPosition(float x, float y);

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
    Vec2 getMouseScreenPosition(float x, float y);

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
    Vec2 getTouchWindowPosition(float x, float y);

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
    Vec2 getTouchScreenPosition(float x, float y);

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
    Vec2 getTouchWindowValue(float x, float y);

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
    Vec2 getTouchScreenValue(float x, float y);
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns a description of the graphics API for this display.
     *
     * For example, if this application is backed by OpenGL, this method
     * returns the OpenGL version. For Vulkan, it returns the Vulkan version.
     * If it is a headless client, it returns "Headless".
     *
     * @return a description of the graphics API for this display.
     */
    const std::string getGraphicsDescription() const;
    
    /**
     * Returns the graphics context for this display.
     *
     * The graphics context is an opaque type storing the information that is
     * needed for the active graphics API (e.g. OpenGL, Vulkan, or Headless)
     * This is only exposed for internal debugging purposes.
     *
     * @return athe graphics context for this display.
     */
    GraphicsContext* getGraphicsContext() const { return _context; }
    
    /** 
     * Returns the refresh rate of the display.
     *
     * If the rate is unspecified, this will return 0.
     *
     * @return the refresh rate of the display.
     */
     float getRefreshRate() const;

    /**
     * Returns the bounds of the external frame for this display.
     *
     * If the display is windowed (as opposed to fullscreen), this method will
     * return the bounds of that window on the screen itself. For fullscreen
     * Display objects, this will return the dimensions of the screen.
     *
     * Note that the units reported by this method are often in points, not
     * pixels. So on Retina display Apple devices, the dimensions of this
     * frame could be half, or even a third, of the {@link #getViewBounds}.
     *
     * In addition, this value is reported using the screen coordinate system.
     * In this coordinate system, the origin is often the top-right.
     *
     * @return the bounds of the external frame for this display.
     */
    const Rect& getFrameBounds() const { return _frame;   }

    /**
     * Resets the the bounds of the external frame of this display.
     *
     * This method will do nothing if the display is fullscreen. Otherwise,
     * it will attempt to resize the window to the given dimensions. Not that
     * the units used by this method are often in points, not pixels. So on
     * Retina display Apple devices, the dimensions of this frame could be
     * half, or even a third, of the {@link #getViewBounds}.
     *
     * In addition, the bounds are specified using the screen coordinate system.
     * In this coordinate system, the origin is often the top-right.
     *
     * @param bounds    The new frame bounds
     *
     * @return true if the bound were successfully updated.
     */
    bool setFrameBounds(const Rect& bounds);

    /**
     * Resizes the external frame of this display.
     *
     * This method will do nothing if the display is fullscreen. Otherwise,
     * it will attempt to resize the window to the given dimensions. Not that
     * the units used by this method are often in points, not pixels. So on
     * Retina display Apple devices, the dimensions of this frame could be
     * half, or even a third, of the {@link #getViewBounds}.
     *
     * @param width     The new frame width
     * @param height    The new frame height
     *
     * @return true if the window was successfully resized
     */
    bool setFrameSize(Uint32 width, Uint32 height);

    /**
     * Resizes the external frame of this display.
     *
     * This method will do nothing if the display is fullscreen. Otherwise,
     * it will attempt to resize the window to the given dimensions. Not that
     * the units used by this method are often in points, not pixels. So on
     * Retina display Apple devices, the dimensions of this frame could be
     * half, or even a third, of the {@link #getViewBounds}.
     *
     * @param size  The new frame size
     *
     * @return true if the window was successfully resized
     */
    bool setFrameSize(const Size& size) {
        return setFrameSize(size.width,size.height);
    }
    
    /**
     * Repositions the external frame of this display
     *
     * This method will do nothing if the display is fullscreen. Otherwise,
     * it will attempt to reposition the window to the given coordinates. Note
     * that the units used by this method are often in points, not pixels. So
     * on Retina display Apple devices, the dimensions of this frame could be
     * half, or even a third, of the {@link #getViewBounds}. In addition,
     * in the screen coordinate system, the origin is often the top-right.
     *
     * @param x The x-coordinate of the frame
     * @param y The x-coordinate of the frame
     *
     * @return true if the window was successfully repositioned
     */
    bool setFrameOrigin(Uint32 x, Uint32 y);

    /**
     * Repositions the external frame of this display
     *
     * This method will do nothing if the display is fullscreen. Otherwise,
     * it will attempt to reposition the window to the given coordinates. Note
     * that the units used by this method are often in points, not pixels. So
     * on Retina display Apple devices, the dimensions of this frame could be
     * half, or even a third, of the {@link #getViewBounds}. In addition,
     * in the screen coordinate system, the origin is often the top-right.
     *
     * @param origin    The frame origin
     *
     * @return true if the window was successfully repositioned
     */
    bool setFrameOrigin(const Vec2& origin) {
        return setFrameOrigin(origin.x,origin.y);
    }

    /**
     * Returns the drawable bounds of this display in pixels.
     *
     * This method returns the internal pixel bounds of the Display window.
     * The origin of this rectangle will always be at (0,0), indicating the 
     * bottom left corner of the window. Note the {@link #getPixelDensity} is
     * the ratio of this size relative to the size of {@link #getFrameBounds}.
     *
     * @return the drawable bounds of this display in pixels.
     */
    const Rect& getViewBounds() const { return _drawable;   }
    
    /**
     * Returns the usable bounds of this display in pixels.
     *
     * Usable is a subjective term defined by the operating system. In general,
     * it means the full screen minus any space used by important user interface 
     * elements, like a status bar (iPhone), menu bar (macOS), or task bar 
     * (Windows), or a notch (modern phones). 
     *
     * The value returned will always be a (potentially unproper) subrectangle 
     * of {@link #getViewBounds}.
     *
     * @return the usable bounds of this display in pixels.
     */
    const Rect& getSafeBounds() const { return _usable; }
    
    /**
     * Returns the number of pixels for each point.
     *
     * A point is a logical screen pixel. If you are using a traditional
     * display, points and pixels are the same.  However, on Retina displays
     * and other high dpi monitors, they may be different. In particular,
     * the number of pixels per point is a scaling factor times the point.
     *
     * This value is defined as the ratio of {@link #getViewBounds} to
     * {@link #getFrameBounds}. In practice, you should not need to use this
     * scaling factor for anything other than determining if the high dpi
     * setting is in use.
     *
     * @return the number of pixels for each point.
     */
    float getPixelDensity() const { return _scale; }
    
    /**
     * Returns true if this device has a notch.
     *
     * Notched devices are edgeless smartphones or tablets that include at
     * dedicated area in the screen for a camera.  Examples include the
     * iPhone X.
     *
     * If a device is notched you should call {@link getSafeBounds()} before
     * laying out UI elements. It is acceptable to animate and draw backgrounds
     * behind the notch, but it is not acceptable to place UI elements outside
     * of these bounds.
     *
     * @return true if this device has a notch.
     */
    bool hasNotch() const { return _notched; }

#pragma mark -
#pragma mark Orientation
    /**
     * Returns the display configuration orientation.
     *
     * Typically, this value is the display orientation at startup. The display 
     * orientation is the orientation of the coordinate space for drawing on a 
     * mobile device. In other words, the origin is at the bottom left of the 
     * screen in this device orientation.
     *
     * In some rare cases this value can change. In particular, it can change
     * on Android devices using large screen behavior in Android 15+. Those
     * devices cannot lock orientation, and autorotation may cause the 
     * configuration (and window size!) to change.
     *
     * The display orientation may or may not agree with the device orientation.  
     * In particular, it will not agree if the display orientation is locked 
     * (to say portrait or landscape only). However, this is the orientation 
     * that is important for drawing to the screen. To get the device 
     * orientation, call {@link getDeviceOrientation()}.
     *
     * @return the initial display orientation.
     */
    Orientation getConfiguredOrientation() const { return _configOrientation; }
    
    /**
     * Returns the initial display orientation.
     *
     * This value is the display orientation at startup. The display orientation
     * is the orientation of the coordinate space for drawing on a mobile
     * device. In other words, the origin is at the bottom left of the screen
     * in this device orientation.
     *
     * The display orientation may or may not agree with the device orientation.  
     * In particular, it will not agree if the display orientation is locked 
     * (to say portrait or landscape only). However, this is the orientation 
     * that is important for drawing to the screen. To get the device 
     * orientation, call {@link getDeviceOrientation()}.
     *
     * @return the initial display orientation.
     */
    Orientation getInitialOrientation() const { return _initialOrientation; }
    
    /**
     * Returns the current display orientation.
     *
     * This value is the current display orientation. The display orientation
     * is the orientation of the coordinate space for drawing on a mobile
     * device. In other words, the origin is at the bottom left of the screen
     * in this device orientation.
     *
     * The display orientation may or may not agree with the device orientation.  
     * In particular, it will not agree if the display orientation is locked 
     * (to say portrait or landscape only). However, this is the orientation 
     * that is important for drawing to the screen. To get the device 
     * orientation, call {@link getDeviceOrientation()}.
     *
     * @return the current display orientation.
     */
    Orientation getDisplayOrientation() const { return _windowOrientation; }
    
    /**
     * Returns the current device orientation.
     *
     * The device orientation is the orientation of a mobile device, as held
     * by the user. It may or may not agree with the display orientation,
     * which is how the screen is drawn. For example, if the screen is
     * locked to landscape orientation, it is still possible for the device
     * to have portrait orientation when it is held on its side.
     *
     * This method is useful when you want to switch game modes for a
     * different orientation (e.g. Powerpuff Girls Flipped Out).
     *
     * @return the current device orientation.
     */
    Orientation getDeviceOrientation() const  { return _deviceOrientation; }
    
    /**
     * Returns the natural orientation of this device.
     *
     * The natural orientation corresponds to the intended orientiation that 
     * this mobile device should be held. For devices with home buttons, this 
     * home button is always expected at the bottom. For the vast majority of 
     * devices, this means the intended orientation is Portrait.  However, some 
     * Samsung tablets have the home button oriented for Landscape.
     *
     * This is important because the accelerometer axis is oriented relative 
     * to the natural orientation. So a natural landscape device will have a 
     * different accelerometer orientation than a portrait device. Unfortunately,
     * this method can have unexpected results on Android 15+ devices with 
     * large-screen behavior. If you wish to actually determine if the
     * accelerometer axes are swapped, use {@link #hasSwappedAxes}
     *
     * @return the natural orientation of this device.
     */
    Orientation getNaturalOrientation() const;
    
    /**
     * Returns true if the accelerometer axes as swapped.
     *
     * By default, accelerometer axes assume that the user is holding the
     * device in portrait mode. So the x-axis is the short edge, while the
     * y-axis is the long edge. If this function returns true, these axes are
     * swapped so that the x-axis is the long edge, while the y-axis is the 
     * short edge.
     *
     * @return true if the accelerometer axes as swapped.
     */
    bool hasSwappedAxes() const;
    
    /**
     * Returns true if this display (not device) has a landscape orientation
     *
     * @return true if this display (not device)  has a landscape orientation
     */
    bool isLandscape() const;
    
    /**
     * Returns true if this display (not device)  has a portrait orientation
     *
     * @return true if this display (not device)  has a portrait orientation
     */
    bool isPortrait() const;
    
    /**
     * Returns true if this display has an orientation listener
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
     * The display may only have one orientation listener at a time.
     *
     * @return true if this display has an orientation listener
     */
    bool hasOrientationListener() const { return _orientationListener != nullptr; }
    
    /**
     * Returns the listener for the display orientation.
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
     * The display may only have one orientation listener at a time. If there is
     * no listener, this method returns nullptr.
     *
     * @return the orientation listener (if any) for this display
     */
    const Listener getOrientationListener() const { return _orientationListener; }
    
    /**
     * Sets the orientation listener for this display.
     *
     * This listener handles changes in either the device orientation (see
     * {@link getDeviceOrientation()} or the display orientation (see
     * {@link getDeviceOrientation()}. Since the device orientation will always
     * change when the display orientation does, this callback can easily safely
     * handle both. The boolean parameter in the callback indicates whether or
     * not a display orientation change has happened as well.
     *
     * A display may only have one orientation listener at a time.  If this
     * display already has an orientation listener, this method will replace it
     * for the once specified.
     *
     * Unlike other events, this listener will be invoked at the end of an
     * animation frame, after the screen has been drawn.  So it will be
     * processed before any input events waiting for the next frame.
     *
     * @param listener  The listener to use
     */
    void setOrientationListener(Listener listener) { _orientationListener = listener; }
    
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
    bool removeOrientationListener();

#pragma mark -
#pragma mark Virtual Keyboard
    /**
     * Enable text events on the given display.
     *
     * The method is used by {@link TextInput} to activate text input events.
     * On devices without keyboards, calling this method will typically create
     * a virtual keyboard on the display. You can dismiss it by calling
     * {@link #stopTextEvents}
     *
     * @return true if events were successfully enabled
     */
    bool startTextEvents();

    /**
     * Disables text events on the given display.
     *
     * The method is used by {@link TextInput} to deactivate text input events.
     * If {@link #startTextEvents} created a virtual keyboard, this method will
     * dismiss it.
     *
     * @return true if events were successfully disabled
     */
    bool stopTextEvents();


#pragma mark -
#pragma mark Drawing Support
    /**
     * Locks the display, preventing any drawing commands.
     *
     * This method provides some safety with regard to shader usage.
     */
    void lock();

    /**
     * Unlocks the display, allowing drawing commands to proceed
     *
     * This method provides some safety with regard to shader usage.
     */
    void unlock();
    
    /**
     * Clears the screen to the given clear color.
     *
     * This method should be called before any user drawing happens.
     *
     * @param color The clear color
     */
    void clear(Color4f color);
    
    /**
     * Sets up the initial transform.
     *
     * This helper is called by {@link #init} to determine the transform.
     */
    void config();
    
    /**
     * Resynchronizes this object with the physical display.
     *
     * This method is called in response to a resizing event for windows that
     * can be resized. This may also adjust the configuration orientation.
     */
    void resync();
    
    /**
     * Refreshes the display.
     *
     * This method will swap the framebuffers, drawing the screen. This 
     * method should be called after any user drawing happens.
     *
     * It will also reassess the orientation state and call the listener as
     * necessary
     */
    void refresh();
    
    /** This is called by the application loop */
    friend class Application;
};
    
}

#endif /* __CU_DISPLAY_H__ */
