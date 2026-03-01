//
//  CUImageArray.h
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
#ifndef __CU_IMAGE_ARRAY_H__
#define __CU_IMAGE_ARRAY_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUSize.h>

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

// An opaque class for the graphics API
class ImageArrayData;

/**
 * This is a class representing an image.
 *
 * This class represents an array of 2d images, each of which is a rectangular
 * collection of texels. The color format for these texels is determined by the
 * value {@link TexelFormat}. This value is a slightly optimized version of
 * {@link Image} that allows for multiple images in a single object. While less
 * valuable in Vulkan, it is a desirable optimization for OpenGL.
 *
 * Historically, many game engines treat images and textures as the same thing.
 * However, this is not the case in Vulkan and modern graphics APIs, where a
 * texture is actually an image plus a sampler. To maximize compatibility with
 * Vulkan, we have split textures into this class and the class {@link Sampler}.
 *
 * Each entry in an image array stores the pixel information for a texture, but
 * does not specify how to resize it or how to handle texture coordinates that
 * are out-of-bounds. Note that in Vulkan, a texture must explicitly specify the
 * number of mipmap levels, while this value is implicit in OpenGL. Our
 * interface assumes that all OpenGL images with mipmap level > 0 are the same.
 *
 * Note that image arrays can only be constructed on the main thread when using
 * OpenGL. However, in Vulkan, images can be safely constructed on any thread.
 */
class ImageArray {
#pragma mark Values
protected:
    /** The decriptive name of this image array */
    std::string _name;
    
    /** The width in texels */
    Uint32 _width;
    
    /** The height in texels */
    Uint32 _height;

    /** The number of images in the array */
    Uint32 _layers;

    /** The graphics API image array data  */
    ImageArrayData* _data;
    
    /** The access for this image array */
    ImageAccess _access;
    
    /** The pixel format of the image array */
    TexelFormat _format;
    
    /** Whether this image array has associated mipmaps */
    bool _mipmaps;
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new empty image array with no size.
     *
     * This method performs no allocations. You must call init to generate
     * a proper image.
     */
    ImageArray();
    
    /**
     * Deletes this image array, disposing all resources
     */
    ~ImageArray() { dispose(); }
    
    /**
     * Deletes the image array and resets all attributes.
     *
     * You must reinitialize the image to use it.
     */
    void dispose();
    
