//
//  CUGLOpaques.cpp
//  Cornell University Game Library (CUGL)
//
//  This module contains the OpenGL versions of the opaque types necessary to
//  provide a uniform pipeline for OpenGL and Vulkan. It also provides some
//  very simple OpenGL debugging tools.
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
#include <cugl/graphics/CUFrameBuffer.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CUImage.h>
#include "CUGLOpaques.h"


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
 * @param mipmaps   A flag to generate mipmaps
 * @param format    The image data format
 *
 * @return true if initialization is successful
 */
bool ImageData::init(const void *data, int width, int height, bool mipmaps, TexelFormat format) {
    GLenum error;
    glGenTextures(1, &image);
    if (image == 0) {
        error = glGetError();
        CULogError("Could not allocate image: %s", gl_error_name(error).c_str());
        return false;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image);

    GLint  internal = gl_internal_format(format);
    GLenum symbolic = gl_symbolic_format(format);
    GLenum datatype = gl_format_type(format);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, symbolic, datatype, data);
    
    error = glGetError();
    if (error) {
        CULogError("Could not initialize image: %s", gl_error_name(error).c_str());
        glDeleteTextures(1, &image);
        image = 0;
        return false;
    }

    if (mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

/**
 * Deallocates the image object
 *
 * You must call {@link #init} again to use the object
 */
void ImageData::dispose() {
    if (image != 0) {
        glDeleteTextures(1, &image);
        image = 0;
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
 * @param mipmaps   A flag to generate mipmaps
 * @param format    The image data format
 *
 * @return true if initialization is successful
 */
bool ImageArrayData::init(const void *data, int width, int height, int layers,
                          bool mipmaps, TexelFormat format) {
    GLenum error;
    glGenTextures(1, &images);
    if (images == 0) {
        error = glGetError();
        CULogError("Could not allocate images: %s", gl_error_name(error).c_str());
        return false;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, images);

    GLint  internal = gl_internal_format(format);
    GLenum symbolic = gl_symbolic_format(format);
    GLenum datatype = gl_format_type(format);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal, width, height, layers,
                 0, symbolic, datatype, data);
    
    error = glGetError();
    if (error) {
        CULogError("Could not initialize images: %s", gl_error_name(error).c_str());
        glDeleteTextures(1, &images);
        images = 0;
        return false;
    }

    if (mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    return true;
}

/**
 * Deallocates the image object
 *
 * You must call {@link #init} again to use the object
 */
void ImageArrayData::dispose() {
    if (images != 0) {
        glDeleteTextures(1, &images);
        images = 0;
    }
}


/**
 * Initializes an empty sampler data object
 *
 * You should set the image buffer to initialize this struct.
 */
SamplerData::SamplerData() {
    magFilter = GL_LINEAR;
    minFilter = GL_LINEAR;
    wrapS = GL_CLAMP_TO_EDGE;
    wrapT = GL_CLAMP_TO_EDGE;
}

/**
 * Applies the sampler to the currently bound texture.
 *
 * Note that this method does not rebind to the image attribute. So if
 * the current texture is a different texture, this will push the sample
 * attributes to that texture.
 */
void SamplerData::apply() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

}


#pragma mark -
#pragma mark Buffer Data
/**
 * Initializes an empty buffer object
 *
 * You should call {@link #init} to initialize this struct.
 */
BufferData::BufferData() :
    buffer(0),
    ready(false),
    store(nullptr),
    size(0),
    blocks(0),
    start(0),
    end(0) {
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
    if (access != BufferAccess::STATIC && access != BufferAccess::DYNAMIC) {
        CUAssertLog(false, "OpenGL does not support compute buffers");
        return false;
    }
    this->access = access;
    this->size = size;
    
    glGenBuffers(1, &buffer);
    if (!buffer) {
        GLenum error = glGetError();
        CULogError("Could not create vertex buffer. %s", gl_error_name(error).c_str());
        return false;
    }
    
    GLenum usage = access == BufferAccess::STATIC ? GL_STATIC_DRAW : GL_STREAM_DRAW;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, usage);
    ready = true;
    return true;
}

/**
 * Initializes a mapped buffer with the given size and access
 *
 * This buffer will have a backing store and the access will by dynamic.
 *
 * @param size      The buffer size
 *
 * @return true if the buffer was created successfully
 */
bool BufferData::initMapped(size_t size) {
    this->size = size;
    access = BufferAccess::DYNAMIC;
    
    glGenBuffers(1, &buffer);
    if (!buffer) {
        GLenum error = glGetError();
        CULogError("Could not create vertex buffer. %s", gl_error_name(error).c_str());
        return false;
    }

    store = (std::byte*)malloc(size*sizeof(std::byte));
    if (store != NULL) {
        std::memset(store, 0, size * sizeof(std::byte));
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
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
    if (buffer != 0) {
        glDeleteBuffers(1,&buffer);
        buffer = 0;
    }
    if (store != nullptr) {
        free(store);
        store = nullptr;
    }
    size = 0;
    ready = false;
}

/**
 * Loads the data into this buffer.
 *
 * This method loads directly into the buffer, not the backing store.
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
    
    if (access == BufferAccess::STATIC) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        ready = false;
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, this->size, NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);

    }
    return true;
}

/**
 * Assigns the given region of the backing store to the given data.
 *
 * This method only loads the data into the backing store. It does not
 * update the buffer. You should call {@link #flush} to subsequently
 * update the buffer. This method does nothing if the buffer has no
 * backing store.
 *
 * @param data  The data to load
 * @param size  The number of bytes to load
 *
 * @return true if the data was successfully assigned
 */
bool BufferData::setData(const std::byte* data, size_t offset, size_t size) {
    if (store == nullptr) {
        CUAssertLog(false, "buffer has no backing store");
        return false;
    }
    std::memcpy(store+offset, data, size);
    start = std::min(start, offset);
    end   = std::max(end,   offset + size);
    return true;
}

/**
 * Loads the backing store into the buffer.
 *
 * This method does nothing if the buffer is not dynamic or there is no
 * dirty region.
 *
 * @return true if the buffer was updated
 */
bool BufferData::flush() {
    if (start == end) {
        return false;
    }
    
    // The threshold to redo the whole buffer
    const float THRESHOLD = 0.3f;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    if (end-start > THRESHOLD*size) {
        glBufferData(GL_ARRAY_BUFFER, this->size, NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, store);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, start, end-start, store);
    }
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
    // Clear any previous errors
    glGetError();

    // This is required because iOS does NOT use framebuffer 0
    GLint temp;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,  &temp);
    framebo = (GLuint)temp;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &temp);
    renderbo = (GLuint)temp;
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
    
    if (prepareBuffer(width,height,renderpass)) {
        for(GLuint ii = 0; ii < outsize; ii++) {
            if (!attachTexture(ii, width, height, renderpass.colorFormats->at(ii))) {
                return false;
            }
        }
        return completeBuffer();
    }
    
    return false;
}

