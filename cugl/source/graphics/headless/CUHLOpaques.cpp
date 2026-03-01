//
//  CUHLOpaques.cpp
//  Cornell University Game Library (CUGL)
//
//  This module contains the headless versions of the opaque types used in the
//  backend. This allows for CUGL code to compile even when neither OpenGL nor
//  Vulkan are present.
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
//  Version: 3/30/25 (Vulkan Integration)
//
#include <cugl/core/assets/CUJsonValue.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CUImage.h>
#include "CUHLOpaques.h"


using namespace cugl;
using namespace cugl::graphics;

#pragma mark Image Data
/**
 * Initializes this texture with the given data and format
 *
 * Note that data can be nullptr. In that case, it is an empty texture.
 *
 * @param data      The image data (size width*height*format)
 * @param width     The image width in pixels
 * @param height    The image height in pixels
 * @param format    The image data format
 *
 * @return true if initialization is successful
 */
bool ImageData::init(const void* data, int width, int height, TexelFormat format) {
    this->width = width;
    this->height = height;
    bytesize = (Uint32)texel_stride(format);
    size_t size = width * height * bytesize;
    this->data = (std::byte*)malloc(size);
    if (this->data != NULL) {
        std::memcpy(this->data, data, size);
        return true;
    }
    return false;
}

/**
 * Deallocates the image object
 *
 * You must call {@link #init} again to use the object
 */
void ImageData::dispose() {
    if (data != NULL) {
        free(data);
        data = NULL;
    }
}

/**
 * Initializes this image with the given data and format
 *
 * Note that data can be nullptr. In that case, it is an empty texture.
 *
 * @param data      The image data (size width*height*format)
 * @param width     The image width in pixels
 * @param height    The image height in pixels
 * @param layers    The image layers
 * @param format    The image data format
 *
 * @return true if initialization is successful
 */
bool ImageArrayData::init(const void *data, int width, int height, int layers,
                          TexelFormat format) {
    this->width = width;
    this->height = height;
    this->layers = layers;
    bytesize = (Uint32)texel_stride(format);
    size_t size = width*height*layers*bytesize;
    this->data = (std::byte*)malloc(size);
    if (this->data != NULL) {
        std::memcpy(this->data, data, size);
        return true;
    }
    return false;
}

/**
 * Deallocates the image object
 *
 * You must call {@link #init} again to use the object
 */
void ImageArrayData::dispose() {
    if (data != NULL) {
        free(data);
        data = NULL;
    }
}

#pragma mark -
#pragma mark Buffer Data
/**
 * Initializes an empty buffer object
 *
 * You should call {@link #init} to initialize this struct.
 */
BufferData::BufferData() :
    buffer(NULL),
    ready(false),
    size(0),
    blocks(0) {
    access = BufferAccess::STATIC;
}

/**
 * Initializes a buffer with the given size and access
 *
 * This buffer will not have a backing store.
 *
 * @param size      The buffer size
 * @param access    The buffer access policy
 *
 * @return true if the buffer was created successfully
 */
bool BufferData::init(size_t size, BufferAccess access) {
    this->size = size;
    this->access = access;
    
    buffer = (std::byte*)malloc(size*sizeof(std::byte));
    if (buffer != NULL) {
        std::memset(buffer, 0, size * sizeof(std::byte));
        ready = true;
        return true;
    }
    return false;
}

/**
 * Deallocates the buffer object
 *
 * You must call {@link #init} again to use the object
 */
void BufferData::dispose() {
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    size = 0;
    ready = false;
}

/**
 * Loads the data into this buffer.
 *
 * If this is not a dynamic buffer, then this method can only be called
 * once. Additional attempts to load into a static buffer will fail.
 *
 * @param data  The data to load
 * @param size  The number of bytes to load
 *
 * @return true if the data was successfully loaded
 */
bool BufferData::loadData(const void* data, size_t size) {
    if (!ready) {
        return false;
    } else if (size > this->size) {
        CUAssertLog(false, "data exceeds buffer capacity %zu", this->size);
        return false;
    }
    
    std::memcpy(buffer,data,size);
    if (access == BufferAccess::STATIC) {
        ready = false;
    }
    return true;
}

/**
 * Assigns the given region of the backing store to the given data.
 *
 * This method will fail on a static buffer.
 *
 * @param data  The data to load
 * @param size  The number of bytes to load
 *
 * @return true if the data was successfully assigned
 */
bool BufferData::setData(const std::byte* data, size_t offset, size_t size) {
    std::memcpy(buffer+offset, data, size);
    return true;
}

#pragma mark -
#pragma mark FrameBuffer

/**
 * Initializes a default frame buffer object.
 *
 * The frame buffer will be configured for the display
 *
 * @return true if initialization is successful
 */
bool FrameBufferData::init() {
    return true;
}

/**
 * Initializes a frame buffer with the given renderpass.
 *
 * The framebuffer will be configured for offscreen rendering.
 *
 * @param width         The framebuffer width
 * @param height        The framebuffer height
 * @param renderpass    The renderpass for offscreen rendering
 *
 * @return true if initialization is successful
 */
bool FrameBufferData::init(Uint32 width, Uint32 height, const RenderPassData& renderpass) {
    outsize = renderpass.colorFormats->size();
    outputs.resize(outsize,nullptr);
    return true;
}

