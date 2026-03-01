//
//  CURenderPass.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a render pass. Previously, we had hoped
//  to integrate this with RenderTarget. However, offscreen rendering requires
//  that we have the ability to swap out framebuffers. So we needed an extra
//  class to represented when such a swap is valid.
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
#ifndef __CU_RENDER_PASS_H__
#define __CU_RENDER_PASS_H__
#include <cugl/core/math/CUColor4.h>
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
// An opaque class for the graphics API
class RenderPassData;
    
/**
 * This is a class representing a render pass.
 *
 * A render pass defines the basic structure of a drawing pass. This includes
 * the layout of the color attachment(s), whether there is a depth buffer, and
 * the clear values. Note that this is not the same as {@link FrameBuffer}.
 * It does not define the framebuffers. Instead, it provides type information
 * about what kinds of framebuffers are supported by a given render pass.
 *
 * This class is necesary for Vulkan, and implements a Vulkan render pass behind
 * the scenes. In OpenGL, this class primarily works as a form of type checking
 * to ensure that our choice of target for the {@link GraphicsShader} is valid.
 */
class RenderPass {
private:
    /** The render pass for the display */
    static std::weak_ptr<RenderPass> g_display;
    
    /** The graphics API implementation of this renderpass */
    RenderPassData* _data;
    /** The color formats for this render pass */
    std::vector<TexelFormat> _colorFormats;
    /** The depth format for this render pass (UNDEFINED if no buffer) */
    TexelFormat _depthFormat;

    /**
     * Initializes a render pass for the primary display.
     *
     * This render pass is guaranteed to have a color attachment at location 0
     * and a depthStencil attachment at location 1.
     *
     * @return true if initialization was successful.
     */
    bool initForDisplay();
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized render pass.
     *
     * You must initialize the render pass to use it.
     */
    RenderPass() : _data(nullptr) {
        _depthFormat = TexelFormat::UNDEFINED;
    }
    
    /**
     * Deletes this render pass, disposing all resources.
     */
    ~RenderPass() { dispose(); }
    
    /**
     * Deletes the render pass and resets all attributes.
     *
     * You must reinitialize the render pass to use it.
     */
    void dispose();
    
    /**
     * Initializes this pass with a single COLOR_RGBA attachment.
     *
     * The color attachment will be assigned to location 0. If depthStencil is
     * true, a depth/stencil attachment will be added to position 1.
     *
     * If depthStencil is true, this render pass is guaranteed to be compatible
     * with the default render pass.
     *
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return true if initialization was successful.
     */
    bool init(bool depthStencil=true) {
        return init(1,depthStencil);
    }
    
    /**
     * Initializes this pass with multiple COLOR_RGBA attachments.
     *
     * The attachments will be assigned locations 0..attachments-1. If
     * depthStencil is true, a depth/stencil attachment will be added to
     * position attachments. If attachments is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param attachments   The number of color attachments
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return true if initialization was successful.
     */
    bool init(size_t attachments,bool depthStencil=true);
    
    /**
     * Initializes this pass with color attachments for the given formats.
     *
     * The attachments will be assigned locations 0..\#formats-1, with each
     * attachment assigned the given format. If depthStencil is true, a
     * depth/stencil attachment will be added to position \#formats.
     *
     * If the size of the formats parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param formats       The list of desired image formats
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return true if initialization was successful.
     */
    bool initWithFormats(const std::vector<TexelFormat>& formats, bool depthStencil=true) {
        return initWithFormats(formats.data(),formats.size(),depthStencil);
    }
    
