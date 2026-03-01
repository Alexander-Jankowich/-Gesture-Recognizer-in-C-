//
//  CUTexture.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a 2D texture. In the move
//  to Vulkan, we split most of the functionality of this class into two
//  different classes: Image and Sampler. So this class is largely a pair of
//  these two classes.
//
//  With that said, this class does provide one additional feature. It provides
//  some support for simple texture atlases. Any non-repeating texture can
//  produce a subtexture. A subtexture wraps the same mage data (and so
//  does not require a context switch in the rendering pipeline), but has
//  different start and end boundaries, as defined by minS, maxS, minT and maxT.
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
//  Version: 12/2/25 (SDL3 Integration)
//
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/util/CUFiletools.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CUImage.h>
#include <cugl/graphics/CUSampler.h>
#include <sstream>
#include "CUGLOpaques.h"
#include <cugl/core/CUApplication.h>

using namespace cugl;
using namespace cugl::graphics;

#pragma mark Internal Helpers
#pragma mark Internal Helpers
/**
 * This array is the data of a white image with 2 by 2 dimension.
 * It's used for creating a default texture when the texture is a nullptr.
 */
static uint8_t cu_2x2_white_image[] = {
    // RGBA8888
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

/** The blank texture corresponding to cu_2x2_white_image */
std::shared_ptr<Texture> Texture::_blank = nullptr;

#pragma mark -
#pragma mark Constructors
/**
 * Creates a new empty texture with no size.
 *
 * This method performs no allocations. You must call init to generate
 * a proper texture.
 */
Texture::Texture() :
_name("") {
}

/**
 * Deletes the texture and resets all attributes.
 *
 * You must reinitialize the texture to use it.
 */
void Texture::dispose() {
    if (_image != nullptr) {
        _image   = nullptr;
        _sampler = nullptr;
        _name = "";
    }
}

/**
 * Initializes an empty texture with the given dimensions.
 *
 * The image will have all of its texels set to 0. You should access the
 * image with {@link #getImage} and use {@link Image#set} method to load
 * data into the image.
 *
 * The sampler will be the default, using a {@link TextureFilter#LINEAR}
 * filter and {@link TextureWrap#CLAMP} on both axes.
 *
 * @param width     The image width in texels
 * @param height    The image height in texels
 * @param format    The texture data format
 * @param access    The image access
 * @param mipmaps   Flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool Texture::initWithData(const std::byte *data, Uint32 width, Uint32 height,
                           TexelFormat format, ImageAccess access, bool mipmaps) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    }
    
    _image = Image::allocWithData(data, width, height, format, access, mipmaps);
    _sampler = Sampler::alloc();
    std::stringstream ss;
    ss << "@" << data;
    setName(ss.str());
    
    return _image != nullptr && _sampler != nullptr;
}

/**
 * Initializes an texture with the data from the given file.
 *
 * This method can load any file format supported by SDL_Image. This
 * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX. The
 * image will be stored in RGBA format, even if it is a file format that
 * does not support transparency (e.g. JPEG).
 *
 * If the parameter mipmaps is true, the image will generate an appropriate
 * number of mipmaps determined by its size.
 *
 * The sampler will be the default, using a {@link TextureFilter#LINEAR}
 * filter and {@link TextureWrap#CLAMP} on both axes.
 *
 * IMPORTANT: In CUGL, relative path names always refer to the asset
 * directory. If you wish to load a texture from somewhere else, you must
 * use an absolute pathname.
 *
 * @param filename  The file supporting the texture file.
 * @param access    The image access
 * @param mipmaps   Flag to generate mipmaps
 *
 * @return true if initialization was successful.
 */
bool Texture::initWithFile(const std::string filename, ImageAccess access, bool mipmaps) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    }

    _image = Image::allocWithFile(filename,access,mipmaps);
    _sampler = Sampler::alloc();
    
    setName(filename);
    return _image != nullptr && _sampler != nullptr;
}

/**
 * Initializes an texture with the given image.
 *
 * This texture will cover the entire image, and not just a subregion.
 * Not that this texture does not copy the image, so any changes to the
 * image elsewhere will affect this texture.
 *
 * The sampler will be the default, using a {@link TextureFilter#LINEAR}
 * filter and {@link TextureWrap#CLAMP} on both axes.
 *
 * @param image     The texture image
 *
 * @return true if initialization was successful.
 */
bool Texture::initWithImage(const std::shared_ptr<Image>& image) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    } else if (image == nullptr) {
        return false;
    }

    _image = image;
    _sampler = Sampler::alloc();
    
    setName(image->getName());
    return _sampler != nullptr;
}

/**
 * Initializes an texture with the given image and sampler.
 *
 * This texture will cover the entire image, and not just a subregion.
 * Not that this texture does not copy the image, so any changes to the
 * image elsewhere will affect this texture.
 *
 * @param image     The texture image
 * @param sampler   The texture sampler
 *
 * @return true if initialization was successful.
 */