/**
 * Deallocated any memory associated with this frame buffer
 *
 * The frame buffer cannot be used until an init method is called.
 */
void FrameBufferData::dispose() {
    if (owns && framebo != 0) {
        glDeleteFramebuffers(1, &framebo);
        framebo = 0;
    }
    if (owns && renderbo != 0) {
        glDeleteRenderbuffers(1, &renderbo);
        renderbo = 0;
    }
    outputs.clear();
    bindpoints.clear();
    depthst = nullptr;
    owns = false;
}

/**
 * Activates the given framebuffer with the given viewport
 *
 * @param x         The x-coordinate of the origin
 * @param y         The y-coordinate of the origin
 * @param width     The viewport width
 * @param height    The viewport height
 */
void FrameBufferData::activate(int x, int y, int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbo);
    glViewport(x,y,width,height);
}

/**
 * Clears the contents of this framebuffer.
 *
 * @param renderpass    The renderpass defining the clear state
 */
void FrameBufferData::clear(const RenderPassData* renderpass) {
    if (renderpass->clearColors.size() > 1) {
        GLfloat c[4];
        for(int ii = 0; ii < renderpass->clearColors.size(); ii++) {
            Color4f color = renderpass->clearColors[ii];
            c[0] = color.r;
            c[1] = color.g;
            c[2] = color.b;
            c[3] = color.a;
            glClearBufferfv(GL_COLOR, ii, c);
        }
    } else {
        Color4f color = renderpass->clearColors.back();
        glClearColor(color.r,color.g,color.b,color.a);
    }
    glClearDepthf(renderpass->clearDepth);
    glClearStencil(renderpass->clearStencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}


/**
 * Initializes the framebuffer and associated render buffer
 *
 * This method also initializes the depth/stencil buffer. It allocates the
 * arrays for the output textures and bindpoints. However, it does not
 * initialize the output textures. That is done in {@lin #attachTexture}.
 *
 * If this method fails, it will safely clean up any allocated objects
 * before quitting.
 *
 * @param width         The framebuffer width
 * @param height        The framebuffer height
 * @param renderpass    The renderpass for offscreen rendering
 *
 * @return true if initialization was successful.
 */
bool FrameBufferData::prepareBuffer(Uint32 width, Uint32 height, const RenderPassData& renderpass) {
    GLenum error;
    glGenFramebuffers(1, &framebo);
    if (!framebo) {
        error = glGetError();
        CULogError("Could not create frame buffer. %s", gl_error_name(error).c_str());
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebo);
    
    // Attach the depth buffer first
    if (*renderpass.depthStencilFormat != TexelFormat::UNDEFINED) {
        depthst = Image::alloc(width,height,TexelFormat::DEPTH_STENCIL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D,  depthst->getImplementation()->image, 0);
        if (depthst == nullptr) {
            dispose();
            FrameBuffer::getDisplay()->activate();
            return false;
        }
    
        glGenRenderbuffers(1, &renderbo);
        if (!renderbo) {
            error = glGetError();
            CULogError("Could not create render buffer. %s", gl_error_name(error).c_str());
            dispose();
            FrameBuffer::getDisplay()->activate();
            return false;
        }
        
        glBindRenderbuffer(GL_RENDERBUFFER, renderbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, renderbo);
        error = glGetError();
        if (error) {
            CULogError("Could not attach render buffer to frame buffer. %s",
                       gl_error_name(error).c_str());
            dispose();
            FrameBuffer::getDisplay()->activate();
            return false;
        }
    }

    return true;
}

/**
 * Attaches an output texture with the given format to framebuffer.
 *
 * This method allocates the texture and binds it in the correct place
 * (e.g. GL_COLOR_ATTACHMENT0+index). The texture will be the same size
 * as this frame buffer.
 *
 * If this method fails, it will safely clean up any previously allocated
 * objects before quitting.
 *
 * @param index     The index location to attach this output texture
 * @param width     The framebuffer width
 * @param height    The framebuffer height
 * @param format    The texture pixel format
 *
 * @return true if initialization was successful.
 */
bool FrameBufferData::attachTexture(GLuint index, Uint32 width, Uint32 height, TexelFormat format) {
    GLenum error;

    auto image = Image::alloc(width,height,format);
    outputs.push_back(image);
    bindpoints.push_back(GL_COLOR_ATTACHMENT0+(GLint)index);
    if (image == nullptr) {
        dispose();
        FrameBuffer::getDisplay()->activate();
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+(GLint)index,
                           GL_TEXTURE_2D,  image->getImplementation()->image, 0);
    error = glGetError();
    if (error) {
        CULogError("Could not attach output textures to frame buffer. %s",
                   gl_error_name(error).c_str());
        dispose();
        FrameBuffer::getDisplay()->activate();
        return false;
    }
    return true;
}

/**
 * Completes the framebuffer after all attachments are finalized
 *
 * This sets the draw buffers and checks the framebuffer status. If OpenGL
 * says that it is complete, it returns true.
 *
 * If this method fails, it will safely clean up any previously allocated
 * objects before quitting.
 *
 * @return true if the framebuffer was successfully finalized.
 */
bool FrameBufferData::completeBuffer() {
    glDrawBuffers((int)outsize, bindpoints.data());
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        CULogError("Could not bind frame buffer. %s",
                   gl_error_name(status).c_str());
        dispose();
        FrameBuffer::getDisplay()->activate();
        return false;
    }

    FrameBuffer::getDisplay()->activate();
    return true;
}

