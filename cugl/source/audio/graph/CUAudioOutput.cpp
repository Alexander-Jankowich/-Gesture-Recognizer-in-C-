//
//  CUAudioOutput.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides interface to an audio output device. As such, it is
//  often the final node in an audio stream DAG. It is analogous to AVAudioEngine
//  in Apple's AVFoundation API. The main differences is that it does not have
//  a dedicated mixer node.  Instead, you attach the single terminal node of the
//  audio graph.  In addition, it is possible to have a distinct audio graph for
//  each output device.
//
//  The audio graph and its nodes will always be accessed by two threads: the
//  main thread and the audio thread.  The audio graph is designed to safely
//  coordinate between these two threads.  However, it is minimizes locking
//  and instead relies on a fail-fast model.  If part of the audio graph is
//  not in a state to be used by the audio thread, it will skip over that part
//  of the graph until the next render frame.  Hence some changes should only
//  be made if the graph is paused.  When there is some question about the
//  thread safety, the methods are clearly marked.
//
//  It is NEVER safe to access the audio graph outside of the main thread. The
//  coordination algorithms only assume coordination between two threads.
//
//  CUGL MIT License:
//
//     This software is provided 'as-is', without any express or implied
//     warranty.  In no event will the authors be held liable for any damages
//     arising from the use of this software.
//
//     Permission is granted to anyone to use this software for any purpose,
//     including commercial applications, and to alter it and redistribute it
//     freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not
//     be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 12/2/25 (SDL3 Integration)
//
#include <cugl/audio/graph/CUAudioOutput.h>
#include <cugl/audio/graph/CUAudioResampler.h>
#include <cugl/audio/graph/CUAudioRedistributor.h>
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/util/CUTimestamp.h>
#include <atomic>
#include <cstring>


using namespace cugl::audio;

#pragma mark Static Attributes

#pragma mark -
#pragma mark SDL Audio Loop
/**
 * The SDL callback function
 *
 * This is the function that SDL uses to populate the audio buffer for a
 * playback device.
 */
static void audio_callback(void*  userdata, SDL_AudioStream *stream, int additional, int total) {
    if (additional > 0) {
        Uint8 *data = SDL_stack_alloc(Uint8, additional);
        if (data) {
            AudioOutput* device = (AudioOutput*)userdata;
            Uint32 amt = device->poll(data,additional);
            memset(data+amt,0,additional-amt);
            SDL_PutAudioStreamData(stream,data,additional);
            SDL_stack_free(data);
        }
    }
}

/**
 * Returns the estimated sample period (in frames) for the given streeam
 *
 * If the stream is not actually doing any resampling, this assumes that the
 * period matches the sample rate of the device. Otherwise, it assumes this
 * is a playback device that is resampling.
 *
 * @param device    The audio device
 * @param stream    The stream attached to this device
 */
static Uint32 period_estimate(SDL_AudioDeviceID device, SDL_AudioStream* stream) {
    int samples;
    SDL_AudioSpec in, out, dev;
    SDL_GetAudioDeviceFormat(device, &dev, &samples);
    SDL_GetAudioStreamFormat(stream, &in, &out);
    
    if (in.freq == dev.freq) {
        return (Uint32)samples;
    }

    double ratio  = (double)in.freq / (double)dev.freq;
    double approx = samples * ratio;
    return (Uint32)(approx+1);
}

#pragma mark -
#pragma mark AudioDevices Methods
/**
 * Creates a degenerate audio output node.
 *
 * The node has not been initialized, so it is not active.  The node
 * must be initialized to be used.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a node on
 * the heap, use one of the static constructors instead.
 */
AudioOutput::AudioOutput() : AudioNode(),
_dvname(""),
_overhd(0),
_locked(false),
_input(nullptr),
_stream(NULL) {
    _classname = "AudioOutput";
}

/**
 * Deletes the audio output node, disposing of all resources
 */
AudioOutput::~AudioOutput() {
    dispose();
}

