//
//  CUAudioResampler.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a graph node for converting from one sample rate to
//  another. This is is necessary for cross-platform reasons as iPhones are
//  very stubborn about delivering any requested sampling rates other than 48000.
//
//  Historically, this module exists because SDL_AudioStream was broken in
//  early versions of SDL2, occasionally causing audio to cut out. While it
//  has since been fixed in SDL3, the interface is still not a good match
//  for our DSP graph. In addition, we like to have a little more control
//  over the resampling algorithm.
//
//  Both this resampler and the original SDL_AudioStream are an implementation of
//  a bandlimited interpolation resampler as described here:
//
//      https://ccrma.stanford.edu/~jos/resample/Implementation.html
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
#include <cugl/core/CUBase.h>
#include <cugl/audio/graph/CUAudioResampler.h>
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/util/CUDebug.h>
#include <cmath>

using namespace cugl::audio;

#pragma mark Resampler Support

/**
 * The callback function used to provide data to the resampler
 *
 * This function will fill in silence if there is not enough data to send
 * to the resampler.
 *
 * @param userdata  The resampler object
 * @param stream    The stream to write to
 * @param len       The number of bytes requested
 */
static size_t audio_callback(void* userdata, Uint8* stream, size_t len) {
    AudioResampler* resampler = (AudioResampler*)userdata;
    Uint32 channels = resampler->getChannels();
    float* buffer = reinterpret_cast<float*>(stream);
    Uint32 frames = (Uint32)(len/(channels*sizeof(float)));
    Uint32 actual = resampler->poll(buffer, frames);
    if (actual < frames) {
        std::memset(buffer+actual*channels,0,(frames-actual)*(channels*sizeof(float)));
    }
    return len;
}



#pragma mark -
#pragma mark Constructors
/** The default number of zero crossings */
#define ZERO_CROSSINGS  5
/** The default bit precision */
#define BITS_PER_SAMPLE 16
/** The default stoppband attenuation */
#define STOPBAND_ATTEN  80.0

/**
 * Creates a degenerate audio resampler.
 *
 * The node has not been initialized, so it is not active.  The node
 * must be initialized to be used.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a node on
 * the heap, use the factory in {@link AudioManager}.
 */
AudioResampler::AudioResampler() : AudioNode(),
_inputrate(0),
_stopband(STOPBAND_ATTEN),
_zero_cross(ZERO_CROSSINGS),
_precision(BITS_PER_SAMPLE),
_resampler(NULL),
_intime(0.0),
_mktime(0.0) {
    _input = nullptr;
    _classname = "AudioResampler";
}

/**
 * Initializes a resampler with 2 channels at 48000 Hz.
 *
 * This sample rate of the output of this node is 48000 Hz, but the input
 * sample rate depends on the input node, which can change over time. However,
 * the input node must agree with number of channels, which is fixed.
 *
 * @return true if initialization was successful
 */
bool AudioResampler::init() {
    return init(DEFAULT_CHANNELS,DEFAULT_SAMPLING);
}

/**
 * Returns a newly allocated resampler with the given channels and sample rate.
 *
 * This sample rate is the output rate of this node. The input same rate
 * depends on the input node, which can change over time. However, the
 * input node must agree with number of channels, which is fixed.
 *
 * @param channels  The number of audio channels
 * @param rate      The output sample rate (frequency) in HZ
 *
 * @return a newly allocated resampler with the given channels and sample rate.
 */
bool AudioResampler::init(Uint8 channels, Uint32 rate) {
    if (AudioNode::init(channels,rate)) {
        _inputrate = rate;
        return true;
    }
    return false;
}

/**
 * Initializes a resampler with the given input node and sample rate.
 *
 * This node acquires the channels of the input, but will use the given
 * sample rate as its output rate. If input is nullptr, this method will
 * fail.
 *
 * @param input     The audio node to resample
 * @param rate      The output sample rate (frequency) in Hz
 *
 * @return true if initialization was successful
 */
bool AudioResampler::init(const std::shared_ptr<AudioNode>& input, Uint32 rate) {
    if (init(input->getChannels(),rate)) {
        attach(input);
        return true;
    }
    return false;
}

/**
 * Disposes any resources allocated for this resampler.
 *
 * The state of the node is reset to that of an uninitialized constructor.
 * Unlike the destructor, this method allows the node to be reinitialized.
 */
void AudioResampler::dispose() {
    if (_booted) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        AudioNode::dispose();
        _input = nullptr;
        if (_resampler) {
            ATK_FreeResampler(_resampler);
            _resampler = NULL;
        }
        _inputrate = 0;
        _stopband  = STOPBAND_ATTEN;
        _zero_cross = ZERO_CROSSINGS;
        _precision  = BITS_PER_SAMPLE;
        _intime = 0;
        _mktime = 0;
    }
}

#pragma mark -
#pragma mark Audio Graph
/**
 * Attaches an audio node to this resampler.
 *
 * This method will reset the resampler stream if the input has a different
 * rate than the previous input value (and is not the same rate as the
 * output).  It will fail if the input does not have the same number of
 * channels as this resampler.
 *
 * @param node  The audio node to resample
 *
 * @return true if the attachment was successful
 */
