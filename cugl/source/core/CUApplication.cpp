//
//  CUApplication.cpp
//  Cornell University Game Library (CUGL)
//
//  This class provides the core application class. In initializes both the
//  SDL and CUGL settings, and creates the core loop. You should inherit from
//  this class to make your root game class.
//
//  Because SDL3 now utilizes a callback model, you should not allocate any
//  instances of this class (or even subclasses). Instead, you should use
//  the macro CU_ROOTCLASS to register your subclass as the main application
//  driver.
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
#include <cugl/core/CUBase.h>
#include <cugl/core/CUApplication.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/core/input/CUInput.h>
#include <cugl/core/util/CUDebug.h>
#include <algorithm>
#include <vector>

/** The default screen width */
#define DEFAULT_WIDTH   1024
/** The default screen height */
#define DEFAULT_HEIGHT  576
/** The default smoothing window for fps calculation */
#define FPS_WINDOW      10
/** The default number of files for streaming */
#define STREAM_LIMIT	8

using namespace cugl;

#pragma mark -
#pragma mark Application Class
/**
 * Creates a degnerate application with no graphics context
 *
 * You must initialize the application to use it. However, you may
 * set any of the attributes before initialization.
 */
Application::Application() :
_name("CUGL Game"),
_version("1.0"),
_creator("GDIAC"),
_org("GDIAC"),
_copyright("MIT License"),
_url("https://gdiac.cs.cornell.edu"),
_savesdir(""),
_assetdir(""),
_state(State::NONE),
_fullscreen(false),
_resizable(false),
_highdpi(true),
_fps(0),
_vsync(true),
_funcid(0),
_fixstep(0),
_fixedCounter(0),
_fixedRemainder(0),
_fixed(false),
_clearColor(Color4f::CORNFLOWER) // Ah, XNA
{
    _display.size.set(DEFAULT_WIDTH,DEFAULT_HEIGHT);
    setFPS(60.0f);
    
#if (CU_PLATFORM == CU_PLATFORM_IPHONE || CU_PLATFORM == CU_PLATFORM_ANDROID)
    _fullscreen = true;
#endif

#if (CU_PLATFORM == CU_PLATFORM_WINDOWS || CU_PLATFORM == CU_PLATFORM_MACOS)
    _multisamp = false;
#else
    _multisamp = false;
#endif
    
}

/**
 * Disposes all of the resources used by this application.
 *
 * A disposed Application has no graphics context, and cannot be used.
 * However, it can be safely reinitialized.
 */
void Application::dispose() {
    _name = "CUGL Game";
    _state = State::NONE;
    _display.set(0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT);
    _highdpi = true;
    _fpswindow.clear();
    _fixed = false;
    _fixstep = 0;
    _fps = 0;
    _fixedCounter = 0;
    _fixedRemainder = 0;
    _clearColor = Color4f::CORNFLOWER;
    setFPS(60.0f);
}

/**
 * Returns the current running application
 *
 * There can only be one application running at a time. While this should
 * never happen, this method will return nullptr if there is no application.
 *
 * @return the current running application
 */
Application* Application::get() {
    static std::once_flag once;
    static std::unique_ptr<Application> ptr;
    std::call_once(once, []{ ptr = factoryRef()(); });
    return ptr.get();
}


/**
 * Initializes this application, creating an graphics context.
 *
 * The initialization will use the current value of all of the attributes,
 * like application name, orientation, and size.  These values should be
 * set before calling init().
 *
 * CUGL only supports one application running at a time.  This method will
 * fail if there is another application object.
 *
 * You should not override this method to initialize user-defined attributes.
 * Use the method onStartup() instead.
 *
 * @return true if initialization was successful.
 */