/**
 * Initializes the default output device with 2 channels at 48000 Hz.
 *
 * This node will have a read size (the number of frames the node plays
 * at a time) of {@link AudioDevices#getReadSize()} frames. By default,
 * this value is 512 frames. This means that, at stereo 48000 Hz, the
 * node has a potential lag of 21 ms, which is slightly more than an
 * animation frame at 60 fps.
 *
 * An output device is initialized with both active and paused as false.
 * That means it will begin playback as soon as {@link AudioDevices} sets
 * this device to active.
 *
 * This node is always logically attached to the default output device.
 * That means it will switch devices whenever the default output changes.
 * This method may fail if the default device is in use.
 *
 * @return true if initialization was successful
 */
bool AudioOutput::init() {
    CUAssertLog(AudioDevices::get(),"Attempt to allocate a node without an active audio device manager");
    return init(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                DEFAULT_CHANNELS,DEFAULT_SAMPLING,
                AudioDevices::get()->getReadSize());
}

/**
 * Initializes the default output device with the given channels and sample rate.
 *
 * This node will have a read size (the number of frames the node plays
 * at a time) of {@link AudioDevices#getReadSize()} frames. By default,
 * this value is 512 frames. This means that, at stereo 48000 Hz, the
 * node has a potential lag of 21 ms, which is slightly more than an
 * animation frame at 60 fps.
 *
 * An output device is initialized with both active and paused as false.
 * That means it will begin playback as soon as {@link AudioDevices} sets
 * this device to active.
 *
 * This node is always logically attached to the default output device.
 * That means it will switch devices whenever the default output changes.
 * This method may fail if the default output device is in use.
 *
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in Hz
 *
 * @return true if initialization was successful
 */
bool AudioOutput::init(Uint8 channels, Uint32 rate) {
    CUAssertLog(AudioDevices::get(),"Attempt to allocate a node without an active audio device manager");
    return init(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,channels,rate,
                AudioDevices::get()->getReadSize());
}

/**
 * Initializes the default output device with the given channels and sample rate.
 *
 * The read size is the number of frames collected at each poll. Smaller
 * values clearly tax the CPU, as the device is collecting data at a higher
 * rate. Furthermore, if the value is too small, the time to collect the
 * data may be larger than the time to play it. This will result in pops
 * and crackles in the audio.
 *
 * However, larger values increase the audio lag. For example, a buffer
 * of 512 stereo frames for a sample rate of 48000 Hz corresponds to 21
 * milliseconds. This is the delay between when sound is gathered and
 * it is played. A value of 512 is the perfered value for 60 fps framerate.
 * With that said, many devices cannot handle this rate and need a buffer
 * size of 1024 instead.
 *
 * An output device is initialized with both active and paused as false.
 * That means it will begin playback as soon as {@link AudioDevices} sets
 * this device to active.
 *
 * This node is always logically attached to the default output device.
 * That means it will switch devices whenever the default output changes.
 * This method may fail if the default output device is in use.
 *
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in Hz
 * @param readsize  The size of the buffer to output audio
 *
 * @return true if initialization was successful
 */
bool AudioOutput::init(Uint8 channels, Uint32 rate, Uint32 readsize) {
    return init(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,channels,rate,readsize);
}

/**
 * Initializes the given output device with 2 channels at 48000 Hz.
 *
 * This node will have a read size (the number of frames the node plays
 * at a time) of {@link AudioDevices#getReadSize()} frames. By default,
 * this value is 512 frames. This means that, at stereo 48000 Hz, the
 * node has a potential lag of 21 ms, which is slightly more than an
 * animation frame at 60 fps.
 *
 * An output device is initialized with both active and paused as false.
 * That means it will begin playback as soon as {@link AudioDevices} sets
 * this device to active.
 *
 * This method may fail if the given device is in use.
 *
 * @param device    The name of the output device
 *
 * @return true if initialization was successful
 */
bool AudioOutput::init(SDL_AudioDeviceID device) {
    CUAssertLog(AudioDevices::get(),"Attempt to allocate a node without an active audio device manager");
    return init(device,DEFAULT_CHANNELS,DEFAULT_SAMPLING,AudioDevices::get()->getReadSize());
}

