//
//  CUFrameBuffer.cpp
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
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/CUApplication.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/graphics/CUFrameBuffer.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CURenderPass.h>
#include "CUGLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/** The default frame buffer */
std::shared_ptr<FrameBuffer> FrameBuffer::_gFramebuffer = nullptr;

#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized framebuffer with no output textures.
 *
 * You must initialize the framebuffer to create an output texture.
 */
FrameBuffer::FrameBuffer() :
_attachments(0),
_data(nullptr),
_x(0),
_y(0),
_width(0),
_height(0),
_display(false) {
}

/**
 * Deletes the framebuffer and resets all attributes.
 *
 * You must reinitialize the framebuffer to use it.
 */
void FrameBuffer::dispose() {
    if (_data != nullptr && !_display) {
        delete _data;
    }
    
    _textures.clear();
    _display = false;
    _data = nullptr;
    _width = 0;
    _height = 0;
    _attachments = 0;
    _renderPass = nullptr;
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
bool FrameBuffer::init(Uint32 width, Uint32 height, size_t outputs) {
    _renderPass = RenderPass::alloc(outputs,true);
    if (_renderPass == nullptr) {
        return false;
    }
    
    auto imp = _renderPass->getImplementation();
    _data = new FrameBufferData();
    if (!_data->init(width,height,*(imp))) {
        delete _data;
        _data = nullptr;
        return false;
    }
    
    _width = width;
    _height = height;
    _attachments = outputs;
    _textures.resize(outputs,nullptr);
    restoreViewPort();
    return true;
}

/**
 * Initializes this target with multiple images of the given format.
 *
 * The output images will have the given width and size. They will be
 * assigned the appropriate format at locations 0..\#outputs-1. These
 * locations should be bound with the layout keyword in any shader used
 * with this render target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 *
 * @param width     The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs   The list of desired texture formats
 *
 * @return true if initialization was successful.
 */
bool FrameBuffer::initWithFormats(Uint32 width, Uint32 height,
                                  std::initializer_list<TexelFormat> outputs) {
    std::vector<TexelFormat> formats;
    size_t size = 0;
    for(auto it = outputs.begin(); it != outputs.end(); size++, ++it) {
        formats.push_back(*it);
    }
    _renderPass = RenderPass::allocWithFormats(formats,true);
    if (_renderPass == nullptr) {
        return false;
    }

    auto imp = _renderPass->getImplementation();
    _data = new FrameBufferData();
    if (!_data->init(width,height,*(imp))) {
        delete _data;
        _data = nullptr;
        return false;
    }
    
    _width = width;
    _height = height;
    _attachments = formats.size();
    _textures.resize(_attachments,nullptr);
    restoreViewPort();
    return true;
}

/**
 * Initializes this target with multiple images of the given format.
 *
 * The output images will have the given width and size. They will
 * be assigned the appropriate format at locations 0..\#outsize-1. These
 * locations should be bound with the layout keyword in any shader used
 * with this render target. Otherwise the results are not well-defined.
 *
 * If the size of the outputs parameter is larger than the number of
 * possible shader outputs for this platform, this method will fail.
 *
 * @param width     The drawing width of this render target
 * @param height    The drawing width of this render target
 * @param outputs   The list of desired texture formats
 * @param outsize   The number of elements in outputs
 *
 * @return true if initialization was successful.
 */
bool FrameBuffer::initWithFormats(Uint32 width, Uint32 height,
                                   TexelFormat* outputs, size_t outsize) {
    _renderPass = RenderPass::allocWithFormats(outputs, outsize, true);
    if (_renderPass == nullptr) {
        return false;
    }
    auto imp = _renderPass->getImplementation();
    _data = new FrameBufferData();
    if (!_data->init(width,height,*(imp))) {
        delete _data;
        _data = nullptr;
        return false;
    }
    
    _width = width;
    _height = height;
    _attachments = outsize;
    _textures.resize(_attachments,nullptr);
    restoreViewPort();
    return true;
}

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
bool FrameBuffer::initWithRenderPass(Uint32 width, Uint32 height, const std::shared_ptr<RenderPass>& pass) {
    _attachments = pass->getColorFormats().size();
    _data = new FrameBufferData();
    auto imp = pass->getImplementation();
    if (!_data->init(width,height,*(imp))) {
        delete _data;
        _data = nullptr;
        return false;
    }

    _width = width;
    _height = height;
    _renderPass = pass;
    restoreViewPort();
    return true;
}

#pragma mark -
#pragma mark Attributes
/**
 * Returns true if rendering to this framebuffer happens immediately
 *
 * An immediate framebuffer guarantees that its image(s) are available as
 * soon as {@link #end} is called. Otherwise, there may be a delay between
 * when {@link #end} is called and {@link #getImage} is available. There is
 * no indication for when such an image is available, short of progressing
 * to the next frame.
 *
 * This is an optimization for subpass rendering on Vulkan. It has no effect
 * in OpenGL. This value cannot be changed during the middle of an active
 * render pass on this framebuffer.
 *
 * @return true if rendering to this framebuffer happens immediately
 */
bool FrameBuffer::isImmediate() const {
    CUAssertLog(_data != nullptr, "The render target has not been initialized");
    return true;
}

/**
 * Sets whether rendering to this framebuffer happens immediately
 *
 * An immediate framebuffer guarantees that its image(s) are available as
 * soon as {@link #end} is called. Otherwise, there may be a delay between
 * when {@link #end} is called and {@link #getImage} is available. There is
 * no indication for when such an image is available, short of progressing
 * to the next frame.
 *
 * This is an optimization for subpass rendering on Vulkan. It has no effect
 * in OpenGL. This value cannot be changed during the middle of an active
 * render pass on this framebuffer.
 *
 * @param value Whether rendering to this framebuffer happens immediately
 */
void FrameBuffer::setImmediate(bool value) {
    CUAssertLog(_data != nullptr, "The render target has not been initialized");
    // Ignored
}

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
const std::shared_ptr<Image> FrameBuffer::getImage(size_t index) const {
    CUAssertLog(_data != nullptr, "The render target has not been initialized");
    return _data->outputs[index];
}

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
const std::shared_ptr<Texture> FrameBuffer::getTexture(size_t index) {
    CUAssertLog(_data != nullptr, "The render target has not been initialized");
    if (_textures[index] == nullptr) {
        _textures[index] = Texture::allocWithImage(_data->outputs[index]);
    }
    return _textures[index];
}

/**
 * Returns the depth/stencil buffer for this framebuffer
 *
 * The framebuffer for a render target always uses a combined depth and
 * stencil buffer image. It uses 24 bits for the depth and 8 bits for the
 * stencil. This should be sufficient in most applications.
 *
 * @return the depth/stencil buffer for this framebuffer
 */
const std::shared_ptr<Image> FrameBuffer::getDepthStencil() const {
    CUAssertLog(_data != nullptr, "The render target has not been initialized");
    return _data->depthst;
}

#pragma mark -
#pragma mark Rendering
/**
 * Returns the framebuffer for the window display.
 *
 * This is the default framebuffer for on-screen drawing.
 */
std::shared_ptr<FrameBuffer> FrameBuffer::getDisplay() {
    if (!_gFramebuffer) {
        _gFramebuffer = std::make_shared<FrameBuffer>();
        auto data = Display::get()->getGraphicsContext()->getFrameBuffer();
        Rect bounds = Display::get()->getViewBounds();
        _gFramebuffer->_name = "__display__";
        _gFramebuffer->_display = "__true__";
        _gFramebuffer->_data = data;
        _gFramebuffer->_attachments = data->outputs.size();
        _gFramebuffer->_renderPass = RenderPass::getDisplay();
        _gFramebuffer->_textures.resize(data->outputs.size(),nullptr);
        _gFramebuffer->_width = bounds.size.width;
        _gFramebuffer->_height = bounds.size.height;
        _gFramebuffer->_viewport[0] = data->offset.x;
        _gFramebuffer->_viewport[1] = data->offset.y;
        _gFramebuffer->_viewport[2] = bounds.size.width;
        _gFramebuffer->_viewport[3] = bounds.size.height;
    }
    
    return _gFramebuffer;
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
void FrameBuffer::setViewPort(int x, int y, int width, int height) {
    _viewport[0] = x;
    _viewport[1] = y;
    _viewport[2] = width;
    _viewport[3] = height;
}

/**
 * Restores the viewport for this framebuffer
 *
 * The viewport is reset to the default, matching {@link #getSize}.
 */
void FrameBuffer::restoreViewPort() {
    _viewport[0] = _x;
    _viewport[1] = _y;
    _viewport[2] = _width;
    _viewport[3] = _height;
}

/**
 * Activates the framebuffer for drawing
 */
void FrameBuffer::activate() {
    _data->activate(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
}

/**
 * Clears the contents of this framebuffer.
 *
 * The framebuffer will use the {@link RenderPass} settings to clear this
 * framebuffer.
 */
void FrameBuffer::clear() {
    _data->activate(_x,_y,_width,_height);
    _data->clear(_renderPass->getImplementation());
}
