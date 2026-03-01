//
//  CUGamepad.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for generic game controllers. Despite the name,
//  this classes are an interface built on top of SDL's Joystick interface, and
//  not its more robust Gamepad interface. That is because this is still
//  an experimental input device and we need a few more iterations to get this
//  one right.
//
//  This input devices are singletons and should never be allocated directly.
//  They should only be accessed via the Input dispatcher.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty. In no event will the authors be held liable for any damages
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
#include <cugl/core/input/CUGamepad.h>
#include <cugl/core/CUDisplay.h>
#include <limits.h>
#include <sstream>
#include <iomanip>

using namespace cugl;

/**
 * Returns the float equivalent of a Sint16 value
 *
 * This function is used to normalize axis results
 *
 * @param value The value to convert
 *
 * @return the float equivalent of a Sint16 value
 */
static float Sint16tofloat(Sint16 value) {
    if (value == -32768) {
        return -1.0;
    }
    return value/32767.0f;
}

#pragma mark -
#pragma mark Gamepad
/**
 * Creates a degnerate gamepad
 *
 * This gamepad is not actually attached to any devices. To activate a
 * gamepad, use {@link GamepadInput#open} instead.
 */
Gamepad::Gamepad() : _input(NULL), _dpadState(false), _focus(0) {
    _uid = "";
    _name = "";
}

/**
 * Initializes this device, acquiring any necessary resources
 *
 * @param joyid The SDL joystick id
 * @param uid   The device uid (assigned by {@link GamepadInput})
 *
 * @return true if initialization was successful
 */
bool Gamepad::init(SDL_JoystickID joyid, const std::string uid) {
    _input = SDL_OpenGamepad(joyid);
    if (_input != NULL) {
        _uid = uid;
        _name = SDL_GetGamepadName(_input);
        
        for(int ii = 0; ii < SDL_GAMEPAD_AXIS_COUNT; ii++) {
            _axisState.push_back(false);
        }
        for(int ii = 0; ii < SDL_GAMEPAD_BUTTON_COUNT; ii++) {
            _buttonState.push_back(false);
        }
        
        return true;
    }
    return false;
}

/**
 * Unintializes this device, returning it to its default state
 *
 * An uninitialized device may not work without reinitialization.
 */
void Gamepad::dispose() {
    if (_input) {
        SDL_CloseGamepad(_input);
        _input = NULL;
    }
    _uid = "";
    _name = "";
    _axisState.clear();
    _buttonState.clear();
    _dpadState = false;
}

#pragma mark Input Methods
/**
 * Cleans up a gamepad after it has been disconnected.
 *
 * This method is similar to {@link dispose} except that it is aware that
 * SDL has closed the gamepad already.
 */
void Gamepad::forceClose() {
    _input = NULL;
    dispose();
}

/**
 * Closes this gamepad, releasing all resources.
 *
 * This method invalidates this gamepad, so any shared pointers still
 * referring to this gamepad are no longer usable. The only way to access
 * the gamepad again is to call {@link GamepadInput#open}.
 *
 * It is often better to call the method {@link GamepadInput#close}
 * instead of this one.
 */
void Gamepad::close() {
    Input::get<GamepadInput>()->close(_uid);
}

/**
 * Clears the state of this input device, readying it for the next frame.
 *
 * Many devices keep track of what happened "this" frame. This method is
 * necessary to advance the frame.
 */
void Gamepad::clearState() {
    for(auto it = _axisState.begin(); it != _axisState.end(); ++it) {
        *it = false;
    }
    for(auto it = _buttonState.begin(); it != _buttonState.end(); ++it) {
        *it = false;
    }
    _dpadState = false;
}

/**
 * Records that a {@link GamepadAxisEvent} has occured
 *
 * @param axis  The axis index
 * @param value The axis value in [-1,1]
 * @param stamp The event timestamp
 */