#pragma mark -
#pragma mark Graphics Shader
/**
 * Creates an initialized graphics shader
 *
 * You should call {@link #init} to initialize the shader
 */
GraphicsShaderData::GraphicsShaderData() :
linked(false),
program(0),
vertShader(0),
fragShader(0),
vertArray(0),
enableBlend(true),
scissorTest(false),
stencilTest(false),
depthTest(false),
depthWrite(false),
colorMask(0xf),
dirtyInput(false),
dirtyPass(false),
dirtyState(0) {
    scissor[0] = 0;
    scissor[1] = 0;
    scissor[2] = 0;
    scissor[3] = 0;
    frontFace = FrontFace::CCW;
    command = GL_TRIANGLES;
    drawMode = DrawMode::TRIANGLES;
    depthFunc = CompareOp::LESS;
    colorBlend.set(BlendMode::PREMULT);
}

/**
 * Initializes shader from the given sources
 *
 * @param vsource   The source for the vertex shader.
 * @param fsource   The source for the fragment shader.
 *
 * @return true if initialization was successful.
 */
bool GraphicsShaderData::init(const std::string& vsource, const std::string& fsource) {
    CUAssertLog(!vsource.empty(), "Vertex shader source is not defined");
    CUAssertLog(!fsource.empty(), "Fragment shader source is not defined");
    CUAssertLog(!program,   "This shader is already compiled");
    
    program = glCreateProgram();
    if (!program) {
        CULogError("Unable to allocate shader program");
        return false;
    }
    
    // Create vertex shader and compile it
    vertShader = glCreateShader( GL_VERTEX_SHADER );
    const char* source = vsource.c_str();
    glShaderSource( vertShader, 1, &source, nullptr );
    glCompileShader( vertShader );
    
    // Validate and quit if failed
    if (!validateShader(vertShader, "vertex")) {
        dispose();
        return false;
    }

    // Create fragment shader and compile it
    fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    source = fsource.c_str();
    glShaderSource( fragShader, 1, &source, nullptr );
    glCompileShader( fragShader );
    
    // Validate and quit if failed
    if (!validateShader(fragShader, "fragment")) {
        dispose();
        return false;
    }
    
    return true;
}

/**
 * Deallocates any memory assigned to this graphics shader
 */
void GraphicsShaderData::dispose() {
    glUseProgram(0);
    if (fragShader) { glDeleteShader(fragShader); fragShader = 0;}
    if (vertShader) { glDeleteShader(vertShader); vertShader = 0;}
    if (program) { glDeleteShader(program); program = 0;}
    if (vertArray) { glDeleteVertexArrays(1, &vertArray); vertArray = 0;}
}

/**
 * Links the vertex and fragment shaders to this shader program
 *
 * This also creates the vertex arrays to associate with this shader.
 * If linking fails, it will display error messages on the log.
 *
 * @param arrays    the number of vertex arrays to allocate
 *
 * @return true if linking was successful.
 */
