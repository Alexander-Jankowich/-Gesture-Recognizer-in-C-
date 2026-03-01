//
//  CUIndexBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a buffer for providing index data to a shader. In
//  previous versions of CUGL we combined this with the vertex buffer,
//  representing the data as as single unit. However, this did not play well
//  with either instancing or Vulkan shaders, so we factored out this
//  functionality.
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
//  Version: 3/31/25 (Vulkan Integration)
//
#ifndef __CU_INDEX_BUFFER_H__
#define __CU_INDEX_BUFFER_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUMat4.h>
#include <cugl/core/util/CUDebug.h>
#include <string>
#include <vector>
#include <unordered_map>

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
 * This class defines an index buffer for drawing with a shader.
 *
 * A index buffer is a collection of integers refering to elements of a shader's
 * vertex buffer(s). Typically a {@link GraphicsShader} takes both a vertex
 * buffer and an index buffer, and previous versions of CUGL bundled them
 * together. These are separate objects to allow shaders to work with multiple
 * vertex buffers, one for each binding.
 *
 * Index buffers can either be 16-bit (shorts) or 32-bit. This choice is made
 * at creation time and cannot be changed. The variations of {@link #loadData}
 * will convert the data accordingly.
 */
class IndexBuffer {
protected:
    /** The name of this index buffer */
    std::string _name;
    /** The graphics API implementation of this buffer */
    BufferData* _data;
    /** The maximum number of indices supported */
    size_t _capacity;
    /** The buffer access */
    BufferAccess _access;
    /** Whether to use short indices */
    bool _short;
    /** The number of indices loaded into the buffer */
    size_t _size;

    /**
     * Allocates the initial buffers for a index buffer
     *
     * @param size  The index byte size
     */
    void setup(size_t size);
    
public:
#pragma mark Constructors
    /**
     * Creates an uninitialized index buffer.
     *
     * You must initialize the index buffer to allocate buffer memory.
     */
    IndexBuffer();
    
    /**
     * Deletes this index buffer, disposing all resources.
     */
    ~IndexBuffer() { dispose(); }
    
    /**
     * Deletes the index buffer, freeing all resources.
     *
     * You must reinitialize the index buffer to use it.
     */
    void dispose();
    
    /**
     * Initializes this index buffer to support 32-bit integers
     *
     * The size given is the maximum number of integers to store in this buffer,
     * not the number of bytes.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of integers in this buffer
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    bool init(size_t size, BufferAccess access=BufferAccess::DYNAMIC);

    /**
     * Initializes this index buffer to support 16-bit integers
     *
     * The size given is the maximum number of short integers to store in this
     * buffer, not the number of bytes.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of short integers in this buffer
     * @param access    The buffer access
    *
     * @return true if initialization was successful.
     */
    bool initShort(size_t size, BufferAccess access=BufferAccess::DYNAMIC);
    
    /**
     * Returns a new index buffer to support 32-bit integers
     *
     * The size given is the maximum number of integers to store in this buffer,
     * not the number of bytes.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of integers in this buffer
     * @param access    The buffer access
     *
     * @return a new index buffer to support 32-bit integers
     */
    static std::shared_ptr<IndexBuffer> alloc(size_t size, BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<IndexBuffer> result = std::make_shared<IndexBuffer>();
        return (result->init(size,access) ? result : nullptr);
    }
    
    /**
     * Returns a new index buffer to support 16-bit integers
     *
     * The size given is the maximum number of short integers to store in this
     * buffer, not the number of bytes.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of short integers in this buffer
     * @param access    The buffer access
     *
     * @return a new index buffer to support 16-bit integers
     */
    static std::shared_ptr<IndexBuffer> allocShort(size_t size, BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<IndexBuffer> result = std::make_shared<IndexBuffer>();
        return (result->initShort(size,access) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this index buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @param name  The name of this index buffer.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this index buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @return the name of this index buffer.
     */
    const std::string getName() const { return _name; }

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
     * Returns the maximum capacity of this buffer.
     *
     * The size determines the number of elements that can be loaded with the
     * method {@link #loadData}.
     *
     * @return the maximum capacity of this buffer.
     */
    size_t getCapacity() const { return _capacity; }

    /**
     * Returns the access policy of this index buffer
     *
     * @return the access policy of this index buffer
     */
    BufferAccess getAccess() const { return _access; }

    /**
     * Returns the number of indices currently stored in this buffer
     *
     * @return the number of indices currently stored in this buffer
     */
    size_t getSize() const { return _size; }
    
    /**
     * Returns true if this buffer uses short (16-bit) indices
     *
     * If the value is false, this buffer used 32-bit indices.
     *
     * @return true if this buffer uses short (16-bit) indices
     */
    bool isShort() const { return _short; }
    
#pragma mark -
#pragma mark Index Processing
    /**
     * Loads the given index buffer with the given data.
     *
     * If this is not a dynamic buffer, then this method can only be called
     * once. Additional attempts to load into a static buffer will fail.
     *
     * If this buffer only supports 16-bit indices, they will be converted
     * before loading into this buffer. Note that this conversion may lose
     * data.
     *
     * @param data  The data to load
     * @param size  The number of elements to load
     *
     * @return true if the data was successfully loaded
     */
    bool loadData(const Uint32* data, size_t size);
    
    /**
     * Loads the given index buffer with the given data.
     *
     * If this is not a dynamic buffer, then this method can only be called
     * once. Additional attempts to load into a static buffer will fail.
     *
     * If this buffer only supports 16-bit indices, they will be converted
     * before loading into this buffer. Note that this conversion may lose
     * data.
     *
     * @param data  The data to load
     *
     * @return true if the data was successfully loaded
     */
    bool loadData(const std::vector<Uint32>& data) {
        return loadData(data.data(),data.size());
    }
    
    /**
     * Loads the given index buffer with the given data.
     *
     * If this is not a dynamic buffer, then this method can only be called
     * once. Additional attempts to load into a static buffer will fail.
     *
     * If this buffer only supports 32-bit indices, they will be converted
     * before loading into this buffer.
     *
     * @param data  The data to load
     * @param size  The number of elements to load
     *
     * @return true if the data was successfully loaded
     */
    bool loadData(const Uint16* data, size_t size);
    
    /**
     * Loads the given index buffer with the given data.
     *
     * If this is not a dynamic buffer, then this method can only be called
     * once. Additional attempts to load into a static buffer will fail.
     *
     * If this buffer only supports 32-bit indices, they will be converted
     * before loading into this buffer.
     *
     * @param data  The data to load
     *
     * @return true if the data was successfully loaded
     */
    bool loadData(const std::vector<Uint16>& data) {
        return loadData(data.data(),data.size());
    }
    
};

    }
}

#endif /* __CU_INDEX_BUFFER_H__ */