void Gamepad::reportAxis(Axis axis, float value, const Timestamp& stamp) {
    if (!_axisListeners.empty()) {
        GamepadAxisEvent event(_uid,axis,value,stamp);
        for(auto it = _axisListeners.begin(); it != _axisListeners.end(); ++it) {
            it->second(event,it->first == _focus);
        }
    }
    if (axis != Axis::INVALID) {
        _axisState[(int)axis] = true;
    }
}

/**
 * Records that a {@link GamepadButtonEvent} has occured
 *
 * @param button    The button index
 * @param down      Whether the button is down
 * @param stamp     The event timestamp
 */
void Gamepad::reportButton(Button button, bool down, const Timestamp& stamp) {
    if (!_buttonListeners.empty()) {
        GamepadButtonEvent event(_uid,button,down,stamp);
        for(auto it = _buttonListeners.begin(); it != _buttonListeners.end(); ++it) {
            it->second(event,it->first == _focus);
        }
    }
    switch (button) {
        case Button::INVALID:
            break;
        case Button::DPAD_UP:
        case Button::DPAD_DOWN:
        case Button::DPAD_LEFT:
        case Button::DPAD_RIGHT:
            reportDPad(stamp);
        default:
            _buttonState[(int)button] = true;
            break;
    }
}

/**
 * Records that a {@link GamepadHatEvent} has occured
 *
 * @param hat   The hat index
 * @param value The hat position
 * @param stamp The event timestamp
 */
void Gamepad::reportDPad(const Timestamp& stamp) {
    // Determine the correct D-Pad state
    DPad state = getDPadPosition();
 
    if (!_dpadListeners.empty()) {
        GamepadDPadEvent event(_uid,state,stamp);
        for(auto it = _dpadListeners.begin(); it != _dpadListeners.end(); ++it) {
            it->second(event,it->first == _focus);
        }
    }
    _dpadState = true;
}

#pragma mark Attributes
/**
 * Returns the internal SDL identifier for this gamepad
 *
 * @return the internal SDL identifier for this gamepad
 */
SDL_JoystickID Gamepad::getJoystickID() const {
    SDL_Joystick* joy = SDL_GetGamepadJoystick(_input);
    return SDL_GetJoystickID(joy);
}

#pragma mark Haptics
/**
 * Returns true if this gamepad supports general rumble effects
 *
 * @return true if this gamepad supports general rumble effects
 */
bool Gamepad::hasRumble() const {
    if (_input == NULL) {
        return false;
    }
    SDL_PropertiesID propid = SDL_GetGamepadProperties(_input);
    return SDL_GetBooleanProperty(propid, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
}

/**
 * Returns true if this gamepad supports trigger rumble effects
 *
 * @return true if this gamepad supports trigger rumble effects
 */
bool Gamepad::hasRumbleTriggers() const {
    if (_input == NULL) {
        return false;
    }
    SDL_PropertiesID propid = SDL_GetGamepadProperties(_input);
    return SDL_GetBooleanProperty(propid, SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN, false);
}

/**
 * Starts a rumble effect for this gamepad.
 *
 * @param low_freq  The intensity of the low frequency (left) rumble motor
 * @param high_freq The intensity of the high frequency (right) rumble motor
 * @param duration  The rumble duration in milliseconds
 */
void Gamepad::applyRumble(Uint16 low_freq, Uint16 high_freq, Uint32 duration) {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return ;
    }
    SDL_RumbleGamepad(_input, low_freq, high_freq, duration);
}

/**
 * Starts a rumble effect for this gamepad.
 *
 * @param left      The intensity of the left trigger rumble motor
 * @param right     The intensity of the right trigger rumble motor
 * @param duration  The rumble duration in milliseconds
 */
void Gamepad::hasRumbleTriggers(Uint16 left, Uint16 right, Uint32 duration) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return ;
    }
    SDL_RumbleGamepadTriggers(_input, left, right, duration);
}

#pragma mark Listener Methods
/**
 * Requests focus for the given identifier
 *
 * Only an active listener can have focus. This method returns false if
 * the key does not refer to an active listener (of any type). Note that
 * keys may be shared across listeners of different types, but must be
 * unique for each listener type.
 *
 * @param key   The identifier for the focus object
 *
 * @return false if key does not refer to an active listener
 */