bool AudioResampler::attach(const std::shared_ptr<AudioNode>& node) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized audio node");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getChannels() != _channels) {
        CUAssertLog(false,"Input node has wrong number of channels: %d", node->getChannels());
        return false;
    }
    
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    Uint32 inrate = _inputrate.load(std::memory_order_relaxed);
    
    if (input) {
        detach();
    }

    if (node->getRate() != inrate) {
        setInputRate(node->getRate());
    }
    
    if (node->getReadSize() != _readsize) {
        node->setReadSize(_readsize);
    }
    
    {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _intime = 0;
    }

    std::atomic_store_explicit(&_input,node,std::memory_order_relaxed);
    return true;
}

/**
 * Detaches an audio node from this resampler.
 *
 * If the method succeeds, it returns the audio node that was removed.
 * This method will not automatically reset the sampling stream.
 *
 * @return  The audio node to detach (or null if failed)
 */
std::shared_ptr<AudioNode> AudioResampler::detach() {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot detach from an uninitialized audio node");
        return nullptr;
    }
    
    std::shared_ptr<AudioNode> result = std::atomic_exchange_explicit(&_input,{},std::memory_order_relaxed);
    return result;
}

/**
 * Sets the typical read size of this node.
 *
 * Some audio nodes need an internal buffer for operations like mixing or
 * resampling. In that case, it helps to know the requested {@link read}
 * size ahead of time. The capacity is the minimal required read amount
 * of the {@link AudioEngine} and corresponds to {@link AudioEngine#getReadSize}.
 *
 * It is not actually necessary to set this size. However for nodes with
 * internal buffer, setting this value can optimize performance.
 *
 * This method is not synchronized because it is assumed that this value
 * will **never** change while the audio engine in running. The average
 * user should never call this method explicitly. You should always call
 * {@link AudioEngine#setReadSize} instead.
 *
 * @param size  The typical read size of this node.
 */
void AudioResampler::setReadSize(Uint32 size) {
    if (_readsize != size) {
        _readsize = size;
        // This forces a buffer refresh
        setInputRate(_inputrate);
    }
}

#pragma mark -
#pragma mark Filter Properties
/**
 * Sets the input sample rate of this filter.
 *
 * This value is distinct from {@link AudioNode#getRate()}, which is the *output*
 * sample rate of this node. Instead, this value is the sample rate of any audio
 * node connected to this one via the {@link #attach} method.
 *
 * Normally this value is assigned when a new audio node is attached. However,
 * changing this value requires that the underlying read buffer be resized. Hence,
 * by setting this value ahead of time (and making sure that all attached input
 * nodes match this sample rate), you can improve the performance of this filter.
 *
 * Assigning this value while there is still an attached audio node has undefined
 * behavior.
 *
 * @param value The input sample rate of this filter.
 */
void AudioResampler::setInputRate(Uint32 value) {
    std::unique_lock<std::mutex> lk(_buffmtex);
    _inputrate = value;

    if (_resampler) {
        ATK_FreeResampler(_resampler);
        _resampler = NULL;
    }
    
    ATK_ResamplerDef def;
    def.channels  = _channels;
    def.inrate    = value;
    def.outrate   = _sampling;
    def.stopband  = _stopband;
    def.zerocross = _zero_cross;
    def.bitdepth  = _precision;
    def.buffsize  = _readsize;
    def.callback  = audio_callback;
    def.userdata  = this;
    
    _resampler = ATK_AllocResampler(&def);
}

/**
 * Sets the stopband attentuation for this filter in dB
 *
 * This value is described in more detail here:
 *
 *     https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
 *
 * By default, this value is 80.0.
 *
 * @param value The ripple factor for this filter in dB
 */
void AudioResampler::setStopband(float value) {
    float oldval = getStopband();
    if (value != oldval) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _stopband = value;
        // Force a rebuild
        setInputRate(_inputrate);
    }
}

/**
 * Sets the bit precision for audio sent to this filter.
 *
 * Even though CUGL processes all audio data as floats, that does not mean that the
 * audio on this platform is guaranteed to have 32 bit precision.  Indeed, on Android,
 * most audio is processed at 16 bit precision, and many audio files are recorded at
 * this level of precision as well. Hence this filter assumes 16 bit precision by default.
 *
 * This is relevant for the size of the filter to process the audio. Each additional bit
 * doubles the size of the filter table used for the convolution.  A 16 bit filter uses
 * a very reasonable 512 entries per zero crossing. On the other hand, a 32 bit filter
 * would require 131072 entries per zero crossing. Given the limitations of real-time
 * resampling, it typically does not make much sense to assume more than 16 bits.
 *
 * @param value The bit precision for audio sent to this filter.
 */
void AudioResampler::setBitPrecision(Uint32 value) {
    float oldval = getBitPrecision();
    if (value != oldval) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _precision = value;
        // Force a rebuild
        setInputRate(_inputrate);
    }
}

