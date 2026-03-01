//
//  CUGLOpaques.h
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
#ifndef __CU_GL_OPAQUES_H__
#define __CU_GL_OPAQUES_H__
#include <cugl/core/math/CUColor4.h>
#include <cugl/core/math/CUIVec2.h>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUShaderSource.h>
#include <cugl/graphics/CUNativeGraphics.h>

namespace cugl {

// Forward declaration
class JsonValue;

    /**
     * The classes and functions needed to construct a graphics pipeline.
     *
     * Initially these were part of the core CUGL module. However, in the
     * transition to Vulkan, we discovered that we need to factor this out.
     * This allows us to provide options for OpenGL, Vulkan, and even headless
     * clients.
     */
    namespace graphics {

// Forward references
class Sampler;
class Image;
class ImageArray;
class Texture;
class TextureArray;
class VertexBuffer;
class IndexBuffer;
class UniformBuffer;
class StorageBuffer;

// TODO: I am undecided if we need to encapsulate these better.
#pragma mark Opaque Types
/**
 * A struct containing the data necessary for representing an image.
 */
class ImageData {
public:
    /** A reference to the allocated image in OpenGL; 0 is not allocated. */
    GLuint image;

    /**
     * Initializes an empty image data object
     *
     * You should allocate the buffer to initialize this struct.
     */
    ImageData() : image(0) {}
    
    /**
     * Destroys this image data
     */
    ~ImageData() { dispose(); }

    /**
     * Initializes this image with the given data and format
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
    bool init(const void *data, int width, int height, bool mipmaps, TexelFormat format);
    
    /**
     * Deallocates the image object
     *
     * You must call {@link #init} again to use the object
     */
    void dispose();
    
    /**
     * Returns a unique identifier for this data
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this data
     */
    size_t id() const { return (size_t)image; }
};

/**
 * A struct containing the data necessary for representing an image array.
 */
class ImageArrayData {
public:
    /** A reference to the allocated array in OpenGL; 0 is not allocated. */
    GLuint images;
    /** The number of layers in this image array */
    Uint32 layers;

    /**
     * Initializes an empty image array data object
     *
     * You should allocate the buffer to initialize this struct.
     */
    ImageArrayData() : images(0), layers(0) {}
    
    /**
     * Destroys this image array data
     */
    ~ImageArrayData() { dispose(); }

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
    bool init(const void *data, int width, int height, int layers, bool mipmaps, TexelFormat format);
    
    /**
     * Deallocates the image object
     *
     * You must call {@link #init} again to use the object
     */
    void dispose();
    
    /**
     * Returns a unique identifier for this data
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this data
     */
    size_t id() const { return (size_t)images; }
};


/**
 * A struct containing the data necessary for representing a sampler.
 */
class SamplerData {
public:
    // TODO: We may not need this
    /** The image buffer associated with this sampler; 0 if there is no buffer */
    //GLuint image;
    /** The minimization algorithm */
    GLenum minFilter;
    /** The maximization algorithm */
    GLenum magFilter;
    /** The wrap-style for the horizontal texture coordinate */
    GLenum wrapS;
    /** The wrap-style for the vertical texture coordinate */
    GLenum wrapT;
    
    /**
     * Initializes an empty sampler data object
     *
     * You should set the image buffer to initialize this struct.
     */
    SamplerData();
    
    /**
     * Returns a unique identifier for this data
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this data
     */
    size_t id() const { return (size_t)this; }
    
    /**
     * Applies the sampler to the currently bound texture.
     *
     * Note that this method does not rebind to the image attribute. So if
     * the current texture is a different texture, this will push the sample
     * attributes to that texture.
     */
    void apply();
};

/**
 * A struct containing the data necessary for representing a data buffer.
 *
 * This struct can be used for any buffer type: vertex, index, or uniform.
 * An optional backing store is provided for things that need direct memory
 * access like UniformBuffers.
 */
class BufferData {
public:
    /** The access policy for this buffer */
    BufferAccess access;
    /** The underlying buffer data; 0 if not allocated */
    GLuint buffer;
    /** An in-memory backing store */
    std::byte* store;
    /** The buffer size (in bytes) */
    size_t size;
    /** The number of blocks in this buffer */
    size_t blocks;
    /** Whether the buffer is ready to receive data */
    bool ready;
    /** The start of the dirty region */
    size_t start;
    /** The end of the dirty region (= start if not dirty) */
    size_t end;
    