bool GraphicsShaderData::link(size_t arrays) {
    glAttachShader( program, vertShader );
    glAttachShader( program, fragShader );
    glLinkProgram( program );
    
    //Check for errors
    GLint programSuccess = GL_TRUE;
    glGetProgramiv( program, GL_LINK_STATUS, &programSuccess );
    if( programSuccess != GL_TRUE ) {
        CULogError( "Unable to link program %d.\n", program );
        logProgramError(program);
        dispose();
        return false;
    }
    
    glGenVertexArrays(1, &vertArray);
    linked = true;
    return true;
}

/**
 * Flushes all state to OpenGL.
 *
 * This will push any dirty state to OpenGL.
 */
void GraphicsShaderData::flush() {
    if (dirtyState & SHADER_DIRTY_SCISSOR) {
        if (scissorTest) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }
    if (dirtyState & SHADER_DIRTY_DRAWMODE) {
        command = gl_draw_mode(drawMode);
    }
    if (dirtyState & SHADER_DIRTY_FRONTFACE) {
        glFrontFace(frontFace == FrontFace::CW ? GL_CW : GL_CCW);
    }
    if (dirtyState & SHADER_DIRTY_CULLMODE) {
        switch (cullMode) {
            case CullMode::NONE:
                glDisable(GL_CULL_FACE);
                break;
            case CullMode::BACK:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                break;
            case CullMode::FRONT:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                break;
            case CullMode::FRONT_BACK:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT_AND_BACK);
                break;
        }
    }
    if (dirtyState & SHADER_DIRTY_STENCIL) {
        if (stencilTest) {
            glEnable(GL_STENCIL_TEST);
            if (stencilFront != stencilBack) {
                glStencilMaskSeparate(GL_FRONT, stencilFront.writeMask);
                glStencilMaskSeparate(GL_BACK,  stencilBack.writeMask);
                glStencilFuncSeparate(GL_FRONT, gl_compare_op(stencilFront.compareOp),
                                      stencilFront.reference, stencilFront.compareMask);
                glStencilFuncSeparate(GL_BACK, gl_compare_op(stencilBack.compareOp),
                                      stencilBack.reference, stencilBack.compareMask);
                glStencilOpSeparate(GL_FRONT, gl_stencil_op(stencilFront.failOp),
                                    gl_stencil_op(stencilFront.depthFailOp),
                                    gl_stencil_op(stencilFront.passOp));
                glStencilOpSeparate(GL_BACK, gl_stencil_op(stencilBack.failOp),
                                    gl_stencil_op(stencilBack.depthFailOp),
                                    gl_stencil_op(stencilBack.passOp));
            } else {
                glStencilMask(stencilFront.writeMask);
                glStencilFunc(gl_compare_op(stencilFront.compareOp),
                              stencilFront.reference, stencilFront.compareMask);
                glStencilOp(gl_stencil_op(stencilFront.failOp),
                            gl_stencil_op(stencilFront.depthFailOp),
                            gl_stencil_op(stencilFront.passOp));
            }
        } else {
            glDisable(GL_STENCIL_TEST);
        }
        // Follow Vulkan conventions
        glColorMask(colorMask & 0x01 ? GL_TRUE : GL_FALSE,
                    colorMask & 0x02 ? GL_TRUE : GL_FALSE,
                    colorMask & 0x04 ? GL_TRUE : GL_FALSE,
                    colorMask & 0x08 ? GL_TRUE : GL_FALSE);
    }
    if (dirtyState & SHADER_DIRTY_BLEND) {
        if (enableBlend) {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(gl_blend_func(colorBlend.srcFactor),
                                gl_blend_func(colorBlend.dstFactor),
                                gl_blend_func(colorBlend.srcAlphaFactor),
                                gl_blend_func(colorBlend.dstAlphaFactor));
            glBlendEquationSeparate(gl_blend_eq(colorBlend.colorEq),
                                    gl_blend_eq(colorBlend.alphaEq));
            glBlendColor(colorBlend.color.r, colorBlend.color.g,
                         colorBlend.color.b, colorBlend.color.a);
        } else {
            glDisable(GL_BLEND);
        }
    }
    if (dirtyState & SHADER_DIRTY_DEPTH) {
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(gl_compare_op(depthFunc));
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(depthWrite ? GL_TRUE : GL_FALSE);
    }
    dirtyState = 0;
}

/**
 * Returns true if the shader was compiled properly.
 *
 * If compilation fails, it will display error messages on the log.
 *
 * @param shader    The shader to test
 * @param type      The shader type (vertex or fragment)
 *
 * @return true if the shader was compiled properly.
 */
bool GraphicsShaderData::validateShader(GLuint shader, const std::string type) {
    CUAssertLog( glIsShader( shader ), "Shader %d is not a valid shader", shader);
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &shaderCompiled );
    if( shaderCompiled != GL_TRUE ) {
        CULogError( "Unable to compile %s shader %d.\n", type.c_str(), shader );
        logShaderError(shader);
        return false;
    }
    return true;
}

/**
 * Displays the shader compilation errors to the log.
 *
 * If there were no errors, this method will do nothing.
 *
 * @param shader    The shader to test
 */
void GraphicsShaderData::logShaderError(GLuint shader) {
    CUAssertLog( glIsShader( shader ), "Shader %d is not a valid shader", shader);
    //Make sure name is shader

    //Shader log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;
        
    //Get info string length
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
    //Allocate string
    char* infoLog = new char[ maxLength ];
        
    //Get info log
    glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
    CULog("Log length %d",infoLogLength);
    if( infoLogLength > 0 ) {
        // Print Log
        CULog( "%s\n", infoLog );
    }
        
    //Deallocate string
    delete[] infoLog;
}