/**
 * Sets the number of zero-crossings of this filter.
 *
 * The zero-crossings of a sinc filter are relevant because the determine the number
 * of coefficients in a single filter convolution. For X zero-crossings, a single
 * output sample requires 2*(X-1) input computations. Increasing this value can give
 * some increased value in filter. However, the droppoff for sinc filters is large
 * enough that eventually that large enough values will have no discernable effect.
 *
 * The default number of zero crossing is 5, meaning that this filter roughly causes
 * an 8x-10x decrease in performance when processing audio (when taking all the
 * relevant overhead into account).  This value is that one recommended by this
 * tutorial website:
 *
 *     https://www.dsprelated.com/freebooks/pasp/Windowed_Sinc_Interpolation.html
 *
 * @param value The number of zero-crossings of this filter.
 */
void AudioResampler::setZeroCrossings(Uint32 value) {
    float oldval = getZeroCrossings();
    if (value != oldval) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _zero_cross = value;
        // Force a rebuild
        setInputRate(_inputrate);
    }
}

#pragma mark -
#pragma mark Playback Control
/**
 * Returns true if this resampler has no more data.
 *
 * An audio node is typically completed if it return 0 (no frames read) on
 * subsequent calls to {@link read()}.  However, for infinite-running
 * audio threads, it is possible for this method to return true even when
 * data can still be read; in that case the node is notifying that it
 * should be shut down.
 *
 * @return true if this audio node has no more data.
 */
bool AudioResampler::completed() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input != nullptr) {
        bool complete = input->completed();
        if (_resampler) {
            complete = complete && ATK_PeekResampler(_resampler);
        }
        return complete;
    }
    return true;
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
Uint32 AudioResampler::read(float* buffer, Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_seq_cst);
    Uint32 cnvrate = _inputrate.load(std::memory_order_seq_cst);
    
    Uint32 take = 0;
    if (input == nullptr || _paused.load(std::memory_order_relaxed)) {
        std::memset(buffer,0,frames*_channels*sizeof(float));
        take = frames;
    } else if (_resampler == NULL) {
        take = input->read(buffer,frames);
    } else {
        std::unique_lock<std::mutex> lk(_buffmtex);
        // Prevent a subtle race
        if (cnvrate != input->getRate()) {
            std::memset(buffer,0,frames*_channels*sizeof(float));
            take = frames;
        } else {
            bool abort = false;
            while (take < frames && !abort) {
                Sint64 amount = ATK_PollResampler(_resampler, buffer+take*_channels, frames-take);
                if (amount <= 0) {
                    abort = true;
                } else {
                    take += amount;
                }
            }
        }
    }
    
    ATK_VecScale(buffer,_ndgain.load(std::memory_order_relaxed),buffer,take*_channels);
    return take;
}

/**
 * Reads up to the specified number of frames into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly.
 * This is an internal version of {@link #read} which is used by the
 * resampler to feed the buffer. It reads directly from the input node
 * and does not perform any conversion.
 *
 * @param buffer    The read buffer to store the results
 * @param frames    The maximum number of frames to read
 *
 * @return the actual number of frames read
 */
Uint32 AudioResampler::poll(float* buffer, Uint32 frames) {
    Uint32 take = 0;
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_seq_cst);
    if (input == nullptr) {
        std::memset(buffer,0,frames*_channels*sizeof(float));
        take = frames;
    } else {
        take = input->read(buffer,frames);
    }
    return take;
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
bool AudioResampler::mark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        bool result = input->mark();
        if (result) {
            _mktime = _intime;
        }
        return result;
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
bool AudioResampler::unmark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        bool result = input->unmark();
        if (result) {
            _mktime = 0;
        }
        return result;
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
bool AudioResampler::reset() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        bool result = input->reset();
        if (result) {
            _intime = _mktime;
        }
        return result;
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
Sint64 AudioResampler::advance(Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        double inrate  = _inputrate.load(std::memory_order_relaxed);
        double outrate = getRate();
        Sint64 actual = input->advance(frames*inrate/outrate);
        Sint64 adjust = actual*outrate/inrate;
        _intime += adjust*inrate/outrate;
        return adjust;
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
Sint64 AudioResampler::getPosition() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        double inrate  = _inputrate.load(std::memory_order_relaxed);
        double outrate = getRate();
        return input->getPosition()*inrate/outrate;
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
Sint64 AudioResampler::setPosition(Uint32 position) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        double inrate  = _inputrate.load(std::memory_order_relaxed);
        double outrate = getRate();
        Sint64 actual = input->setPosition(position*inrate/outrate);
        Sint64 adjust = actual*outrate/inrate;
        _intime = adjust*inrate/outrate;
        return adjust;
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
double AudioResampler::getElapsed() const {
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
double AudioResampler::setElapsed(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        double inrate  = _inputrate;
        double outrate = getRate();
        double actual = input->setElapsed(time);
        Sint64 pos = input->getPosition();
        _intime = pos*outrate/inrate;
        return actual;
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
double AudioResampler::getRemaining() const {
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
double AudioResampler::setRemaining(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        double inrate  = _inputrate;
        double outrate = getRate();
        double actual = input->setRemaining(time);
        Sint64 pos = input->getPosition();
        _intime = pos*outrate/inrate;
        return actual;
    }
    return -1;
}
