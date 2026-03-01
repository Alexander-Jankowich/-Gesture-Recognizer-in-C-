//
//  CUFrameBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for an offscreen framebuffer. This is a
//  object with attached images (potentially more than one) as output buffers.
//  This allows us to draw to a texture for potential post processing.
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
//  Version: 4/1/25 (Vulkan Integration)
//
#ifndef __CU_FRAME_BUFFER_H__
#define __CU_FRAME_BUFFER_H__
#include <cugl/core/math/CUColor4.h>
#include <cugl/core/math/CUIVec2.h>
#include <cugl/graphics/CUGraphicsBase.h>
#include <memory>
#include <vector>
#include <initializer_list>

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
class Texture;
class RenderPass;
// An opaque class for the graphics API
class FrameBufferData;

/**
 * This is a class representing an offscreen framebuffer
 *
 * A framebuffer allows the user to draw to an image before drawing to a screen.
 * This allows for potential post-processing effects. To draw to a framebuffer,
 * simply provide it to {@link GraphicsShader#begin} when starting a new render
 * pass.
 *
 * While framebuffers must have at least one output image, they can support
 * multiple images as long as the active fragment shader has multiple output
 * variables. The locations of these outputs should be set explicitly and
 * sequentially with the layout keyword.
 *
 * This class greatly simplifies offscreen framebuffers at the cost of some
 * flexibility. The only support for depth and stencil is a combined 24/8 depth
 * and stencil buffer. In addition, output textures must have one of the
 * simplified formats defined by {@link TexelFormat}.
 */
class FrameBuffer {
private:
    /** The default frame buffer */
    static std::shared_ptr<FrameBuffer> _gFramebuffer;
    /** The name of this frame buffer */
    std::string _name;
    /** The graphics API implementation of this framebuffer */
    FrameBufferData* _data;
    /** Whether this is the framebuffer for the primary display */
    bool _display;
    /** The active view port framebuffer */
    int _viewport[4];

    /** The framebuffer x-offset */
    Uint32 _x;
    /** The framebuffer y-offset */
    Uint32 _y;
    /** The framebuffer "screen" width */
    Uint32 _width;
    /** The framebuffer "screen" height */
    Uint32 _height;
    /** The number of attachments for this framebuffer */
    size_t _attachments;
    /** The render pass for this framebuffer */
    std::shared_ptr<RenderPass> _renderPass;

    /** The array of output textures (can be empty on main framebuffer) */
    std::vector<std::shared_ptr<Texture>> _textures;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized framebuffer with no output textures.
     *
     * You must initialize the framebuffer to create an output texture.
     */
    FrameBuffer();
    
    /**
     * Deletes this framebuffer, disposing all resources.
     */
    ~FrameBuffer() { dispose(); }
    
    /**
     * Deletes the framebuffer and resets all attributes.
     *
     * You must reinitialize the framebuffer to use it.
     */
    void dispose();
    
    /**
     * Initializes this framebuffer with a single COLOR_RGBA output image.
     *
     * The output image will have the given width and size.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     *
     * @return true if initialization was successful.
     */
    bool init(Uint32 width, Uint32 height) {
        return init(width,height,1);
    }
    
    /**
     * Initializes this framebuffer with multiple COLOR_RGBA output image.
     *
     * The output images will have the given width and size. They will be
     * assigned locations 0..outputs-1. These locations should be bound with
     * the layout keyword in any shader used with this framebuffer. Otherwise
     * the results are not well-defined.
     *
     * If outputs is larger than the number of possible shader outputs for this
     * platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The number of output images
     *
     * @return true if initialization was successful.
     */
    bool init(Uint32 width, Uint32 height, size_t outputs);
    
    /**
     * Initializes this framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will be
     * assigned the appropriate format at locations 0..\#outputs-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     *
     * @return true if initialization was successful.
     */
    bool initWithFormats(Uint32 width, Uint32 height, std::initializer_list<TexelFormat> outputs);
    
    /**
     * Initializes this framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will
     * be assigned the appropriate format at locations 0..\#outputs-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     *
     * @return true if initialization was successful.
     */
    bool initWithFormats(Uint32 width, Uint32 height, std::vector<TexelFormat> outputs) {
        return initWithFormats(width, height, outputs.data(), outputs.size());
    }
    
    /**
     * Initializes this framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will
     * be assigned the appropriate format at locations 0..\#outsize-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     * @param outsize   The number of elements in outputs
     *
     * @return true if initialization was successful.
     */
    bool initWithFormats(Uint32 width, Uint32 height, TexelFormat* outputs, size_t outsize);