    /**
     * Initializes an empty buffer object
     *
     * You should call {@link #init} to initialize this struct.
     */
    BufferData();
    
    /** Deleted this buffer object */
    ~BufferData() { dispose(); }
    
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
    bool init(size_t size, BufferAccess access);
    
    /**
     * Initializes a mapped buffer with the given size and access
     *
     * This buffer will have a backing store and the access will by dynamic.
     *
     * @param size      The buffer size
     *
     * @return true if the buffer was created successfully
     */
    bool initMapped(size_t size);
    
    /**
     * Deallocates the buffer object
     *
     * You must call {@link #init} again to use the object
     */
    void dispose();
    
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
    bool loadData(const void* data, size_t size);

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
    bool setData(const std::byte* data, size_t offset, size_t size);
    
    /**
     * Loads the backing store into the buffer.
     *
     * This method does nothing if the buffer is not dynamic or there is no
     * dirty region.
     *
     * @return true if the buffer was updated
     */
    bool flush();

    /**
     * Returns a unique identifier for this data
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this data
     */
    size_t id() const { return (size_t)buffer; }
};
    
#pragma mark -
#pragma mark FrameBuffer
/**
 * A struct representing a renderpass
 *
 * In OpenGL, this is primarily used for validation, as well as clearing the
 * attachments.
 */
class RenderPassData {
public:
    /** The formats for the color attachments */
    std::vector<TexelFormat>* colorFormats;
    /** The format for the depth/stencil attachment (UNDEFINED for none) */
    TexelFormat* depthStencilFormat;
    /** The clear colors for this render pass */
    std::vector<Color4f> clearColors;
    /** The depth clear value */
    float  clearDepth;
    /** The stencil clear value */
    Uint32 clearStencil;
    
    /**
     * Creates a basic RenderPass
     */
    RenderPassData() :
    clearDepth(1),
    clearStencil(0),
    colorFormats(nullptr),
    depthStencilFormat(nullptr) {
    }
};

/**
 * A struct containing the data necessary for representing a frame buffer
 *
 * This can represent either the primary frame buffer or an offscreen buffer.
 */
class FrameBufferData {
public:
    /** The framebuffer associated with this render target */
    GLuint framebo;
    /** The backing renderbuffer for the framebuffer */
    GLuint renderbo;
    /** Whether this object owns its framebuffer/renderbuffer */
    bool owns;
    /** The number of output textures for this render target */
    size_t outsize;
    /** The combined depth and stencil buffer (can be null) */
    std::shared_ptr<Image>  depthst;
    /** The array of output textures (can be empty on main framebuffer) */
    std::vector<std::shared_ptr<Image>> outputs;
    /** The bind points for linking up the shader output variables */
    std::vector<GLuint> bindpoints;
    /** The framebuffer offset (for large screen behavior */
    IVec2 offset;
    
    /**
     * Creates an uninitialized frame buffer object.
     *
     * Call one of the init methods to initialize the framebuffer.
     */
    FrameBufferData() : framebo(0), renderbo(0), owns(false), outsize(0) {
    }
    
    /**
     * Deletes a frame buffer object.
     */
    ~FrameBufferData() { dispose(); }
    
    /**
     * Initializes a default frame buffer object.
     *
     * The frame buffer will be configured for the display
     *
     * @return true if initialization is successful
     */
    bool init();
    
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
    bool init(Uint32 width, Uint32 height, const RenderPassData& renderpass);
    
    /**
     * Deallocated any memory associated with this frame buffer
     *
     * The frame buffer cannot be used until an init method is called.
     */
    void dispose();
    
    /**
     * Activates the given framebuffer with the given viewport
     *
     * @param x         The x-coordinate of the origin
     * @param y         The y-coordinate of the origin
     * @param width     The viewport width
     * @param height    The viewport height
     */
    void activate(int x, int y, int width, int height);
    