    /**
     * Initializes an empty image array with the given dimensions.
     *
     * The image will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #initWithData} for mipmap support.
     *
     * @param width     The image width in texels
     * @param height    The image height in texels
     * @param layers    The number of image layers
     * @param format    The image data format
     * @param access    The image access
     *
     * @return true if initialization was successful.
     */
    bool init(Uint32 width, Uint32 height, Uint32 layers,
              TexelFormat format = TexelFormat::COLOR_RGBA,
              ImageAccess access=ImageAccess::READ_WRITE) {
        return initWithData(NULL, width, height, layers, format, access, false);
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
    bool initWithData(const std::byte *data, Uint32 width, Uint32 height, Uint32 layers,
                      TexelFormat format = TexelFormat::COLOR_RGBA,
                      ImageAccess access=ImageAccess::READ_WRITE,
                      bool mipmaps = false);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new empty image array with the given dimensions.
     *
     * The image will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #allocWithData} for mipmap support.
     *
     * @param width     The image width in texels
     * @param height    The image height in texels
     * @param layers    The number of image layers
     * @param format    The image data format
     * @param access    The image access
     *
     * @return a new empty image array with the given dimensions.
     */
    static std::shared_ptr<ImageArray> alloc(Uint32 width, Uint32 height, Uint32 layers,
                                             TexelFormat format = TexelFormat::COLOR_RGBA,
                                             ImageAccess access=ImageAccess::READ_WRITE) {
        std::shared_ptr<ImageArray> result = std::make_shared<ImageArray>();
        return (result->init(width, height, layers, format, access) ? result : nullptr);
    }
    
    /**
     * Returns a new image array with the given data.
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
     * @return a new image array with the given data.
     */
    static std::shared_ptr<ImageArray> allocWithData(const std::byte *data,
                                                     Uint32 width, Uint32 height, Uint32 layers,
                                                     TexelFormat format = TexelFormat::COLOR_RGBA,
                                                     ImageAccess access=ImageAccess::READ_WRITE,
                                                     bool mipmaps = false) {
        std::shared_ptr<ImageArray> result = std::make_shared<ImageArray>();
        return (result->initWithData(data, width, height, layers, format, access, mipmaps) ? result : nullptr);
        
    }
    
#pragma mark -
#pragma mark Data Access
    /**
     * Returns the platform-specific implementation for this image array
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this image
     */
    ImageArrayData* getImplementation() { return _data; }

    /**
     * Sets this image array to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*layers*bytesize. See {@link #getByteSize} for
     * a description of the latter.
     *
     * Note that some images created by CUGL are read-only. For any such
     * images, this method will fail.
     *
     * @param data  The buffer to read into the image
     *
     * @return a reference to this (modified) image for chaining.
     */
    const ImageArray& operator=(const std::byte *data) {
        return set(data);
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
     * @return a reference to this (modified) image for chaining.
     */
    const ImageArray& set(const std::byte *data);
    
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
     * @return a reference to this (modified) image for chaining.
     */
    const ImageArray& set(const std::vector<std::byte>& data) {
        return set(data.data());
    }

    /**
     * Sets a single layer of this image to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link #getByteSize} for a
     * description of the latter.
     *
     * This method will fail if the access does not support CPU-side writes.
     *
     * @param layer The layer to read from
     * @param data  The buffer to read into the image
     *
     * @return true if the data was successfully set
     */
    bool setLayer(Uint32 layer, const std::byte *data);
    
    /**
     * Sets a single layer of this image to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link #getByteSize} for a
     * description of the latter.
     *
     * This method will fail if the access does not support CPU-side writes.
     *
     * @param layer The layer to read from
     * @param data  The buffer to read into the image
     *
     * @return true if the data was successfully set
     */
    bool setLayer(Uint32 layer, const std::vector<std::byte>& data) {
        return setLayer(layer, data.data());
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
    const size_t get(std::vector<std::byte>& data);
    
	/**
     * Fills the given buffer with a single layer in this texture.
     *
     * This method will append the data to the given vector. This method will
     * fail if the access does not support CPU-side reads.
     *
     * @param layer  The layer to read from
     * @param data   The buffer to store the texture data
     *
     * @return the number of bytes read into the buffer
     */
    const size_t getLayer(Uint32 layer, std::vector<std::byte>& data);
    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this image array.
     *
     * A name is a user-defined way of identifying a image array.
     *
     * @param name  The name of this image array.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this image array.
     *
     * A name is a user-defined way of identifying a image array.
     *
     * @return the name of this image array.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns a unique identifier for this image array
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this image array
     */
    size_t id() const;

    /**
     * Returns the access policy of this image array
     *
     * @return the access policy of this image array
     */
    ImageAccess getAccess() const { return _access; }
    
    /**
     * Returns the width of this image in texels.
     *
     * @return the width of this image in texels.
     */
    Uint32 getWidth()  const { return _width;  }
    
    /**
     * Returns the height of this image in texels.
     *
     * @return the height of this image in texels.
     */
    Uint32 getHeight() const { return _height; }
    
    /**
     * Returns the number of layers in this image array.
     *
     * @return the number of layers in this image array.
     */
    Uint32 getLayers() const { return _layers; }

    /**
     * Returns the size of this image in texels.
     *
     * @return the size of this image in texels.
     */
    Size getSize() const { return Size((float)_width,(float)_height); }
    
    /**
     * Returns the number of bytes in a single texel of this image array.
     *
     * @return the number of bytes in a single texel of this image array.
     */
    size_t getByteSize() const;
    
    /**
     * Returns the data format of this image array.
     *
     * The data format determines what type of data can be assigned to this
     * image.
     *
     * @return the data format of this image array.
     */
    TexelFormat getFormat() const { return _format; }
    
    /**
     * Returns whether this image array has generated mipmaps.
     *
     * This method will not indicate the number of mipmap levels in Vulkan.
     * Instead, it treats mipmaps as all-or-nothing (like OpenGL).
     *
     * @return whether this image array has generated mipmaps.
     */
    bool hasMipMaps() const { return _mipmaps; }
    
    /**
     * Returns a string representation of this image array for debugging purposes.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this image array for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Casts from ImageArray to a string. */
    operator std::string() const { return toString(); }
    
};

    }

}

#endif /* __CU_IMAGE_H__ */