    /**
     * Initializes this framebuffer with the given render pass
     *
     * The framebuffer will use the formats of the render pass to construct
     * its images.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param pass      The associated render pass
     *
     * @return true if initialization was successful.
     */
    bool initWithRenderPass(Uint32 width, Uint32 height, const std::shared_ptr<RenderPass>& pass);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new framebuffer with a single COLOR_RGBA output image.
     *
     * The output image will have the given width and size.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     *
     * @return a new render target with a single COLOR_RGBA output image.
     */
    static std::shared_ptr<FrameBuffer> alloc(Uint32 width, Uint32 height) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->init(width,height) ? result : nullptr);
    }
    
    /**
     * Returns a new framebuffer with multiple COLOR_RGBA output images.
     *
     * The output images will have the given width and size. They will be
     * assigned locations 0..outputs-1. These locations should be bound with
     * the layout keyword in any shader used with this framebuffer. Otherwise
     * the results are not well-defined.
     *
     * If outputs is larger than the number of possible shader outputs
     * for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The number of output images
     *
     * @return a new render target with multiple COLOR_RGBA output images.
     */
    static std::shared_ptr<FrameBuffer> alloc(Uint32 width, Uint32 height, size_t outputs) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->init(width,height,outputs) ? result : nullptr);
    }
    
    /**
     * Returns a new framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will be
     * assigned the appropriate format at locations 0..\#outputs-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     *
     * @return a new framebuffer with multiple images of the given format.
     */
    static std::shared_ptr<FrameBuffer> allocWithFormats(Uint32 width, Uint32 height,
                                                         std::initializer_list<TexelFormat> outputs) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->initWithFormats(width,height,outputs) ? result : nullptr);
    }
    
    /**
     * Returns a new framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will be
     * assigned the appropriate format at locations 0..\#outputs-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     *
     * @return a new framebuffer with multiple images of the given format.
     */
    static std::shared_ptr<FrameBuffer> allocWithFormats(int width, int height,
                                                         std::vector<TexelFormat> outputs) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->initWithFormats(width,height,outputs) ? result : nullptr);
    }
    
    /**
     * Returns a new framebuffer with multiple images of the given format.
     *
     * The output images will have the given width and size. They will be
     * assigned the appropriate format at locations 0..\#outputs-1. These
     * locations should be bound with the layout keyword in any shader used
     * with this framebuffer. Otherwise the results are not well-defined.
     *
     * If the size of the outputs parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param outputs   The list of desired image formats
     * @param outsize   The number of elements in outputs
     *
     * @return a new framebuffer with multiple images of the given format.
     */
    static std::shared_ptr<FrameBuffer> allocWithFormats(int width, int height,
                                                         TexelFormat* outputs, size_t outsize) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->initWithFormats(width,height,outputs,outsize) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated framebuffer with the given render pass
     *
     * The framebuffer will use the formats of the render pass to construct
     * its images.
     *
     * @param width     The drawing width of this framebuffer
     * @param height    The drawing width of this framebuffer
     * @param pass      The associated render pass
     *
     * @return a newly allocated framebuffer with the given render pass
     */
    static std::shared_ptr<FrameBuffer>  allocWithRenderPass(Uint32 width, Uint32 height,
                                                             const std::shared_ptr<RenderPass>& pass) {
        std::shared_ptr<FrameBuffer> result = std::make_shared<FrameBuffer>();
        return (result->initWithRenderPass(width,height,pass) ? result : nullptr);
    }
    
    /**
     * Returns the framebuffer for the window display.
     *
     * This is the default framebuffer for on-screen drawing.
     */
    static std::shared_ptr<FrameBuffer> getDisplay();
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the platform-specific implementation for this framebuffer
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this framebuffer
     */
    FrameBufferData* getImplementation() { return _data; }
    
    /**
     * Sets the name of this frame buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @param name  The name of this frame buffer.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this frame buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @return the name of this frame buffer.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns the origin offset of this framebuffer
     *
     * Offsets are necessary for when the Application window is a smaller
     * subset of the actual display. This can happen on Android devices
     * exhibiting large screen behavior.
     *
     * @return the origin offset of this framebuffer
     */
    const IVec2 getOffset() const {
        return IVec2(_x,_y);
    }

    /**
     * Sets the origin offset of this framebuffer
     *
     * Offsets are necessary for when the Application window is a smaller
     * subset of the actual display. This can happen on Android devices
     * exhibiting large screen behavior.
     *
     * @param offset    The origin offset of this framebuffer
     */
    void setOffset(const IVec2& offset) {
        _x = offset.x;
        _y = offset.y;
    }

    /**
     * Sets the origin offset of this framebuffer
     *
     * Offsets are necessary for when the Application window is a smaller
     * subset of the actual display. This can happen on Android devices
     * exhibiting large screen behavior.
     *
     * @param x The x offset of this framebuffer
     * @param y The y offset of this framebuffer
     */
    void setOffset(Uint32 x, Uint32 y) {
        _x = x;
        _y = y;
    }
    
    /**
     * Returns the size of this framebuffer
     *
     * @return the size of this framebuffer
     */
    const Size getSize() const { return Size(_width,_height); }
    
    /**
     * Returns the width of this framebuffer
     *
     * @return the width of this framebuffer
     */
    int getWidth() const { return _width; }
    
    /**
     * Returns the height of this framebuffer
     *
     * @return the height of this framebuffer
     */
    int getHeight() const { return _height; }
    
    /**
     * Returns the render pass for this framebuffer
     *
     * @return the render pass for this framebuffer
     */
    const std::shared_ptr<RenderPass>& getRenderPass() const { return _renderPass; }
    
    /**
     * Returns true if rendering to this framebuffer happens immediately
     *
     * An immediate framebuffer guarantees that its image(s) are available as
     * soon as {@link GraphicsShader#end} is called. Otherwise, there may be a
     * delay between when {@link GraphicsShader#end} is called and
     * {@link #getImage} is available. There is no indication for when such an
     * image is available, short of progressing to the next frame.
     *
     * This is an optimization for subpass rendering on Vulkan. It has no effect
     * in OpenGL. This value cannot be changed during the middle of an active
     * render pass on this framebuffer.
     *
     * @return true if rendering to this framebuffer happens immediately
     */
    bool isImmediate() const;
    
    /**
     * Sets whether rendering to this framebuffer happens immediately
     *
     * An immediate framebuffer guarantees that its image(s) are available as
     * soon as {@link GraphicsShader#end} is called. Otherwise, there may be a
     * delay between when {@link GraphicsShader#end} is called and
     * {@link #getImage} is available. There is no indication for when such an
     * image is available, short of progressing to the next frame.
     *
     * This is an optimization for subpass rendering on Vulkan. It has no effect
     * in OpenGL. This value cannot be changed during the middle of an active
     * render pass on this target.
     *
     * @param value Whether rendering to this framebuffer happens immediately
     */
    void setImmediate(bool value);
    
    /**
     * Returns the number of color attachments for this framebuffer
     *
     * If the render target has been successfully initialized, this value is
     * guaranteed to be at least 1.
     *
     * @return the number of color attachments for this framebuffer
     */
    size_t getAttachments() const { return _attachments; }
    
    /**
     * Returns the output image for the given index.
     *
     * The index should be a value between 0..getAttachments-1. By default it
     * is 0, the primary output image.
     *
     * @param index    The output index
     *
     * @return the output image for the given index.
     */
    const std::shared_ptr<Image> getImage(size_t index = 0) const;

    /**
     * Returns the output texture for the given index.
     *
     * This method is similar to {@link #getImage} except that it attaches a
     * default sampler to the frame buffer.
     *
     * @param index    The output index
     *
     * @return the output texture for the given index.
     */
    const std::shared_ptr<Texture> getTexture(size_t index = 0);

    /**
     * Returns the depth/stencil buffer for this framebuffer
     *
     * The framebuffer for a render target always uses a combined depth and
     * stencil buffer image. It uses 24 bits for the depth and 8 bits for the
     * stencil. This should be sufficient in most applications.
     *
     * @return the depth/stencil buffer for this framebuffer
     */
    const std::shared_ptr<Image> getDepthStencil() const;