/**
 * Displays the program linker errors to the log.
 *
 * If there were no errors, this method will do nothing.
 *
 * @param program    The program to test
 */
void GraphicsShaderData::logProgramError(GLuint program) {
    CUAssertLog( glIsProgram( program ), "Program %d is not a valid program", program);

    //Program log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;
        
    //Get info string length
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
        
    //Allocate string
    char* infoLog = new char[ maxLength ];
        
    //Get info log
    glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
    if( infoLogLength > 0 ) {
        // Print Log
        CULogError( "%s\n", infoLog );
    }
    
    //Deallocate string
    delete[] infoLog;
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
GraphicsContext::GraphicsContext(bool multisample) :
_glContext(NULL),
_framebuffer(NULL),
_multisample(false),
_rotation(0) {
    SDL_ClearError();
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    
#if CU_GRAPHICS_API == CU_OPENGLES
    int profile = SDL_GL_CONTEXT_PROFILE_ES;
    int version = 3; // Force 3 on mobile
#else
    int profile = SDL_GL_CONTEXT_PROFILE_CORE;
    int version = 4; // Force 4 on desktop
    if (multisample) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
        _multisample = true;
    }
#endif
    
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile)) {
        CULogError("OpenGL is not supported on this platform: %s", SDL_GetError());
        SDL_SetError("OpenGL is not supported");
        return;
    }
    
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version)) {
        CULogError("OpenGL %d is not supported on this platform: %s", version, SDL_GetError());
        SDL_SetError("OpenGL is not supported");
        return;
    }
        
    // Enable stencil support for sprite batch
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

/**
 * Disposed the OpenGL graphics context.
 */
GraphicsContext::~GraphicsContext() {
    if (_glContext != NULL) {
        SDL_GL_DestroyContext(_glContext);
        _glContext = NULL;
    }
    if (_framebuffer != nullptr) {
        delete _framebuffer;
        _framebuffer = nullptr;
    }
}

/**
 * Initializes the OpenGL context
 *
 * This method must be called after the Window is created.
 *
 * @param window    The associated SDL window
 *
 * @return true if initialization was successful
 */
bool GraphicsContext::init(SDL_Window* window) {
#if CU_GRAPHICS_API == CU_OPENGL
    if (_multisample) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }
#endif
    
    // Create the OpenGL context
    _glContext = SDL_GL_CreateContext( window );
    if( _glContext == NULL )  {
        CULogError("Could not create OpenGL context: %s", SDL_GetError() );
        return false;
    }
    
    // Multisampling support
#if CU_GRAPHICS_API == CU_OPENGL
    glEnable(GL_LINE_SMOOTH);
    if (_multisample) {
        glEnable(GL_MULTISAMPLE);
    }
#endif
    
#if CU_PLATFORM == CU_PLATFORM_WINDOWS || CU_PLATFORM == CU_PLATFORM_LINUX
    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("Error initializing GLEW: %s", glewGetErrorString(glewError));
    }
#endif
    
    _framebuffer = new FrameBufferData();
    _framebuffer->init();
    return true;
}

/**
 * Returns true if this display supports multisampling
 *
 * @return true if this display supports multisampling
 */
bool GraphicsContext::supportsMultisample() {
#if CU_GRAPHICS_API == CU_OPENGLES
    return false;
#endif
    return true;
}

#pragma mark -
#pragma mark Conversion Functions
/**
 * Returns the OpenGL type for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the OpenGL type for the given GLSL type
 */
GLenum cugl::graphics::glsl_attribute_type(GLSLType type) {
    switch (type) {
        case GLSLType::INT:
        case GLSLType::IVEC2:
        case GLSLType::IVEC3:
        case GLSLType::IVEC4:
            return GL_INT;
        case GLSLType::UINT:
        case GLSLType::UVEC2:
        case GLSLType::UVEC3:
        case GLSLType::UVEC4:
            return GL_UNSIGNED_INT;
        case GLSLType::FLOAT:
        case GLSLType::VEC2:
        case GLSLType::VEC3:
        case GLSLType::VEC4:
        case GLSLType::MAT2:
        case GLSLType::MAT3:
        case GLSLType::MAT4:
        case GLSLType::COLOR:
            return GL_FLOAT;
        case GLSLType::UCOLOR:
            return GL_UNSIGNED_BYTE;
        case GLSLType::BLOCK:
        case GLSLType::NONE:
            return GL_INVALID_ENUM;
    }
    return GL_INVALID_ENUM;
}

/**
 * Returns the number of columns of this attribute
 *
 * This is value is 1 for any non-matrix type.
 *
 * @param type  The GLSL type
 *
 * @return the number of columns of this attribute
 */
Uint32 cugl::graphics::glsl_attribute_columns(GLSLType type) {
    switch (type) {
        case GLSLType::MAT2:
            return 2;
        case GLSLType::MAT3:
            return 3;
        case GLSLType::MAT4:
            return 4;
        default:
            return 1;
    }
}

