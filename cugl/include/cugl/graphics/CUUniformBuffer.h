//
//  CUUniformBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a uniform buffer for sending batched uniforms to a
//  shader (graphics or compute). Uniforms buffers are necessary for any
//  uniforms that are too large to be expressed as push constants (in Vulkan
//  most platforms only allow 196 bytes of push constants -- enough for three
//  matrices).
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
#ifndef __CU_UNIFORM_BUFFER_H__
#define __CU_UNIFORM_BUFFER_H__
#include <string>
#include <vector>
#include <unordered_map>
#include <cugl/core/math/cu_math.h>
#include <cugl/graphics/CUGraphicsBase.h>

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
class BufferData;
    
/**
 * This class defines a uniform buffer for a {@link GraphicsShader}.
 *
 * Technically, a shader is associated with a uniform block, not a uniform
 * buffer, since a uniform buffer may have multiple blocks. In the case of a
 * uniform buffer with multiple blocks, the current block is managed through
 * the shader (either {@link GraphicsShader} or {@link ComputeShader}).
 *
 * Uniform buffers are required for Vulkan. However, CUGL uses them extensively
 * in OpenGL as well. They ideal in two use cases. First of all, they are great
 * for uniforms that are shared across multiple shaders. But it is also
 * worthwhile to have a buffer for a single shader if (1) that shader has a
 * large number of uniforms and (2) those uniforms change semi-frequently
 * through out a render pass.  In that case, the uniform buffer should be
 * allocated with enough blocks so that all of the possible uniform values can
 * be assigned at the start of the render pass, each to a different block.
 * Once the shader starts to receive vertices, the uniforms should be managed
 * via the {@link GraphicsShader#setBlock} method.
 *
 * Note that we do not support {@link BufferAccess} types in uniform buffers.
 * That is because is makes very little sense to allow compute access to a
 * uniform buffer. Uniform buffers use std140, but any output buffer to a
 * compute shader must use std430. This is conversion is complicated enough
 * that we elected not to support it in CUGL.
 */
class UniformBuffer {
public:
    /** The byte position of an invalid offset */
    const static size_t INVALID_OFFSET;

private:
    /** The name of this uniform buffer */
    std::string _name;
    /** The graphics API implementation of this buffer */
    BufferData* _data;
    /** A mapping of struct names to their std140 offsets */
    std::unordered_map<std::string, size_t> _offsets;
    /** The number of blocks assigned to the uniform buffer. */
    Uint32 _blockcount;
    /** The capacity of a single block in the uniform buffer. */
    size_t _blocksize;
    /** The alignment stride of a single block. */
    size_t _blockstride;
    /** Whether this buffer supports streaming */
    bool _stream;
    /** Flag to prevent the update of a non-streaming buffer */
    bool _filled;
    /** Whether this buffer needs to be flushed */
    bool _dirty;

public:
#pragma mark Constructors
    /**
     * Creates an uninitialized uniform buffer.
     *
     * You must initialize the uniform buffer to allocate memory.
     */
    UniformBuffer();
    
    /**
     * Deletes this uniform buffer, disposing all resources.
     */
    ~UniformBuffer() { dispose(); }
    
    /**
     * Deletes the uniform buffer, freeing all resources.
     *
     * You must reinitialize the uniform buffer to use it.
     */
    void dispose();
    
    /**
     * Initializes this uniform buffer to support a block of the given capacity.
     *
     * This uniform buffer will only support a single block. The block capacity
     * is measured in bytes. In std140 format, all scalars are 4 bytes, vectors
     * are 8 or 16 bytes, and matrices are treated as an array of 8 or 16 byte
     * column vectors.
     *
     * Typically uniform buffers supports streaming, meaning that their values
     * updated every frame or so. If stream is false, then this buffer can only
     * be set once (typically at the start of the frame), and later calls to
     * either {@link #flush} or {@link #loadData} will fail.
     *
     * @param capacity  The block capacity in bytes
     * @param stream    Whether data can be streamed to the buffer
     *
     * @return true if initialization was successful.
     */
    bool init(size_t capacity, bool stream=true) {
        return init(capacity,1,stream);
    }
    
