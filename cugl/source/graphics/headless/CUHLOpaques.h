//
//  CUHLOpaques.h
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
#ifndef __CU_GL_OPAQUES_H__
#define __CU_GL_OPAQUES_H__
#include <cugl/core/math/CUColor4.h>
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
    /** The image width */
    Uint32 width;
    /** The image height */
    Uint32 height;
    /** The pixel size in bytes */
    Uint32 bytesize;
    /** The underlying byte data */
    std::byte* data;
    
    /**
     * Initializes an empty image data object
     *
     * You should allocate the buffer to initialize this struct.
     */
    ImageData() : width(0), height(0), bytesize(0), data(NULL) {}
    
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
     * @param format    The image data format
     *
     * @return true if initialization is successful
     */
    bool init(const void *data, int width, int height, TexelFormat format);
    
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
    size_t id() const { return (size_t)data; }
};

/**
 * A struct containing the data necessary for representing an image array.
 */
class ImageArrayData {
public:
    /** The image width */
    Uint32 width;
    /** The image height */
    Uint32 height;
    /** The pixel size in bytes */
    Uint32 bytesize;
    /** The underlying byte data */
    std::byte* data;
    /** The number of layers in this image array */
    Uint32 layers;
    
    /**
     * Initializes an empty image array data object
     *
     * You should allocate the buffer to initialize this struct.
     */
    ImageArrayData() : width(0), height(0), bytesize(0), layers(0), data(NULL) {}
    
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
     * @param format    The image data format
     *
     * @return true if initialization is successful
     */
    bool init(const void *data, int width, int height, int layers, TexelFormat format);
    
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
    size_t id() const { return (size_t)data; }
};


/**
 * A struct containing the data necessary for representing a sampler.
 */
class SamplerData {
public:
    /**
     * Initializes an empty sampler data object
     *
     * You should set the image buffer to initialize this struct.
     */
    SamplerData() {}
    
    /**
     * Returns a unique identifier for this data
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this data
     */
    size_t id() const { return (size_t)this; }
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
    /** The data buffer */
    std::byte* buffer;
    /** The buffer size (in bytes) */
    size_t size;
    /** The number of blocks in this buffer */
    size_t blocks;
    /** Whether the buffer is ready to receive data */
    bool ready;
    
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
     * Deallocates the buffer object
     *
     * You must call {@link #init} again to use the object
     */
    void dispose();
    
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
    bool loadData(const void* data, size_t size);
    
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
    bool setData(const std::byte* data, size_t offset, size_t size);
    
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
    RenderPassData() : clearDepth(1), clearStencil(0), colorFormats(nullptr), depthStencilFormat(nullptr) {
    }
};

/**
 * A struct containing the data necessary for representing a frame buffer
 *
 * This can represent either the primary frame buffer or an offscreen buffer.
 */
class FrameBufferData {
public:
    /** The number of output textures for this render target */
    size_t outsize;
    /** The combined depth and stencil buffer (can be null) */
    std::shared_ptr<Image>  depthst;
    /** The array of output images (can be empty on main framebuffer) */
    std::vector<std::shared_ptr<Image>> outputs;
    /** The array of output textures (can be empty on main framebuffer) */
    std::vector<std::shared_ptr<Texture>> wrapped;

    /**
     * Creates an uninitialized frame buffer object.
     *
     * Call one of the init methods to initialize the framebuffer.
     */
    FrameBufferData() : outsize(0) {
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
};

#pragma mark -
#pragma mark Shaders
/**
 * A struct refering to the graphics internals of a graphics shader
 */
class GraphicsShaderData {
public:
    /** Whether the shader is "compiled" */
    bool linked;
    /** The AABB scissor */
    int scissor[4];
    
    
    // Cached data for shader state
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
    /** Line width (only if wideLines supported and enabled) */
    float lineWidth;

    /**
     * Creates a graphics shader
     */
    GraphicsShaderData();
    
    /**
     * Deletes this graphics shader
     */
    ~GraphicsShaderData() {}

};
    
/**
 * A struct refering to the graphics internals of a compute shader
 */
class ComputeShaderData {
public:
    /**
     * Creates a compute shader
     */
    ComputeShaderData() {}
    
    /**
     * Deletes this graphics shader
     */
    ~ComputeShaderData() {}
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
    std::vector<std::shared_ptr<Texture>> sources;
    
    /**
     * Initializes an empty texture wrapper
     *
     * You should add to the texture vector to initialize this struct.
     */
    ResourceTexture() : ResourceData() {
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
    std::vector<std::shared_ptr<TextureArray>> sources;
    
    /**
     * Initializes an empty texture array wrapper
     *
     * You should add to the texture array vector to initialize this struct.
     */
    ResourceTextureArray() : ResourceData() {
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
    std::vector<std::shared_ptr<Image>> sources;

    /**
     * Initializes an empty image wrapper
     *
     * You should add to the image vector to initialize this struct.
     */
    ResourceImage() : ResourceData() {
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
    std::vector<std::shared_ptr<ImageArray>> sources;

    /**
     * Initializes an empty image array wrapper
     *
     * You should add to the image array vector to initialize this struct.
     */
    ResourceImageArray() : ResourceData() {
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
    std::vector<std::shared_ptr<Sampler>> sources;

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
    std::vector<std::shared_ptr<UniformBuffer>> sources;
    /** The current buffer block */
    Uint32 block;
    /** The uniform buffer stride (per block) */
    size_t stride;

    /**
     * Initializes an empty buffer wrapper
     *
     * You should add to the source vector to initialize this struct.
     */
    ResourceUniform() : ResourceData(), block(0), stride(0) {
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
    /** The CUGL uniform buffers */
    std::vector<std::shared_ptr<StorageBuffer>> sources;
    /** The current buffer block */
    Uint32 block;
    /** The uniform buffer stride (per block) */
    size_t stride;

    /**
     * Initializes an empty buffer wrapper
     *
     * You should add to the source vector to initialize this struct.
     */
    ResourceStorage() : ResourceData(), block(0), stride(0) {
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
 * Returns the number of components of the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the number of components of the given GLSL type
 */
Uint32 glsl_attribute_size(GLSLType type);
    
/**
 * Returns the data stride for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the data stride for the given GLSL type
 */
Uint32 glsl_attribute_stride(GLSLType type);

/**
 * Returns the byte size for the given texel format
 *
 * @param format  The Texel format
 *
 * @return the byte size for the given texel format
 */
Uint32 texel_stride(TexelFormat format);

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
    /** The framebuffer data for this context */
    graphics::FrameBufferData* _framebuffer;
    
public:
    /**
     * Creates an unitialized OpenGL graphics context.
     *
     * This object must be instantiated before the window is created. However,
     * it does not initialize the OpenGL values. That is done via a call to
     * {@link #init} once the window is created.
     */
    GraphicsContext();

    /**
     * Disposes the headless graphics context.
     */
    ~GraphicsContext();
    
    /**
     * Restores the default frame/render buffer.
     *
     * This is necessary when you are using a {@link FrameBuffer} and want
     * to restore control the frame buffer. It is necessary because 0 is
     * NOT necessarily the correct id of the default framebuffer (particularly
     * on iOS).
     */
    graphics::FrameBufferData* getFrameBuffer() { return _framebuffer; }
};
    
}
#endif /* __CU_GL_OPAQUES_H__ */