/**
 * Returns the number of components of the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the number of components of the given GLSL type
 */
GLint cugl::graphics::glsl_attribute_size(GLSLType type) {
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
GLint cugl::graphics::glsl_attribute_stride(GLSLType type) {
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
 * Returns the OpenGL equivalent of a DrawMode
 *
 * @param mode  The DrawMode
 *
 * @return the OpenGL equivalent of a DrawMode
 */
GLenum cugl::graphics::gl_draw_mode(DrawMode mode) {
    switch (mode) {
        case DrawMode::UNDEFINED:
            return GL_INVALID_ENUM;
        case DrawMode::POINTS:
            return GL_POINTS;
        case DrawMode::LINES:
            return GL_LINES;
        case DrawMode::LINE_STRIP:
            return GL_LINE_STRIP;
        case DrawMode::TRIANGLES:
            return GL_TRIANGLES;
        case DrawMode::TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case DrawMode::TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
    }
    return GL_INVALID_ENUM;
}

/**
 * Returns the OpenGL equivalent of a CompareOp
 *
 * @param op    The CompareOp
 *
 * @return the OpenGL equivalent of a CompareOp
 */
GLenum cugl::graphics::gl_compare_op(CompareOp op) {
    switch (op) {
        case CompareOp::NEVER:
            return GL_NEVER;
        case CompareOp::EQUAL:
            return GL_EQUAL;
        case CompareOp::NOT_EQUAL:
            return GL_NOTEQUAL;
        case CompareOp::LESS:
            return GL_LESS;
        case CompareOp::LESS_OR_EQUAL:
            return GL_LEQUAL;
        case CompareOp::GREATER:
            return GL_GREATER;
        case CompareOp::GREATER_OR_EQUAL:
            return GL_GEQUAL;
        case CompareOp::ALWAYS:
            return GL_ALWAYS;
    }
    return GL_ALWAYS;
}

/**
 * Returns the OpenGL equivalent of a StencilOp
 *
 * @param op    The StencilOp
 *
 * @return the OpenGL equivalent of a StencilOp
 */
GLenum cugl::graphics::gl_stencil_op(StencilOp op) {
    switch (op) {
        case StencilOp::KEEP:
            return GL_KEEP;
        case StencilOp::REPLACE:
            return GL_REPLACE;
        case StencilOp::INVERT:
            return GL_INVERT;
        case StencilOp::INCR_WRAP:
            return GL_INCR_WRAP;
        case StencilOp::INCR_CLAMP:
            return GL_INCR;
        case StencilOp::DECR_WRAP:
            return GL_DECR_WRAP;
       case StencilOp::DECR_CLAMP:
            return GL_DECR;
        case StencilOp::ZERO:
            return GL_ZERO;
    }
    return GL_ZERO;
}

/**
 * Returns the OpenGL equivalent of a BlendFactor
 *
 * @param func  The BlendFactor
 *
 * @return the OpenGL equivalent of a BlendFactor
 */
GLenum cugl::graphics::gl_blend_func(BlendFactor func) {
    switch(func) {
        case BlendFactor::ZERO:
            return GL_ZERO;
        case BlendFactor::ONE:
            return GL_ONE;
        case BlendFactor::SRC_COLOR:
            return GL_SRC_COLOR;
        case BlendFactor::ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DST_COLOR:
            return GL_DST_COLOR;
        case BlendFactor::ONE_MINUS_DST_COLOR:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SRC_ALPHA:
            return GL_SRC_ALPHA;
        case BlendFactor::ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DST_ALPHA:
            return GL_DST_ALPHA;
        case BlendFactor::ONE_MINUS_DST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::CONSTANT_COLOR:
            return GL_CONSTANT_COLOR;
        case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::CONSTANT_ALPHA:
            return GL_CONSTANT_ALPHA;
        case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SRC_ALPHA_SATURATE:
            return GL_SRC_ALPHA_SATURATE;
    }
    return GL_ONE;
}

/**
 * Returns the OpenGL equivalent of a BlendEq
 *
 * @param eq    The BlendEq
 *
 * @return the OpenGL equivalent of a BlendEq
 */
GLenum cugl::graphics::gl_blend_eq(BlendEq eq) {
    switch(eq) {
        case BlendEq::ADD:
            return GL_FUNC_ADD;
        case BlendEq::SUBTRACT:
            return GL_FUNC_SUBTRACT;
        case BlendEq::REVERSE_SUBTRACT:
            return GL_FUNC_REVERSE_SUBTRACT;
        case BlendEq::MIN:
            return GL_MIN;
        case BlendEq::MAX:
            return GL_MAX;
    }
    return GL_FUNC_ADD;
}

/**
 * Returns the OpenGL equivalent of a TextureFilter
 *
 * @param filter    The TextureFilter
 *
 * @return the OpenGL equivalent of a TextureFilter
 */
GLenum cugl::graphics::gl_filter(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::NEAREST:
            return GL_NEAREST;
        case TextureFilter::LINEAR:
            return GL_LINEAR;
        case TextureFilter::NEAREST_MIPMAP_NEAREST:
            return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilter::NEAREST_MIPMAP_LINEAR:
            return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilter::LINEAR_MIPMAP_NEAREST:
            return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilter::LINEAR_MIPMAP_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
    }
    
    return GL_LINEAR;
}

/**
 * Returns the OpenGL equivalent of a TextureWrap
 *
 * @param wrap    The TextureWrap
 *
 * @return the OpenGL equivalent of a TextureWrap
 */
GLenum cugl::graphics::gl_wrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::CLAMP:
            return GL_CLAMP_TO_EDGE;
        case TextureWrap::REPEAT:
            return GL_REPEAT;
        case TextureWrap::MIRROR:
            return GL_MIRRORED_REPEAT;
        case TextureWrap::BORDER:
#if CU_GRAPHICS_API == CU_OPENGLES
            return GL_CLAMP_TO_EDGE;
#else
            return GL_CLAMP_TO_BORDER;
#endif
    }
    return GL_CLAMP_TO_EDGE;
}


