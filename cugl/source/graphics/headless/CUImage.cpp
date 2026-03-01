//
//  CUImage.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D image source. In
//  OpenGL (and many game engines), image and texture are synonymous. However,
//  in modern graphics APIs like Vulkan, they are slightly different. This
//  headless version stores image data (and will even load images from a file)
//  but does not communicate with a graphics card.
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
#include <cugl/core/CUApplication.h>
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/util/CUFiletools.h>
#include <cugl/graphics/CUImage.h>
#include <sstream>
#include <limits>
#include "CUHLOpaques.h"

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
 * Creates a new empty image with no size.
 *
 * This method performs no allocations. You must call init to generate
 * a proper image.
 */
Image::Image() :
_width(0),
_height(0),
_name(""),
_data(nullptr) {
    _format = TexelFormat::UNDEFINED;
    _access = ImageAccess::SHADER_ONLY;
}

/**
 * Deletes the image and resets all attributes.
 *
 * You must reinitialize the image to use it.
 */
void Image::dispose() {
    if (_data != nullptr) {
        _data->dispose();
        delete _data;
        _data = nullptr;
        _width = 0; _height = 0;
        _format = TexelFormat::UNDEFINED;
        _name = "";
    }
}

/**
 * Initializes an image with the given data and access.
 *
 * The data format must match the one specified by the format parameter.
 * If the parameter mipmaps is true, the image will generate an appropriate
 * number of mipmaps determined by its size.
 *
 * @param data      The image data (size width*height*format)
 * @param width     The image width in pixels
 * @param height    The image height in pixels
 * @param format    The image data format
 * @param access    The image access
 * @param mipmaps   A flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool Image::initWithData(const std::byte *data, Uint32 width, Uint32 height,
                         TexelFormat format, ImageAccess access, bool mipmaps) {
    if (_data != nullptr) {
        CUAssertLog(false, "Image is already initialized");
        return false; // In case asserts are off.
    }
    
    _data = new ImageData();
    if (!_data->init(data, width, height, format)) {
        delete _data;
        return false;
    }

    _access = access;
    _width  = width;
    _height = height;
    _format = format;
    _mipmaps = mipmaps;

    std::stringstream ss;
    ss << "@" << data;
    setName(ss.str());
    return true;
}

/**
 * Initializes an image with the data from the given file.
 *
 * This method can load any file format supported by SDL_Image. This
 * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX. The
 * image will be stored in RGBA format, even if it is a file format that
 * does not support transparency (e.g. JPEG).
 *
 * If the parameter mipmaps is true, the image will generate an appropriate
 * number of mipmaps determined by its size.
 *
 * If the file path is relative, it is assumed to be with respect to
 * {@link Application#getAssetDirectory}. If you wish to load an image from
 * somewhere else, you must use an absolute path.
 *
 * @param filename  The file supporting the texture file.
 * @param mipmaps   A flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool Image::initWithFile(const std::string filename, ImageAccess access, bool mipmaps) {
    if (_data != nullptr) {
        CUAssertLog(false, "Image is already initialized");
        return false; // In case asserts are off.
    }
    
    std::string fullpath;
    if (!filetool::is_absolute(filename)) {
        fullpath = Application::get()->getAssetDirectory();
        fullpath.append(filename);
        fullpath = filetool::canonicalize_path(fullpath);
    } else {
        fullpath = filetool::canonicalize_path(filename);
    }
    SDL_Surface* surface = IMG_Load(fullpath.c_str());
    if (surface == nullptr) {
        CULogError("Could not load file %s. %s", filename.c_str(), SDL_GetError());
        return false;
    }

    SDL_Surface* normal = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (normal == nullptr) {
        CULogError("Could not process file %s. %s", filename.c_str(), SDL_GetError());
        return false;
    }
    
    sdl_flip_vertically(normal);
    bool result = initWithData(reinterpret_cast<std::byte*>(normal->pixels),
                               normal->w, normal->h,
                               TexelFormat::COLOR_RGBA,
                               access, mipmaps);
    
    SDL_DestroySurface(normal);
    if (result) setName(filename);
    
    return result;

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
const Image& Image::set(const std::byte *data) {
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot set data to an uninitialized image");
        return *this;
    } else if (!can_write(_access)) {
        CUAssertLog(false, "This image does not support write access");
        return *this;
    }
    
    size_t size = _width*_height*texel_stride(_format);
    std::memcpy(_data->data,data,size);
    return *this;
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
const size_t Image::get(std::vector<std::byte>& data) {
    size_t end = data.size();
    size_t size = _width*_height*texel_stride(_format);
    data.resize(end+size);
    
    
    uint8_t* start = reinterpret_cast<uint8_t*>(data.data());
    start += end;
    std::memcpy(start, _data->data, size);
    return size;
}

/**
 * Returns true if able to save the image to the given file.
 *
 * The image will be saved as a PNG file. If the suffix of file is not a
 * .png, then this suffix will be added.
 *
 * Note that some images, such as those produced by {@link RenderTarget},
 * have an internal representation that cannot be exported. For any such
 * image, this method will fail, returning false.
 *
 * IMPORTANT: In CUGL, relative path names always refer to the asset
 * directory, which is a read-only directory. Therefore, the file must
 * must be specified with an absolute path. Using a relative path for this
 * method will cause this method to fail.
 *
 * @param file  The name of the file to write to
 *
 * @return true if able to save the image to the given file.
 */