bool Texture::initWithImage(const std::shared_ptr<Image>& image,
                            const std::shared_ptr<Sampler>& sampler) {
    if (_image != nullptr) {
        CUAssertLog(false, "Texture is already initialized");
        return false; // In case asserts are off.
    } else if (image == nullptr || sampler == nullptr) {
        return false;
    }
    
    _image = image;
    _sampler = sampler;
    setName(image->getName());
    return true;
}

/**
 * Returns a blank texture that can be used to make solid shapes.
 *
 * Allocating a texture requires the use of texture offset 0.  Any texture
 * bound to that offset will be unbound.  In addition, once initialization
 * is done, this texture will not longer be bound as well.
 *
 * This is the texture used by {@link SpriteBatch} when the active texture
 * is nullptr. It is a 2x2 texture of all white pixels. Using this texture
 * means that all shapes and outlines will be drawn with a solid color.
 *
 * This texture is a singleton. There is only one of it.  All calls to
 * this method will return a reference to the same object.
 *
 * @return a blank texture that can be used to make solid shapes.
 */
const std::shared_ptr<Texture>& Texture::getBlank() {
    if (_blank == nullptr) {
        const std::byte* bytes = reinterpret_cast<const std::byte *>(cu_2x2_white_image);
        auto image = Image::allocWithData(bytes, 2, 2, TexelFormat::COLOR_RGBA,
                                          ImageAccess::READ_WRITE, false);
        auto samp  = Sampler::allocWithWrap(TextureWrap::REPEAT, TextureWrap::REPEAT);
        _blank = Texture::allocWithImage(image, samp);
    }
    return _blank;
}


#pragma mark -
#pragma mark Data Access
/**
 * Returns the size of this texture in texels.
 *
 * If this is a subtexture, this will return the height of the subregion.
 *
 * @return the size of this texture in pixels.
 */
Size Texture::getSize() const {
    float w = _image->getWidth();
    float h = _image->getHeight();
    return Size(w,h);
}

/**
 * Sets this texture to have the contents of the given buffer.
 *
 * The buffer must have the correct data format. In addition, the buffer
 * must be size width*height*bytesize. See {@link #getByteSize} for
 * a description of the latter.
 *
 * This method will fail on subtextures.
 *
 * @param data  The buffer to read into the texture
 *
 * @return a reference to this (modified) texture for chaining.
 */
const Texture& Texture::set(const std::byte *data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture");
        return *this;
    }
    
    _image->set(data);
    return *this;
}

/**
 * Fills the given buffer with data from this texture.
 *
 * This method will append the data to the given vector. In the case of
 * subtextures, this will only extract the pixels in the region.
 *
 * @param data  The buffer to store the texture data
 *
 * @return the number of bytes read into the buffer
 */
const size_t Texture::get(std::vector<std::byte>& data) {
    if (_image == nullptr) {
        CUAssertLog(false, "Cannot access an uninitialzed texture");
        return 0;
    }
    return _image->get(data);
}

#pragma mark -
#pragma mark Atlas Support
/**
 * Returns a subtexture with the given dimensions.
 *
 * The values x,y,w, and h must specify a subrectangle inside of the texure
 * image. All values are in texels. In defining this rectangle, we put the
 * origin in the bottom left corner with positive y values go up in the
 * texture image.
 *
 * It is the responsibility of the user to rescale the texture coordinates
 * of a mesh when using a subtexture. Otherwise, the graphics pipeline will
 * just use the original texture instead. See the method internal method
 * {@link SpriteBatch#prepare} for an example of how to scale texture
 * coordinates.
 *
 * It is possible to make a subtexture of a subtexture. However, in that
 * case, the parent of the new subtexture will be the original root texture.
 * So no tree of subtextures is more than one level deep.
 *
 * Note that subtextures
 * changes to these objects will be reflected in the parent as well.
 *
 * @param x The x-coordinate of the bottom left in texels
 * @param x The y-coordinate of the bottom left in texels
 * @param w The region width in texels
 * @param h The region height in texels
 *
 * @return a subtexture with the given dimensions.
 */
std::shared_ptr<Texture> Texture::getSubTexture(Uint32 x, Uint32 y,
                                                Uint32 w, Uint32 h) {
    auto image = _image->getSubImage(x, y, w, h);
    auto sampler = Sampler::alloc(_sampler->getWrapS(), _sampler->getWrapT(),
                                  _sampler->getMagFilter(),
                                  _sampler->getMinFilter());
    
    std::shared_ptr<Texture> result = Texture::allocWithImage(image, sampler);
    result->_name = _name;
    return result;
}

#pragma mark -
#pragma mark Conversions
/**
 * Returns a unique identifier for this texture
 *
 * This value can be used in memory-based hashes.
 *
 * @return a unique identifier for this texture
 */
size_t Texture::id() const {
    CUAssertLog(_image != nullptr, "The texture is not initialized");
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
std::string Texture::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::graphics::Texture[" : "[");
    ss << getName() << ",";
    ss << "w:" << getWidth() << ",";
    ss << "h:" << getHeight();
    if (isSubTexture()) {
        ss << ",";
        ss << " (" << getMinS() << "," << getMaxS() << ")";
        ss << "x(" << getMinT() << "," << getMaxT() << ")";
    }
    ss << "]";
    return ss.str();
}
