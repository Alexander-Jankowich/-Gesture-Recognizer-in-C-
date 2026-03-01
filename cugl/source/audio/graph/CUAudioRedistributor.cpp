//
//  CUAudioRedistributor.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a graph node for converting from one set of channels
//  to a different set of channels (e.g. mono to stereo or 5.1 down to mono).
//  It is necessary because some devices (notably MSI laptops) will always
//  give you audio devices with 5.1 or 7.1 channels, even when you ask for
//  stereo.
//
//  Historically, this conversion was done with SDL_AudioStream. And unlike
//  resampling, this is a conversion that works properly in SDL_AudioStream.
//  However, because we had to drop SDL_AudioStream for resampling, we decided
//  to drop it entirely.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//  CUGL MIT License:
//
//     This software is provided 'as-is', without any express or implied
//     warranty. In no event will the authors be held liable for any damages
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
#include <cugl/audio/graph/CUAudioRedistributor.h>
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/core/util/CUDebug.h>
#include <cmath>

using namespace cugl::audio;

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate channel redistributor.
 *
 * The redistributor has no channels, so read options will do nothing. The node must
 * be initialized to be used.
 */
AudioRedistributor::AudioRedistributor() :
_distributor(NULL),
_buffer(nullptr),
_conduits(0) {
    _input = nullptr;
    _classname = "AudioRedistributor";

}

/**
 * Initializes the redistributor with default stereo settings
 *
 * The number of channels is two, for stereo output. The sample rate is
 * the modern standard of 48000 HZ.
 *
 * These values determine the buffer the structure for all {@link read}
 * operations. In addition, they also detemine whether this node can
 * serve as an input to other nodes in the audio graph.
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init() {
    return AudioNode::init();
}

/**
 * Initializes the redistributor with the given number of channels and sample rate
 *
 * These values determine the buffer the structure for all {@link read}
 * operations. In addition, they also detemine whether this node can
 * serve as an input to other nodes in the audio graph.
 *
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in HZ
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(Uint8 channels, Uint32 rate) {
    return AudioNode::init(channels,rate);
}

/**
 * Initializes the redistributor with the given input and number of channels
 *
 * The node acquires the sample rate of the input, but uses the given number
 * of channels as its output channels. The redistributor will use the default
 * redistribution algorithm for the given number of channels. If input is
 * nullptr, this method will fail.
 *
 * @param input     The audio node to redistribute
 * @param channels  The number of audio channels
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(const std::shared_ptr<AudioNode>& input, Uint8 channels) {
    if (input && AudioNode::init(channels,input->getRate())) {
        attach(input);
        return true;
    }
    return false;
}

/**
 * Initializes the redistributor with the given input and matrix
 *
 * The node acquires the sample rate of the input, but uses the given number
 * of channels as its output channels. The redistributor will use the given
 * matrix to redistribute the audio. If input is nullptr, this method will
 * fail.
 *
 * The matrix should be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied. This method will not acquire ownership of the given matrix.
 *
 * @param input     The audio node to redistribute
 * @param channels  The number of audio channels
 * @param matrix    The redistribution matrix
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(const std::shared_ptr<AudioNode>& input, Uint8 channels, const float* matrix) {
    if (input && AudioNode::init(channels,input->getRate())) {
        attach(input,matrix);
        return true;
    }
    return false;
}

/**
 * Disposes any resources allocated for this redistributor
 *
 * The state of the node is reset to that of an uninitialized constructor.
 * Unlike the destructor, this method allows the node to be reinitialized.
 */
void AudioRedistributor::dispose() {
    if (_booted) {
        AudioNode::dispose();
        _input = nullptr;
        if (_distributor != NULL) {
            ATK_FreeRedistributor(_distributor);
            _distributor = NULL;
        }
        if (_buffer != nullptr) {
            free(_buffer);
            _buffer = nullptr;
        }
    }
}

/**
 * Allocates the intermediate buffer for conversion
 */
void AudioRedistributor::allocateBuffer() {
    Uint8 number = _conduits.load(std::memory_order_acquire);
    if (_buffer != nullptr) {
        free(_buffer);
        _buffer = nullptr;
    }
    size_t pagesize = _readsize*number;
    _buffer = (float*)malloc(pagesize*sizeof(float));
}


#pragma mark -
#pragma mark Audio Graph
/**
 * Attaches an audio node to this redistributor.
 *
 * The redistributor will use the use the default redistribution algorithm
 * for the current number of channels.
 *
 * @param node  The audio node to redistribute
 *
 * @return true if the attachment was successful
 */
bool AudioRedistributor::attach(const std::shared_ptr<AudioNode>& node) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized audio node");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getRate() != _sampling) {
        CUAssertLog(false,"Input node has wrong sample rate: %d", node->getRate());
        return false;
    }
    
    // Reset the read size if necessary
    if (node->getReadSize() != _readsize) {
        node->setReadSize(_readsize);
    }
    
    setConduits(node->getChannels());
    std::atomic_exchange_explicit(&_input,node,std::memory_order_acq_rel);
    return true;
}

