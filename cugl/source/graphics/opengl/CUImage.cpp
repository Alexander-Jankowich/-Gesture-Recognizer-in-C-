//
//  CUImage.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D image source. In
//  OpenGL (and many game engines), image and texture are synonymous. However,
//  in modern graphics APIs like Vulkan, they are slightly different.
//  Technically a texture is an image plus a sampler. Since CUGL 4.0, we have
//  separated these two concepts for forward-looking compatibility with
//  Vulkan.
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
#include "CUGLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

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
 * Removes the alpha premultiplication from the given surface.
 *
 * This should be done right before the surface is saved to a file. This
 * prevents the image from being premultiplied twice on a reload.
 *
 * @param surface   The surface to adjust
 */
static void unpremultiply(SDL_Surface* surface) {
    for (int y = 0; y < surface->h; ++y) {
        uint8_t* row = reinterpret_cast<uint8_t*>(surface->pixels) + y * surface->pitch;
        for (int x = 0; x < surface->w; ++x) {
            uint8_t* px = row + 4 * x;
            uint8_t a = px[3];
            if (a == 0) {
                px[0] = px[1] = px[2] = 0;
            } else {
                px[0] = (uint8_t)std::min(255, (px[0] * 255 + a / 2) / a);
                px[1] = (uint8_t)std::min(255, (px[1] * 255 + a / 2) / a);
                px[2] = (uint8_t)std::min(255, (px[2] * 255 + a / 2) / a);
            }
        }
    }
}

#pragma mark -
#pragma mark Constructors

/**
 * Creates a new empty image with no size.
 *
 * This method performs no allocations. You must call init to generate
 * a proper image.
 */
Image::Image() :
_name(""),
_originx(0),
_originy(0),
_width(0),
_height(0),
_premult(false),
_mipmaps(false),
_data(nullptr),
_parent(nullptr) {
    _format = TexelFormat::UNDEFINED;
    _access = ImageAccess::SHADER_ONLY;
    _minS = _minT = 0;
    _maxS = _maxT = 1;
}

/**
 * Deletes the image and resets all attributes.
 *
 * You must reinitialize the image to use it.
 */
void Image::dispose() {
    if (_data != nullptr) {
        // Do we own the image?
        if (_parent == nullptr) {
            _data->dispose();
            delete _data;
        }
        _data = nullptr;
        _originx = 0; _originy = 0;
        _width = 0;   _height = 0;
        _parent = nullptr;
        _mipmaps = false;
        _premult = false;
        _format = TexelFormat::UNDEFINED;
        _access = ImageAccess::SHADER_ONLY;
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
    if (!_data->init(data, width, height, mipmaps, format)) {
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
    
    // We may need to adjust packing
    bool repack = false;
    SDL_PremultiplySurfaceAlpha(normal, false);
    if (normal->pitch != normal->w*sizeof(Uint32)) {
        int pixels_per_row = normal->pitch / SDL_BYTESPERPIXEL(normal->format);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pixels_per_row);
        repack = true;
    }
    
    sdl_flip_vertically(normal);
    bool result = initWithData(reinterpret_cast<std::byte*>(normal->pixels),
                               normal->w, normal->h,
                               TexelFormat::COLOR_RGBA,
                               access, mipmaps);
    
    // Restore the row length
    if (repack) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    SDL_DestroySurface(normal);
    if (result) setName(filename);
    _premult = true;
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

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    glBindTexture(GL_TEXTURE_2D, _data->image);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height,
                    gl_symbolic_format(_format),
                    gl_format_type(_format),
                    bytes);
    
    if (_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
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
#if CU_GRAPHICS_API == CU_OPENGLES
    CUAssertLog(false, "Image read-access is not currently supported in OpenGLES");
    return 0;
#else
    // TODO: No subimage support right now
    if (_parent != nullptr) {
        CUAssertLog(false, "Image read-access is not currently supported for subimages");
        return 0;
    }

    if (_data == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialized image");
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
    
    glBindTexture(GL_TEXTURE_2D, _data->image);
    glGetTexImage(GL_TEXTURE_2D, 0,
                  gl_symbolic_format(_format),
                  gl_format_type(_format),
                  start);
    
    GLenum error = glGetError();
    if (error) {
        CULogError("Could not read from image %s: %s", _name.c_str(), gl_error_name(error).c_str());
        return 0;
    }

    return size;
#endif
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
#if CU_GRAPHICS_API == CU_OPENGLES
    CUAssertLog(false, "Image read-access is not currently supported in OpenGLES");
    return 0;
#else
    // TODO: No subimage support right now
    if (_parent != nullptr) {
        CUAssertLog(false, "Image read-access is not currently supported for subimages");
        return 0;
    }
    
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
    unpremultiply(surface);

    std::string fullpath = filetool::normalize_path(file);
    bool success = IMG_SavePNG(surface, fullpath.c_str());
    if (!success) {
        CULogError("Could not save to file %s: %s", file.c_str(), SDL_GetError());
    }
    SDL_DestroySurface(surface);
    return success;
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
size_t Image::id() const {
    CUAssertLog(_data != nullptr, "Cannot access an uninitialzed image");
    return _data->id();
}

/**
 * Returns the number of bytes in a single texel of this image.
 *
 * @return the number of bytes in a single texel of this image.
 */
size_t Image::getByteSize() const {
    return gl_format_stride(_format);
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
    int maxt = 0;   // How big a texture does this platform support?
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxt);
    return maxt > 0 ? (Uint32)maxt : 0;
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
    if (_parent != nullptr) {
        ss << ",";
        ss << " (" << _minS << "," << _maxS << ")";
        ss << "x(" << _minT << "," << _maxT << ")";
    }
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
