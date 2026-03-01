//
//  CUImageArray.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing an array of 2D image sources.
//  In OpenGL (and many game engines), image and texture are synonymous, and so
//  are image array and texture array. However, in modern graphics APIs like
//  Vulkan, they are slightly different. Technically a texture array is an image
//  array plus a sampler. Since CUGL 4.0, we have separated these two concepts
//  for forward-looking compatibility with Vulkan.
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
#include <cugl/core/CUBase.h>
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/util/CUFiletools.h>
#include <cugl/graphics/CUImageArray.h>
#include <sstream>
#include "CUGLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

#pragma mark -
#pragma mark Constructors
/**
 * Returns true if the given image access support CPU writes
 *
 * @param access    The image access permissions
 *
 * @return true if the given image access support CPU writes
 */
static bool can_write(ImageAccess access) {
    switch (access) {
        case ImageAccess::WRITE_ONLY:
        case ImageAccess::READ_WRITE:
        case ImageAccess::COMPUTE_ALL:
            return true;
        default:
            return false;
    }
}

/**
 * Returns true if the given image access support CPU reads
 *
 * @param access    The image access permissions
 *
 * @return true if the given image access support CPU reads
 */
static bool can_read(ImageAccess access) {
    switch (access) {
        case ImageAccess::READ_ONLY:
        case ImageAccess::READ_WRITE:
        case ImageAccess::COMPUTE_READ:
        case ImageAccess::COMPUTE_ALL:
            return true;
        default:
            return false;
    }
}

/**
 * Creates a new empty image array with no size.
 *
 * This method performs no allocations. You must call init to generate
 * a proper image.
 */
ImageArray::ImageArray() :
_width(0),
_height(0),
_layers(0),
_name(""),
_data(nullptr) {
    _format = TexelFormat::UNDEFINED;
    _access = ImageAccess::SHADER_ONLY;
}

/**
 * Deletes the image array and resets all attributes.
 *
 * You must reinitialize the image to use it.
 */
void ImageArray::dispose() {
    if (_data != nullptr) {
        delete _data;
        _data = nullptr;
        _width = 0; _height = 0; _layers = 0;
        _format = TexelFormat::UNDEFINED;
        _name = "";
    }
}

/**
 * Initializes an image array with the given data and access.
 *
 * The data format must match the one specified by the format parameter.
 * If the parameter mipmaps is true, the image will generate an appropriate
 * number of mipmaps determined by its size.
 *
 * @param data      The image data (size width*height*format)
 * @param width     The image width in pixels
 * @param height    The image height in pixels
 * @param layers    The number of image layers
 * @param format    The image data format
 * @param access    The image access
 * @param mipmaps   A flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool ImageArray::initWithData(const std::byte *data,
                              Uint32 width, Uint32 height, Uint32 layers,
                              TexelFormat format, ImageAccess access, bool mipmaps) {
    if (_data != nullptr) {
        CUAssertLog(false, "Image array is already initialized");
        return false; // In case asserts are off.
    }
    
    _data = new ImageArrayData();
    if (!_data->init(data, width, height, layers, mipmaps, format)) {
        delete _data;
        return false;
    }

    _access = access;
    _width  = width;
    _height = height;
    _layers = layers;
    _format = format;
    _mipmaps = mipmaps;

    std::stringstream ss;
    ss << "@" << data;
    setName(ss.str());
    return true;
}


#pragma mark -
#pragma mark Data Access
/**
 * Sets this image to have the contents of the given buffer.
 *
 * The buffer must have the correct data format. In addition, the buffer
 * must be size width*height*bytesize. See {@link #getByteSize} for
 * a description of the latter.
 *
 * Note that some images created by CUGL are read-only. For any such
 * images, this method will fail.
 *
 * @param data  The buffer to read into the image
 *
 * @return a reference to this (modified) image for chaining.
 */
const ImageArray& ImageArray::set(const std::byte *data) {
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot set data to an uninitialzed image array");
        return *this;
    } else if (!can_write(_access)) {
        CUAssertLog(false, "This image array does not support write access");
        return *this;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _data->images);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                    _layers, _width, _height, 1,
                    gl_symbolic_format(_format),
                    gl_format_type(_format),
                    bytes);
    
    if (_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    return *this;
}