/**
 * Attaches an audio node to this redistributor.
 *
 * The redistributor will use the given matrix to redistribute the audio. The
 * matrix should be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied. This method will not acquire ownership of the given matrix.
 *
 * @param node      The audio node to redistribute
 * @param matrix    The redistribution matrix
 *
 * @return true if the attachment was successful
 */
bool AudioRedistributor::attach(const std::shared_ptr<AudioNode>& node, const float* matrix) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized audio node");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getRate() != _sampling) {
        CUAssertLog(false,"Input node has wrong sample rate: %d", node->getRate());
        return false;
    }
    
    setConduits(node->getChannels(),matrix);
    std::atomic_exchange_explicit(&_input,node,std::memory_order_acq_rel);
    return true;

}

/**
 * Detaches an audio node from this redistributor.
 *
 * If the method succeeds, it returns the audio node that was removed.
 *
 * @return  The audio node to detach (or null if failed)
 */
std::shared_ptr<AudioNode> AudioRedistributor::detach() {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot detach from an uninitialized audio node");
        return nullptr;
    }
    
    std::shared_ptr<AudioNode> result = std::atomic_exchange_explicit(&_input,{},std::memory_order_release);
    return result;
}

/**
 * Sets the number of input channels for this redistributor.
 *
 * Normally this number is inferred from the whatever input node is attached
 * to the redistributor. If no node has yet been attached, this this method
 * returns 0 by default.
 *
 * However, changing this value may require that the underlying read buffer
 * be resized (particularly when the number of input channels is larger
 * than the number of output channels.). Hence by setting this value ahead
 * of time (and making sure that all attached input nodes match this size),
 * you can improve the performance of this filter.
 *
 * Assigning this value while there is still an attached audio node has undefined
 * behavior.
 *
 * @param number    The number of input channels for this redistributor.
 */
void AudioRedistributor::setConduits(Uint8 number) {
    Uint8 original = _conduits.load(std::memory_order_acquire);
    if (original != number) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _conduits = number;
        if (_distributor != NULL) {
            ATK_FreeRedistributor(_distributor);
            _distributor = NULL;
        }
        _distributor = ATK_AllocRedistributor(number, _channels, NULL);
    }
}

/**
 * Sets the number of input channels for this redistributor.
 *
 * Normally this number is inferred from the whatever input node is attached
 * to the redistributor. If no node has yet been attached, this this method
 * returns 0 by default.
 *
 * However, changing this value may require that the underlying read buffer
 * be resized (particularly when the number of input channels is larger
 * than the number of output channels.). Hence by setting this value ahead
 * of time (and making sure that all attached input nodes match this size),
 * you can improve the performance of this filter.
 *
 * This version of the method will also allow you to set the matrix at the
 * same time (so that it matches the number of input channels). The matrix
 * will be an MxN matrix in row major order, where N is the number of input
 * channels and M is the number of output channels. The provided matrix will
 * be copied. This method will not acquire ownership of the given matrix.
 *
 * Assigning this value while there is still an attached audio node has undefined
 * behavior.
 *
 * @param number    The number of input channels for this redistributor.
 */
void AudioRedistributor::setConduits(Uint8 number, const float* matrix) {
    Uint8 original = _conduits.load(std::memory_order_acquire);
    if (original != number) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _conduits = number;
        if (_distributor != NULL) {
            ATK_FreeRedistributor(_distributor);
            _distributor = NULL;
        }
        _distributor = ATK_AllocRedistributor(_conduits, _channels, const_cast<float*>(matrix));
    }
}

/**
 * Returns the current redistribution matrix for this redistributor.
 *
 * The matrix will be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. If the redistributor
 * is currently using a default redistribution algorithm (based on the number
 * of input channels), then this method will return nullptr.
 *
 * @return the current redistribution matrix for this redistributor.
 */
const float* const AudioRedistributor::getMatrix() {
    std::unique_lock<std::mutex> lk(_buffmtex);
    if (_distributor) {
        return ATK_GetRedistributorMatrix(_distributor);
    }
    return nullptr;
}

/**
 * Sets the current redistribution matrix for this redistributor.
 *
 * The matrix will be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied. This method will not acquire ownership of the given matrix.
 *
 * If the redistributor is currently using a default redistribution algorithm
 * (based on the number of input channels), then this method will return nullptr.
 *
 * @param matrix    The redistribution matrix
 */