    /**
     * Clears the contents of this framebuffer.
     *
     * @param renderpass    The renderpass defining the clear state
     */
    void clear(const RenderPassData* renderpass);
    
private:
    // Offscreen helpers
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
    bool prepareBuffer(Uint32 width, Uint32 height, const RenderPassData& renderpass);
    
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
    bool attachTexture(GLuint index, Uint32 width, Uint32 height, TexelFormat format);
    
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
    bool completeBuffer();
};
    
#pragma mark -
#pragma mark Graphics Shader
/** The shade scissor has changed */
#define SHADER_DIRTY_SCISSOR           0x001
/** The shader drawmode has changed */
#define SHADER_DIRTY_DRAWMODE          0x002
/** The shader_front face has changed */
#define SHADER_DIRTY_FRONTFACE         0x004
/** The shader cull mode has changed */
#define SHADER_DIRTY_CULLMODE          0x008
/** The shader stencil has changed */
#define SHADER_DIRTY_STENCIL           0x010
/** The shader blend mode has changed */
#define SHADER_DIRTY_BLEND             0x020
/** The shader depth settings have changed */
#define SHADER_DIRTY_DEPTH             0x040
/** All values have changed */
#define SHADER_DIRTY_ALL_VALS          0x08F
    
/**
 * A struct refering to the graphics internals of an OpenGL graphics shader
 */
class GraphicsShaderData {
public:
    /** The OpenGL program for this shader */
    GLuint program;
    /** The OpenGL vertex shader for this shader */
    GLuint vertShader;
    /** The OpenGL fragment shader for this shader */
    GLuint fragShader;
    /** The associated vertex array */
    GLuint vertArray;
    /** Whether the shader is linked */
    bool linked;
    
    /** The AABB scissor */
    int scissor[4];

    
    // Cached data for shader state
    /** The GL draw command */
    GLenum command;
    /** The pipeline drawing mode (triangles, lines, points) */
    DrawMode  drawMode;
    /** The front face orientation for triangles */
    FrontFace frontFace;
    /** Whether or not to activate triangle face culling */
    CullMode  cullMode;
    /** The depth comparison function for depth culling */
    CompareOp depthFunc;
    /** The stencil operations for front-facing triangles */
    StencilState stencilFront;
    /** The stencil operations for back-facing triangles */
    StencilState stencilBack;
    /** The color blend settings (alpha, additive, etc.) */
    BlendState colorBlend;
    /** Whether to enable color blending */
    bool enableBlend;
    /** Whether to enable the scissor operations */
    bool scissorTest;
    /** Whether to enable the stencil operations */
    bool stencilTest;
    /** Whether to enable depth testing */
    bool depthTest;
    /** Whether to enable depth writing */
    bool depthWrite;
    /** The color mask settings to suppress stencil drawing */
    Uint8 colorMask;
    
    /** The which attributes are dirty (update before draw) */
    Uint32 dirtyState;
    /** The shader input is dirty (update before draw) */
    bool dirtyInput;
    /** The shader pass is dirty (update before draw) */
    bool dirtyPass;

    /**
     * Creates an uninitialized graphics shader
     *
     * You should call {@link #init} to initialize the shader
     */
    GraphicsShaderData();
    
    /**
     * Deletes this graphics shader
     */
    ~GraphicsShaderData() { dispose(); }

    /**
     * Initializes shader from the given sources.
     *
     * This sets up and validates the shader, but it does not link it.
     *
     * @param vsource   The source for the vertex shader.
     * @param fsource   The source for the fragment shader.
     *
     * @return true if initialization was successful.
     */
    bool init(const std::string& vsource, const std::string& fsource);
    
    /**
     * Deallocates any memory assigned to this graphics shader
     */
    void dispose();

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
    bool link(size_t arrays);
    
    /**
     * Flushes all state to OpenGL.
     *
     * This will push any dirty state to OpenGL.
     */
    void flush();
    
    
    
private:
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
    static bool validateShader(GLuint shader, const std::string type);
    