/**
 * Initializes the output device with the given channels and sample rate.
 *
 * The read size is the number of frames collected at each poll. Smaller
 * values clearly tax the CPU, as the node is collecting data at a higher
 * rate. Furthermore, if the value is too small, the time to collect the
 * data may be larger than the time to play it. This will result in pops
 * and crackles in the audio.
 *
 * However, larger values increase the audio lag. For example, a buffer
 * of 512 stereo frames for a sample rate of 48000 Hz corresponds to 21
 * milliseconds. This is the delay between when sound is gathered and
 * it is played. A value of 512 is the perfered value for 60 fps framerate.
 * With that said, many devices cannot handle this rate and need a buffer
 * size of 1024 instead.
 *
 * An output device is initialized with both active and paused as false.
 * That means it will begin playback as soon as {@link AudioDevices} sets
 * this device to active.
 *
 * This method may fail if the given device is in use.
 *
 * @param device    The name of the output device
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in Hz
 * @param readsize  The size of the buffer to output audio
 *
 * @return true if initialization was successful
 */
bool AudioOutput::init(SDL_AudioDeviceID device, Uint8 channels, Uint32 rate, Uint32 readsize) {

    if (!SDL_IsAudioDevicePlayback(device)) {
        CUAssertLog(false, "Device is not a playback device");
        return false;
    }
    
    if (!AudioNode::init(channels,rate)) {
        return false;
    }
    _device = device;
    _readsize = readsize;
    
    _audiospec.freq = rate;
    _audiospec.channels = channels;
    _audiospec.format = SDL_AUDIO_F32;

    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, std::to_string(readsize).c_str());

    if (!reopenDevice()) {
        return false;
    }
    
    _booted = true;
    _active = false;
    _paused = false;
    return true;
}

/**
 * Disposes any resources allocated for this output device node.
 *
 * The state of the node is reset to that of an uninitialized constructor.
 * Unlike the destructor, this method allows the node to be reinitialized.
 */
void AudioOutput::dispose() {
    if (_booted) {
        _active = false;
        _locked = false;
        SDL_PauseAudioDevice(_device);
        SDL_DestroyAudioStream(_stream);
        _device = 0;
        _overhd = 0;
        detach();

        _input = nullptr;
        AudioNode::dispose();
    }
}

/**
 * Sets the active status of this node.
 *
 * An active device will have its {@link read()} method called at regular
 * intervals.  This setting is to allow {@link AudioDevices} to pause and
 * resume an output device without override the user pause settings.
 *
 * @param active    Whether to set this node to active
 */
void AudioOutput::setActive(bool active) {
    _active.store(active,std::memory_order_relaxed);
    if (!_paused.load(std::memory_order_relaxed)) {
        //_device = SDL_GetAudioStreamDevice(_stream);
        if (active) {
            SDL_ResumeAudioDevice(_device);
        } else {
            SDL_PauseAudioDevice(_device);
        }
    }
}

#pragma mark -
#pragma mark (Re)initialization Methods
/**
 * Opens a device according to the wanted specification
 *
 * This method is necessary because sometimes we need to close and reopen
 * a device, particularly on a format change.
 *
 * @return true if the device was successfully reinitialized
 */
bool AudioOutput::reopenDevice() {
    if (_stream) {
        SDL_PauseAudioDevice(_device);
        SDL_DestroyAudioStream(_stream);
    }
    bool active = _active;
    _active = false;

    _stream = SDL_OpenAudioDeviceStream(_device, &_audiospec, audio_callback, this);
    if (_stream == NULL) {
        CULogError("[AUDIO] %s", SDL_GetError());
        return false;
    } else if (!SDL_GetAudioStreamFormat(_stream,&_audiospec,NULL)) {
        CULogError("[AUDIO] Failed to initialize stream.");
        return false;
    }
    
    _device = SDL_GetAudioStreamDevice(_stream);
    const char* name = SDL_GetAudioDeviceName(_device);
    if (name != NULL) {
        _dvname = name;
    } else if (_device == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK) {
        _dvname = "Default Playback";
    }

    // Get the buffer size
    _readsize = period_estimate(_device,_stream);
    _channels = _audiospec.channels;
    _sampling = _audiospec.freq;
    
    if (active) {
        _active = true;
        SDL_ResumeAudioDevice(_device);
    }
    
    return true;
}

