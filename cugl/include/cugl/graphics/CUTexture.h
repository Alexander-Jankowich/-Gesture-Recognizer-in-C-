//
//  CUTexture.h
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
#ifndef __CU_TEXTURE_H__
#define __CU_TEXTURE_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUImage.h>
#include <cugl/graphics/CUSampler.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUSize.h>
#include <cugl/core/math/CUVec2.h>

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
class Image;

/**
 * This is a class representing a texture.
 *
 * A class is mostly a pair of an {@link Image} and a {@link Sampler}. Most of
 * its functionality can be accessed through those two attributes.
 *
 * However, this class also provides support for texture atlases. Any
 * non-repeating texture can produce a subtexture. A subtexture creates a
 * subimage from the underlying image using {@link Image#getSubImage}, but it
 * has its own sampler. See {@link #getSubTexture} for more information.
 *
 * Textures are intended to be used with {@link GraphicsShader} objects. As
 * they can support either Vulkan or OpenGL, we do not support the binding API
 * of OpenGL. This functionality has been abstracted away.
 *
 * Note that we talk about texture coordinates in terms of S (horizontal) and
 * T (vertical). This is different from other implementations that use UV
 * instead.
 *
 * Textures can only be constructed on the main thread when using OpenGL.
 * However, in Vulkan, images can be safely constructed on any thread.
 */
class Texture : public std::enable_shared_from_this<Texture> {
#pragma mark Values
private:
    /** An all purpose blank texture for coloring */
    static std::shared_ptr<Texture> _blank;

    /** The decriptive texture name */
    std::string _name;

    /** The image data */
    std::shared_ptr<Image> _image;
    /** The texture sampler */
    std::shared_ptr<Sampler> _sampler;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new empty texture with no size.
     *
     * This method performs no allocations. You must call init to generate
     * a proper texture.
     */
    Texture();
    
    /**
     * Deletes this texture, disposing all resources
     */
    ~Texture() { dispose(); }
    
    /**
     * Deletes the texture and resets all attributes.
     *
     * You must reinitialize the texture to use it.
     */
    void dispose();
    
    /**
     * Initializes an empty texture with the given dimensions.
     *
     * The image will have all of its texels set to 0. It will not support
     * mipmaps. You should use {@link #allocWithData} for mipmap support.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
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
              ImageAccess access = ImageAccess::READ_WRITE) {
        return initWithData(NULL, width, height, format, access, false);
    }
    
    /**
     * Initializes an texture with the given data.
     *
     * The data format must match the one specified by the format parameter.
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * This initializer assumes that alpha is not premultiplied. You should call
     * {@link #setPremultiplied} if this is not the case.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param data      The texture data (size width*height*format)
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     * @param access    The image access
     * @param mipmaps   Flag to generate mipmaps
     *
     * @return true if initialization was successful.
     */
    bool initWithData(const std::byte *data, Uint32 width, Uint32 height,
                      TexelFormat format = TexelFormat::COLOR_RGBA,
                      ImageAccess access = ImageAccess::READ_WRITE,
                      bool mipmaps = false);
    
    /**
     * Initializes an texture with the data from the given file.
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
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * If the file path is relative, it is assumed to be with respect to
     * {@link Application#getAssetDirectory}. If you wish to load a texture 
     * from somewhere else, you must use an absolute path.
     *
     * @param filename  The file supporting the texture file.
     * @param access    The image access
     * @param mipmaps   Flag to generate mipmaps
     *
     * @return true if initialization was successful.
     */
    bool initWithFile(const std::string filename,
                      ImageAccess access=ImageAccess::READ_WRITE,
                      bool mipmaps = false);
    
    /**
     * Initializes an texture with the given image.
     *
     * This texture will cover the entire image, and not just a subregion.
     * Note that this texture does not copy the image, so any changes to the
     * image elsewhere will affect this is texture.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param image     The texture image
     *
     * @return true if initialization was successful.
     */
    bool initWithImage(const std::shared_ptr<Image>& image);