bool Gamepad::requestFocus(Uint32 key)  {
    if (isListener(key)) {
        _focus = key;
        return true;
    }
    return false;
}

/**
 * Returns true if key represents a listener object
 *
 * An object is a listener if it is a listener for any of the three actions:
 * axis movement, button press/release, or D-Pad movement
 *
 * @param key   The identifier for the listener
 *
 * @return true if key represents a listener object
 */
bool Gamepad::isListener(Uint32 key) const {
    bool result = _axisListeners.find(key) != _axisListeners.end();
    result = result || _buttonListeners.find(key) != _buttonListeners.end();
    result = result || _dpadListeners.find(key) != _dpadListeners.end();
    return result;
}

#pragma mark Axis State
/**
 * Returns true if this gamepad supports the specified axis
 *
 * Note that not all gamepads support all axes. In particular, the classic
 * Nintendo controllers have no axes at all.
 *
 * @param axis  The axis to query
 *
 * @return true if this gamepad supports the specified axis
 */
bool Gamepad::hasAxis(Axis axis) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    return SDL_GamepadHasAxis(_input, (SDL_GamepadAxis)axis);
}

/**
 * Returns the current axis position.
 *
 * The default value of any axis is 0. The joysticks all range from -1 to 1
 * (with negative values being left and down). The triggers all range from
 * 0 to 1.
 *
 * Note that the SDL only guarantees that a trigger at rest will be within
 * 0.2 of zero. Most applications implement "dead zones" to ignore values
 * in this range. However, this class does not implement any dead zones;
 * that is the responsibility of the user.
 *
 * If the axis is not supported by this gamepad, this method will return 0.
 *
 * @param axis  The axis index
 *
 * @return the current axis position.
 */
float Gamepad::getAxisPosition(Axis axis) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return 0.0f;
    }
    
    SDL_GamepadAxis internal = (SDL_GamepadAxis)axis;
    if (SDL_GamepadHasAxis(_input,internal)) {
        Sint16 value = SDL_GetGamepadAxis(_input, internal);
        return Sint16tofloat(value);
    }
    return 0.0f;
}

/**
 * Returns true if the given axis changed position this frame.
 *
 * @param axis  The axis index
 *
 * @return true if the given axis changed position this frame.
 */
bool Gamepad::axisDidChange(Axis axis) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    int index = (int)axis;
    if (index != -1) {
        return _axisState[index];
    }
    return false;
}

/**
 * Returns the axis motion listener for the given object key
 *
 * This listener is invoked when an axis changes position.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the axis motion listener for the given object key
 */