/**
 * Temporarily locks this output device
 *
 * A locked output device cannot play any audio. Locking an output
 * device makes it safe to perform arbitrary destructive methods on
 * the entire audio graph.
 *
 * **IMPORTANT**: You must call unlock on this device to use it again.
 * The device will only unlock itself on destruction.
 */
void AudioOutput::lock() {
    SDL_LockAudioStream(_stream);
    _locked = true;
}

/**
 * Unlocks this output device
 *
 * A locked output device cannot play any audio. Locking an output
 * device makes it safe to perform arbitrary destructive methods on
 * the entire audio graph.
 */
void AudioOutput::unlock() {
    SDL_UnlockAudioStream(_stream);
    _locked = true;
}

#pragma mark -
#pragma mark Audio Graph
/**
 * Sets the read size of this output node.
 *
 * The read size is the number of frames collected at each poll. Smaller
 * values clearly tax the CPU, as the node is collecting data at a higher
 * rate. Furthermore, if the value is too small, the time to collect the
 * data may be larger than the time to play it. This will result in pops
 * and crackles in the audio.
 *
 * However, larger values increase the audio lag. For example, a buffer
 * of 512 stereo frames for a sample rate of 48000 Hz corresponds to 21
 * milliseconds. This is the delay between when sound is gathered and
 * it is played. A value of 512 is the perfered value for 60 fps framerate.
 * With that said, many devices cannot handle this rate and need a buffer
 * size of 1024 instead.
 *
 * This method is not synchronized because it is assumed that this value
 * will **never** change while the audio engine in running. The average
 * user should never call this method explicitly. You should always call
 * {@link AudioEngine#setReadSize} instead.
 *
 * @param size  The read size of this output node.
 */
void AudioOutput::setReadSize(Uint32 size) {
    bool changed = _readsize != size;
    if (changed) {
        _readsize = size;
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, std::to_string(size).c_str());
        if (!reopenDevice()) {
            dispose();
        }
        
        std::shared_ptr<AudioNode> node = _input;
        if (node != nullptr) {
            node->setReadSize(_readsize);
        }
    }
}

/**
 * Attaches an audio graph to this output node.
 *
 * This method will fail if the channels of the audio graph do not agree
 * with the number of the channels of this node.
 *
 * @param node  The terminal node of the audio graph
 *
 * @return true if the attachment was successful
 */
bool AudioOutput::attach(const std::shared_ptr<AudioNode>& node) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized output device");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getChannels() != _channels) {
        CUAssertLog(false,"Terminal node of audio graph has wrong number of channels: %d",
                    node->getChannels());
        return false;
    } else if (node->getRate() != _sampling) {
        CUAssertLog(false,"Terminal node of audio graph has wrong sample rate: %d",
                    node->getRate());
        return false;
    }
    
    // Reset the read size if necessary
    if (node->getReadSize() != _readsize) {
        node->setReadSize(_readsize);
    }
    
    std::atomic_exchange_explicit(&_input,node,std::memory_order_relaxed);
    return true;
}

/**
 * Detaches an audio graph from this output node.
 *
 * If the method succeeds, it returns the terminal node of the audio graph.
 *
 * @return  the terminal node of the audio graph (or null if failed)
 */
std::shared_ptr<AudioNode> AudioOutput::detach() {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot detach from an uninitialized output device");
        return nullptr;
    }

    std::shared_ptr<AudioNode> result = std::atomic_exchange_explicit(&_input,{},std::memory_order_relaxed);
    return result;
}


#pragma mark -
#pragma mark Playback Control

/**
 * Pauses this node, preventing any data from being read.
 *
 * If the node is already paused, this method has no effect. Pausing will
 * not go into effect until the next render call in the audio thread.
 *
 * @return true if the node was successfully paused
 */
