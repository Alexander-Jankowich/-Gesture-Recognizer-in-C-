//
//  CUTextureArray.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D texture array. In the
//  move to Vulkan, we split most of the functionality of this class into two
//  different classes: ImageArray and Sampler. So this class is largely a pair
//  of these two classes.
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
#include <cugl/core/util/CUDebug.h>
#include <cugl/graphics/CUTextureArray.h>
#include <cugl/graphics/CUImageArray.h>
#include <cugl/graphics/CUSampler.h>
#include "CUGLOpaques.h"
#include <sstream>

using namespace cugl;
using namespace cugl::graphics;

#pragma mark -
#pragma mark Constructors
/**
 * Creates a new empty texture array with no size.
 *
 * This method performs no allocations. You must call init to generate
 * a proper texture.
 */
TextureArray::TextureArray() :
_layers(0),
_width(0),
_height(0),
_name(""){
}

/**
 * Deletes the texture array and resets all attributes.
 *
 * You must reinitialize the texture array to use it.
 */
void TextureArray::dispose() {
    if (_image != nullptr) {
        _image   = nullptr;
        _sampler = nullptr;
        _width = 0; _height = 0;
        _layers = 0;
        _name = "";
    }
}

/**
 * Initializes a texture array with the given data.
 *
 * The data format must match the one specified by the format parameter.
 * If the parameter mipmaps is true, the image will generate an appropriate
 * number of mipmaps determined by its size.
 *
 * The sampler will be the default, using a {@link TextureFilter#LINEAR}
 * filter and {@link TextureWrap#CLAMP} on both axes.
 *
 * @param data      The texture data (size width*height*format)
 * @param width     The texture width in pixels
 * @param height    The texture height in pixels
 * @param layers    The number of layers in the array
 * @param format    The texture data format
 * @param access    The image access
 * @param mipmaps   Flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool TextureArray::initWithData(const std::byte *data,
                                Uint32 width, Uint32 height, Uint32 layers,
                                TexelFormat format, ImageAccess access, bool mipmaps) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture array is already initialized");
        return false; // In case asserts are off.
    }
    
    _image = ImageArray::allocWithData(data, width, height, layers, format, access, mipmaps);
    _sampler = Sampler::alloc();

    std::stringstream ss;
    ss << "@" << data;
    setName(ss.str());
    
    return _image != nullptr && _sampler != nullptr;
}

/**
 * Initializes an texture array with the given image array.
 *
 * Note that this texture does not copy the image array, so any changes to
 * the image array elsewhere will affect this is texture array.
 *
 * The sampler will be the default, using a {@link TextureFilter#LINEAR}
 * filter and {@link TextureWrap#CLAMP} on both axes.
 *
 * @param images    The texture image array
 *
 * @return true if initialization was successful.
 */
bool TextureArray::initWithImages(const std::shared_ptr<ImageArray>& images) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    } else if (images == nullptr) {
        return false;
    }

    _image = images;
    _sampler = Sampler::alloc();

    setName(images->getName());
    return _sampler != nullptr;
}

/**
 * Initializes an texture with the given image array and sampler.
 *
 * Note that this texture does not copy the image, so any changes to the
 * image elsewhere will affect this is texture.
 *
 * @param images    The texture image array
 * @param sampler   The texture sampler
 *
 * @return true if initialization was successful.
 */
bool TextureArray::initWithImages(const std::shared_ptr<ImageArray>& images,
                                  const std::shared_ptr<Sampler>& sampler) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    } else if (images == nullptr || sampler == nullptr) {
        return false;
    }
    
    _image = images;
    _sampler = sampler;
    setName(images->getName());
    return true;
}

#pragma mark -
#pragma mark Data Access
/**
 * Sets this texture array to have the contents of the given buffer.
 *
 * This method is the same as in {@link ImageArray}.
 *
 * The buffer must have the correct data format. In addition, the buffer
 * must be size width*height*layers*bytesize. See {@link ImageArray#getByteSize}
 * for a description of the latter.
 *
 * @param data  The buffer to read into the texture array
 *
 * @return a reference to this (modified) texture array for chaining.
 */
const TextureArray& TextureArray::set(const std::byte *data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture array");
        return *this;
    }
    
    _image->set(data);
    return *this;
}

/**
 * Sets the specified layer to have the contents of the given buffer.
 *
 * This method is the same as in {@link ImageArray}. If layer is not a
 * valid layer, this method does nothing.
 *
 * The buffer must have the correct data format. In addition, the buffer
 * must be size width*height*bytesize. See {@link ImageArray#getByteSize}
 * for a description of the latter.
 *
 * @param layer The texture layer to modify
 * @param data  The buffer to read into the texture array
 *
 * @return true if the data was successfully set
 */
bool TextureArray::setLayer(Uint32 layer, const std::byte *data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture array");
        return false;
    }
    
    return _image->setLayer(layer,data);
}

/**
 * Fills the given buffer with data from this texture array.
 *
 * This method will append the data to the given vector.
 *
 * @param data  The buffer to store the texture data
 *
 * @return the number of bytes read into the buffer
 */
const size_t TextureArray::get(std::vector<std::byte>& data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture array");
        return 0;
    }
    
    return _image->get(data);
}

/**
 * Fills the given buffer with data from the specified layer
 *
 * This method will append the data to the given vector.
 *
 * @param layer The texture layer to modify
 * @param data  The buffer to store the texture data
 *
 * @return the number of bytes read into the buffer
 */
const size_t TextureArray::getLayer(Uint32 layer, std::vector<std::byte>& data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture array");
        return 0;
    }
    
    return _image->getLayer(layer,data);
}

#pragma mark -
#pragma mark Conversions
/**
 * Returns a unique identifier for this texture array
 *
 * This value can be used in memory-based hashes.
 *
 * @return a unique identifier for this texture array
 */
size_t TextureArray::id() const {
    CUAssertLog(_image != nullptr, "The texture array is not initialized");
    return _image->id()+37*_sampler->id();
}

/**
 * Returns a string representation of this texture for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this texture for debugging purposes.
 */
std::string TextureArray::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::graphics::TextureArray[" : "[");
    ss << getName() << ",";
    ss << "w:" << getWidth() << ",";
    ss << "h:" << getHeight() << ",";
    ss << "l:" << getLayers();
    ss << "]";
    return ss.str();
}
