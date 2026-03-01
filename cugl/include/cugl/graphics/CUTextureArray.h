//
//  CUTextureArray.h
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
#ifndef __CU_TEXTURE_ARRAY_H__
#define __CU_TEXTURE_ARRAY_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUSize.h>
#include <cugl/core/math/CUVec2.h>
#include <vector>

namespace cugl {

    /**
     * The classes and functions needed to construct a graphics pipeline.
     *
     * Initially these were part of the core CUGL module. However, in the 
     * transition to Vulkan, we discovered that we need to factor this out.
     * This allows us to provide options for OpenGL, Vulkan, and even headless
     * clients.
     */
namespace graphics {

// Forward declarations
class ImageArray;
class Sampler;

/**
 * This is a class representing a texture array.
 *
 * A class is mostly a pair of an {@link ImageArray} and a {@link Sampler}.
 * Most of its functionality can be accessed through those two attributes.
 * Unlike the class {@link Texture}, we do not currently have texture atlas
 * support for texture arrays.
 *
 * Texture arrays are intended to be used with {@link GraphicsShader} objects.
 * As they can support either Vulkan or OpenGL, we do not support the binding
 * API of OpenGL. This functionality has been abstracted away.
 *
 * Note that texture arrays can only be constructed on the main thread when
 * using OpenGL. However, in Vulkan, images can be safely constructed on any
 * thread.
 */
class TextureArray {
#pragma mark Values
private:
    /** The decriptive texture name */
    std::string _name;

    /** The width in texels */
    Uint32 _width;
    /** The height in texels */
    Uint32 _height;
    /** The number of layers in the array */
    Uint32 _layers;

    /** The image data */
    std::shared_ptr<ImageArray> _image;
    /** The texture sampler */
    std::shared_ptr<Sampler> _sampler;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new empty texture array with no size.
     *
     * This method performs no allocations. You must call init to generate
     * a proper texture.
     */
    TextureArray();
    
    /**
     * Deletes this texture array, disposing all resources
     */
    ~TextureArray() { dispose(); }
    
    /**
     * Deletes the texture array and resets all attributes.
     *
     * You must reinitialize the texture array to use it.
     */
    void dispose();
    