    /**
     * Initializes an texture with the given image and sampler.
     *
     * This texture will cover the entire image, and not just a subregion.
     * Note that this texture does not copy the image, so any changes to the
     * image elsewhere will affect this is texture.
     *
     * @param image     The texture image
     * @param sampler   The texture sampler
     *
     * @return true if initialization was successful.
     */
    bool initWithImage(const std::shared_ptr<Image>& image,
                       const std::shared_ptr<Sampler>& sampler);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new empty texture with the given dimensions.
     *
     * The image will have all of its texels set to 0. You should access the
     * image with {@link #getImage} and use {@link Image#set} method to load
     * data into the image.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     * @param access    The image access
     *
     * @return a new empty texture with the given dimensions.
     */
    static std::shared_ptr<Texture> alloc(Uint32 width, Uint32 height,
                                          TexelFormat format = TexelFormat::COLOR_RGBA,
                                          ImageAccess access = ImageAccess::READ_WRITE) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->init(width, height, format, access) ? result : nullptr);
    }
    
    /**
     * Returns a new texture with the given data.
     *
     * The data format must match the one specified by the format parameter.
     * If the parameter mipmaps is true, the image will generate an appropriate
     * number of mipmaps determined by its size.
     *
     * This initializer assumes that alpha is not premultiplied. You should call
     * {@link #setPremultiplied} if this is not the case.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param data      The texture data (size width*height*format)
     * @param width     The texture width in pixels
     * @param height    The texture height in pixels
     * @param format    The texture data format
     * @param access    The image access
     * @param mipmaps   Flag to generate mipmaps
     *
     * @return a new texture with the given data.
     */
    static std::shared_ptr<Texture> allocWithData(const std::byte *data, Uint32 width, Uint32 height,
                                                  TexelFormat format = TexelFormat::COLOR_RGBA,
                                                  ImageAccess access = ImageAccess::READ_WRITE,
                                                  bool mipmaps = false) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithData(data, width, height, format, access, mipmaps) ? result : nullptr);
        
    }
    
    /**
     * Returns a new texture with the data from the given file.
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
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * If the file path is relative, it is assumed to be with respect to
     * {@link Application#getAssetDirectory}. If you wish to load a texture 
     * from somewhere else, you must use an absolute path.
     *
     * @param filename  The file supporting the texture file.
     * @param access    The image access
     * @param mipmaps   Flag to generate mipmaps
     *
     * @return a new texture with the given data
     */
    static std::shared_ptr<Texture> allocWithFile(const std::string filename,
                                                  ImageAccess access = ImageAccess::READ_WRITE,
                                                  bool mipmaps = false) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithFile(filename, access, mipmaps) ? result : nullptr);
    }
    
    /**
     * Returns a new texture with the given image.
     *
     * This texture will cover the entire image, and not just a subregion.
     * Note that this texture does not copy the image, so any changes to the
     * image elsewhere will affect this is texture.
     *
     * The sampler will be the default, using a {@link TextureFilter#LINEAR}
     * filter and {@link TextureWrap#CLAMP} on both axes.
     *
     * @param image     The texture image
     *
     * @return a new texture with the given image.
     */
    static std::shared_ptr<Texture> allocWithImage(const std::shared_ptr<Image>& image) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithImage(image) ? result : nullptr);
    }

    /**
     * Returns a new texture with the given image and sampler.
     *
     * This texture will cover the entire image, and not just a subregion.
     * Note that this texture does not copy the image, so any changes to the
     * image elsewhere will affect this is texture.
     *
     * @param image     The texture image
     * @param sampler   The texture sampler
     *
     * @return a new texture with the given image and sampler.
     */
    static std::shared_ptr<Texture> allocWithImage(const std::shared_ptr<Image>& image,
                                                   const std::shared_ptr<Sampler>& sampler) {
        std::shared_ptr<Texture> result = std::make_shared<Texture>();
        return (result->initWithImage(image,sampler) ? result : nullptr);
    }
    
    /**
     * Returns a blank texture that can be used to make solid shapes.
     *
     * This is the texture used by {@link SpriteBatch} when the active texture
     * is nullptr. It is a 2x2 texture of all white pixels. Using this texture
     * means that all shapes and outlines will be drawn with a solid color.
     *
     * This texture is a singleton. There is only one of it. All calls to
     * this method will return a reference to the same object.
     *
     * @return a blank texture that can be used to make solid shapes.
     */
    static const std::shared_ptr<Texture>& getBlank();
    
    