bool AudioOutput::pause() {
    bool success = !_paused.exchange(true);
    if (success && _active.load(std::memory_order_relaxed)) {
        SDL_PauseAudioDevice(_device);
    }
    return success;
}

/**
 * Resumes this previously paused node, allowing data to be read.
 *
 * If the node is not paused, this method has no effect.  It is possible to
 * resume an node that is not yet activated by {@link AudioDevices}.  When
 * that happens, data will be read as soon as the node becomes active.
 *
 * @return true if the node was successfully resumed
 */
bool AudioOutput::resume() {
    bool success = _paused.exchange(false);
    if (success && _active.load(std::memory_order_relaxed)) {
        SDL_ResumeAudioDevice(_device);
    }
    return success;
}

/**
 * Returns true if this audio node has no more data.
 *
 * An audio node is typically completed if it return 0 (no frames read) on
 * subsequent calls to {@link read()}.  However, for infinite-running
 * audio threads, it is possible for this method to return true even when
 * data can still be read; in that case the node is notifying that it
 * should be shut down.
 *
 * @return true if this audio node has no more data.
 */
bool AudioOutput::completed() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    return (input == nullptr || input->completed());
}

/**
 * Reads up to the specified number of frames into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly.
 * The only exception is when the user needs to create a custom subclass
 * of this AudioOutput.
 *
 * The buffer should have enough room to store frames * channels elements.
 * The channels are interleaved into the output buffer.
 *
 * This method will always forward the read position.
 *
 * @param buffer    The read buffer to store the results
 * @param frames    The maximum number of frames to read
 *
 * @return the actual number of frames read
 */
Uint32 AudioOutput::read(float* buffer, Uint32 frames) {
    Timestamp start;

    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    Uint32 take = 0;
    if (input == nullptr || _paused.load(std::memory_order_relaxed)) {
        std::memset(buffer,0,frames*_audiospec.channels*sizeof(float));
        take = frames;
    } else {
        take = input->read(buffer,frames);
    }
    
    // Buck stops here.  Fill remainder with 0s.
    if (take < frames) {
        std::memset(buffer+take*_audiospec.channels,0,
                    (frames-take)*_audiospec.channels*sizeof(float));
    }

    Timestamp end;
    Uint64 micros = Timestamp::ellapsedMicros(start,end);
    _overhd.store(micros,std::memory_order_relaxed);
    return frames;
}

/**
 * Reads up to the specified number of bytes into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly.
 * The only exception is when the user needs to create a custom subclass
 * of this AudioOutput.
 *
 * This method is used by the SDL audio interface to process the audio
 * graph. It should never be called by the developer.
 *
 * @param stream    The read buffer to store the results
 * @param len       The maximum number of bytes to read
 *
 * @return the actual number of bytes read
 */
Uint32 AudioOutput::poll(Uint8* stream, int len) {
    Uint32 wordsize = SDL_AUDIO_BITSIZE(_audiospec.format)/8;
    Uint32 frames = len/(_audiospec.channels*wordsize);
    Uint32 taken = read(reinterpret_cast<float*>(stream),frames);
    return taken*(_audiospec.channels*wordsize);
}

/**
 * Returns the number of microseconds needed to render the last audio frame.
 *
 * This method is primarily for debugging.
 *
 * @return the number of microseconds needed to render the last audio frame.
 */
Uint64 AudioOutput::getOverhead() const {
    return _overhd.load(std::memory_order_relaxed);
}


#pragma mark -
#pragma mark Optional Methods
/**
 * Marks the current read position in the audio steam.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * This method is typically used by {@link reset()} to determine where to
 * restore the read position. For some nodes (like {@link AudioInput}),
 * this method may start recording data to a buffer, which will continue
 * until {@link clear()} is called.
 *
 * It is possible for {@link reset()} to be supported even if this method
 * is not.
 *
 * @return true if the read position was marked.
 */
bool AudioOutput::mark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->mark();
    }
    return false;
}