bool Application::init() {
    _state = State::STARTUP;

    // Set the basic metadata
    if (!SDL_SetAppMetadata(_name.c_str(), _version.c_str(), _appid.c_str())) {
        SDL_Log("SDL_AppInit: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Configure additional metadata
    bool success = true;
    success = success && SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, _url.c_str());
    success = success && SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, _creator.c_str());
    success = success && SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, _copyright.c_str());
    success = success && SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    if (!success) {
        SDL_Log("SDL_AppInit: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Initializate the video
    Uint32 flags = Display::INIT_CENTERED;
    if (_fullscreen) {
        flags |= Display::INIT_FULLSCREEN;
    }
    if (_highdpi) {
        flags |= Display::INIT_HIGH_DPI;
    }
    if (_multisamp) {
        flags |= Display::INIT_MULTISAMPLED;
    }
    if (_resizable) {
        flags |= Display::INIT_RESIZABLE;
    }
    if (_vsync) {
        flags |= Display::INIT_VSYNC;
    }
    
    if (!Display::start(_name,_display,flags)) {
        return false;
    }

    _display  = Display::get()->getFrameBounds();
    _drawable = Display::get()->getViewBounds();
    _safearea = Display::get()->getSafeBounds();
    
    float rate = Display::get()->getRefreshRate();
    if (_fps == 0) {
        setFPS(rate > 0 ? rate : 60.0f);
    }
    
    if (_fixstep == 0) {
        _fixstep = 1000000/_fps;
    }
    
    _fpswindow.resize(FPS_WINDOW,1.0f/_fps);
	ATK_init(STREAM_LIMIT);
    Input::start();
    _bootup.mark();
    _state = State::STARTUP;
    
    _fixedCounter = 0;
    _fixedRemainder = 0;
    Display::get()->show();
    return true;
}


#pragma mark -
#pragma mark Configuration Methods
/**
 * Sets the name of this application
 *
 * On a desktop, the name will be displayed at the top of the window. The
 * name also defines the preferences directory -- the place where it is
 * safe to write save files.
 *
 * This method is intended to be set at the time the application is created.
 * It cannot be changed once {@link #onStartUp} is called.
 *
 * @param name  The name of this application
 */
void Application::setName(const std::string name) {
    CUAssertLog(_state == State::NONE, "Cannot update name after application start-up");
    _name = name;
    Display* display = Display::get();
    if (display != nullptr) {
        display->setTitle(name);
    }
    _savesdir.clear();
}

/**
 * Sets the version for this application
 *
 * This version is simply used for metadata. If it is not set, then 1.0
 * will be set as the version. This method is intended to be set at the
 * time the application is created. It cannot be changed once
 * {@link #onStartUp} is called.
 *
 * @param value  The version for this application
 */
void Application::setVersion(const std::string value) {
    CUAssertLog(_state == State::NONE, "Cannot update version after application start-up");
    _version = value;
}

/**
 * Sets the appid for this application
 *
 * This should match the appid specified in the project configuration YML,
 * but this is not enforced. If it is not set, then it will just be
 * edu.cornell.gdiac. This method is intended to be set at the time the
 * application is created. It cannot be changed once {@link #onStartUp} is
 * called.
 *
 * @param value The appid for this application
 */
void Application::setAppId(const std::string value) {
    CUAssertLog(_state == State::NONE, "Cannot update appid after application start-up");
    _appid = value;
}

/**
 * Sets the creator name for this application
 *
 * This creator is simply used for metadata. If it is not set, then GDIAC
 * will be set as the creator. This method is intended to be set at the
 * time the application is created. It cannot be changed once
 * {@link #onStartUp} is called.
 *
 * @param name  The creator name for this application
 */
void Application::setCreator(const std::string name) {
    CUAssertLog(_state == State::NONE, "Cannot update creator after application start-up");
    _creator = name;
}

/**
 * Sets the organization name for this application
 *
 * This name defines the preferences directory -- the place where it is
 * safe to write save files. Applications of the same organization will
 * save in the same location.
 *
 * This method may be safely changed at any time while the application
 * is running.
 *
 * @param name  The organization name for this application
 */
void Application::setOrganization(const std::string name) {
    CUAssertLog(_state == State::NONE, "Cannot update organization after application start-up");
    _org = name;
    _savesdir.clear();
}

/**
 * Sets the copyright information for this application
 *
 * This copyright is simply used for metadata. If it is not set, then
 * MIT License will be set as the copyright. This method is intended to be
 * set at the time the application is created. It cannot be changed once
 * {@link #onStartUp} is called.
 *
 * @param value The copyright information for this application
 */
void Application::setCopyright(const std::string value) {
    CUAssertLog(_state == State::NONE, "Cannot update copyright after application start-up");
    _copyright = value;
}

/**
 * Sets the URL information for this application
 *
 * This URL is simply used for metadata. If it is not set, then the site
 *
 *        https://gdiac.cs.cornell.edu
 *
 * will be used. This method is intended to be set at the time the
 * application is created. It cannot be changed once {@link #onStartUp} is
 * called.
 *
 * @param value    The URL information for this application
 */
void Application::setURL(const std::string value) {
    CUAssertLog(_state == State::NONE, "Cannot update URL after application start-up");
    _url = value;
}

/**
 * Sets whether this application obeys the display refresh rate
 *
 * A vsync-enabled application will always match the refresh rate of the
 * display. Otherwise, the application will attempt to match the value of
 * {@link #getFPS()}, which could be faster than the refresh rate.
 *
 * Note that some platforms (notably macOS running an OpenGL backend) will
 * always use vsync no matter the settings. In that case, setting this value
 * to false will actually hurt the performance of your application. As a
 * general rule, it is best to set this value to true, and perform any
 * simulations that must be done at a faster rate in a separate thread.
 *
 * This method is intended to be set at the time the application is
 * created. It cannot be changed once {@link #onStartUp} is called.
 *
 * @param vsync Whether this application obeys the display refresh rate
 */
void Application::setVSync(bool vsync) {
    CUAssertLog(_state == State::NONE, "Cannot update vsync after application start-up");
    _vsync = vsync;
}

/**
 * Sets whether this application is running fullscreen
 *
 * On desktop/laptop platforms, going fullscreen will hide the mouse. The
 * mouse cursor is only visible in windowed mode.
 *
 * If this is false, the application will launch as a window centered in
 * the middle of the display.
 *
 * This method is intended to be set at the time the application is
 * created. It cannot be changed once {@link #onStartUp} is called.
 *
 * @param value  Whether this application is running fullscreen
 */
void Application::setFullscreen(bool value) {
    CUAssertLog(_state == State::NONE, "Cannot set application to fullscreen after start-up");
    _fullscreen = value;
}

/**
 * Sets whether the application window is resizable
 *
 * If true, the user can resize the window simply by dragging it. This
 * setting is ignored by for fullscreen applications.
 *
 * This method is intended to be set at the time the application is
 * created. It cannot be changed once {@link #onStartUp} is called.
 *
 * @param flag    Whether this application is resizable
 */
void Application::setResizable(bool flag) {
    CUAssertLog(_state == State::NONE, "Cannot make application resizable after start-up");
    _resizable = flag;
}

/**
 * Sets whether this application supports high dpi resolution.
 *
 * For devices that have high dpi screens (e.g. a pixel ration greater than
 * 1), this will enable that feature. Otherwise, this value will do nothing.
 * The effects of this setting can be seen in the method
 * {@link #getPixelScale}.
 *
 * Setting high dpi to true is highly recommended for devices that support
 * it (e.g. iPhones). It makes the edges of textures much smoother. However,
 * rendering is slightly slower as it effectively doubles (and in some cases
 * triples) the resolution.
 *
 * This method is intended to be set at the time the application is
 * created. It cannot be changed once {@link #onStartUp} is called.
 *
 * @param highDPI   Whether to enable high dpi
 */
void Application::setHighDPI(bool highDPI) {
    CUAssertLog(_state == State::NONE, "Cannot set application resolution after start-up");
    _highdpi = highDPI;
}

/**
 * Sets whether this application supports graphics multisampling.
 *
 * Multisampling adds anti-aliasing to the graphics so that polygon edges
 * are not so hard and jagged. This does add some extra overhead, and is
 * not really necessary on Retina or high DPI displays. However, it is
 * pretty much a must in Windows and normal displays. By default, this is
 * false on any platform other than Windows.
 *
 * Note that OpenGL does not support multisampling on mobile platforms.
 *
 * This method is intended to be set at the time the application is
 * created. It cannot be changed once {@link #onStartUp} is called.
 *
 * @param flag    Whether this application should support graphics multisampling.
 */
void Application::setMultiSampled(bool flag) {
    CUAssertLog(_state == State::NONE, "Cannot set application resolution after start-up");
    _multisamp = flag;
}


#pragma mark -
#pragma mark Runtime Attributes
/**
 * Sets the target frames per second of this application.
 *
 * The application does not guarantee that the fps target will always be
 * met. In particular, if the update() and draw() methods are expensive,
 * it may run slower. However, it does guarantee that the program never
 * runs faster than this FPS value.
 *
 * This method may be safely changed at any time while the application
 * is running.
 *
 * By default, this value is the screen fresh (or 60 if undefined)
 *
 * @param fps   The target frames per second
 */
void Application::setFPS(float fps) {
    _fps = fps;
    if (_fps > 0 && _fps <= 500.0f) {
        _delay = (Uint32)(1000000/_fps);
    } else {
        _delay = 0;
    }
}

/**
 * Returns the average frames per second over the last 10 frames.
 *
 * The method provides a way of computing the curren frames per second that
 * smooths out any one-frame anomolies. The FPS is averages over the exact
 * rate of the past 10 frames.
 *
 * @return the average frames per second over the last 10 frames.
 */
float Application::getAverageFPS() const {
    float total = 0;
    for(auto it=_fpswindow.begin(); it != _fpswindow.end(); ++it) {
        total += *it;
    }
    return total/_fpswindow.size();
}

/**
 * Instructs the application to use the deterministic loop.
 *
 * If this value is set to false, then the application will use the simple
 * structure of alternating between {@link #update} and {@link #draw}.
 * However, if it is set to true, it will use a more complicated loop in
 * place of {@link #update}, consisting of a call to {@link #preUpdate},
 * followed by zero or more calls to {@link #fixedUpdate}.
 *
 * This method may be safely changed at any time while the application
 * is running. By default, this value is false.
 *
 * @param value Whether to use the deterministic loop
 */
void Application::setDeterministic(bool value) {
    if (value == _fixed) {
        return;
    } else if (value) {
        _fixedCounter = 0;
        _fixedRemainder = 0;
    }
    _fixed = value;
}

/**
 * Sets the simulation timestep of this application.
 *
 * The value defines the rate at which {@link #fixedUpdate} is called.
 * The rate is a logical value, not a wall-clock value.  That is, if
 * {@link #draw} is called at time T, then the method {@link #fixedUpdate}
 * will have been called T/s times, where s is the simulation timestep.
 *
 * This timestep is set in microseconds for the purposes of precision.
 * Note that this value does nothing if {@link #setDeterministic} is set
 * to false. In that case, {@link #fixedUpdate} is never called, and
 * {@link #update} is called instead.
 *
 * This method may be safely changed at any time while the application
 * is running. By default, this value will match the initial FPS of the
 * application.
 *
 * @param step  The simulation timestep
 */
void Application::setFixedStep(Uint64 step) {
    _fixstep = step;
}


#pragma mark -
#pragma mark File Directories
/**
 * Returns the base directory for all assets (e.g. the assets folder).
 *
 * The assets folder is a READ-ONLY folder for providing assets for the
 * game.  Its path depends on the platform involved.  Android uses
 * this to refer to the dedicated assets folder, while MacOS/iOS refers
 * to the resource bundle.  On Windows, this is the working directory.
 *
 * The value returned is an absolute path in UTF-8 encoding, and has the
 * appropriate path separator for the given platform ('\\' on Windows,
 * '/' most other places). In addition, it is guaranteed to end with a
 * path separator, so that you can append a file name to the path.
 *
 * It is possible that the the string is empty.  For example, the assets
 * directory for Android is not a proper directory (unlike the save
 * directory) and should not be treated as such.  In particular, this
 * string should never be converted to a {@link Pathname} object.
 *
 * Asset loaders use this directory by default.
 *
 * @return the base directory for all assets (e.g. the assets folder).
 */
std::string Application::getAssetDirectory() {
#if defined (SDL_PLATFORM_ANDROID)
#elif defined (SDL_PLATFORM_WIN32)
    if (_assetdir.empty()) {
        char* path = SDL_GetCurrentDirectory();
        _assetdir.append(path);
        SDL_free(path);
    }
#else
    if (_assetdir.empty()) {
        _assetdir.append(SDL_GetBasePath());
    }
#endif
    return _assetdir;
}

/**
 * Returns the base directory for writing save files and preferences.
 *
 * The save folder is a READ-WRITE folder for storing saved games and
 * preferences.  The folder is unique to the current user.  On desktop
 * platforms, it is typically in the user's home directory.  You must
 * use this folder (and not the asset folder) if you are writing any
 * files.
 *
 * The value returned is an absolute path in UTF-8 encoding, and has the
 * appropriate path separator for the given platform ('\\' on Windows,
 * '/' most other places). In addition, it is guaranteed to end with a
 * path separator, so that you can append a file name to the path.
 *
 * I/O classes (both readers and writers) use this directory by default.
 * However, if you are want to use this directory in an asset loader (e.g.
 * for a saved game file), you you may want to refer to the path directly.
 *
 * @return the base directory for writing save files and preferences.
 */
std::string Application::getSaveDirectory() {
    if (_savesdir.empty()) {
        char* path = SDL_GetPrefPath(_org.c_str(),_name.c_str());
        _savesdir.append(path);
        SDL_free(path);
    }
    return _savesdir;
}

#pragma mark -
#pragma mark Display Bounds
/**
 * Sets the screen size of this application.
 *
 * If the application is set to be full screen, this method will be ignored.
 * In that case the application size will be the same as that of the
 * {@link Display}. Note that the size should be in screen coordinates,
 * which may use points instead of pixels.
 *
 * This method may be safely called before the application is initialized.
 * Once {@link #onStartup} is called, it may only be changed if it is set
 * to be resizable.
 *
 * @param width     The screen width
 * @param height    The screen height
 */
void Application::setDisplaySize(int width, int height) {
    _display.size.set(width,height);
    if (_state != State::NONE) {
        Display::get()->setFrameSize(width,height);
        _drawable = Display::get()->getViewBounds();
        _safearea = Display::get()->getSafeBounds();
    }
}

/**
 * Sets the position of this application.
 *
 * If the application is set to be full screen, this method will be ignored.
 * Otherwise, this will set the position in screen coordinates, which
 * typically puts the origin at the top left corner.
 *
 * @param x     The x-coordinate of the top-left corner
 * @param y     The y-coordinate of the top-left corner
 */
void Application::setDisplayPosition(int x, int y) {
    _display.origin.set(x,y);
    if (_state != State::NONE) {
        Display::get()->setFrameOrigin(x,y);
    }
}

/**
 * Sets the screen bounds of this application.
 *
 * If the application is set to be full screen, this method will be ignored.
 * In that case the application size will be the same as that of the
 * {@link Display}. Note that the size should be in screen coordinates,
 * which may use points instead of pixels.
 *
 * This method may be safely called before the application is initialized.
 * Once {@link #onStartup} is called, it may only be changed if it is set
 * to be resizable.
 *
 * @param bounds    The screen width
 */
void Application::setDisplayBounds(const Rect& bounds) {
    _display = bounds;
    if (_state != State::NONE) {
        Display::get()->setFrameBounds(bounds);
        _drawable = Display::get()->getViewBounds();
        _safearea = Display::get()->getSafeBounds();
    }
}


#pragma mark -
#pragma mark Event Scheduling
/**
 * Schedules a callback function time milliseconds in the future.
 *
 * This method allows the user to delay an operation until a certain
 * length of time has passed.  If time is 0, it will be called the next
 * animation frame.  Otherwise, it will be called the first animation
 * frame equal to or more than time steps in the future (so there is
 * no guarantee that the callback will be invoked at exactly time
 * milliseconds in the future).
 *
 * At any given time, the callback can terminate by returning false. Once
 * that happens, it will not be invoked again.  Otherwise, the callback
 * will continue to be executed on a regular basis.  After each call, the
 * timer will be reset and it will not be called for another time milliseconds.
 * If the callback started late, that extra time waited will be credited to
 * the next call.
 *
 * The callback is guaranteed to be executed in the main thread, so it
 * is safe to access the OpenGL context or any low-level SDL operations.
 * It will be executed after the input has been processed, but before
 * the main {@link update} thread.
 *
 * @param callback  The callback function
 * @param time      The number of milliseconds to delay the callback.
 *
 * @return a unique identifier to unschedule the callback
 */
Uint32 Application::schedule(std::function<bool()> callback, Uint32 time) {
    scheduable item;
    item.callback = callback;
    item.period = time;
    item.timer  = time;
    {
        std::unique_lock<std::mutex> lk(_queueMutex);
        _callbacks.emplace(_funcid++, item);
    }
    return _funcid-1;
}

/**
 * Schedules a reoccuring callback function time milliseconds in the future.
 *
 * This method allows the user to delay an operation until a certain
 * length of time has passed.  If time is 0, it will be called the next
 * animation frame.  Otherwise, it will be called the first animation
 * frame equal to or more than time steps in the future (so there is
 * no guarantee that the callback will be invoked at exactly time
 * milliseconds in the future).
 *
 * At any given time, the callback can terminate by returning false. Once
 * that happens, it will not be invoked again.  Otherwise, the callback
 * will continue to be executed on a regular basis.  After each call, the
 * timer will be reset and it will not be called for another period
 * milliseconds. If the callback started late, that extra time waited will
 * be credited to the next call.
 *
 * The callback is guaranteed to be executed in the main thread, so it
 * is safe to access the OpenGL context or any low-level SDL operations.
 * It will be executed after the input has been processed, but before
 * the main {@link update} thread.
 *
 * @param callback  The callback function
 * @param time      The number of milliseconds to delay the callback.
 *
 * @return a unique identifier to unschedule the callback
 */
Uint32 Application::schedule(std::function<bool()> callback, Uint32 time, Uint32 period) {
    scheduable item;
    item.callback = callback;
    item.period = period;
    item.timer  = time;
    {
        std::unique_lock<std::mutex> lk(_queueMutex);
        _callbacks.emplace(_funcid++, item);
    }
    return _funcid-1;
}

/**
 * Stops a callback function from being executed.
 *
 * This method may be used to disable a reoccuring callback. If called
 * soon enough, it can also disable a one-time callback that is yet to
 * be executed.  Once unscheduled, a callback must be re-scheduled in
 * order to be activated again.
 *
 * The callback is identified by its function pointer. Therefore, you
 * should be careful when scheduling anonymous closures.
 */
void Application::unschedule(Uint32 id) {
    std::unique_lock<std::mutex> lk(_queueMutex);
    auto it = _callbacks.find(id);
    if (it != _callbacks.end()) {
        _callbacks.erase(it);
    }
}

/**
 * Cleanly shuts down the application.
 *
 * This method will shutdown the application in a way that is guaranteed
 * to call onShutdown() for clean-up.  You should use this method instead
 * of a general C++ exit() function.
 *
 * This method uses the SDL event system.  Therefore, the application will
 * quit at the start of the next animation frame, when all events are
 * processed.
 */
void Application::quit() {
    SDL_Event quit;
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

#pragma mark -
#pragma mark Virtual Methods
/**
 * The method called after OpenGL is initialized, but before running the application.
 *
 * This is the method in which all user-defined program intialization should
 * take place.  You should not create a new init() method.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to FOREGROUND,
 * causing the application to run.
 */
void Application::onStartup() {
    // Switch states and show to user
    Display::get()->show();
    _state = State::FOREGROUND;
    _steptime.mark();
}

/**
 * The method called when the application is ready to quit.
 *
 * This is the method to dispose of all resources allocated by this
 * application.  As a rule of thumb, everything created in onStartup()
 * should be deleted here.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to NONE,
 * causing the application to be deleted.
 */
void Application::onShutdown() {
    // Switch states
    Input::stop();
    ATK_quit();
    _state = State::NONE;
}


#pragma mark -
#pragma mark Application Loop
/**
 * Processes a single animation frame.
 *
 * This method processes the input, calls the update method, and then
 * draws it. It also updates any running statics, like the average FPS.
 *
 * @return false if the application should quit next frame
 */
bool Application::step() {
    Timestamp current;
    Uint32 micros = (Uint32)current.ellapsedMicros(_steptime);
    _steptime.mark();

    if (_state == State::FOREGROUND) {
        processCallbacks((micros)/1000);

        _fpswindow.pop_front();
        _fpswindow.push_back(1000000.0f/micros);
        
        Uint32 simtime = micros + _fixedRemainder;

        if (_fixed) {
            preUpdate(micros / 1000000.0f);

            for (; simtime >= _fixstep; simtime -= _fixstep) {
                fixedUpdate(_fixstep);
                _fixedCounter++;
            }
            _fixedRemainder = simtime;

            postUpdate(micros / 1000000.0f);
        } else {
            update(micros/1000000.0f);
        }
        
		Display::get()->clear(_clearColor);
        draw();
        Display::get()->refresh();
    } else if (_state != State::BACKGROUND) {
        return false;
    }

    Timestamp endloop;
    micros = (Uint32)endloop.ellapsedMicros(current);
    if (micros < _delay) {
        Uint32 actual = (_delay-micros);
        SDL_DelayPrecise(actual*1000);
    }

    // TODO: Do we want to measure the time from here to start of next loop?
    return true;
}

/**
 * Resychronizes this application with the display.
 *
 * The method should be called whenever the window resizes or the safe area is
 * updated.
 */
void Application::resync() {
    Display::get()->resync();
    _display  = Display::get()->getFrameBounds();
    _drawable = Display::get()->getViewBounds();
    _safearea = Display::get()->getSafeBounds();
}

/**
 * Processes all of the scheduled callback functions.
 *
 * This method wakes up any sleeping callbacks that should be executed. If they
 * are a one time callback, or if they return false, they are deleted. If they
 * are a reoccuring callback and return true, the timer is reset.
 *
 * @param millis    The number of milliseconds since last called
 */
void Application::processCallbacks(Uint32 millis) {
    std::vector<Uint32> indeces;
    std::vector<scheduable> actives;
    {
        std::unique_lock<std::mutex> lk(_queueMutex);
        for (auto it = _callbacks.begin(); it != _callbacks.end(); ++it) {
            if (it->second.timer < millis) {
                indeces.push_back(it->first);
                actives.push_back(it->second);
                it->second.timer -= std::min(it->second.timer, millis);
                it->second.timer += it->second.period;
            } else {
                it->second.timer -= millis;
            }
        }
    }

    // These can take a while, so do them outside lock
    for (int ii = 0; ii < actives.size(); ii++) {
        if (actives[ii].callback()) {
            indeces[ii] = (Uint32)-1;
        }
    }

    {
        std::unique_lock<std::mutex> lk(_queueMutex);
        for (auto it = indeces.begin(); it != indeces.end(); ++it) {
            if (*it != (Uint32)-1) {
                _callbacks.erase(*it);
            }
        }
    }
}

/**
 * A registration method for setting the root class.
 *
 * This method is used by CU_ROOTCLASS to set the main application class.
 *
 * @param f The factory structure created by CU_ROOTCLASS.
 */
void Application::setFactory(Factory f) {
    factoryRef() = f ? f : getDefaultFactory();
}

/**
 * Returns the default factory (e.g. no root class assigned)
 *
 * @return the default factory (e.g. no root class assigned)
 */
Application::Factory Application::getDefaultFactory() {
    return []() { return std::make_unique<Application>(); };
}

/**
 * Returns a reference to the current factory
 *
 * @return a reference to the current factory
 */
Application::Factory& Application::factoryRef() {
    static Factory f = getDefaultFactory();
    return f;
}


#pragma mark -
#pragma mark SDL Hooks
/**
 * Initializes the application object and calls the start-up method.
 *
 * This function is called after the application singleton is constructed,
 * but before it is properly initialized.
 *
 * This is a hook to the SDL callbacks. You should never call this function
 * yourself.
 *
 * @param appstate  Pointer to store the appstate (e.g. the Application object)
 * @param argc      The number of command line arguments (unused)
 * @param argv      The command line arguments (unused)
 *
 * @return SDL_APP_CONTINUE on successful initialization
 */
SDL_AppResult cugl::app_init(void **appstate, int argc, char *argv[]) {
    // Configure the application
    Application* app = Application::get();
    if (!app->init()) {
        return SDL_APP_FAILURE;
    }
    
    // Perform subclass specific initialization
    app->onStartup();
    *appstate = app;
    return app->getState() == Application::State::ABORT ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

/**
 * Processes an input event for the application
 *
 * This may be called zero or more times before each invocation of
 * {@link #app_iterate}.
 *
 * This is a hook to the SDL callbacks. You should never call this function
 * yourself.
 *
 * @param appstate  A reference to the application object
 * @param event     The event to process
 *
 * @return the app running state
 */
SDL_AppResult cugl::app_event(void *appstate, SDL_Event *event) {
    // Register the event with our input handler
    if (!Input::get()->update(*event)) {
        return SDL_APP_SUCCESS;
    }
    
    // Process low level events
    Application* app = (Application*)appstate;
    switch (event->type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_TERMINATING:
            app->setState(Application::State::SHUTDOWN);
            break;
        case SDL_EVENT_LOW_MEMORY:
            app->onLowMemory();
            break;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            if (app->getState() == Application::State::FOREGROUND) {
                app->onSuspend();
            }
            break;
        case SDL_EVENT_DID_ENTER_BACKGROUND:
            app->setState(Application::State::BACKGROUND);
            break;
        case SDL_EVENT_WILL_ENTER_FOREGROUND:
            if (app->getState() == Application::State::BACKGROUND) {
                app->onResume();
            }
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            app->setState(Application::State::FOREGROUND);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_DISPLAY_ORIENTATION:
            app->resync();
            app->onResize();
            break;
        default:
            // Ignore the event.
            break;
    }
    
    // See if the subclass wants to process it
    app->onEvent(event);
    
    switch (app->getState()) {
        case Application::State::SHUTDOWN:
            return SDL_APP_SUCCESS;
        case Application::State::ABORT:
            return SDL_APP_FAILURE;
        default:
            return SDL_APP_CONTINUE;
    }
}

/**
 * Processes a single update/draw loop step of the application.
 *
 * This is a hook to the SDL callbacks. You should never call this function
 * yourself.
 *
 * @param appstate  A reference to the application object
 *
 * @return the app running state
 */
SDL_AppResult cugl::app_step(void *appstate) {
    Application* app = (Application*)appstate;
    app->step();
    
    // Clear the input state HERE!
    Input::get()->clear();
    switch (app->getState()) {
        case Application::State::SHUTDOWN:
            return SDL_APP_SUCCESS;
        case Application::State::ABORT:
            return SDL_APP_FAILURE;
        default:
            return SDL_APP_CONTINUE;
    }
}

/**
 * Cleanly shuts down the application
 *
 * This is a hook to the SDL callbacks. You should never call this function
 * yourself.
 *
 * @param appstate  A reference to the application object
 * @param result    The shutdown category (unused)
 */
void cugl::app_quit(void *appstate, SDL_AppResult result) {
    Application* app = (Application*)appstate;
    app->onShutdown();
}