/**
 * Deallocated any memory associated with this frame buffer
 *
 * The frame buffer cannot be used until an init method is called.
 */
void FrameBufferData::dispose() {
    outputs.clear();
    depthst = nullptr;
}

#pragma mark -
#pragma mark Graphics Shader
/**
 * Creates a graphics shader
 */
GraphicsShaderData::GraphicsShaderData() :
linked(false),
enableBlend(true),
scissorTest(false),
stencilTest(false),
depthTest(false),
depthWrite(false),
colorMask(0xf),
lineWidth(1.0f) {
    scissor[0] = 0;
    scissor[1] = 0;
    scissor[2] = 0;
    scissor[3] = 0;
    drawMode = DrawMode::TRIANGLES;
    depthFunc = CompareOp::ALWAYS;
    colorBlend.set(BlendMode::ALPHA);
}

#pragma mark -
#pragma mark Graphics Context
/**
 * Creates an unitialized OpenGL graphics context.
 *
 * This object must be instantiated before the window is created. However,
 * it does not initialize the OpenGL values. That is done via a call to
 * {@lin #init} once the window is created.
 *
 * @param multisample   Whether to support multisampling.
 */
GraphicsContext::GraphicsContext() :
_framebuffer(NULL) {
    _framebuffer = new FrameBufferData();
    _framebuffer->init();
}

/**
 * Disposed the OpenGL graphics context.
 */
GraphicsContext::~GraphicsContext() {
    if (_framebuffer != nullptr) {
        delete _framebuffer;
        _framebuffer = nullptr;
    }
}

#pragma mark -
#pragma mark Conversion Functions

/**
 * Returns the number of components of the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the number of components of the given GLSL type
 */
Uint32 cugl::graphics::glsl_attribute_size(GLSLType type) {
    switch (type) {
        case GLSLType::INT:
        case GLSLType::UINT:
        case GLSLType::FLOAT:
            return 1;
        case GLSLType::VEC2:
        case GLSLType::IVEC2:
        case GLSLType::UVEC2:
        case GLSLType::MAT2:
            return 2;
        case GLSLType::VEC3:
        case GLSLType::IVEC3:
        case GLSLType::UVEC3:
        case GLSLType::MAT3:
            return 3;
        case GLSLType::VEC4:
        case GLSLType::IVEC4:
        case GLSLType::UVEC4:
        case GLSLType::COLOR:
        case GLSLType::UCOLOR:
        case GLSLType::MAT4:
            return 4;
        case GLSLType::NONE:
        case GLSLType::BLOCK:
            return 0;
    }
    return 0;
}

/**
 * Returns the data stride for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the data stride for the given GLSL type
 */
Uint32 cugl::graphics::glsl_attribute_stride(GLSLType type) {
    switch (type) {
        case GLSLType::NONE:
            CUAssertLog(false, "Attributes cannot have type 'void'");
            return 0;
        case GLSLType::INT:
            return sizeof(Sint32);
        case GLSLType::UINT:
            return sizeof(Uint32);
        case GLSLType::FLOAT:
            return sizeof(float);
        case GLSLType::VEC2:
            return sizeof(float)*2;
        case GLSLType::VEC3:
            return sizeof(float)*3;
        case GLSLType::VEC4:
            return sizeof(float)*4;
        case GLSLType::IVEC2:
            return sizeof(Sint32)*2;
        case GLSLType::IVEC3:
            return sizeof(Sint32)*3;
        case GLSLType::IVEC4:
            return sizeof(Sint32)*4;
        case GLSLType::UVEC2:
            return sizeof(Uint32)*2;
        case GLSLType::UVEC3:
            return sizeof(Uint32)*3;
        case GLSLType::UVEC4:
            return sizeof(Uint32)*4;
        case GLSLType::MAT2:
            return sizeof(float)*4;
        case GLSLType::MAT3:
            return sizeof(float)*9;
        case GLSLType::MAT4:
            return sizeof(float)*16;
        case GLSLType::COLOR:
            return sizeof(float)*4;
        case GLSLType::UCOLOR:
            return sizeof(Uint32);
        case GLSLType::BLOCK:
            CUAssertLog(false, "Attributes cannot have a custom type");
            return 0;
    }
    return 0;
}

/**
 * Returns the byte size for the given texel format
 *
 * @param format  The Texel format
 *
 * @return the byte size for the given texel format
 */
Uint32 cugl::graphics::texel_stride(TexelFormat format) {
    switch (format) {
        case TexelFormat::COLOR_RGB:
            return 3;
        case TexelFormat::COLOR_RGBA4:
        case TexelFormat::COLOR_BGRA4:
        case TexelFormat::COLOR_RGB4:
        case TexelFormat::COLOR_RG:
            return 2;
        case TexelFormat::COLOR_RG4:
        case TexelFormat::COLOR_RED:
        case TexelFormat::STENCIL:
            return 1;
        case TexelFormat::COLOR_RGBA:
        case TexelFormat::COLOR_BGRA:
        case TexelFormat::DEPTH:
        case TexelFormat::DEPTH_STENCIL:
        default:
            return 4;
    }
    
    return 4;
}