    /**
     * Displays the shader compilation errors to the log.
     *
     * If there were no errors, this method will do nothing.
     *
     * @param shader    The shader to test
     */
    static void logShaderError(GLuint shader);
    
    /**
     * Displays the program linker errors to the log.
     *
     * If there were no errors, this method will do nothing.
     *
     * @param program    The program to test
     */
    static void logProgramError(GLuint program);
    
};
    
#pragma mark -
#pragma mark Resources

/**
 * A struct refering to a shader resource
 *
 * A resource is anything that can be assigned to a descriptor set in Vulkan,
 * like at uniform buffer, a texture, a texture array, and so on. We need this
 * struct and its subclasses in OpenGL to guarantee typing.
 */
class ResourceData {
public:
    /** The resource type */
    ResourceType type;
    /** Whether this resource has recently been updated */
    bool dirty;
    
    /**
     * Initializes a degenerate resource data
     */
    ResourceData() : dirty(false) {
        type = ResourceType::MONO_BUFFER;
    }
};

/**
 * A struct refering to a texture (image + sampler) object.
 *
 * This struct wraps the texture and allows us to extract the appropriate
 * Vulkan internals each frame.
 */
class ResourceTexture : public ResourceData {
public:
    /** The CUGL texture data */
    std::shared_ptr<Texture> source;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The shader variable location */
    Uint32 location;

    /**
     * Initializes an empty texture wrapper
     *
     * You should add to the texture vector to initialize this struct.
     */
    ResourceTexture() : ResourceData(), bindpoint(0) {
        type = ResourceType::TEXTURE;
    }
};

/**
 * A struct refering to a texture array (image array + sampler) object.
 *
 * This struct wraps the texture array and allows us to extract the appropriate
 * Vulkan internals each frame.
 */
class ResourceTextureArray : public ResourceData {
public:
    /** The CUGL texture array data */
    std::shared_ptr<TextureArray> sources;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The shader variable location */
    Uint32 location;

    /**
     * Initializes an empty texture array wrapper
     *
     * You should add to the texture array vector to initialize this struct.
     */
    ResourceTextureArray() : ResourceData(), bindpoint(0) {
        type = ResourceType::TEXTURE_ARRAY;
    }
};

/**
 * A struct refering to an image object.
 *
 * This is not used by OpenGL. This exists solely for simulation purposes.
 */
class ResourceImage : public ResourceData {
public:
    /** The CUGL image data */
    std::shared_ptr<Image> source;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The shader variable location */
    Uint32 location;

    /**
     * Initializes an empty image wrapper
     *
     * You should add to the image vector to initialize this struct.
     */
    ResourceImage() : ResourceData(), bindpoint(0) {
        type = ResourceType::IMAGE;
    }
};

/**
 * A struct refering to an image array object.
 *
 * This is not used by OpenGL. This exists solely for simulation purposes.
 */
class ResourceImageArray : public ResourceData {
public:
    /** The CUGL image array data */
    std::shared_ptr<ImageArray> sources;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The shader variable location */
    Uint32 location;

    /**
     * Initializes an empty image array wrapper
     *
     * You should add to the image array vector to initialize this struct.
     */
    ResourceImageArray() : ResourceData(), bindpoint(0) {
        type = ResourceType::IMAGE_ARRAY;
    }
};

/**
 * A struct refering to a sampler object.
 *
 * This is not used by OpenGL. This exists solely for simulation purposes.
 */
class ResourceSampler : public ResourceData {
public:
    /** The CUGL sampler data */
    std::shared_ptr<Sampler> source;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The shader variable location */
    Uint32 location;

    /**
     * Initializes an empty sampler wrapper
     *
     * You should add to the sampler vector to initialize this struct.
     */
    ResourceSampler() : ResourceData() {
        type = ResourceType::SAMPLER;
    }
};

/**
 * A struct refering to a uniform buffer
 *
 * This struct wraps the buffer and allows us to extract the appropriate
 * Vulkan internals each frame.
 */
class ResourceUniform : public ResourceData {
public:
    /** The CUGL uniform buffers */
    std::shared_ptr<UniformBuffer> source;
    /** The OpenGL bindpoint */
    Uint32 bindpoint;
    /** The current buffer block */
    Uint32 block;
    /** The uniform buffer stride (per block) */
    size_t stride;
    /** Whether the block has been updated */
    bool dirtyBlock;
    