bool Image::save(const std::string file) {
    if (_data == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture");
        return false;
    } else if (!can_read(_access)) {
        CUAssertLog(false, "This image does not support read access");
        return false;
    }
    
    std::vector<std::byte> buffer;
    if (get(buffer) == 0) {
        return false;
    }
    
    SDL_Surface *surface = SDL_CreateSurface(_width, _height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        CULogError("Could not convert image format: %s", SDL_GetError());
        return false;
    }

    uint8_t *flipped = reinterpret_cast<uint8_t *>(surface->pixels);
    for (int y = 0; y < _height; y++) {
        std::memcpy(flipped + (_height - 1 - y) * _width * 4,
                    buffer.data()  + y * _width * 4,
                    _width * 4
                    );
    }

    std::string fullpath = filetool::normalize_path(file);
    bool success = IMG_SavePNG(surface, fullpath.c_str());
    if (!success) {
        CULogError("Could not save to file %s: %s", file.c_str(), SDL_GetError());
    }
    SDL_DestroySurface(surface);
    return success;
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
size_t Image::id() const {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed texture");
    return _data->id();
}

/**
 * Returns the number of bytes in a single texel of this image.
 *
 * @return the number of bytes in a single texel of this image.
 */
size_t Image::getByteSize() const {
    return texel_stride(_format);
}

/**
 * Returns the maximum image size (per dimension) for this platform
 *
 * This value is useful for when we want to allocate an image buffer
 * and want to make sure we do not exceed the platform limitations
 *
 * @return the maximum image size (per dimension) for this platform
 */
Uint32 Image::getMaximumSize() {
    return std::numeric_limits<Uint32>::max();
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
std::string Image::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::graphics::Image[" : "[");
    ss << getName() << ",";
    ss << "w:" << getWidth() << ",";
    ss << "h:" << getHeight();
    ss << "]";
    return ss.str();
}

#pragma mark -
#pragma mark Atlas Support
/**
 * Returns a subimage with the given dimensions.
 *
 * The values x,y,w, and h must specify a subrectangle inside of the image.
 * All values are in texels. In defining this rectangle, we put the origin
 * in the bottom left corner. Positive y values go up the image from the
 * bottom.
 *
 * It is the responsibility of the user to rescale the texture coordinates
 * of a mesh when using a subimage. Otherwise, the graphics pipeline will
 * just use the original image instead. See the method internal method
 * {@link SpriteBatch#prepare} for an example of how to scale texture
 * coordinates.
 *
 * It is possible to make a subimage of a subimage. However, in that case,
 * the parent of the new subimage will be the original root image. So no
 * tree of subtextures is more than one level deep.
 *
 * @param x The x-coordinate of the bottom left in texels
 * @param x The y-coordinate of the bottom left in texels
 * @param w The region width in texels
 * @param h The region height in texels
 *
 * @return a subimage with the given dimensions.
 */
std::shared_ptr<Image> Image::getSubImage(Uint32 x, Uint32 y, Uint32 w, Uint32 h) {
    CUAssertLog(_data != nullptr, "The image is not initialized");
    CUAssertLog(x >= _originx && x <= _originx+getWidth(), "Value x=%d is out of range",x);
    CUAssertLog(x+w <= getWidth(), "Value w=%d is out of range",w);
    CUAssertLog(y >= _originy && y <= _originy+getHeight(), "Value y=%d is out of range",y);
    CUAssertLog(y+h <= getHeight(), "Value h=%d is out of range",h);

    std::shared_ptr<Image> result = std::make_shared<Image>();

    // Make sure the tree is not deep
    std::shared_ptr<Image> source = (_parent == nullptr ? shared_from_this() : _parent);

    // Shared values
    result->_parent = source;
    result->_name = source->_name;
    result->_data = source->_data;
    result->_access = source->_access;
    result->_format = source->_format;
    result->_mipmaps = source->_mipmaps;

    // Set the size information
    float tw = getWidth();
    float th = getHeight();
    result->_originx = _originx+x;
    result->_originy = _originx+y;
    result->_width  = w;
    result->_height = h;
    result->_minS = ((float)result->_originx)/tw;
    result->_maxS = ((float)(result->_originx+w))/tw;
    result->_minT = ((float)result->_originy)/th;
    result->_maxT = ((float)(result->_originy+h))/th;
    
    return result;
}