/**
 * Clears the current marked position.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * If the method {@link mark()} started recording to a buffer (such as
 * with {@link AudioInput}), this method will stop recording and release
 * the buffer.  When the mark is cleared, {@link reset()} may or may not
 * work depending upon the specific node.
 *
 * @return true if the read position was marked.
 */
bool AudioOutput::unmark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->unmark();
    }
    return false;
}

/**
 * Resets the read position to the marked position of the audio stream.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * When no {@link mark()} is set, the result of this method is node
 * dependent.  Some nodes (such as {@link AudioPlayer}) will reset to the
 * beginning of the stream, while others (like {@link AudioInput}) only
 * support a rest when a mark is set. Pay attention to the return value of
 * this method to see if the call is successful.
 *
 * @return true if the read position was moved.
 */
bool AudioOutput::reset() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->reset();
    }
    return false;
}

/**
 * Advances the stream by the given number of frames.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * This method only advances the read position, it does not actually
 * read data into a buffer. This method is generally not supported
 * for nodes with real-time input like {@link AudioInput}.
 *
 * @param frames    The number of frames to advace
 *
 * @return the actual number of frames advanced; -1 if not supported
 */
Sint64 AudioOutput::advance(Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->advance(frames);
    }
    return -1;
}

/**
 * Returns the current frame position of this audio node
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * In some nodes like {@link AudioInput}, this method is only supported
 * if {@link mark()} is set.  In that case, the position will be the
 * number of frames since the mark. Other nodes like {@link AudioPlayer}
 * measure from the start of the stream.
 *
 * @return the current frame position of this audio node.
 */
Sint64 AudioOutput::getPosition() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getPosition();
    }
    return -1;
}

/**
 * Sets the current frame position of this audio node.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * In some nodes like {@link AudioInput}, this method is only supported
 * if {@link mark()} is set.  In that case, the position will be the
 * number of frames since the mark. Other nodes like {@link AudioPlayer}
 * measure from the start of the stream.
 *
 * @param position  the current frame position of this audio node.
 *
 * @return the new frame position of this audio node.
 */
Sint64 AudioOutput::setPosition(Uint32 position) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setPosition(position);
    }
    return -1;
}

/**
 * Returns the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * In some nodes like {@link AudioInput}, this method is only supported
 * if {@link mark()} is set.  In that case, the times will be the
 * number of seconds since the mark. Other nodes like {@link AudioPlayer}
 * measure from the start of the stream.
 *
 * @return the elapsed time in seconds.
 */
double AudioOutput::getElapsed() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getElapsed();
    }
    return -1;
}

/**
 * Sets the read position to the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * In some nodes like {@link AudioInput}, this method is only supported
 * if {@link mark()} is set.  In that case, the new time will be meaured
 * from the mark. Other nodes like {@link AudioPlayer} measure from the
 * start of the stream.
 *
 * @param time  The elapsed time in seconds.
 *
 * @return the new elapsed time in seconds.
 */
double AudioOutput::setElapsed(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setElapsed(time);
    }
    return -1;
}

/**
 * Returns the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node or if this method is unsupported
 * in that node
 *
 * In some nodes like {@link AudioInput}, this method is only supported
 * if {@link setRemaining()} has been called.  In that case, the node will
 * be marked as completed after the given number of seconds.  This may or may
 * not actually move the read head.  For example, in {@link AudioPlayer} it
 * will skip to the end of the sample.  However, in {@link AudioInput} it
 * will simply time out after the given time.
 *
 * @return the remaining time in seconds.
 */
double AudioOutput::getRemaining() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getRemaining();
    }
    return -1;
}

/**
 * Sets the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node or if this method is unsupported
 * in that node
 *
 * If this method is supported, then the node will be marked as completed
 * after the given number of seconds.  This may or may not actually move
 * the read head.  For example, in {@link AudioPlayer} it will skip to the
 * end of the sample.  However, in {@link AudioInput} it will simply time
 * out after the given time.
 *
 * @param time  The remaining time in seconds.
 *
 * @return the new remaining time in seconds.
 */
double AudioOutput::setRemaining(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setRemaining(time);
    }
    return -1;
}