    /**
     * Initializes this pass with color attachments for the given formats.
     *
     * The attachments will be assigned locations 0..size-1, with each
     * attachment assigned the given format. If depthStencil is true, a
     * depth/stencil attachment will be added to position size.
     *
     * If the size of the formats parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param formats       The list of desired image formats
     * @param size          The number color attachments
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return true if initialization was successful.
     */
    bool initWithFormats(const TexelFormat* formats, size_t size, bool depthStencil=true);

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated render pass with a single COLOR_RGBA attachment.
     *
     * The color attachment will be assigned to location 0. If depthStencil is
     * true, a depth/stencil attachment will be added to position 1.
     *
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return a newly allocated render pass with a single COLOR_RGBA attachment.
     */
    static std::shared_ptr<RenderPass> alloc(bool depthStencil=true) {
        std::shared_ptr<RenderPass> result = std::make_shared<RenderPass>();
        return result->init(depthStencil) ? result : nullptr;
    }
    
    /**
     * Returns a newly allocated render pass with multiple COLOR_RGBA attachments.
     *
     * The attachments will be assigned locations 0..attachments-1. If
     * depthStencil is true, a depth/stencil attachment will be added to
     * position attachments. If attachments is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param attachments   The number of color attachments
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return a newly allocated render pass with multiple COLOR_RGBA attachments.
     */
    static std::shared_ptr<RenderPass> alloc(size_t attachments, bool depthStencil=true) {
        std::shared_ptr<RenderPass> result = std::make_shared<RenderPass>();
        return result->init(attachments,depthStencil) ? result : nullptr;
    }
    
    /**
     * Returns a newly allocated pass with color attachments for the given formats.
     *
     * The attachments will be assigned locations 0..\#formats-1, with each
     * attachment assigned the given format. If depthStencil is true, a
     * depth/stencil attachment will be added to position \#formats.
     *
     * If the size of the formats parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param formats       The list of desired image formats
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return a newly allocated pass with color attachments for the given formats.
     */
    static std::shared_ptr<RenderPass> allocWithFormats(const std::vector<TexelFormat>& formats,
                                                        bool depthStencil=true) {
        std::shared_ptr<RenderPass> result = std::make_shared<RenderPass>();
        return result->initWithFormats(formats,depthStencil) ? result : nullptr;
    }
    
    /**
     * Returns a newly allocated pass with color attachments for the given formats.
     *
     * The attachments will be assigned locations 0..size-1, with each
     * attachment assigned the given format. If depthStencil is true, a
     * depth/stencil attachment will be added to position size.
     *
     * If the size of the formats parameter is larger than the number of
     * possible shader outputs for this platform, this method will fail.
     *
     * @param formats       The list of desired image formats
     * @param size          The number color attachments
     * @param depthStencil  Whether to include a depth/stencil attachment
     *
     * @return a newly allocated pass with color attachments for the given formats.
     */
    static std::shared_ptr<RenderPass> allocWithFormats(const TexelFormat* formats,
                                                        size_t size,
                                                        bool depthStencil=true) {
        std::shared_ptr<RenderPass> result = std::make_shared<RenderPass>();
        return result->initWithFormats(formats,size,depthStencil) ? result : nullptr;
    }
    
    /**
     * Returns the render pass for the primary display.
     *
     * This render pass is guaranteed to have a color attachment at location 0
     * and a depthStencil attachment at location 1.
     *
     * @return the render pass for the primary display.
     */
    static std::shared_ptr<RenderPass> getDisplay();
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the platform-specific implementation for this render pass
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this render pass
     */
    RenderPassData* getImplementation() { return _data; }
    
    /**
     * Returns the number of color attachments for this render pass
     *
     * This number does not include optional depth/stencil attachment or resolve
     * attachment for MSAA.
     */
    size_t getColorAttachments() const { return _colorFormats.size(); }
    
    /**
     * Returns the color attachment formats for this render pass
     *
     * This vector does not include the format for optional depth/stencil
     * attachment or resolve attachment for MSAA.
     */
    const std::vector<TexelFormat>& getColorFormats() const { return _colorFormats; }
    
    /**
     * Returns the color attachment format at the given position
     *
     * @param pos   The attachment position
     *
     * @return the color attachment format at the given position
     */
    TexelFormat getColorFormat(size_t pos) const;
    