    /**
     * Initializes an empty buffer wrapper
     *
     * You should add to the source vector to initialize this struct.
     */
    ResourceUniform() : ResourceData(), block(0), stride(0), dirtyBlock(false) {
        type = ResourceType::MONO_BUFFER;
    }

};

/**
 * A struct refering to a storage buffer
 *
 * This is not used by OpenGL. This exists solely for simulation purposes.
 */
class ResourceStorage : public ResourceData {
public:
    /** The CUGL storage buffers */
    std::shared_ptr<StorageBuffer> sources;
    /** The current buffer block */
    Uint32 block;
    /** The storage buffer stride (per block) */
    size_t stride;
    /** Whether the block has been updated */
    bool dirtyBlock;
    
    /**
     * Initializes an empty buffer wrapper
     *
     * You should add to the source vector to initialize this struct.
     */
    ResourceStorage() : ResourceData(), block(0), stride(0), dirtyBlock(false) {
        type = ResourceType::MONO_STORAGE;
    }
};

/**
 * A struct refering to a vertex buffer
 *
 * This struct wraps the buffer and allows us to extract the appropriate
 * Vulkan internals each frame.
 */
class ResourceVertex : public ResourceData {
public:
    /** The CUGL vertex buffer */
    std::shared_ptr<VertexBuffer> source;
    
    /**
     * Initializes an empty buffer wrapper
     *
     * You should assign to the source variable to initialize this struct.
     */
    ResourceVertex() : ResourceData() {
        type = ResourceType::COMPUTE_VERTEX;
    }
};

/**
 * A struct refering to an index buffer
 *
 * This struct wraps the buffer and allows us to extract the appropriate
 * Vulkan internals each frame.
 */
class ResourceIndex : public ResourceData {
public:
    /** The CUGL index buffer */
    std::shared_ptr<IndexBuffer> source;
    