#pragma mark -
#pragma mark Data Access
    /**
     * Returns the image component of this texture
     *
     * In modern graphics APIs, textures are composed of two objects: an image
     * and a sampler. Modifications to this image will be reflected in the
     * texture during sampling.
     *
     * If this is a subtexture (e.g. part of a texture atlas), this method
     * will return the image for the root texture, and is not limited to
     * the image for this subregion.
     *
     * @return the image component of this texture
     */
    const std::shared_ptr<Image> getImage() const { return _image; }

    /**
     * Returns the sampler component of this texture
     *
     * In modern graphics APIs, textures are composed of two objects: an image
     * and a sampler. Modifications to this sampler will be reflected in the
     * texture during sampling.
     *
     * @return the sampler component of this texture
     */
    const std::shared_ptr<Sampler> getSampler() const { return _sampler; }

    /**
     * Sets this texture to have the contents of the given buffer.
     *
     * If this is a subtexture, then this data is confined to the size of the
     * texture region. Otherwise, this method is the same as in {@link Image}.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link Image#getByteSize} for a
     * description of the latter.
     *
     * @param data  The buffer to read into the texture
     *
     * @return a reference to this (modified) texture for chaining.
     */
    const Texture& operator=(const std::byte *data) {
        return set(data);
    }
    
    /**
     * Sets this texture to have the contents of the given buffer.
     *
     * If this is a subtexture, then this data is confined to the size of the
     * texture region. Otherwise, this method is the same as in {@link Image}.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link Image#getByteSize} for a
     * description of the latter.
     *
     * @param data  The buffer to read into the texture
     *
     * @return a reference to this (modified) texture for chaining.
     */
    const Texture& set(const std::byte *data);
    
    /**
     * Sets this texture to have the contents of the given buffer.
     *
     * If this is a subtexture, then this data is confined to the size of the
     * texture region. Otherwise, this method is the same as in {@link Image}.
     *
     * The buffer must have the correct data format. In addition, the buffer
     * must be size width*height*bytesize. See {@link Image#getByteSize} for a
     * description of the latter.
     *
     * @param data  The buffer to read into the texture
     *
     * @return a reference to this (modified) texture for chaining.
     */
    const Texture& set(const std::vector<std::byte>& data) {
        set(data.data());
        return *this;
    }

    /**
     * Fills the given buffer with data from this texture.
     *
     * This method will append the data to the given vector. In the case of
     * subtextures, this will only extract the texels in the texture region.
     *
     * @param data  The buffer to store the texture data
     *
     * @return the number of bytes read into the buffer
     */
    const size_t get(std::vector<std::byte>& data);
    
    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this texture.
     *
     * A name is a user-defined way of identifying a texture. Subtextures are
     * permitted to have different names than their parents.
     *
     * @param name  The name of this texture.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this texture.
     *
     * A name is a user-defined way of identifying a texture. Subtextures are
     * permitted to have different names than their parents.
     *
     * @return the name of this texture.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns a unique identifier for this texture
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this texture
     */
    size_t id() const;

    /**
     * Returns the width of this texture in texels.
     *
     * If this is a subtexture, this will return the width of the subregion.
     *
     * @return the width of this texture in texels.
     */
    Uint32 getWidth() const { return _image->getWidth(); }
    
    /**
     * Returns the height of this texture in texels.
     *
     * If this is a subtexture, this will return the height of the subregion.
     *
     * @return the height of this texture in texels.
     */
    Uint32 getHeight() const { return _image->getHeight(); }
    
    /**
     * Returns the size of this texture in texels.
     *
     * If this is a subtexture, this will return the height of the subregion.
     *
     * @return the size of this texture in pixels.
     */
    Size getSize() const;
    
    /**
     * Returns texture if this image uses premultiplied alpha.
     *
     * Changing this value does not actually change the image data. It only
     * changes how the texture is interpretted by the graphics backend.
     *
     * By default, this value is true if the texture was read from a file 
     * (because of how SDL3 processes image files) and false otherwise.
     *
     * @return true if this texture uses premultiplied alpha.
     */
    bool isPremultiplied() const { return _image->isPremultiplied(); }

    /**
     * Sets whether this texture uses premultiplied alpha.
     *
     * Changing this value does not actually change the image data. It only
     * changes how the texture is interpretted by the graphics backend.
     *
     * By default, this value is true if the texture was read from a file 
     * (because of how SDL3 processes image files) and false otherwise.
     *
     * @param value Whether this texture uses premultiplied alpha.
     */
    void setPremultiplied(bool value) { _image->setPremultiplied(value); }
    