    /**
     * Returns true if this render pass supports a depth/stencil attachment
     *
     * @return true if this render pass supports a depth/stencil attachment
     */
    bool hasDepthStencil() const { return _depthFormat != TexelFormat::UNDEFINED; }
    
    /**
     * Returns the format for the depth/stencil attachment to this render pass
     *
     * This will return VK_FORMAT_UNDEFINED if this render pass does not support
     * a depth/stencil attachment.
     *
     * @return the format for the depth/stencil attachment to this render pass
     */
    TexelFormat getDepthStencilFormat() const { return _depthFormat; }

    /**
     * Returns true if this render pass is compatible with the given one.
     *
     * Compatible render passes can be swapped in a {@link GraphicsShader} on
     * demand. Two render passes are compatible if they have the same number
     * of color attachments (even though formats may change) and both agree on
     * the presence of a depth buffer.
     *
     * @param pass  The render pass to compare
     *
     * Returns true if this render pass is compatible with the given one.
     */
    bool isCompatible(const std::shared_ptr<RenderPass>& pass) {
        return (pass != nullptr &&
                _depthFormat == pass->_depthFormat &&
                _colorFormats.size() == pass->_colorFormats.size());
    }

    /**
     * Returns true if this is compatible with the default render pass.
     *
     * Default renderpasses can be used on the display.
     *
     * Returns true if this is compatible with the default render pass.
     */
    bool isDefaultCompatible() {
        return (_depthFormat == TexelFormat::DEPTH_STENCIL &&
                _colorFormats.size() == 1);
    }

#pragma mark -
#pragma mark Clear States
    /**
     * Returns the clear color(s) for this render pass.
     *
     * These values are used to clear the color attachments when the method
     * {@link FrameBuffer#clear} is called.
     *
     * @return the clear color(s) for this render pass.
     */
    const std::vector<Color4f>& getClearColors() const;

    /**
     * Returns the clear color for the given attachment.
     *
     * This value used to clear the appropriate color attachments when the
     * method {@link FrameBuffer#clear} is called.
     *
     * @param pos   The attachment location
     *
     * @return the clear color for the given attachment.
     */
    Color4f getClearColor(size_t pos) const;

    /**
     * Sets the clear color for the given attachment.
     *
     * This value used to clear the appropriate color attachments when the
     * method {@link FrameBuffer#clear} is called. If this value is changed
     * after a render pass has started, it will not be applied until the next
     * render pass.
     *
     * @param pos   The attachment location
     * @param color The clear color for the given attachment.
     */
    void setClearColor(size_t pos, const Color4f& color);

    /**
     * Returns the clear value for the depth buffer
     *
     * This value is used to clear the depth buffer when the method
     * {@link FrameBuffer#clear} is called.
     *
     * @return the clear value for the depth buffer
     */
    float getClearDepth() const;

    /**
     * Sets the clear value for the depth buffer
     *
     * This value is used to clear the depth buffer when the method
     * {@link FrameBuffer#clear} is called. If this value is changed after a
     * render pass has started, it will not be applied until the next
     * render pass.
     *
     * @param value The clear value for the depth buffer
     */
    void setClearDepth(float value);

    /**
     * Returns the clear value for the stencil buffer
     *
     * This value is used to clear the stencil buffer when the method
     * {@link FrameBuffer#clear} is called.
     *
     * @return the clear value for the stencil buffer
     */
    Uint32 getClearStencil() const;

    /**
     * Sets the clear value for the stencil buffer
     *
     * This value is used to clear the stencil buffer when the method
     * {@link FrameBuffer#clear} is called. If this value is changed after a
     * render pass has started, it will not be applied until the next
     * render pass.
     *
     * @param value The clear value for the stencil buffer
     */
    void setClearStencil(Uint32 value);
    
};
    }
}

#endif /* __CU_RENDER_PASS_H__ */