    /**
     * Initializes an empty buffer wrapper
     *
     * You should assign to the source variable to initialize this struct.
     */
    ResourceIndex() : ResourceData() {
        type = ResourceType::COMPUTE_INDEX;
    }
};
    
#pragma mark -
#pragma mark Conversion Functions
/**
 * Returns the OpenGL type for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the OpenGL type for the given GLSL type
 */
GLenum glsl_attribute_type(GLSLType type);

/**
 * Returns the number of columns of this attribute
 *
 * This is value is 1 for any non-matrix type.
 *
 * @param type  The GLSL type
 *
 * @return the number of columns of this attribute
 */
Uint32 glsl_attribute_columns(GLSLType type);

/**
 * Returns the number of components of the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the number of components of the given GLSL type
 */
GLint glsl_attribute_size(GLSLType type);
    
/**
 * Returns the data stride for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the data stride for the given GLSL type
 */
GLint glsl_attribute_stride(GLSLType type);

/**
 * Returns the OpenGL equivalent of a DrawMode
 *
 * @param mode  The DrawMode
 *
 * @return the OpenGL equivalent of a DrawMode
 */
GLenum gl_draw_mode(DrawMode mode);

/**
 * Returns the OpenGL equivalent of a CompareOp
 *
 * @param op    The CompareOp
 *
 * @return the OpenGL equivalent of a CompareOp
 */
GLenum gl_compare_op(CompareOp op);

/**
 * Returns the OpenGL equivalent of a StencilOp
 *
 * @param op    The StencilOp
 *
 * @return the OpenGL equivalent of a StencilOp
 */
GLenum gl_stencil_op(StencilOp op);

/**
 * Returns the OpenGL equivalent of a BlendFactor
 *
 * @param func  The BlendFactor
 *
 * @return the OpenGL equivalent of a BlendFactor
 */
GLenum gl_blend_func(BlendFactor func);

/**
 * Returns the OpenGL equivalent of a BlendEq
 *
 * @param eq    The BlendEq
 *
 * @return the OpenGL equivalent of a BlendEq
 */
GLenum gl_blend_eq(BlendEq eq);
    
/**
 * Returns the OpenGL equivalent of a TextureFilter
 *
 * @param filter    The TextureFilter
 *
 * @return the OpenGL equivalent of a TextureFilter
 */
GLenum gl_filter(TextureFilter filter);

/**
 * Returns the OpenGL equivalent of a TextureWrap
 *
 * @param wrap    The TextureWrap
 *
 * @return the OpenGL equivalent of a TextureWrap
 */
GLenum gl_wrap(TextureWrap wrap);
        
/**
 * Returns the symbolic OpenGL format for the texel format
 *
 * Symbolic formats are limited to those listed in glTexImage2D.
 *
 * @param format    The explicit texel format
 *
 * @return the symbolic OpenGL format for the pixel format
 */
GLint gl_symbolic_format(TexelFormat format);

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
GLint gl_internal_format(TexelFormat format);

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
GLenum gl_format_type(TexelFormat format);

/**
 * Returns the byte size for the given texel format
 *
 * @param format  The Texel format
 *
 * @return the byte size for the given texel format
 */
Uint32 gl_format_stride(TexelFormat format);

    
#pragma mark -
#pragma mark Debugging Utilities
/**
 * Returns a string description of an OpenGL error type
 *
 * @param error The OpenGL error type
 *
 * @return a string description of an OpenGL error type
 */
std::string gl_error_name(GLenum error);

/**
 * Returns a string description of an OpenGL data type
 *
 * @param error The OpenGL error type
 *
 * @return a string description of an OpenGL data type
 */
std::string gl_type_name(GLenum error);
        
    }

#pragma mark -
#pragma mark Graphics Contexts
/**
 * This class represents a graphics context designed for OpenGL.
 *
 * This class is used by the Display to manage state.
 */
class GraphicsContext {
private:
    /** The associated OpenGL drawing context */
    SDL_GLContext _glContext;
    /** The framebuffer data for this context */
    graphics::FrameBufferData* _framebuffer;
    /** Whether to support multisampling */
    bool _multisample;
    /** Rotation orientation (-1 for -90°, 1 or 90°) */
    int _rotation;
    
public:
    /**
     * Creates an unitialized OpenGL graphics context.
     *
     * This object must be instantiated before the window is created. However,
     * it does not initialize the OpenGL values. That is done via a call to
     * {@lin #init} once the window is created.
     *
     * @param multisample   Whether to support multisampling.
     */
    GraphicsContext(bool multisample);

    /**
     * Disposed the OpenGL graphics context.
     */
    ~GraphicsContext();

    /**
     * Initializes the OpenGL context
     *
     * This method must be called after the Window is created.
     *
     * @param window    The associated SDL window
     *
     * @return true if initialization was successful
     */
    bool init(SDL_Window* window);
    
    /**
     * Restores the default frame/render buffer.
     *
     * This is necessary when you are using a {@link FrameBuffer} and want
     * to restore control the frame buffer. It is necessary because 0 is
     * NOT necessarily the correct id of the default framebuffer (particularly
     * on iOS).
     */
    graphics::FrameBufferData* getFrameBuffer() { return _framebuffer; }
    
    /**
     * Sets the rotation value for this context
     *
     * A value of 0 is unrotated.  -1 is a rotation of -90°, while 1 is a
     * rotation of 90°. Anything else is invauld
     *
     * @param rot   The rotation value
     */
    void setRotation(int rot) { _rotation = rot; }
    
    /**
     * Returns the rotation value for this context
     *
     * A value of 0 is unrotated.  -1 is a rotation of -90°, while 1 is a
     * rotation of 90°. Anything else is invauld
     *
     * @return the rotation value for this context
     */
    int getRotation() const { return _rotation; }
    
    /**
     * Returns true if this context supports multisampling
     *
     * @return true if this context supports multisampling
     */
    static bool supportsMultisample();
    
};

}
#endif /* __CU_GL_OPAQUES_H__ */