#pragma mark -
#pragma mark Sampler Control
    /**
     * Returns the magnification filter of the associated sampler.
     *
     * This is a convenience method that queries the result from the sampler.
     * The magnification filter is the algorithm hint that the graphics card
     * uses to make an image larger. The default is LINEAR.
     *
     * @return the magnification filter of the associated sampler.
     */
    TextureFilter getMagFilter() const { return _sampler->getMagFilter(); }
    
    /**
     * Sets the magnification filter of the associated sampler.
     *
     * This is a convenience method that pushes directly to the sampler.
     * The magnification filter is the algorithm hint that the graphics card
     * uses to make an image larger. The default is LINEAR.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param filter    The magnification filter of the associated sampler.
     */
    void setMagFilter(TextureFilter filter) {
        _sampler->setMagFilter(filter);
    }
    
    /**
     * Returns the minimization filter of the associated sampler.
     *
     * This is a convenience method that queries the result from the sampler.
     * The minimization filter is the algorithm hint that the graphics card
     * uses to make an image smaller. The default is LINEAR.
     *
     * @return the minimization filter of the associated sampler.
     */
    TextureFilter getMinFilter() const { return _sampler->getMinFilter(); }
    
    /**
     * Sets the minimization filter of the associated sampler.
     *
     * This is a convenience method that pushes directly to the sampler.
     * The minimization filter is the algorithm hint that the graphics card
     * uses to make an image smaller. The default is LINEAR.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param filter    The min filter of the associated sampler.
     */
    void setMinFilter(TextureFilter filter) {
        _sampler->setMinFilter(filter);
    }
    
    /**
     * Returns the horizontal wrap of the associated sampler.
     *
     * This is a convenience method that queries the result from the sampler.
     * The default is CLAMP.
     *
     * @return the horizontal wrap of the associated sampler.
     */
    TextureWrap getWrapS() const { return _sampler->getWrapS(); }
    
    /**
     * Sets the horizontal wrap of the associated sampler.
     *
     * This is a convenience method that pushes directly to the sampler.
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * The default is CLAMP.
     *
     * @param wrap  The horizontal wrap setting of the associated sampler.
     */
    void setWrapS(TextureWrap wrap) {
        _sampler->setWrapS(wrap);
    }
    
    /**
     * Returns the vertical wrap of the associated sampler.
     *
     * This is a convenience method that queries the result from the sampler.
     * The default is CLAMP.
     *
     * @return the vertical wrap of the associated sampler.
     */
    TextureWrap getWrapT() const { return _sampler->getWrapT(); }
    
    /**
     * Sets the vertical wrap of the associated sampler.
     *
     * This is a convenience method that pushes directly to the sampler.
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * The default is CLAMP.
     *
     * @param wrap  The vertical wrap setting of the associated sampler.
     */
    void setWrapT(TextureWrap wrap)  {
        _sampler->setWrapT(wrap);
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
     * Subtextures are constructed using {@link Image#getSubImage} to construct
     * a subimage of the texture image. So any changes to the parent image will
     * affect the subtexture. On the other hand, subtextures always have their
     * own sampler distinct from that of the original texture. So changes to
     * the sampler of the parent texture will not affect the subtexture.
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
     * @param x The x-coordinate of the bottom left in texels
     * @param y The y-coordinate of the bottom left in texels
     * @param w The region width in texels
     * @param h The region height in texels
     *
     * @return a subtexture with the given dimensions.
     */
    std::shared_ptr<Texture> getSubTexture(Uint32 x, Uint32 y, Uint32 w, Uint32 h);
    
    /**
     * Returns true if this texture is a subtexture.
     *
     * This method is the same as checking if the parent is not nullptr.
     *
     * @return true if this texture is a subtexture.
     */
    bool isSubTexture() const { return _image->isSubImage(); }
    
    /**
     * Returns the x-coordinate of the origin texel.
     *
     * If this is not a subtexture, this method returns 0. Otherwise, this
     * method is given by the origin of the subtexture region.
     *
     * @return the x-coordinate of the origin texel.
     */
    Sint32 getX() const { return _image->getX(); }

    /**
     * Returns the y-coordinate of the origin texel.
     *
     * If this is not a subtexture, this method returns 0. Otherwise, this
     * method is given by the origin of the subtexture region.
     *
     * @return the y-coordinate of the origin texel.
     */
    Sint32 getT() const { return _image->getY(); }

    /**
     * Returns the origin texel.
     *
     * If this is not a subtexture, this method returns the origin (0,0).
     * Otherwise, this method returns the bottom left corner of the subtexture
     * region.
     *
     * @return the y-coordinate of the origin texel.
     */
    Vec2 getOrigin() const { return _image->getOrigin(); }
    
    /**
     * Returns the minimum horizontal texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 0.
     *
     * @return the minimum horizontal texture coordinate for this texture.
     */
    float getMinS() const { return _image->getMinS(); }
    
    /**
     * Returns the minimum vertical texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 0.
     *
     * @return the minimum vertical texture coordinate for this texture.
     */
    float getMinT() const { return _image->getMinT(); }
    
    /**
     * Returns the maximum horizontal texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 1.
     *
     * @return the maximum horizontal texture coordinate for this texture.
     */
    float getMaxS() const { return _image->getMaxS(); }
    
    /**
     * Returns the maximum vertical texture coordinate for this texture.
     *
     * When rescaling texture coordinates for a subtexture, this value is
     * used in place of 1.
     *
     * @return the maximum vertical texture coordinate for this texture.
     */
    float getMaxT() const { return _image->getMaxT(); }
    
    
#pragma mark -
#pragma mark Conversions
    /**
     * Returns a string representation of this texture for debugging purposes.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this texture for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Casts from Texture to a string. */
    operator std::string() const { return toString(); }
    
};

    }

}

#endif /* __CU_TEXTURE_H__ */