#pragma mark -
#pragma mark Rendering
    /**
     * Returns the active viewport for this framebuffer
     *
     * By default, the viewport is the entire scope of the framebuffer.
     * Changing this value restricts the portion of the framebuffer that a
     * {@link GraphicsShader} will draw to. The viewport origin is from the
     * bottom left for all graphics APIs.
     *
     * @return the active viewport for this framebuffer
     */
    const Rect getViewPort() const {
        return Rect(_viewport[0],_viewport[1],_viewport[2],_viewport[3]);
    }
    
    /**
     * Sets the active viewport for this framebuffer
     *
     * By default, the viewport is the entire scope of the framebuffer.
     * Changing this value restricts the portion of the framebuffer that a
     * {@link GraphicsShader} will draw to. The viewport origin is from the
     * bottom left for all graphics APIs.
     *
     * Note that this value will not take affect until the call to the next
     * drawing pass of a {@link GraphicsShader}.
     *
     * @param viewport  The active viewport for this framebuffer
     */
    void setViewPort(const Rect& viewport) {
        setViewPort(viewport.origin.x,viewport.origin.y,
                    viewport.size.width,viewport.size.height);
    }
    
    /**
     * Sets the active viewport for this framebuffer
     *
     * By default, the viewport is the entire scope of the framebuffer.
     * Changing this value restricts the portion of the framebuffer that a
     * {@link GraphicsShader} will draw to. The viewport origin is from the
     * bottom left for all graphics APIs.
     *
     * Note that this value will not take affect until the call to the next
     * drawing pass of a {@link GraphicsShader}.
     *
     * @param x         The x-coordinate of the origin
     * @param y         The y-coordinate of the origin
     * @param width     The viewport width
     * @param height    The viewport height
     */
    void setViewPort(int x, int y, int width, int height);
    
    /**
     * Restores the viewport for this framebuffer
     *
     * The viewport is reset to the default, matching {@link #getSize}.
     */
    void restoreViewPort();

    /**
     * Activates the framebuffer for drawing
     */
    void activate();
    
    /**
     * Clears the contents of this framebuffer.
     *
     * The framebuffer will use the {@link RenderPass} settings to clear this
     * framebuffer.
     */
    void clear();

};

    }

}
#endif /* __CU_RENDER_TARGET_H__ */
