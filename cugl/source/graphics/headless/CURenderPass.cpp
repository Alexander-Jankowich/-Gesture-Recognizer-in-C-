//
//  CURenderPass.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a render pass. Previously, we had hoped
//  to integrate this with RenderTarget. However, offscreen rendering requires
//  that we have the ability to swap out framebuffers. So we needed an extra
//  class to represented when such a swap is valid. This headless version
//  stores the render pass state but does not communicate with a graphics card.
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
#include <cugl/graphics/CURenderPass.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

#pragma mark Constructors
/** The render pass for the display */
std::weak_ptr<RenderPass> RenderPass::g_display;

/**
 * Deletes the render pass and resets all attributes.
 *
 * You must reinitialize the render pass to use it.
 */
void RenderPass::dispose() {
    if (_data != nullptr) {
        delete _data;
        _data = nullptr;
        _colorFormats.clear();
        _depthFormat = TexelFormat::UNDEFINED;
    }
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
bool RenderPass::init(size_t attachments,bool depthStencil) {
    _colorFormats.push_back(TexelFormat::COLOR_RGBA);
    if (depthStencil) {
        _depthFormat = TexelFormat::DEPTH_STENCIL;
    }

    _data = new RenderPassData();
    _data->colorFormats = &_colorFormats;
    _data->depthStencilFormat = &_depthFormat;
    return true;
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
bool RenderPass::initWithFormats(const TexelFormat* formats, size_t size, bool depthStencil) {
    _colorFormats.reserve(size);
    for(int ii = 0; ii < size; ii++) {
        _colorFormats.push_back(formats[ii]);
    }
    if (depthStencil) {
        _depthFormat = TexelFormat::DEPTH_STENCIL;
    }

    _data = new RenderPassData();
    _data->colorFormats = &_colorFormats;
    _data->depthStencilFormat = &_depthFormat;
    return true;
}

/**
 * Initializes a render pass for the primary display.
 *
 * This render pass is guaranteed to have a color attachment at location 0
 * and a depthStencil attachment at location 1.
 *
 * @return true if initialization was successful.
 */
bool RenderPass::initForDisplay() {
    _colorFormats.push_back(TexelFormat::COLOR_BGRA);
    _depthFormat = TexelFormat::DEPTH_STENCIL;

    _data = new RenderPassData();
    _data->colorFormats = &_colorFormats;
    _data->depthStencilFormat = &_depthFormat;
    return true;
}

/**
 * Returns the render pass for the primary display.
 *
 * This render pass is guaranteed to have a color attachment at location 0
 * and a depthStencil attachment at location 1.
 *
 * @return the render pass for the primary display.
 */
std::shared_ptr<RenderPass> RenderPass::getDisplay() {
    auto pass = g_display.lock();
    if (pass == nullptr) {
        pass = std::make_shared<RenderPass>();
        if (pass->initForDisplay()) {
            g_display = pass;
            return pass;
        }
    }
    return pass;
}

#pragma mark -
#pragma mark Attributes
/**
 * Returns the color attachment format at the given position
 *
 * @param pos   The attachment position
 *
 * @return the color attachment format at the given position
 */
TexelFormat RenderPass::getColorFormat(size_t pos) const {
    CUAssertLog(pos < _colorFormats.size(), "Position %zu is out of bounds.",pos);
    return _colorFormats[pos];
}
    
/**
 * Returns the clear color(s) for this render pass.
 *
 * These values are used to clear the color attachments when the method
 * {@link #begin} is called.
 *
 * @return the clear color(s) for this render pass.
 */
const std::vector<Color4f>& RenderPass::getClearColors() const {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    return _data->clearColors;
}

/**
 * Returns the clear color for the given attachment.
 *
 * This value used to clear the appropriate color attachments when the
 * method {@link #begin} is called.
 *
 * @param pos   The attachment location
 *
 * @return the clear color for the given attachment.
 */
Color4f RenderPass::getClearColor(size_t pos) const {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    return _data->clearColors[pos];
}

/**
 * Sets the clear color for the given attachment.
 *
 * This value used to clear the appropriate color attachments when the
 * method {@link #begin} is called. If this value is changed after a call
 * to begin, it will not be applied until the next render pass.
 *
 * @param pos   The attachment location
 * @param color The clear color for the given attachment.
 */
void RenderPass::setClearColor(size_t pos, const Color4f& color) {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    _data->clearColors[pos]=color;
}

/**
 * Returns the clear value for the depth buffer
 *
 * This value is used to clear the depth buffer when the method
 * {@link #begin} is called.
 *
 * @return the clear value for the depth buffer
 */
float RenderPass::getClearDepth() const {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    return _data->clearDepth;
}

/**
 * Sets the clear value for the depth buffer
 *
 * This value is used to clear the depth buffer when the method
 * {@link #begin} is called. If this value is changed after a call to
 * begin, it will not be applied until the next render pass.
 *
 * @param value The clear value for the depth buffer
 */
void RenderPass::setClearDepth(float value) {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    _data->clearDepth = value;

}

/**
 * Returns the clear value for the stencil buffer
 *
 * This value is used to clear the stencil buffer when the method
 * {@link #begin} is called.
 *
 * @return the clear value for the stencil buffer
 */
Uint32 RenderPass::getClearStencil() const {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    return _data->clearStencil;

}

/**
 * Sets the clear value for the stencil buffer
 *
 * This value is used to clear the stencil buffer when the method
 * {@link #begin} is called. If this value is changed after a call to
 * begin, it will not be applied until the next render pass.
 *
 * @param value The clear value for the stencil buffer
 */
void RenderPass::setClearStencil(Uint32 value) {
    CUAssertLog(_data != nullptr, "The render pass is not initialized");
    _data->clearStencil = value;
}