const Gamepad::AxisListener Gamepad::getAxisListener(Uint32 key) const {
    if (_axisListeners.find(key) != _axisListeners.end()) {
        return (_axisListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds an axis motion listener for the given object key
 *
 * There can only be one axis listener for a given key (though you may
 * share keys across other listener types). If a listener already exists
 * for the key, the method will fail and return false. You must remove
 * a listener before adding a new one for the same key.
 *
 * This listener is invoked when an axis changes position.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool Gamepad::addAxisListener(Uint32 key, AxisListener listener) {
    if (_axisListeners.find(key) == _axisListeners.end()) {
        _axisListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the axis motion listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when an axis changes position.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool Gamepad::removeAxisListener(Uint32 key) {
    if (_axisListeners.find(key) != _axisListeners.end()) {
        _axisListeners.erase(key);
        return true;
    }
    return false;
}

#pragma mark Button State
/**
 * Returns true if this gamepad supports the specified button
 *
 * Note that not all gamepads support all buttons. In particular, many
 * paddles are currently unique to the XBox Elite controller.
 *
 * @param button    The button to query
 *
 * @return true if this gamepad supports the specified button
 */
bool Gamepad::hasButton(Button button) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    return SDL_GamepadHasButton(_input, (SDL_GamepadButton)button);
}

/**
 * Returns true if the given button is currently down.
 *
 * This method does not distinguish presses or releases and will return
 * true the entire duration of a button hold.
 *
 * If the button is not supported by this gamepad, this method will return
 * false.
 *
 * @param button    The button to query
 *
 * @return true if the given button is currently down.
 */
bool Gamepad::isButtonDown(Button button) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    SDL_GamepadButton internal = (SDL_GamepadButton)button;
    if (SDL_GamepadHasButton(_input,internal)) {
        return SDL_GetGamepadButton(_input, internal);
    }
    return false;
}

/**
 * Returns true if the given button was pressed this frame.
 *
 * A press means that the button is down this animation frame, but was not
 * down the previous frame.
 *
 * If the button is not supported by this gamepad, this method will return
 * false.
 *
 * @param button    The button to query
 *
 * @return true if the given button was pressed this frame.
 */
bool Gamepad::isButtonPressed(Button button) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }

    SDL_GamepadButton internal = (SDL_GamepadButton)button;
    int index = (int)button;
    if (index != -1) {
        return SDL_GetGamepadButton(_input, internal) != 0 && _buttonState[index];
    }
    return false;
}

/**
 * Returns true if the given button was released this frame.
 *
 * A release means that the button is up this animation frame, but was not
 * up the previous frame.
 *
 * If the button is not supported by this gamepad, this method will return
 * false.
 *
 * @param button    The button to query
 *
 * @return true if the given button was released this frame.
 */
bool Gamepad::isButtonReleased(Button button) const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }

    SDL_GamepadButton internal = (SDL_GamepadButton)button;
    int index = (int)button;
    if (index != -1) {
        return SDL_GetGamepadButton(_input, internal) == 0 && _buttonState[index];
    }
    return false;
}

/**
 * Returns the button listener for the given object key
 *
 * This listener is invoked when the button changes state. So it is
 * invoked on a press or a release, but not a hold.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the button listener for the given object key
 */
