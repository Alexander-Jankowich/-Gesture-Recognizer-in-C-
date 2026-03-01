//
//  CUImage.h
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
#ifndef __CU_IMAGE_H__
#define __CU_IMAGE_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUVec2.h>
#include <cugl/core/math/CUSize.h>
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

// An opaque class for the graphics API
class ImageData;

/**
 * This is a class representing an image.
 *
 * This class represents a single 2d image, which is a rectangular collection
 * of texels. The color format for these texels is determined by the value
 * {@link TexelFormat}.
 *
 * Historically, many game engines treat images and textures as the same thing.
 * However, this is not the case in Vulkan and modern graphics APIs, where a
 * texture is actually an image plus a sampler. To maximize compatibility with
 * Vulkan, we have split textures into this class and the class {@link Sampler}.
 *
 * An image stores the pixel information for a texture, but does not specify
 * how to resize it or how to handle texture coordinates that are out-of-bounds.
 * Note that in Vulkan, a texture must explicitly specify the number of mipmap
 * levels, while this value is implicit in OpenGL. Our interface assumes that
 * all OpenGL textures with mipmap level > 0 are the same.
 *
 * Because this class stores the image information for a {@link Texture}, this
 * class is also where we provide support for atlases. An atlas allows us to
 * carve an image up into smaller subimages. A subimage wraps the same image
 * data (and so does not require a context switch in the rendering pipeline),
 * but has different start and end boundaries, as defined by {@link #getMinS},
 * {@link #getMaxS}, {@link #getMinT}, and {@link #getMaxT}. See
 * {@link #getSubImage} for more information.
 *
 * Note that we talk about texture coordinates in terms of S (horizontal) and
 * T (vertical). This is different from other implementations that use UV
 * instead.
 *
 * Images can only be constructed on the main thread when using OpenGL. However,
 * in Vulkan, images can be safely constructed on any thread.
 */
class Image : public std::enable_shared_from_this<Image> {
#pragma mark Values
protected:
    /** The decriptive name of this image */
    std::string _name;
    
    /** The horizontal origin of the texture region */
    Uint32 _originx;
    /** The vertical origin of the texture region */
    Uint32 _originy;
    /** The width in texels */
    Uint32 _width;
    /** The height in texels */
    Uint32 _height;
    
    /** Whether the image uses premultiplied alpha */
    bool _premult;
    
    /** The graphics API image data  */
    ImageData* _data;
    
    /** The access for this image */
    ImageAccess _access;
    
    /** The pixel format of the image */
    TexelFormat _format;
    
    /** Whether this image has associated mipmaps */
    bool _mipmaps;
    
    /// Image atlas support
    /** Our parent, who owns the image (or nullptr if we own it) */
    std::shared_ptr<Image> _parent;
    
    /** The image min S (used for image atlases) */
    float _minS;
    /** The image max S (used for image atlases) */
    float _maxS;
    /** The image min T (used for image atlases) */
    float _minT;
    /** The image max T (used for image atlases) */
    float _maxT;
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new empty image with no size.
     *
     * This method performs no allocations. You must call init to generate
     * a proper image.
     */
    Image();
    
    /**
     * Deletes this image, disposing all resources
     */
    ~Image() { dispose(); }
    
    /**
     * Deletes the image and resets all attributes.
     *
     * You must reinitialize the image to use it.
     */
    void dispose();
    
    /**
     * Initializes an empty image with the given dimensions.
     *
     * The image will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #initWithData} for mipmap support.
     *
     * @param width     The image width in texels
     * @param height    The image height in texels
     * @param format    The image data format
     * @param access    The image access
     *
     * @return true if initialization was successful.
     */
    bool init(Uint32 width, Uint32 height,
              TexelFormat format = TexelFormat::COLOR_RGBA,
              ImageAccess access=ImageAccess::READ_WRITE) {
        return initWithData(NULL, width, height, format, access, false);
    }
    
    /**
     * Initializes an image with the given data and access.
     *
     * The data format must match the one specified by the format parameter.
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * This initializer assumes that alpha is not premultiplied. You should call
     * {@link #setPremultiplied} if this is not the case.
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
    bool initWithData(const std::byte *data, Uint32 width, Uint32 height,
                      TexelFormat format = TexelFormat::COLOR_RGBA,
                      ImageAccess access=ImageAccess::READ_WRITE,
                      bool mipmaps = false);
    
    /**
     * Initializes an image with the data from the given file.
     *
     * This method can load any file format supported by SDL_Image. This
     * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX. The
     * image will be stored in RGBA format, even if it is a file format that
     * does not support transparency (e.g. JPEG). Because of how SDL3 processes
     * image files, any image loaded this way will have premultiplied alpha.
     *
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * If the file path is relative, it is assumed to be with respect to
     * {@link Application#getAssetDirectory}. If you wish to load an image from
     * somewhere else, you must use an absolute path.
     *
     * @param filename  The file supporting the texture file.
     * @param access    The image access
     * @param mipmaps   A flag to generate mipmaps
     *
     * @return true if initialization was successful.
     */
    bool initWithFile(const std::string filename,
                      ImageAccess access=ImageAccess::READ_WRITE,
                      bool mipmaps = false);

    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new empty image with the given dimensions.
     *
     * The image will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #allocWithData} for mipmap support.
     *
     * @param width     The image width in texels
     * @param height    The image height in texels
     * @param format    The image data format
     * @param access    The image access
     *
     * @return a new empty image with the given dimensions.
     */
    static std::shared_ptr<Image> alloc(Uint32 width, Uint32 height,
                                        TexelFormat format = TexelFormat::COLOR_RGBA,
                                        ImageAccess access=ImageAccess::READ_WRITE) {
        std::shared_ptr<Image> result = std::make_shared<Image>();
        return (result->init(width, height, format, access) ? result : nullptr);
    }
    
