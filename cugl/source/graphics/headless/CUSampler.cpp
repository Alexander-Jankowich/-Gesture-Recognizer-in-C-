//
//  CUSampler.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a texture sampler. A
//  sampler instructs the graphics API how to read image data and apply
//  filtering and other transformations for the shader. This headless version
//  stores the sampler state but does not communicate with a graphics card.
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
//  Version: 3/30/25 (Vulkan Integration)
//
#include <cugl/graphics/CUSampler.h>
#include <cugl/core/util/CUDebug.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

#pragma mark -
#pragma mark Constructors
/**
 * Deletes the sampler and resets all attributes.
 *
 * You must reinitialize the sampler to use it.
 */
void Sampler::dispose() {
    if (_data != nullptr) {
        delete _data;
        _data = nullptr;
    }
}

/**
 * Initializes a sampler with the given wrap and filter values
 *
 * @param wrapS     The horizontal wrap
 * @param wrapT     The vertical wrap
 * @param magFilter The filter for magnifying an image
 * @param minFilter The filter for shrinking an image
 *
 * @return true if initialization was successful.
 */
bool Sampler::init(TextureWrap wrapS, TextureWrap wrapT,
                   TextureFilter magFilter, TextureFilter minFilter) {
    if (_data != nullptr) {
        CUAssertLog(false, "Sampler is already initialized");
        return false; // In case asserts are off.
    }
        
    _data = new SamplerData();
    return true;
}

#pragma mark -
#pragma mark Attributes
/**
 * Returns a unique identifier for this sampler
 *
 * This value can be used in memory-based hashes.
 *
 * @return a unique identifier for this sampler
 */
size_t Sampler::id() const {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed sampler");
    return _data->id();
}

/**
 * Sets the magnification filter of this sampler.
 *
 * The magnification filter is the algorithm hint that the graphics card
 * uses to make an image larger. The default is LINEAR.
 *
 * Changing this value will have no effect until a call to {@link #update}.
 * At that point, the value will be pushed to the implementation and all
 * subsequent drawing commands will use the new value. Previous drawing
 * commands (including those in flight) are unaffected.
 *
 * @param filter    The magnification filter of this sampler.
 */
void Sampler::setMagFilter(TextureFilter magFilter) {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed sampler");
    _magFilter = magFilter;
}

/**
 * Sets the minimization filter of this sampler.
 *
 * The minimization filter is the algorithm hint that the graphics card
 * uses to make an image smaller. The default is LINEAR.
 *
 * Changing this value will have no effect until a call to {@link #update}.
 * At that point, the value will be pushed to the implementation and all
 * subsequent drawing commands will use the new value. Previous drawing
 * commands (including those in flight) are unaffected.
 *
 * @param filter    The min filter of this sampler.
 */
void Sampler::setMinFilter(TextureFilter minFilter) {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed sampler");
    _minFilter = minFilter;
}

/**
 * Sets the horizontal wrap of this sampler.
 *
 * The default is CLAMP.
 *
 * Changing this value will have no effect until a call to {@link #update}.
 * At that point, the value will be pushed to the implementation and all
 * subsequent drawing commands will use the new value. Previous drawing
 * commands (including those in flight) are unaffected.
 *
 * @param wrap  The horizontal wrap setting of this sampler
 */
void Sampler::setWrapS(TextureWrap wrap) {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed sampler");
    _wrapS = wrap;
}

/**
 * Sets the vertical wrap of this sampler.
 *
 * The default is CLAMP.
 *
 * Changing this value will have no effect until a call to {@link #update}.
 * At that point, the value will be pushed to the implementation and all
 * subsequent drawing commands will use the new value. Previous drawing
 * commands (including those in flight) are unaffected.
 *
 * @param wrap  The vertical wrap setting of this sampler
 */
void Sampler::setWrapT(TextureWrap wrap) {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed sampler");
    _wrapT = wrap;
}