const Gamepad::ButtonListener Gamepad::getButtonListener(Uint32 key) const {
    if (_buttonListeners.find(key) != _buttonListeners.end()) {
        return (_buttonListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds a button listener for the given object key
 *
 * There can only be one button listener for a given key (though you may
 * share keys across other listener types). If a listener already exists
 * for the key, the method will fail and return false. You must remove
 * a listener before adding a new one for the same key.
 *
 * This listener is invoked when the button changes state. So it is
 * invoked on a press or a release, but not a hold.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool Gamepad::addButtonListener(Uint32 key, ButtonListener listener) {
    if (_buttonListeners.find(key) == _buttonListeners.end()) {
        _buttonListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the button listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when the button changes state. So it is
 * invoked on a press or a release, but not a hold.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool Gamepad::removeButtonListener(Uint32 key) {
    if (_buttonListeners.find(key) != _buttonListeners.end()) {
        _buttonListeners.erase(key);
        return true;
    }
    return false;
}

#pragma mark D-Pad State
/**
 * Returns true if the gamepad has a directional pad.
 *
 * This method is the same a querying all four D-pad buttons.
 *
 * @return true if the gamepad has a directional pad.
 */
bool Gamepad::Gamepad::hasDPad() const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    bool result = true;
    result = result && SDL_GamepadHasButton(_input, SDL_GAMEPAD_BUTTON_DPAD_UP);
    result = result && SDL_GamepadHasButton(_input, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    result = result && SDL_GamepadHasButton(_input, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    result = result && SDL_GamepadHasButton(_input, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

    return result;
}

/**
 * Returns the D-Pad position
 *
 * This method converts the current D-Pad button state into a directional
 * state. This state can be centered (untouched) or one of the eight
 * cardinal directions.
 *
 * If this gamepad does not have a D-Pad, this method will always return
 * CENTERED.
 *
 * @return the D-Pad position
 */
Gamepad::DPad Gamepad::getDPadPosition() const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return DPad::CENTERED;
    }

    DPad result = DPad::CENTERED;
    
    int x = 0;
    int y = 0;
    if (SDL_GetGamepadButton(_input, SDL_GAMEPAD_BUTTON_DPAD_UP)) {
        y += 1;
    }
    if (SDL_GetGamepadButton(_input, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) {
        y -= 1;
    }
    if (SDL_GetGamepadButton(_input, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) {
        x -= 1;
    }
    if (SDL_GetGamepadButton(_input, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) {
        x += 1;
    }
    
    // Just in case
    SDL_ClearError();
    
    switch (x) {
        case -1:
            switch (y) {
                case -1:
                    result = DPad::LEFT_DOWN;
                    break;
                case 0:
                    result = DPad::LEFT;
                    break;
                case 1:
                    result = DPad::LEFT_UP;
                    break;
            }
            break;
        case 0:
            switch (y) {
                case -1:
                    result = DPad::DOWN;
                    break;
                case 0:
                    break;
                case 1:
                    result = DPad::UP;
                    break;
            }
            break;
        case 1:
            switch (y) {
                case -1:
                    result = DPad::RIGHT_DOWN;
                    break;
                case 0:
                    result = DPad::RIGHT;
                    break;
                case 1:
                    result = DPad::RIGHT_UP;
                    break;
            }
            break;
    }
    
    return result;
}

/**
 * Returns true if the D-Pad changed position this frame.
 *
 * @return true if the D-Pad changed position this frame.
 */
bool Gamepad::dPadDidChange() const {
    if (_input == NULL) {
        CUAssertLog(_input, "Gamepad has been closed");
        return false;
    }
    
    return _dpadState;
}

/**
 * Returns the D-Pad listener for the given object key
 *
 * This listener is invoked when the D-Pad changes position.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the D-Pad listener for the given object key
 */
const Gamepad::DPadListener Gamepad::getDPadListener(Uint32 key) const {
    if (_dpadListeners.find(key) != _dpadListeners.end()) {
        return (_dpadListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds a D-Pad listener for the given object key
 *
 * There can only be one D-Pad listener for a given key (though you may
 * share keys across other listener types). If a listener already exists
 * for the key, the method will fail and return false. You must remove
 * a listener before adding a new one for the same key.
 *
 * This listener is invoked when the D-Pad changes position.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool Gamepad::addDPadListener(Uint32 key, DPadListener listener) {
    if (_dpadListeners.find(key) == _dpadListeners.end()) {
        _dpadListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the D-Pad listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when the D-Pad changes position.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool Gamepad::removeDPadListener(Uint32 key) {
    if (_dpadListeners.find(key) != _dpadListeners.end()) {
        _dpadListeners.erase(key);
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark GamepadInput
/**
 * Initializes this device, acquiring any necessary resources
 *
 * @return true if initialization was successful
 */
bool GamepadInput::init() {
    SDL_SetJoystickEventsEnabled(true);
    SDL_SetGamepadEventsEnabled(true);
    scanDevices();
    return true;
}

/**
 * Unintializes this device, returning it to its default state
 *
 * An uninitialized device may not work without reinitialization.
 */
void GamepadInput::dispose() {
    SDL_SetGamepadEventsEnabled(false);
    SDL_SetJoystickEventsEnabled(false);
    for (auto it = _bysdl.begin(); it != _bysdl.end(); ++it) {
        it->second->dispose();
    }
    _devices.clear();
    _names.clear();
    _joyids.clear();
    _bysdl.clear();
    _byname.clear();
    _listeners.clear();
}

#pragma mark Internals
/**
 * Adds a new device to this manager.
 *
 * This method generates the UID that we use to reference the devices.
 * It is called when the manager is first initialized (when it scans
 * all connected devices) and when a new device is connected.
 *
 * @param joyid The SDL joystick id
 *
 * @return the UID of the new device
 */
std::string  GamepadInput::addDevice(SDL_JoystickID joyid) {
    // First create the hash
    std::stringstream ss;
    std::hash<std::string> hasher;
    
    std::string name = SDL_GetJoystickNameForID(joyid);

    // Construct a unique identifier
    Uint64 data = (Uint64)hasher(name);
    Uint16 salt = (joyid >> 16) ^ (joyid & 0xffff);

    ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex
       << ((data >> 48) & 0xffff);
    ss << "-";
    ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex
       << ((data >> 32) & 0xffff);
    ss << "-";
    ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex
       << ((data >> 16) & 0xffff);
    ss << "-";
    ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex
       << ((data & 0xffff) ^ salt);

    std::string uuid = ss.str();

    _devices[uuid] = joyid;
    _names[uuid]   = name;
    _joyids[joyid] = uuid;
    return uuid;
}

/**
 * Removes a new device to this manager.
 *
 * This method will close the associated gamepad, invalidating any references
 * to it. It is called whenever a device becomes disconnected.
 *
 * @param jid   The SDL instance identifier
 *
 * @return the UID of the removed device
 */
std::string  GamepadInput::removeDevice(SDL_JoystickID jid) {
    auto jt = _joyids.find(jid);
    if (jt == _joyids.end()) {
        return "<UNKNOWN>";
    }

    std::string uid = jt->second;
    _devices.erase(jt->second);
    _names.erase(jt->second);
    _joyids.erase(jt);
    
    // See if this joystick was active
    auto kt = _bysdl.find(jid);
    if (kt != _bysdl.end()) {
        kt->second->forceClose();
        _bysdl.erase(kt);
        _byname.erase(uid);
    }
    
    // Reassign available
    int count = 0;
    SDL_JoystickID *sticks = SDL_GetJoysticks(&count);
    for(int ii = 0; ii < count; ii++) {
        SDL_JoystickID key = sticks[ii];
        jt = _joyids.find(key);
        if (jt != _joyids.end()) {
            _devices[jt->second] = ii;
        }
    }
    SDL_free(sticks);
    return uid;
}

/**
 * Scans all connected gamepads, adding them to this manager.
 *
 * This method depends on {@link #doesFilter} for what it considers a
 * valid gamepad.
 */
void GamepadInput::scanDevices() {
    _devices.clear();
    _name.clear();
    _joyids.clear();

    int count = 0;
    SDL_JoystickID *sticks = SDL_GetJoysticks(&count);
    for(int ii = 0; ii < count; ii++) {
        if (!_filter || SDL_IsGamepad(sticks[ii])) {
            addDevice(ii);
        }
    }
    SDL_free(sticks);
}

#pragma mark Attributes
/**
 * Sets whether this gamepad manager filters its devices
 *
 * Our gamepad manager is an interface built on top of the SDL joystick
 * functions. However, SDL has a very broad definition of joystick, and
 * uses it to include things like an accelerometer. If this value is true
 * (which is the default), then only devices which match traditional
 * gamepads will be listed.
 *
 * @param value whether this gamepad manager filters its devices
 */
void GamepadInput::filter(bool value) {
    if (_filter == value) {
        return;
    }
    if (value) {
        _filter = true;
        scanDevices();
        
        // Make sure all open devices correspond to an instance
        for(auto it = _bysdl.begin(); it != _bysdl.end(); ) {
            auto jt = _joyids.find(it->first);
            if (jt == _joyids.end()) {
                // Expensive miss. Find device and add it.
                bool found = false;

                int count = 0;
                SDL_JoystickID *sticks = SDL_GetJoysticks(&count);
                for(int kk = 0; kk < count; kk++) {
                    SDL_JoystickID jid = sticks[kk];
                    if (jid == it->first) {
                        addDevice(kk);
                    }
                }
                SDL_free(sticks);
                
                if (!found) {
                    std::string name = it->second->getName();
                    _byname.erase(name);
                    it->second->dispose();
                    it = _bysdl.erase(it);
                } else {
                    it++;
                }
            } else {
                it++;
            }
        }
    } else {
        _filter = false;
        scanDevices();
    }
}

/**
 * Returns the number of connected devices.
 *
 * This value will be affected by {@link #doesFilter}.
 *
 * @return the number of connected devices.
 */
size_t GamepadInput::size() const {
    return _devices.size();
}

#pragma mark Device Access
/**
 * Returns the list of connected devices.
 *
 * The list is a vector of unique identifiers (UID) used to identify
 * each connected gamepad. These identifiers are not very descriptive,
 * as they are designed to be compact and unique. For a description of
 * each device, use {@link #getName}.
 *
 * @return the list of connected devices.
 */
std::vector<std::string> GamepadInput::devices() const {
    std::vector<std::string> result;
    for(auto it = _devices.begin(); it != _devices.end(); ++it) {
        result.push_back(it->first);
    }
    return result;
}

/**
 * Returns a descriptive name for the given device.
 *
 * The UID for the device should be one listed in {@link #devices()}. If
 * the device does not exist, it will return the empty string.
 *
 * @param uid   The device UID
 *
 * @return a descriptive name for the given device.
 */
std::string GamepadInput::getName(const std::string key) {
    auto jt = _names.find(key);
    if (jt == _names.end()) {
        return "";
    }
    return jt->second;
}

/**
 * Returns a reference to a newly actived gamepad.
 *
 * This method activates the gamepad with the given UID. If the gamepad is
 * already active, it simply returns a reference to the existing gamepad.
 * The UID for the device should be one listed in {@link #devices()}. If the
 * device does not exist, this method will return nullptr.
 *
 * It is generally a good idea to close all gamepads when you are done with
 * them. However, deactivating this input automatically disposes of any
 * active gamepads.
 *
 * @param uid   The device UID
 *
 * @return a reference to a newly actived gamepad.
 */
std::shared_ptr<Gamepad> GamepadInput::open(const std::string uid) {
    // Make the device exists
    auto jt = _devices.find(uid);
    if (jt == _devices.end()) {
        return nullptr;
    }
    SDL_JoystickID jid = jt->second;
    
    // Make sure we have not already opened it
    auto kt = _bysdl.find(jid);
    if (kt != _bysdl.end()) {
        return kt->second;
    }
    
    std::shared_ptr<Gamepad> result = std::make_shared<Gamepad>();
    if (result->init(jid, uid)) {
        _bysdl[jid] = result;
        _byname[uid] = result;
        return result;
    }
    
    return nullptr;
}

/**
 * Returns a reference to the given gamepad.
 *
 * This method assumes the gamepad for this UID has already been activated.
 * The UID for the device should be one listed in {@link #devices()}. If the
 * device does not exist, or the device has not been activate, this method
 * will return nullptr.
 *
 * @param uid   The device UID
 *
 * @return a reference to the given gamepad.
 */
std::shared_ptr<Gamepad> GamepadInput::get(const std::string name) {
    // Make the device exists
    auto jt = _byname.find(name);
    if (jt != _byname.end()) {
        return jt->second;
    }
    return nullptr;
}

/**
 * Closes the gamepad for the given UID
 *
 * This invalidates all references to the gamepad, making them no longer
 * usable. The only way to access the gamepad again is to call
 * {@link #open}.
 *
 * @param uid   The device UID
 */
void GamepadInput::close(const std::string name) {
    // Make the device exists
    auto jt = _byname.find(name);
    if (jt == _byname.end()) {
        return;
    }
    SDL_JoystickID jid = jt->second->getJoystickID();
    jt->second->dispose();
    _bysdl.erase(jid);
    _byname.erase(jt);
}

#pragma mark Listener Methods
/**
 * Requests focus for the given identifier
 *
 * Only a listener can have focus. This method returns false if key
 * does not refer to an active listener
 *
 * @param key   The identifier for the focus object
 *
 * @return false if key does not refer to an active listener
 */
bool GamepadInput::requestFocus(Uint32 key) {
    if (isListener(key)) {
        _focus = key;
        return true;
    }
    return false;
}

/**
 * Returns true if key represents a listener object
 *
 * @param key   The identifier for the listener
 *
 * @return true if key represents a listener object
 */
bool GamepadInput::isListener(Uint32 key) const {
    return _listeners.find(key) != _listeners.end();
}

/**
 * Returns the game controller input listener for the given object key
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the game controller input listener for the given object key
 */
const GamepadInput::Listener GamepadInput::getListener(Uint32 key) const {
    if (isListener(key)) {
        return (_listeners.at(key));
    }
    return nullptr;
}

/**
 * Adds an game controller input listener for the given object key
 *
 * There can only be one listener for a given key. If there is already
 * a listener for the key, the method will fail and return false. You
 * must remove a listener before adding a new one for the same key.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool GamepadInput::addListener(Uint32 key,
                                      GamepadInput::Listener listener) {
    if (!isListener(key)) {
        _listeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the game controller input listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool GamepadInput::removeListener(Uint32 key) {
    if (isListener(key)) {
        _listeners.erase(key);
        return true;
    }
    return false;
}

#pragma mark Input Device Methods
/**
 * Clears the state of this input device, readying it for the next frame.
 *
 * Many devices keep track of what happened "this" frame. This method is
 * necessary to advance the frame.
 */
void GamepadInput::clearState() {
    for(auto it = _bysdl.begin(); it != _bysdl.end(); ++it) {
        it->second->clearState();
    }
}

/**
 * Processes an SDL_Event
 *
 * The dispatcher guarantees that an input device only receives events that
 * it subscribes to.
 *
 * @param event The input event to process
 * @param stamp The event timestamp in CUGL time
 *
 * @return false if the input indicates that the application should quit.
 */
bool GamepadInput::updateState(const SDL_Event& event,
                                      const Timestamp& stamp) {
    switch (event.type) {
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            SDL_JoyAxisEvent jevt = event.jaxis;
            auto jt = _bysdl.find(jevt.which);
            Gamepad::Axis axis = (Gamepad::Axis)jevt.axis;
            if (jt != _bysdl.end()) {
                jt->second->reportAxis(axis,Sint16tofloat(jevt.value),stamp);
            }
        }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            SDL_JoyButtonEvent jevt = event.jbutton;
            auto jt = _bysdl.find(jevt.which);
            Gamepad::Button button = (Gamepad::Button)jevt.button;
            if (jt != _bysdl.end()) {
                bool down = jevt.down;
                jt->second->reportButton(button, down, stamp);
            }
        }
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            SDL_JoyDeviceEvent jevt = event.jdevice;
            std::string uid = addDevice(jevt.which);
            GamepadInputEvent gevent(uid,true,stamp);
            for(auto it = _listeners.begin(); it != _listeners.end(); ++it) {
                it->second(gevent,_focus);
            }
        }
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            SDL_JoyDeviceEvent jevt = event.jdevice;
            std::string uid = removeDevice(jevt.which);
            GamepadInputEvent gevent(uid,false,stamp);
            for(auto it = _listeners.begin(); it != _listeners.end(); ++it) {
                it->second(gevent,_focus);
            }
        }
            break;
        default:
            return true;
    }
    return true;
}

/**
 * Determine the SDL events of relevance and store there types in eventset.
 *
 * An SDL_EventType is really Uint32. This method stores the SDL event
 * types for this input device into the vector eventset, appending them
 * to the end. The Input dispatcher then uses this information to set up
 * subscriptions.
 *
 * @param eventset  The set to store the event types.
 */
void GamepadInput::queryEvents(std::vector<Uint32>& eventset) {
    eventset.push_back((Uint32)SDL_EVENT_GAMEPAD_AXIS_MOTION);
    eventset.push_back((Uint32)SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    eventset.push_back((Uint32)SDL_EVENT_GAMEPAD_BUTTON_UP);
    eventset.push_back((Uint32)SDL_EVENT_GAMEPAD_ADDED);
    eventset.push_back((Uint32)SDL_EVENT_GAMEPAD_REMOVED);
}