    /**
     * Initializes this uniform buffer to support multiple blocks of the given capacity.
     *
     * The block capacity is measured in bytes. In std140 format, all scalars
     * are 4 bytes, vectors are 8 or 16 bytes, and matrices are treated as an
     * array of 8 or 16 byte column vectors.
     *
     * Keep in mind that uniform buffer blocks must be aligned, and so this may
     * take significantly more memory than the number of blocks times the
     * capacity. If the graphics card cannot support that many blocks, this
     * method will return false.
     *
     * Typically uniform buffers supports streaming, meaning that their values
     * updated every frame or so. If stream is false, then this buffer can only
     * be set once (typically at the start of the frame), and later calls to
     * either {@link #flush} or {@link #loadData} will fail.
     *
     * @param capacity  The block capacity in bytes
     * @param blocks    The number of blocks to support
     * @param stream    Whether data can be streamed to the buffer
     *
     * @return true if initialization was successful.
     */
    bool init(size_t capacity, Uint32 blocks, bool stream=true);
    
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new uniform block to support a block of the given capacity.
     *
     * This uniform buffer will only support a single block. The block capacity
     * is measured in bytes. In std140 format, all scalars are 4 bytes, vectors
     * are 8 or 16 bytes, and matrices are treated as an array of 8 or 16 byte
     * column vectors.
     *
     * Typically uniform buffers supports streaming, meaning that their values
     * updated every frame or so. If stream is false, then this buffer can only
     * be set once (typically at the start of the frame), and later calls to
     * either {@link #flush} or {@link #loadData} will fail.
     *
     * @param capacity  The block capacity in bytes
     * @param stream    Whether data can be streamed to the buffer
     *
     * @return a new uniform block to support a block of the given capacity.
     */
    static std::shared_ptr<UniformBuffer> alloc(size_t capacity, bool stream=true) {
        std::shared_ptr<UniformBuffer> result = std::make_shared<UniformBuffer>();
        return (result->init(capacity,stream) ? result : nullptr);
    }
    
    /**
     * Returns a new uniform buffer to support multiple blocks of the given capacity.
     *
     * The block capacity is measured in bytes. In std140 format, all scalars
     * are 4 bytes, vectors are 8 or 16 bytes, and matrices are treated as an
     * array of 8 or 16 byte column vectors.
     *
     * Keep in mind that uniform buffer blocks must be aligned, and so this may
     * take significantly more memory than the number of blocks times the
     * capacity. If the graphics card cannot support that many blocks, this
     * method will return false.
     *
     * Typically uniform buffers supports streaming, meaning that their values
     * updated every frame or so. If stream is false, then this buffer can only
     * be set once (typically at the start of the frame), and later calls to
     * either {@link #flush} or {@link #loadData} will fail.
     *
     * @param capacity  The block capacity in bytes
     * @param blocks    The number of blocks to support
     * @param stream    Whether data can be streamed to the buffer
     *
     * @return a new uniform buffer to support multiple blocks of the given capacity.
     */
    static std::shared_ptr<UniformBuffer> alloc(size_t capacity, Uint32 blocks, bool stream=true) {
        std::shared_ptr<UniformBuffer> result = std::make_shared<UniformBuffer>();
        return (result->init(capacity,blocks,stream) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the platform-specific implementation for this buffer
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this buffer
     */
    BufferData* getImplementation() { return _data; }

    /**
     * Sets the name of this uniform buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is typically the
     * appropriate shader variable name, but this is not necessary for it to
     * function properly.
     *
     * @param name  The name of this uniform buffer.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this uniform buffer.
     *
     * A name is a user-defined way of identifying a buffer.  It is typically
     * the appropriate shader variable name, but this is not necessary for it
     * to function properly.
     *
     * @return the name of this uniform buffer.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns true if this buffer supports streaming.
     *
     * A non-streaming buffer will only support one call to {@link #loadData}
     * and no calls to {@link #flush}. The assumption is that the contents of
     * the buffer are loaded once and remain unchanged for the duration of the
     * buffer.
     *
     * @return true if this buffer supports streaming.
     */
    bool isStream() const { return _stream; }
    
    /**
     * Returns the number of blocks supported by this buffer.
     *
     * A uniform buffer can support multiple uniform blocks at once.
     *
     * @return the number of blocks supported by this buffer.
     */
    Uint32 getBlockCount() const { return _blockcount; }
    
    /**
     * Returns the capacity of a single block in this uniform buffer.
     *
     * The block size is the amount of data necessary to populate the
     * uniforms for a single block. It is measured in bytes.
     *
     * @return the capacity of a single block in this uniform buffer.
     */
    size_t getBlockSize() const { return _blocksize; }
    
    /**
     * Returns the stride of a single block in this uniform buffer.
     *
     * The stride measures the alignment (in bytes) of a block. It is at
     * least as large as the block capacity, but may be more.
     *
     * @return the stride of a single block in this uniform buffer.
     */
    size_t getBlockStride() const { return _blockstride; }
        
#pragma mark -
#pragma mark Data Offsets
    /**
     * Defines the byte offset of the given buffer variable.
     *
     * It is not necessary to call this method to use the uniform buffer. It is
     * always possible to pass data to the uniform block by specifying the byte
     * offset.The shader uses byte offsets to pull data from the uniform buffer
     * and assign it to the appropriate struct variable.
     *
     * However, this method makes use of the uniform buffer easier to follow. It
     * explicitly assigns a variable name to a byte offset. This variable name
     * can now be used in place of the byte offset with passing data to this
     * uniform block. It is akin to declaring uniforms and attributes before
     * compiling a {@link GraphicsShader}.
     *
     * This method can be called any time, even while the buffer is in use.
     * It has no affect on the underlying data.
     *
     * @param name      The variable name to use for this offset
     * @param offset    The buffer offset in bytes
     */
    void setOffset(const std::string name, size_t offset);
    
    /**
     * Returns the byte offset for the given name.
     *
     * This method requires that name be previously associated with an offset
     * via {@link #setOffset}. If it has not been associated with an offset,
     * then this method will return {@link #INVALID_OFFSET} instead.
     *
     * @param name      The variable name to query for an offset
     *
     * @return the byte offset of the given struct variable.
     */
    size_t getOffset(const std::string name) const;
    
    /**
     * Returns the offsets defined for this buffer
     *
     * The vector returned will include the name of every variable set by
     * the method {@link #setOffset}.
     *
     * @return the offsets defined for this buffer
     */
    std::vector<std::string> getOffsets() const;
 
#pragma mark -
#pragma mark Bulk Memory Access
    /**
     * Loads the given data in to this buffer
     *
     * This will load memory into the entire buffer, potentially across multiple
     * blocks. It will call {@link #flush} when done, committing the changes
     * to the graphics card. If there is more than one block, this method is
     * the *only* method that can be called for a non-streaming uniform buffer.
     * And in that case, it can only be called once.
     *
     * Note that if this buffer spans more than one block, each block must
     * align with {@link #getBlockSize}.
     *
     * @param data      The data to load
     * @param amt       The data size in bytes
     *
     * @return the number of bytes loaded
     */
    size_t loadData(const std::byte* data, size_t amt);
    
    /**
     * Loads the given data in to this buffer
     *
     * This will load the data structure into the uniform buffer. It will call
     * {@link #flush} when done, committing the changes to the graphics card.
     *
     * This method is designed for uniform buffers with a single block. It
     * allows the user to specify the block data a structured type. Due to
     * alignment issues, it does not make sense to use this method on a uniform
     * buffer with multiple blocks. Calling this method in that case will fail.
     *
     * @param data      The data to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadData(const T& data) {
        size_t size = sizeof(T);
        CUAssertLog(_blockcount == 1, "Uniform buffer has more than one block");
        CUAssertLog(size == _blocksize, "Data type does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&data);
        return loadData(bytes, size) == size;
    }
    
    /**
     * Loads the given data into the specified block
     *
     * The value amt must be less than or equal to the size of block. Calling
     * this method will update the CPU memory, but not update it on the GPU.
     * You must call {@link #flush} to synchronize the buffer.
     *
     * If there is more than one block, this method will fail on any uniform
     * buffer that is not streaming. If there is only one block, this method
     * can only be called once on non-streaming buffers.
     *
     * @param block     The block to update
     * @param data      The data to load
     * @param amt       The data size in bytes
     *
     * @return the number of bytes loaded
     */
    size_t loadBlock(Uint32 block, const std::byte* data, size_t amt);

    /**
     * Loads the given data into the specified block
     *
     * The type T should be the natural data sizee of the block. Calling
     * this method will update the CPU memory, but not update it on the GPU.
     * You must call {@link #flush} to synchronize the buffer.
     *
     * If there is more than one block, this method will fail on any uniform
     * buffer that is not streaming. If there is only one block, this method
     * can only be called once on non-streaming buffers.
     *
     * @param block     The block to update
     * @param data      The data to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadBlock(Uint32 block, const T& data) {
        size_t size = sizeof(T);
        CUAssertLog(size == _blocksize, "Data type does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&data);
        return loadBlock(block, bytes, size) == size;
    }

    /**
     * Flushes any changes to the graphics card.
     *
     * For any method other than {@link #loadData}, changes to this uniform
     * buffer are only applied to a backing buffer in CPU memory, and are not
     * reflected on the graphics card. This method synchronizes these two
     * memory regions.
     */
    void flush();

#pragma mark -
#pragma mark Push Methods Access
    /**
     * Sets the given uniform variable to a integer value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL int
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param value The value for the uniform
     */
    void pushInt(Sint64 block, const std::string name, Sint32 value);

    /**
     * Sets the given block offset to a integer value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL int
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param value     The value for the uniform
     */
    void pushInt(Sint64 block, size_t offset, Sint32 value);
    
    /**
     * Sets the given uniform variable to an unsigned integer value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uint
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param value The value for the uniform
     */
    void pushUInt(Sint64 block, const std::string name, Uint32 value);
    
    /**
     * Sets the given block offset to an unsigned integer value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uint
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param value     The value for the uniform
     */
    void pushUInt(Sint64 block, size_t offset, Uint32 value);

    /**
     * Sets the given uniform variable to a float value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL float
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param value The value for the uniform
     */
    void pushFloat(Sint64 block, const std::string name, float value);

    /**
     * Sets the given block offset to a float value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL float
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param value     The value for the uniform
     */
    void pushFloat(Sint64 block, size_t offset, float value);

    /**
     * Sets the given uniform variable to a Vector2 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec2(Sint64 block, const std::string name, const Vec2& vec);
    
    /**
     * Sets the given block offset to a Vector2 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param vec       The value for the uniform
     */
    void pushVec2(Sint64 block, size_t offset, const Vec2& vec);

    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     */
    void pushVec2(Sint64 block, const std::string name, float x, float y);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     */
    void pushVec2(Sint64 block, size_t offset, float x, float y);
    
    /**
     * Sets the given uniform variable to a Vector3 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec3(Sint64 block, const std::string name, const Vec3& vec);

    /**
     * Sets the given uniform variable to a Vector3 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param vec       The value for the uniform
     */
    void pushVec3(Sint64 block, size_t offset, const Vec3& vec);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     */
    void pushVec3(Sint64 block, const std::string name, float x, float y, float z);

    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     */
    void pushVec3(Sint64 block, size_t offset, float x, float y, float z);

    /**
     * Sets the given uniform variable to a Vector4 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec4(Sint64 block, const std::string name, const Vec4& vec);
    
    /**
     * Sets the given block offset to a Vector4 value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param vec       The value for the uniform
     */
    void pushVec4(Sint64 block, size_t offset, const Vec4& vec);

    /**
     * Sets the given uniform variable to a quaternion.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param quat  The value for the uniform
     */
    void pushVec4(Sint64 block, const std::string name, const Quaternion& quat);
    
    /**
     * Sets the given block offset to a quaternion.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param quat      The value for the uniform
     */
    void pushVec4(Sint64 block, size_t offset, const Quaternion& quat);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     * @param w     The z-value for the vector
     */
    void pushVec4(Sint64 block, const std::string name, float x, float y, float z, float w);
    
    /**
     * Sets the given uniform to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL vec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     * @param w         The z-value for the vector
     */
    void pushVec4(Sint64 block, size_t offset, float x, float y, float z, float w);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     */
    void pushIVec2(Sint64 block, const std::string name, Sint32 x, Sint32 y);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     */
    void pushIVec2(Sint64 block, size_t offset, Sint32 x, Sint32 y);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     */
    void pushIVec3(Sint64 block, const std::string name, Sint32 x, Sint32 y, Sint32 z);

    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     */
    void pushIVec3(Sint64 block, size_t offset, Sint32 x, Sint32 y, Sint32 z);

    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     * @param w     The z-value for the vector
     */
    void pushIVec4(Sint64 block, const std::string name, Sint32 x, Sint32 y, Sint32 z, Sint32 w);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ivec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     * @param w         The z-value for the vector
     */
    void pushIVec4(Sint64 block, size_t offset, Sint32 x, Sint32 y, Sint32 z, Sint32 w);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     */
    void pushUVec2(Sint64 block, const std::string name, Uint32 x, Uint32 y);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     */
    void pushUVec2(Sint64 block, size_t offset, Uint32 x, Uint32 y);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     */
    void pushUVec3(Sint64 block, const std::string name, Uint32 x, Uint32 y, Uint32 z);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     */
    void pushUVec3(Sint64 block, size_t offset, Uint32 x, Uint32 y, Uint32 z);
    
    /**
     * Sets the given uniform variable to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param x     The x-value for the vector
     * @param y     The x-value for the vector
     * @param z     The z-value for the vector
     * @param w     The z-value for the vector
     */
    void pushUVec4(Sint64 block, const std::string name, Uint32 x, Uint32 y, Uint32 z, Uint32 w);
    
    /**
     * Sets the given block offset to the given values
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL uvec4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param x         The x-value for the vector
     * @param y         The x-value for the vector
     * @param z         The z-value for the vector
     * @param w         The z-value for the vector
     */
    void pushUVec4(Sint64 block, size_t offset, Uint32 x, Uint32 y, Uint32 z, Uint32 w);
        
    /**
     * Sets the given uniform variable to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL color
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void pushColor4f(Sint64 block, const std::string name, const Color4f& color);
    
    /**
     * Sets the given block offset to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL color
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param color     The value for the uniform
     */
    void pushColor4f(Sint64 block, size_t offset, const Color4f& color);

    /**
     * Sets the given uniform variable to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL color
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param red   The uniform red value
     * @param green The uniform green value
     * @param blue  The uniform blue value
     * @param alpha The uniform alpha value
     */
    void pushColor4f(Sint64 block, const std::string name, float red, float green, float blue, float alpha);

    /**
     * Sets the given block offset to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL color
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param red       The uniform red value
     * @param green     The uniform green value
     * @param blue      The uniform blue value
     * @param alpha     The uniform alpha value
     */
    void pushColor4f(Sint64 block, size_t offset, float red, float green, float blue, float alpha);

    /**
     * Sets the given uniform variable to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ucolor
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void pushColor4(Sint64 block, const std::string name, const Color4 color);

    /**
     * Sets the given block offset to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ucolor
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param color     The value for the uniform
     */
    void pushColor4(Sint64 block, size_t offset, const Color4 color);

    /**
     * Sets the given uniform variable to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ucolor
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param red   The uniform red value
     * @param green The uniform green value
     * @param blue  The uniform blue value
     * @param alpha The uniform alpha value
     */
    void pushColor4(Sint64 block, const std::string name, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);
    
    /**
     * Sets the given block offset to a color value.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL ucolor
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param red       The uniform red value
     * @param green     The uniform green value
     * @param blue      The uniform blue value
     * @param alpha     The uniform alpha value
     */
    void pushColor4(Sint64 block, size_t offset, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);
    
    /**
     * Sets the given uniform variable to a 2x2 matrix.
     *
     * The 2x2 matrix will be the non-translational part of the affine transform.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param mat   The value for the uniform
     */
    void pushMat2(Sint64 block, const std::string name, const Affine2& mat);
    
    /**
     * Sets the given block offset to a 2x2 matrix.
     *
     * The 2x2 matrix will be the non-translational part of the affine transform.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param mat       The value for the uniform
     */
    void pushMat2(Sint64 block, size_t offset, const Affine2& mat);
    
    /**
     * Sets the given uniform variable to a 2x2 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param array The values for the uniform
     */
    void pushMat2(Sint64 block, const std::string name, const float* array);
    
    /**
     * Sets the given block offset to a 2x2 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat2
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param array     The values for the uniform
     */
    void pushMat2(Sint64 block, size_t offset, const float* array);
    
    /**
     * Sets the given uniform variable to a 3x3 matrix.
     *
     * The 3x3 matrix will be affine transform in homoegenous coordinates.

     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param mat   The value for the uniform
     */
    void pushMat3(Sint64 block, const std::string name, const Affine2& mat);
    
    /**
     * Sets the given block offset to a 3x3 matrix.
     *
     * The 3x3 matrix will be affine transform in homoegenous coordinates.

     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param mat       The value for the uniform
     */
    void pushMat3(Sint64 block, size_t offset, const Affine2& mat);
    
    /**
     * Sets the given uniform variable to a 3x3 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param array The values for the uniform
     */
    void pushMat3(Sint64 block, const std::string name, const float* array);
    
    /**
     * Sets the given block offset to a 3x3 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat3
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param array     The values for the uniform
     */
    void pushMat3(Sint64 block, size_t offset, const float* array);
    
    /**
     * Sets the given uniform variable to a 4x4 matrix
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param mat   The value for the uniform
     */
    void pushMat4(Sint64 block, const std::string name, const Mat4& mat);

    /**
     * Sets the given block offset to a 4x4 matrix
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param mat       The value for the uniform
     */
    void pushMat4(Sint64 block, size_t offset, const Mat4& mat);
    
    /**
     * Sets the given uniform variable to a 4x4 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block The block in this uniform buffer to access
     * @param name  The name of the uniform
     * @param array The values for the uniform
     */
    void pushMat4(Sint64 block, const std::string name, const float* array);

    /**
     * Sets the given block offset to a 4x4 matrix.
     *
     * The matrix will be read from the array in column major order (not row
     * major).
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * Unlike shaders, uniform buffers do not check for correctness of their
     * variables. Assigning this value to a variable that is not a GLSL mat4
     * can corrupt the memory.
     *
     * This method will fail if the buffer is not streaming.
     *
     * @param block     The block in this uniform buffer to access
     * @param offset    The offset within the block
     * @param array     The values for the uniform
     */
    void pushMat4(Sint64 block, size_t offset, const float* array);
    
    /**
     * Pushes the given data to the specified block at the given offset
     *
     * This method is intended to stream line the push methods, but is not
     * really intended for general us. The value amt must be less than or equal
     * to the size of block minus the offet.
     *
     * If block is -1, it sets this value in every block in this uniform buffer.
     * Values set by this method will not be sent to the graphics card until a
     * call to {@link #flush}.
     *
     * This method will fail on any uniform buffer that is not streaming.
     *
     * @param block     The block to update
     * @param offset    The block offset in bytes
     * @param data      The data to load
     * @param amt       The data size in bytes
     */
    void pushBytes(Sint64 block, size_t offset, const Uint8* data, size_t amt);
};

    }
}

#endif /* __CU_UNIFORM_BUFFER_H__ */