/**
 * Returns the symbolic OpenGL format for the texel format
 *
 * Symbolic formats are limited to those listed in glTexImage2D.
 *
 * @param format    The explicit texel format
 *
 * @return the symbolic OpenGL format for the pixel format
 */
GLint cugl::graphics::gl_symbolic_format(TexelFormat format) {
    switch (format) {
        case TexelFormat::COLOR_RGBA:
        case TexelFormat::COLOR_RGBA4:
            return GL_RGBA;
        case TexelFormat::COLOR_BGRA:
        case TexelFormat::COLOR_BGRA4:
#if CU_GRAPHICS_API == CU_OPENGLES
            CULogError("BGR order is not supported in OpenGLES");
            return GL_RGBA;
#else
            return GL_BGRA;
#endif
        case TexelFormat::COLOR_RGB:
        case TexelFormat::COLOR_RGB4:
            return GL_RGB;
        case TexelFormat::COLOR_RG:
        case TexelFormat::COLOR_RG4:
            return GL_RG;
        case TexelFormat::COLOR_RED:
            return GL_RED;
        case TexelFormat::DEPTH:
            return GL_DEPTH_COMPONENT;
        case TexelFormat::STENCIL:
        case TexelFormat::DEPTH_STENCIL:
            return GL_DEPTH_STENCIL;
        default:
            CUAssertLog(false,"Unsupported texel format");
            break;
    }
    
    return GL_RGBA;
}

/**
 * Returns the internal OpenGL format for the texel format
 *
 * We have standardized internal formats for all platforms. This may not
 * be memory efficient, but it works for 90% of all use cases.
 *
 * @param format    The explicit texel format
 *
 * @return the internal OpenGL format for the pixel format
 */
GLint cugl::graphics::gl_internal_format(TexelFormat format) {
    switch (format) {
        case TexelFormat::COLOR_RGBA:
        case TexelFormat::COLOR_BGRA:
            return GL_RGBA8;
        case TexelFormat::COLOR_RGBA4:
        case TexelFormat::COLOR_BGRA4:
            return GL_RGBA4;
        case TexelFormat::COLOR_RGB:
            return GL_RGB8;
        case TexelFormat::COLOR_RGB4:
#if CU_GRAPHICS_API == CU_OPENGLES
            CULogError("4-byte three-color is not supported in OpenGLES");
            return GL_RGB;
#else
            return GL_RGB4;
#endif
        case TexelFormat::COLOR_RG:
            return GL_RG;
        case TexelFormat::COLOR_RG4:
            CULogError("4-byte two-color is not supported in OpenGL");
            return GL_RG;
        case TexelFormat::COLOR_RED:
            return GL_RED;
        case TexelFormat::DEPTH:
            return GL_DEPTH_COMPONENT;
        case TexelFormat::STENCIL:
        case TexelFormat::DEPTH_STENCIL:
#if CU_GRAPHICS_API == CU_OPENGLES
            return GL_DEPTH24_STENCIL8;
#else
            return GL_DEPTH_STENCIL;
#endif
        default:
            CUAssertLog(false,"Unsupported texel format");
            break;
    }
    
    return GL_RGBA8;
}

/**
 * Returns the data type for the pixel format
 *
 * The data type is derived from the internal format. We have standardized
 * internal formats for all platforms. This may not be memory efficient, but
 * it works for 90% of all use cases.
 *
 * @param format    The explicit pixel format
 *
 * @return the data type for the pixel format
 */
GLenum cugl::graphics::gl_format_type(TexelFormat format) {
    switch (format) {
        case TexelFormat::COLOR_RGBA:
        case TexelFormat::COLOR_BGRA:
        case TexelFormat::COLOR_RGB:
        case TexelFormat::COLOR_RG:
        case TexelFormat::COLOR_RG4:
        case TexelFormat::COLOR_RED:
            return GL_UNSIGNED_BYTE;
        case TexelFormat::COLOR_RGBA4:
        case TexelFormat::COLOR_BGRA4:
        case TexelFormat::COLOR_RGB4:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case TexelFormat::DEPTH:
            return GL_FLOAT;
        case TexelFormat::STENCIL:
        case TexelFormat::DEPTH_STENCIL:
            return GL_UNSIGNED_INT_24_8;
        default:
            CUAssertLog(false,"Unsupported texel format");
            break;
    }
    
    return GL_UNSIGNED_BYTE;
}