void AudioRedistributor::setMatrix(const float* matrix) {
    std::unique_lock<std::mutex> lk(_buffmtex);
    if (_distributor != NULL) {
        ATK_FreeRedistributor(_distributor);
        _distributor = NULL;
    }
    _distributor = ATK_AllocRedistributor(_conduits, _channels, const_cast<float*>(matrix));
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
void AudioRedistributor::setReadSize(Uint32 size) {
    if (_readsize != size) {
        _readsize = size;
        allocateBuffer();
    }
}

#pragma mark -
#pragma mark Overriden Methods
/**
 * Reads up to the specified number of frames into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly.
 * The only exception is when the user needs to create a custom subclass
 * of this AudioNode.
 *
 * The buffer should have enough room to store frames * channels elements.
 * The channels are interleaved into the output buffer.
 *
 * This method will always forward the read position after reading. Reading
 * again may return different data.
 *
 * @param buffer    The read buffer to store the results
 * @param frames    The maximum number of frames to read
 *
 * @return the actual number of frames read
 */
Uint32 AudioRedistributor::read(float* buffer, Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    Uint32 take = 0;
    if (input == nullptr || _paused.load(std::memory_order_relaxed)) {
        std::memset(buffer,0,frames*_channels*sizeof(float));
        take = frames;
    } else {
        std::unique_lock<std::mutex> lk(_buffmtex);
        // Prevent a subtle race
        if (_conduits != input->getChannels() || _distributor == NULL) {
            std::memset(buffer,0,frames*_channels*sizeof(float));
            take = frames;
        } else {
            bool abort = false;
            while (take < frames && !abort) {
                Uint32 amt = frames;
                if (_buffer != nullptr) {
                    amt = _readsize < frames-take ? _readsize : frames-take;
                    amt = input->read(_buffer,amt);
                    ATK_ApplyRedistributor(_distributor, _buffer, buffer, amt);
                } else {
                    amt = input->read(buffer+take*_channels,amt);
                }
                take += amt;
                abort = (amt == 0);
            }
        }
    }
    return take;
}
    
/**
 * Returns true if this audio node has no more data.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns true if there is no input node, indicating there is no data.
 *
 * An audio node is typically completed if it return 0 (no frames read) on
 * subsequent calls to {@link read()}.
 *
 * @return true if this audio node has no more data.
 */
bool AudioRedistributor::completed() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    return (input == nullptr || input->completed());
}

/**
 * Marks the current read position in the audio steam.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * This method is used by {@link reset()} to determine where to restore
 * the read position.
 *
 * @return true if the read position was marked.
 */
bool AudioRedistributor::mark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->mark();
    }
    return false;
}

/**
 * Clears the current marked position.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * Clearing the mark in a player is equivelent to setting the mark at
 * the beginning of the audio asset. Future calls to {@link reset()}
 * will return to the start of the audio stream.
 *
 * @return true if the read position was cleared.
 */
bool AudioRedistributor::unmark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->unmark();
    }
    return false;
}

/**
 * Resets the read position to the marked position of the audio stream.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * If no mark is set, this will reset to the player to the beginning of
 * the audio sample.
 *
 * @return true if the read position was moved.
 */
bool AudioRedistributor::reset() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->reset();
    }
    return false;
}

/**
 * Advances the stream by the given number of frames.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * This method only advances the read position, it does not actually
 * read data into a buffer.
 *
 * @param frames    The number of frames to advace
 *
 * @return the actual number of frames advanced; -1 if not supported
 */
Sint64 AudioRedistributor::advance(Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->advance(frames);
    }
    return -1;
}
    
/**
 * Returns the current frame position of this audio node
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned will always be the absolute frame position regardless
 * of the presence of any marks.
 *
 * @return the current frame position of this audio node.
 */
Sint64 AudioRedistributor::getPosition() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getPosition();
    }
    return -1;
}

/**
 * Sets the current frame position of this audio node.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value set will always be the absolute frame position regardless
 * of the presence of any marks.
 *
 * @param position  the current frame position of this audio node.
 *
 * @return the new frame position of this audio node.
 */
Sint64 AudioRedistributor::setPosition(Uint32 position) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getPosition();
    }
    return -1;
}

/**
 * Returns the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned is always measured from the start of the steam,
 * regardless of the presence of any marks.
 *
 * @return the elapsed time in seconds.
 */
double AudioRedistributor::getElapsed() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getElapsed();
    }
    return -1;
}

/**
 * Sets the read position to the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned is always measured from the start of the steam,
 * regardless of the presence of any marks.
 *
 * @param time  The elapsed time in seconds.
 *
 * @return the new elapsed time in seconds.
 */
double AudioRedistributor::setElapsed(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setElapsed(time);
    }
    return -1;
}

/**
 * Returns the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The remaining time is duration from the current read position to the
 * end of the sample. It is not effected by any fade-out.
 *
 * @return the remaining time in seconds.
 */
double AudioRedistributor::getRemaining() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getRemaining();
    }
    return -1;
}

/**
 * Sets the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node. It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * This method will move the read position so that the distance between
 * it and the end of the same is the given number of seconds.
 *
 * @param time  The remaining time in seconds.
 *
 * @return the new remaining time in seconds.
 */
double AudioRedistributor::setRemaining(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setRemaining(time);
    }
    return -1;
}