/**
 * Sets this image to have the contents of the given buffer.
 *
 * The buffer must have the correct data format. In addition, the buffer
 * must be size width*height*layers*bytesize. See {@link #getByteSize} for
 * a description of the latter.
 *
 * This method will fail if the access does not support CPU-side writes.
 *
 * @param data  The buffer to read into the image
 *
 * @return true if the data was successfully set
 */
bool ImageArray::setLayer(Uint32 layer, const std::byte *data) {
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot set data to an uninitialized image array");
        return false;
    } else if (!can_write(_access)) {
        CUAssertLog(false, "This image array does not support write access");
        return false;
    }

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _data->images);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(GL_TEXTURE_2D, 0, 0, 0,
                    _layers, _width, _height, 1,
                    gl_symbolic_format(_format),
                    gl_format_type(_format),
                    bytes);
    
    if (_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    return true;
}

/**
 * Fills the given buffer with data from this texture.
 *
 * This method will append the data to the given vector.
 *
 * Note that some images, such as those produced by {@link RenderTarget},
 * have an internal representation that cannot be exported. For any such
 * image, this method will fail, returning 0.
 *
 * @param data  The buffer to store the texture data
 *
 * @return the number of bytes read into the buffer
 */
const size_t ImageArray::get(std::vector<std::byte>& data) {
#if CU_GRAPHICS_API == CU_OPENGLES
    CUAssertLog(false, "Image read-access is not currently supported in OpenGLES");
    return 0;
#else
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialized image");
        return 0;
    } else if (!can_read(_access)) {
        CUAssertLog(false, "This image does not support read access");
        return 0;
    }

    size_t end = data.size();
    size_t size = _width*_height*_layers*getByteSize();
    data.resize(end+size);
    
    uint8_t* start = reinterpret_cast<uint8_t*>(data.data());
    start += end;
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, _data->images);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0,
                  gl_symbolic_format(_format),
                  gl_format_type(_format),
                  start);
    GLenum error = glGetError();
    if (error) {
        CULogError("Could not read from image array %s: %s", _name.c_str(), gl_error_name(error).c_str());
        return 0;
    }

    return size;
#endif
}

/**
 * Fills the given buffer with data from this texture.
 *
 * This method will append the data to the given vector. This method will
 * fail if the access does not support CPU-side reads.
 *
 * @param data  The buffer to store the texture data
 *
 * @return the number of bytes read into the buffer
 */
const size_t ImageArray::getLayer(Uint32 layer, std::vector<std::byte>& data) {
#if CU_GRAPHICS_API == CU_OPENGLES
    CUAssertLog(false, "Image read-access is not currently supported in OpenGLES");
    return 0;
#else
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialized image array");
        return 0;
    } else if (!can_read(_access)) {
        CUAssertLog(false, "This image does not support read access");
        return 0;
    }
    
    size_t end = data.size();
    size_t size = _width*_height*getByteSize();
    data.resize(end+size);
    uint8_t* start = reinterpret_cast<uint8_t*>(data.data());
    start += end;
    
    std::vector<uint8_t*> temp;
    temp.resize(_width*_height*_layers*getByteSize());
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, _data->images);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0,
                  gl_symbolic_format(_format),
                  gl_format_type(_format),
                  temp.data());
    GLenum error = glGetError();
    if (error) {
        CULogError("Could not read from image array %s: %s", _name.c_str(), gl_error_name(error).c_str());
        return 0;
    }
    
    std::memcpy(start,temp.data()+layer*size,size);
    return size;
#endif
}


#pragma mark -
#pragma mark Attributes
/**
 * Returns a unique identifier for this image
 *
 * This value can be used in memory-based hashes.
 *
 * @return a unique identifier for this image
 */
size_t ImageArray::id() const {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed texture");
    return _data->id();
}

/**
 * Returns the number of bytes in a single texel of this image.
 *
 * @return the number of bytes in a single texel of this image.
 */
size_t ImageArray::getByteSize() const {
    return gl_format_stride(_format);
}

/**
 * Returns a string representation of this image for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this image for debugging purposes.
 */
std::string ImageArray::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::graphics::ImageArray[" : "[");
    ss << getName() << ",";
    ss << "w:" << getWidth()  << ",";
    ss << "h:" << getHeight() << ",";
    ss << "l:" << getLayers();
    ss << "]";
    return ss.str();
}