    /**
     * Initializes an empty texture array with the given dimensions.
     *
     * The image array will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #initWithData} for mipmap support.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param width     The image width in texels
     * @param height    The image height in texels
     * @param layers    The number of layers in the array
     * @param format    The image data format
     * @param access    The image access
     *
     * @return true if initialization was successful.
     */
    bool init(Uint32 width, Uint32 height, Uint32 layers,
              TexelFormat format = TexelFormat::COLOR_RGBA,
              ImageAccess access = ImageAccess::READ_WRITE) {
        return initWithData(NULL, width, height, layers, format, access, false);
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
    bool initWithData(const std::byte *data,
                      Uint32 width, Uint32 height, Uint32 layers,
                      TexelFormat format = TexelFormat::COLOR_RGBA,
                      ImageAccess access = ImageAccess::READ_WRITE,
                      bool mipmaps = false);
    
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
    bool initWithImages(const std::shared_ptr<ImageArray>& images);

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
    bool initWithImages(const std::shared_ptr<ImageArray>& images,
                        const std::shared_ptr<Sampler>& sampler);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new empty texture with the given dimensions.
     *
     * The image array will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #allocWithData} for mipmap support.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param layers    The number of layers in the array
     * @param format    The texture data format
     * @param access    The image access
     *
     * @return a new empty texture with the given dimensions.
     */
    static std::shared_ptr<TextureArray> alloc(Uint32 width, Uint32 height, Uint32 layers,
                                               TexelFormat format = TexelFormat::COLOR_RGBA,
                                               ImageAccess access = ImageAccess::READ_WRITE) {
        std::shared_ptr<TextureArray> result = std::make_shared<TextureArray>();
        return (result->init(width, height, layers, format, access) ? result : nullptr);
    }
    
    /**
     * Returns a new texture array with the given data.
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
     * @return a new texture array with the given data.
     */
    static std::shared_ptr<TextureArray> allocWithData(const std::byte *data,
                                                       Uint32 width, Uint32 height, Uint32 layers,
                                                       TexelFormat format = TexelFormat::COLOR_RGBA,
                                                       ImageAccess access = ImageAccess::READ_WRITE,
                                                  bool mipmaps = false) {
        std::shared_ptr<TextureArray> result = std::make_shared<TextureArray>();
        return (result->initWithData(data, width, height, layers, format, access, mipmaps) ? result : nullptr);
        
    }
    
    /**
     * Returns a new texture array with the given image array.
     *
     * Note that this texture does not copy the image array, so any changes to
     * the image array elsewhere will affect this is texture array.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param images    The texture image array
     *
     * @return a new texture array with the given image array.
     */
    static std::shared_ptr<TextureArray> allocWithImages(const std::shared_ptr<ImageArray>& images) {
        std::shared_ptr<TextureArray> result = std::make_shared<TextureArray>();
        return (result->initWithImages(images) ? result : nullptr);
    }

    /**
     * Returns a new texture array with the given image  array and sampler.
     *
     * Note that this texture does not copy the image array, so any changes to
     * the image array elsewhere will affect this is texture array.
     *
     * @param images    The texture image array
     * @param sampler   The texture sampler
     *
     * @return a new texture array with the given image  array and sampler.
     */
    static std::shared_ptr<TextureArray> allocWithImages(const std::shared_ptr<ImageArray>& images,
                                                        const std::shared_ptr<Sampler>& sampler) {
        std::shared_ptr<TextureArray> result = std::make_shared<TextureArray>();
        return (result->initWithImages(images,sampler) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Data Access
    /**
     * Returns the image array component of this texture array
     *
     * In modern graphics APIs, texture arrays are composed of two objects: an
     * image array and a sampler. Modifications to this image will be reflected
     * in the texture during sampling.
     *
     * @return the image array component of this texture
     */
    const std::shared_ptr<ImageArray> getImages() const { return _image; }

    /**
     * Returns the sampler component of this texture
     *
     * In modern graphics APIs, texture arrays are composed of two objects: an
     * image array and a sampler. Modifications to this sampler will be
     * reflected in the texture during sampling.
     *
     * @return the sampler component of this texture
     */
    const std::shared_ptr<Sampler> getSampler() const { return _sampler; }

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
    const TextureArray& operator=(const std::byte *data) {
        return set(data);
    }
    
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
    const TextureArray& set(const std::byte *data);
    
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
    const TextureArray& set(const std::vector<std::byte>& data) {
        return set(data.data());
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
    bool setLayer(Uint32 layer, const std::byte *data);
    
    /**
     * Sets the specified layer to have the contents of the given buffer.
     *
     * This method is the same as in {@link ImageArray}. If layer is not a
     * valid layer, this method does nothing.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*layers*bytesize. See {@link ImageArray#getByteSize}
     * for a description of the latter.
     *
     * @param layer The texture layer to modify
     * @param data  The buffer to read into the texture array
     *
     * @return true if the data was successfully set
     */
    bool setLayer(Uint32 layer, const std::vector<std::byte>& data) {
        return setLayer(layer,data.data());
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
    const size_t get(std::vector<std::byte>& data);

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
    const size_t getLayer(Uint32 layer, std::vector<std::byte>& data);

    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this texture array.
     *
     * A name is a user-defined way of identifying a texture. Subtextures are
     * permitted to have different names than their parents.
     *
     * @param name  The name of this texture array.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this texture array.
     *
     * A name is a user-defined way of identifying a texture. Subtextures are
     * permitted to have different names than their parents.
     *
     * @return the name of this texture array.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns a unique identifier for this texture array
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this texture array
     */
    size_t id() const;
    
    /**
     * Returns the width of this texture array in texels.
     *
     * @return the width of this texture array in texels.
     */
    Uint32 getWidth() const { return _width; }
    
    /**
     * Returns the height of this texture array in texels.
     *
     * @return the height of this texture array in texels.
     */
    Uint32 getHeight() const { return _height; }
    
    /**
     * Returns the number of layers in this texture array.
     *
     * @return the number of layers in this texture array.
     */
    Uint32 getLayers() const { return _layers; }
    
    /**
     * Returns the size of this texture array in texels.
     *
     * @return the size of this texture array in pixels.
     */
    Size getSize() { return Size((float)_width,(float)_height); }

    
#pragma mark -
#pragma mark Conversions
    /**
     * Returns a string representation of this texture array for debugging.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this texture array for debugging.
     */
    std::string toString(bool verbose = false) const;
    
    /** Casts from Texture to a string. */
    operator std::string() const { return toString(); }
    
};

    }

}

#endif /* __CU_TEXTURE_ARRAY_H__ */