    /**
     * Returns a new image with the given data.
     *
     * The data format must match the one specified by the format parameter.
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * This initializer assumes that alpha is not premultiplied. You should call
     * {@link #setPremultiplied} if this is not the case.
     *
     * @param data      The image data (size width*height*format)
     * @param width     The image width in pixels
     * @param height    The image height in pixels
     * @param format    The image data format
     * @param access    The image access
     * @param mipmaps   A flag to generate mipmaps
     *
     * @return a new image with the given data.
     */
    static std::shared_ptr<Image> allocWithData(const std::byte *data, Uint32 width, Uint32 height,
                                                TexelFormat format = TexelFormat::COLOR_RGBA,
                                                ImageAccess access=ImageAccess::READ_WRITE,
                                                bool mipmaps = false) {
        std::shared_ptr<Image> result = std::make_shared<Image>();
        return (result->initWithData(data, width, height, format, access, mipmaps) ? result : nullptr);
        
    }
    
    /**
     * Returns a new image with the data from the given file.
     *
     * This method can load any file format supported by SDL_Image. This
     * includes (but is not limited to) PNG, JPEG, GIF, TIFF, BMP and PCX. The
     * image will be stored in RGBA format, even if it is a file format that
     * does not support transparency (e.g. JPEG). Because of how SDL3 processes
     * image files, any image loaded this way will have premultiplied alpha.
     *
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * If the file path is relative, it is assumed to be with respect to
     * {@link Application#getAssetDirectory}. If you wish to load an image from
     * somewhere else, you must use an absolute path.
     *
     * @param filename  The file supporting the texture file.
     * @param access    The image access
     * @param mipmaps   A flag to generate mipmaps
     *
     * @return a new image with the given data
     */
    static std::shared_ptr<Image> allocWithFile(const std::string filename,
                                                ImageAccess access=ImageAccess::READ_WRITE,
                                                bool mipmaps = false) {
        std::shared_ptr<Image> result = std::make_shared<Image>();
        return (result->initWithFile(filename, access, mipmaps) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Data Access
    /**
     * Returns the platform-specific implementation for this image
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this image
     */
    ImageData* getImplementation() { return _data; }

    /**
     * Sets this image to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link #getByteSize} for a
     * description of the latter.
     *
     * Note that some images created by CUGL are read-only. For any such
     * images, this method will fail.
     *
     * @param data  The buffer to read into the image
     *
     * @return a reference to this (modified) image for chaining.
     */
    const Image& operator=(const std::byte *data) {
        return set(data);
    }
    
    /**
     * Sets this image to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link #getByteSize} for
     * a description of the latter.
     *
     * This method will fail if the access does not support CPU-side writes.
     *
     * @param data  The buffer to read into the image
     *
     * @return a reference to this (modified) image for chaining.
     */
    const Image& set(const std::byte *data);
    
    /**
     * Sets this image to have the contents of the given buffer.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link #getByteSize} for
     * a description of the latter.
     *
     * This method will fail if the access does not support CPU-side writes.
     *
     * @param data  The buffer to read into the image
     *
     * @return a reference to this (modified) image for chaining.
     */
    const Image& set(const std::vector<std::byte>& data) {
        set(data.data());
        return *this;
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
     * Returns true if able to save the image to the given file.
     *
     * The image will be saved as a PNG file. If the suffix of file is not a
     * .png, then this suffix will be added.
     *
     * This method will fail if the access does not support CPU-side reads.
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
    bool save(const std::string file);
    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this image.
     *
     * A name is a user-defined way of identifying a image.
     *
     * @param name  The name of this image.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this image.
     *
     * A name is a user-defined way of identifying a image.
     *
     * @return the name of this image.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns a unique identifier for this image
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this image
     */
    size_t id() const;

    /**
     * Returns the access policy of this image
     *
     * @return the access policy of this image
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
     * Returns the size of this image in texels.
     *
     * @return the size of this image in texels.
     */
    Size getSize() { return Size((float)_width,(float)_height); }
    
    /**
     * Returns the number of bytes in a single texel of this image.
     *
     * @return the number of bytes in a single texel of this image.
     */
    size_t getByteSize() const;
    
    /**
     * Returns the data format of this image.
     *
     * The data format determines what type of data can be assigned to this
     * image.
     *
     * @return the data format of this image.
     */
    TexelFormat getFormat() const { return _format; }
    
    /**
     * Returns true if this image uses premultiplied alpha.
     *
     * Changing this value does not actually change the image data. It only
     * changes how the image is interpretted by the graphics backend.
     *
     * By default, this value is true if the image was read from a file (because
     * of how SDL3 processes image files) and false otherwise.
     *
     * @return true if this image uses premultiplied alpha.
     */
    bool isPremultiplied() const { return _premult; }

    /**
     * Sets whether this image uses premultiplied alpha.
     *
     * Changing this value does not actually change the image data. It only
     * changes how the image is interpretted by the graphics backend.
     *
     * By default, this value is true if the image was read from a file (because
     * of how SDL3 processes image files) and false otherwise.
     *
     * @param value Whether this image uses premultiplied alpha.
     */
    void setPremultiplied(bool value) { _premult = value; }

    /**
     * Returns whether this image has generated mipmaps.
     *
     * This method will not indicate the number of mipmap levels in Vulkan.
     * Instead, it treats mipmaps as all-or-nothing (like OpenGL).
     *
     * @return whether this image has generated mipmaps.
     */
    bool hasMipMaps() const { return _mipmaps; }
    
    /**
     * Returns the maximum image size (per dimension) for this platform
     *
     * This value is useful for when we want to allocate an image buffer
     * and want to make sure we do not exceed the platform limitations
     *
     * @return the maximum image size (per dimension) for this platform
     */
    static Uint32 getMaximumSize();
    
    /**
     * Returns a string representation of this image for debugging purposes.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this image for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Casts from Image to a string. */
    operator std::string() const { return toString(); }
    
#pragma mark -
#pragma mark Atlas Support
    /**
     * Returns the parent image of this subimage.
     *
     * This method will return nullptr is this is not a subimage.
     *
     * Returns the parent image of this subtexture.
     */
    const std::shared_ptr<Image>& getParent() const { return _parent; }
    
    /**
     * Returns the parent image of this subimage.
     *
     * This method will return nullptr is this is not a subimage.
     *
     * Returns the parent image of this subtexture.
     */
    std::shared_ptr<Image> getParent() { return _parent; }
    
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
     * @param y The y-coordinate of the bottom left in texels
     * @param w The region width in texels
     * @param h The region height in texels
     *
     * @return a subimage with the given dimensions.
     */
    std::shared_ptr<Image> getSubImage(Uint32 x, Uint32 y, Uint32 w, Uint32 h);
    
    /**
     * Returns true if this image is a subimage.
     *
     * This method is the same as checking if the parent is not nullptr.
     *
     * @return true if this image is a subimage.
     */
    bool isSubImage() const { return _parent != nullptr; }
    
    /**
     * Returns the x-coordinate of the origin texel.
     *
     * If this is not a subimage, this method returns 0. Otherwise, this method
     * is defined by the origin of the subimage region.
     *
     * @return the x-coordinate of the origin texel.
     */
    Uint32 getX() const { return _originx; }

    /**
     * Returns the y-coordinate of the origin texel.
     *
     * If this is not a subimage, this method returns 0. Otherwise, this method
     * is defined by the origin of the subimage region.
     *
     * @return the y-coordinate of the origin texel.
     */
    Uint32 getY() const { return _originy; }

    /**
     * Returns the origin texel.
     *
     * If this is not a subimage, this method returns the origin (0,0).
     * Otherwise, this method returns the bottom left corner of the subimage
     * region.
     *
     * @return the y-coordinate of the origin texel.
     */
    Vec2 getOrigin() const { return Vec2(_originx,_originy); }
    
    /**
     * Returns the minimum horizontal texture coordinate for this image.
     *
     * When rescaling texture coordinates for a subimage, this value is used in
     * place of 0.
     *
     * @return the minimum horizontal texture coordinate for this image.
     */
    float getMinS() const { return _minS; }
    
    /**
     * Returns the minimum vertical texture coordinate for this image.
     *
     * When rescaling texture coordinates for a subimage, this value is used in
     * place of 0.
     *
     * @return the minimum vertical texture coordinate for this image.
     */
    float getMinT() const { return _minT; }
    
    /**
     * Returns the maximum horizontal texture coordinate for this image.
     *
     * When rescaling texture coordinates for a subimage, this value is used in
     * place of 1.
     *
     * @return the maximum horizontal texture coordinate for this image.
     */
    float getMaxS() const { return _maxS; }
    
    /**
     * Returns the maximum vertical texture coordinate for this image.
     *
     * When rescaling texture coordinates for a subimage, this value is used in
     * place of 1.
     *
     * @return the maximum vertical texture coordinate for this image.
     */
    float getMaxT() const { return _maxT; }
};

    }

}

#endif /* __CU_IMAGE_H__ */
