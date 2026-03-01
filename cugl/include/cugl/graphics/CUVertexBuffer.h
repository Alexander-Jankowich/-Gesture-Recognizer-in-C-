//
//  CUVertexBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a buffer for providing vertex data to a shader. In
//  previous versions of CUGL we combined this with the index buffer,
//  representing the data as as single unit. However, this did not play well
//  with either instancing or Vulkan shaders, so we factored out this
//  functionality.
//
//  Note that a vertex buffer does not do any drawing by itself. It has to be
//  assigned to a {@link GraphicsShader}, which issues the drawing commands.
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
#ifndef __CU_VERTEX_BUFFER_H__
#define __CU_VERTEX_BUFFER_H__
#include <string>
#include <unordered_map>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUMat4.h>
#include <cugl/core/util/CUDebug.h>

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
 * This class defines a vertex buffer for drawing with a shader.
 *
 * A vertex buffer is a collection of data meant to be passed to an attribute
 * binding in a {@link GraphicsShader}. Vertex buffers are only intended to
 * attribute data. To ensure maximum flexibility, index buffers are stored
 * separately in an {@link IndexBuffer} object.
 *
 * Vertex buffers with the {@link #COMPUTE} type may also be used as the output
 * of a {@link ComputeShader}. However, this functionality is only supported
 * in Vulkan. The OpenGL limitations of CUGL do not support compute shaders.
 *
 * Vertex buffer object do not store any information about their contents
 * other than their stride (which can be used for minimal validation checking
 * with a shader). They are simply containers.
 */
class VertexBuffer {
protected:
    /** The name of this vertex buffer */
    std::string _name;
    /** The graphics API implementation of this buffer */
    BufferData* _data;
    /** The maximum number of bytes supported */
    size_t _capacity;
    /** The data stride of this buffer (0 if there is only one attribute) */
    size_t _stride;
    /** The buffer access */
    BufferAccess _access;
    /** The number of bytes loaded into the vertex buffer */
    size_t _size;

public:
#pragma mark Constructors
    /**
     * Creates an uninitialized vertex buffer.
     *
     * You must initialize the vertex buffer to allocate buffer memory.
     */
    VertexBuffer();
    
    /**
     * Deletes this vertex buffer, disposing all resources.
     */
    ~VertexBuffer() { dispose(); }
    
    /**
     * Deletes the vertex buffer, freeing all resources.
     *
     * You must reinitialize the vertex buffer to use it.
     */
    void dispose();
    
    /**
     * Initializes this vertex buffer for the given size and access.
     *
     * The size is measured in bytes, not number of vertex elements.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * This method will assume a stride of 0, meaning that no validation with
     * the shader will be attempted.
     *
     * @param size      The maximum number of bytes in this buffer
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    bool init(size_t size, BufferAccess access=BufferAccess::DYNAMIC) {
    	return init(size,0,access);
    }

    /**
     * Initializes this vertex buffer for the given size, stride, and access.
     *
     * The size in this method measured in number of elements. The value stride
     * is used to indicate the size of a single element/vertex.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of elements in this buffer
     * @param stride    The vertex element stride
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    bool init(size_t size, size_t stride, BufferAccess access=BufferAccess::DYNAMIC);
    
    /**
     * Returns a new vertex buffer for the given size and access.
     *
     * The size is measured in bytes, not number of vertex elements.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * This method will assume a stride of 0, meaning that no validation with
     * the shader will be attempted.
     *
     * @param size      The maximum number of bytes in this buffer
     * @param access    The buffer access
     *
     * @return a new vertex buffer for the given size and type.
     */
    static std::shared_ptr<VertexBuffer> alloc(size_t size, BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<VertexBuffer> result = std::make_shared<VertexBuffer>();
        return (result->init(size,access) ? result : nullptr);
    }
    
    /**
     * Returns a new vertex buffer for the given size, stride, and type.
     *
     * The size in this method measured in number of elements. The value stride
     * is used to indicate the size of a single element/vertex.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of elements in this buffer
     * @param stride    The vertex element stride
     * @param access    The buffer access
     *
     * @return a new vertex buffer for the given size, stride, and type.
     */
    static std::shared_ptr<VertexBuffer> alloc(size_t size, size_t stride,
                                               BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<VertexBuffer> result = std::make_shared<VertexBuffer>();
        return (result->init(size,stride,access) ? result : nullptr);
    }
    
    
#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this vertex buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @param name  The name of this vertex buffer.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this vertex buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is used for
     * debugging purposes only, and has no affect on the graphics pipeline.
     *
     * @return the name of this vertex buffer.
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
     * Returns the access policy of this vertex buffer
     *
     * @return the access policy of this vertex buffer
     */
    BufferAccess getAccess() const { return _access; }

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
     * Returns the stride of this vertex buffer
     *
     * If this value is 0, shaders will not attempt to do any validation.
     * Otherwise, the shader will make sure that its stride matches that of the
     * the vertex buffer.
     *
     * @return the stride of this vertex buffer
     */
    size_t getStride() const { return _stride; }
    
    /**
     * Returns the amount of data currently stored in this buffer
     *
     * The value returned is the total number of bytes stored in this buffer
     * so far. If 0, that means that no data has been loaded.
     *
     * @return the amount of data currently stored in this buffer
     */
    size_t getSize() const { return _size; }
    
    /**
     * Returns the number of elements currently stored in this buffer
     *
     * The value returned is {@link #getSize} divided by the stride. If the
     * stride is 0, this just returns the number of bytes.
     *
     * @return the number of elements currently stored in this buffer
     */
    size_t getCount() const { return _stride > 0 ? _size/_stride : _size; }
 
#pragma mark -
#pragma mark Vertex Processing
    /**
     * Loads the given vertex buffer with the given data.
     *
     * Data that is loaded into this buffer can now be read by either a
     * {@link GraphicsShader} or a {@link ComputeShader}.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
     *
     * In this version of the method, size is the number of bytes in the data.
     *
     * @param data  The data to load
     * @param size  The number of bytes to load
     *
     * @return true if the data was successfully loaded
     */
    bool loadData(const std::byte* data, size_t size);
    
    /**
     * Loads the given vertex buffer with the given elements.
     *
     * Data that is loaded into this buffer can now be read by either a
     * {@link GraphicsShader} or a {@link ComputeShader}.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
     *
     * In this version of the method, size is the number of elements of type
     * T in the data array.
     *
     * @param data  The vertices to load
     * @param size  The number of vertices to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadData(const T* data, size_t size) {
        CUAssertLog(sizeof(T) == _stride, "Data type T does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data);
        return loadData(bytes,size*sizeof(T));
    }
    
    /**
     * Loads the given vertex buffer with the given elements.
     *
     * Data that is loaded into this buffer can now be read by either a
     * {@link GraphicsShader} or a {@link ComputeShader}.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
     *
     * @param data  The vertices to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadData(const std::vector<T>& data) {
        CUAssertLog(sizeof(T) == _stride, "Data type T does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data.data());
        return loadData(bytes,data.size()*sizeof(T));
    }

    /**
     * Reads from the buffer into the given data array.
     *
     * VULKAN ONLY: This method allows us to extract the elements of a vertex
     * buffer into any array on the CPU side. It is generally only useful for
     * buffers of type {@link BufferAccess#COMPUTE_READ} or
     * {@link BufferAccess#COMPUTE_ALL} so that we can inspect the results of
     * a compute shader.
     *
     * In this version of the method, size is the number of bytes to read.
     *
     * @param data  The array to receive the data
     * @param size  The number of bytes to read
     *
     * @return the number of bytes read
     */
    size_t readData(std::byte* data, size_t size);

    /**
     * Reads from the buffer into the given data array.
     *
     * VULKAN ONLY: This method allows us to extract the elements of a vertex
     * buffer into any array on the CPU side. It is generally only useful for
     * buffers of type {@link BufferAccess#COMPUTE_READ} or
     * {@link BufferAccess#COMPUTE_ALL} so that we can inspect the results of
     * a compute shader.
     *
     * In this version of the method, size is the number of elements to read.
     *
     * @param data  The array to receive the data
     * @param size  The number of elements to read
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readData(T* data, size_t size) {
        size_t stride = sizeof(T);
        CUAssertLog(stride == _stride, "Data type T does not have the correct size");
        std::byte* bytes = reinterpret_cast<std::byte*>(data);
        size_t amount = readData(bytes,size*stride);
        return amount/stride;
    }
    
    /**
     * Reads from the buffer into the given vector.
     *
     * VULKAN ONLY: This method allows us to extract the elements of a vertex
     * buffer into any array on the CPU side. It is generally only useful for
     * buffers of type {@link BufferAccess#COMPUTE_READ} or
     * {@link BufferAccess#COMPUTE_ALL} so that we can inspect the results of
     * a compute shader.
     *
     * Data read by this method is appended to the end of the vector. Elements
     * already in the vector are preserved.
     *
     * @param data  The vector to receive the data
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readData(std::vector<T>& data) {
        size_t stride = sizeof(T);
        CUAssertLog(stride == _stride, "Data type T does not have the correct size");

        size_t size = data.size();
        data.resize(size+_size*_stride);
        std::byte* bytes = reinterpret_cast<std::byte*>(data.data());
        bytes += size*stride;
        
        size_t amount = readData(bytes,_size*_stride);
        amount /= stride;
        if (amount < _size) {
            data.resize(size+amount);
        }
        return amount;
    }
    
};

    }
}

#endif /* __CU_VERTEX_BUFFER_H__ */