/**
 * Returns the byte size for the given texel format
 *
 * @param format  The Texel format
 *
 * @return the byte size for the given texel format
 */
Uint32 cugl::graphics::gl_format_stride(TexelFormat format) {
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


#pragma mark -
#pragma mark Debugging Functions
/**
 * Returns a string description of an OpenGL error type
 *
 * @param type  The OpenGL error type
 *
 * @return a string description of an OpenGL error type
 */
std::string cugl::graphics::gl_error_name(GLenum type) {
    std::string error = "UNKNOWN";
    
    switch(type) {
        case 0:
            error="NO ERROR";
            break;
        case GL_INVALID_OPERATION:
            error="INVALID_OPERATION";
            break;
        case GL_INVALID_ENUM:
            error="INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error="INVALID_VALUE";
            break;
        case GL_OUT_OF_MEMORY:
            error="OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error="INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            error="FRAMEBUFFER_UNDEFINED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            error="FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            error="FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
#if (CU_GRAPHICS_API == CU_OPENGL)
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            error="FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            error="FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
#endif
        case GL_FRAMEBUFFER_UNSUPPORTED:
            error="FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            error="FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
#if (CU_GRAPHICS_API == CU_OPENGL)
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            error="FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            break;
#endif
    }
    return error;
}

/**
 * Returns a string description of an OpenGL data type
 *
 * @param type  The OpenGL error type
 *
 * @return a string description of an OpenGL data type
 */
std::string cugl::graphics::gl_type_name(GLenum type) {
    switch(type) {
        case GL_FLOAT:
            return "GL_FLOAT";
        case GL_FLOAT_VEC2:
            return "GL_FLOAT";
        case GL_FLOAT_VEC3:
            return "GL_FLOAT_VEC2";
        case GL_FLOAT_VEC4:
            return "GL_FLOAT_VEC4";
        case GL_FLOAT_MAT2:
            return "GL_FLOAT_MAT2";
        case GL_FLOAT_MAT3:
            return "GL_FLOAT_MAT3";
        case GL_FLOAT_MAT4:
            return "GL_FLOAT_MAT4";
        case GL_FLOAT_MAT2x3:
            return "GL_FLOAT_MAT2x3";
        case GL_FLOAT_MAT2x4:
            return "GL_FLOAT_MAT2x4";
        case GL_FLOAT_MAT3x2:
            return "GL_FLOAT_MAT3x2";
        case GL_FLOAT_MAT3x4:
            return "GL_FLOAT_MAT3x4";
        case GL_FLOAT_MAT4x2:
            return "GL_FLOAT_MAT4x2";
        case GL_FLOAT_MAT4x3:
            return "GL_FLOAT_MAT4x3";
        case GL_INT:
            return "GL_INT";
        case GL_INT_VEC2:
            return "GL_INT_VEC2";
        case GL_INT_VEC3:
            return "GL_INT_VEC3";
        case GL_INT_VEC4:
            return "GL_INT_VEC4";
        case GL_UNSIGNED_INT:
            return "GL_UNSIGNED_INT";
        case GL_UNSIGNED_INT_VEC2:
            return "GL_UNSIGNED_INT_VEC2";
        case GL_UNSIGNED_INT_VEC3:
            return "GL_UNSIGNED_INT_VEC3";
        case GL_UNSIGNED_INT_VEC4:
            return "GL_UNSIGNED_INT_VEC4";
#if (CU_GRAPHICS_API == CU_OPENGL)
        case GL_DOUBLE:
            return "GL_GL_DOUBLE";
        case GL_DOUBLE_VEC2:
            return "GL_DOUBLE_VEC2";
        case GL_DOUBLE_VEC3:
            return "GL_DOUBLE_VEC3";
        case GL_DOUBLE_VEC4:
            return "GL_DOUBLE_VEC4";
        case GL_DOUBLE_MAT2:
            return "GL_DOUBLE_MAT2";
        case GL_DOUBLE_MAT3:
            return "GL_DOUBLE_MAT3";
        case GL_DOUBLE_MAT4:
            return "GL_DOUBLE_MAT4";
        case GL_DOUBLE_MAT2x3:
            return "GL_DOUBLE_MAT2x3";
        case GL_DOUBLE_MAT2x4:
            return "GL_DOUBLE_MAT2x4";
        case GL_DOUBLE_MAT3x2:
            return "GL_DOUBLE_MAT3x2";
        case GL_DOUBLE_MAT3x4:
            return "GL_DOUBLE_MAT3x4";
        case GL_DOUBLE_MAT4x2:
            return "GL_DOUBLE_MAT4x2";
        case GL_DOUBLE_MAT4x3:
            return "GL_DOUBLE_MAT4x3";
#endif
        case GL_SAMPLER_2D:
            return "GL_SAMPLER_2D";
        case GL_SAMPLER_3D:
            return "GL_SAMPLER_3D";
        case GL_SAMPLER_CUBE:
            return "GL_SAMPLER_CUBE";
        case GL_SAMPLER_2D_SHADOW:
            return "GL_SAMPLER_2D_SHADOW";
        case GL_UNIFORM_BUFFER:
            return "GL_UNIFORM_BUFFER";
    }
    return "GL_UNKNOWN";
}


