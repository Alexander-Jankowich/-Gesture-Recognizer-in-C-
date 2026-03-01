//
//  CUStorageBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a storage buffer for use with Vulkan. Storage buffers
//  can be used as the input to a shader, or (in the case of compute shaders)
//  on output. They are not supported in OpenGL.
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
#ifndef __CU_STORAGE_BUFFER_H__
#define __CU_STORAGE_BUFFER_H__
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
 * This class is a Vulkan storage buffer
 *
 * Storage buffers are a slower form of uniform buffer that can support large
 * arrays of data. They are only supported in Vulkan. Attempts to initialize
 * an object of this class in OpenGL will fail.
 *
 * Like {@link UniformBuffer}, a storage buffer can be broken into multiple
 * blocks. However, this feature is less useful as storage buffers are
 * themselves arrays of data. Blocks are primarily useful with buffers that
 * have access {@link BufferAccess::STATIC}.
 *
 * Unlike a {@link UniformBuffer}, storage buffers use std430 formatting. This
 * requires less padding in the associated data structures.
 */
class StorageBuffer {
private:
    /** The name of this storage buffer */
    std::string _name;
    /** The graphics API implementation of this buffer */
    BufferData* _data;
    /** The number of blocks assigned to the storage buffer. */
    Uint32 _blockcount;
    /** The alignment stride of a single block. */
    size_t _blockstride;
    /** The byte size of a storage buffer block. */
    size_t _blocksize;
    /** The size of a single element in the storage buffer. */
    size_t _itemsize;
    /** The buffer access */
    BufferAccess _access;
    /** Flag to prevent the update of a non-streaming buffer */
    bool _filled;
    /** Whether this buffer need to be flushed */
    bool _dirty;

public:
#pragma mark Constructors
    /**
     * Creates an uninitialized storage buffer.
     *
     * You must initialize the storage buffer to allocate memory.
     */
    StorageBuffer();
    
    /**
     * Deletes this storage buffer, disposing all resources.
     */
    ~StorageBuffer() { dispose(); }
    
    /**
     * Deletes the storage buffer, freeing all resources.
     *
     * You must reinitialize the storage buffer to use it.
     */
    void dispose();
    
    /**
     * Initializes this storage buffer to support a block of the given capacity.
     *
     * This storage buffer will only support a single block. Unlike a uniform
     * buffer, this buffer is in std430. The element stride is computed from
     * the type T.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The number of elements in this buffer
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    template <typename T>
    bool init(size_t capacity, BufferAccess access=BufferAccess::COMPUTE_ALL) {
        return init<T>(capacity,1,access);
    }
    
    /**
     * Initializes this storage buffer to support multiple blocks of the given capacity.
     *
     * Unlike a uniform buffer, this buffer is in std430. The element stride is
     * computed from the type T. However the blocks must still be aligned, and
     * so this may take significantly more memory than the number of blocks
     * times the capacity. If the graphics card cannot support that many blocks,
     * this method will return false.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The maximum number of elements per block
     * @param blocks    The number of blocks to support
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    template <typename T>
    bool init(size_t capacity, Uint32 blocks, BufferAccess access=BufferAccess::COMPUTE_ALL) {
        return initWithStride(capacity, sizeof(T), blocks, access);
    }
    
    /**
     * Initializes this storage buffer with one block of the given capacity and stride.
     *
     * This storage buffer will only support a single block. Unlike a uniform
     * buffer, this buffer is in std430. The value stride is used to indicate
     * the size of a single element. The byte size is the capacity times the
     * stride.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The number of elements in the buffer
     * @param stride    The element stride
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    bool initWithStride(size_t capacity, size_t stride, BufferAccess access=BufferAccess::DYNAMIC) {
        return initWithStride(capacity,stride,1,access);
    }

    /**
     * Initializes this storage buffer with blocks of the given capacity and stride.
     *
     * The value stride is used to indicate the size of a single element. The
     * byte size is capacity times stride.
     *
     * Unlike a uniform buffer, this buffer is in std430. However the blocks
     * must still be aligned, and so this may take significantly more memory
     * than the number of blocks times the block size. If the graphics card
     * cannot support that many blocks, this method will return false.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The maximum number of elements per block
     * @param stride    The element stride
     * @param blocks    The number of blocks to support
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    bool initWithStride(size_t capacity, size_t stride, Uint32 blocks,
                        BufferAccess access=BufferAccess::DYNAMIC);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated storage buffer with a block of the given capacity.
     *
     * This storage buffer will only support a single block. Unlike a uniform
     * buffer, this buffer is in std430. The element stride is computed from
     * the type T.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The maximum elements of bytes per block
     * @param access    The buffer access
     *
     * @return a newly allocated storage buffer with a block of the given capacity.
     */
    template <typename T>
    static std::shared_ptr<StorageBuffer> alloc(size_t capacity, BufferAccess access=BufferAccess::COMPUTE_ALL) {
        std::shared_ptr<StorageBuffer> result = std::make_shared<StorageBuffer>();
        return (result->init<T>(capacity,access) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated storage buffer with multiple blocks of the given capacity.
     *
     * Unlike a uniform buffer, this buffer is in std430. The element stride is
     * computed from the type T. However the blocks must still be aligned, and
     * so this may take significantly more memory than the number of blocks
     * times the capacity. If the graphics card cannot support that many blocks,
     * this method will return false.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param capacity  The maximum number of elements per block
     * @param blocks    The number of blocks to support
     * @param access    The buffer access
     *
     * @return a newly allocated storage buffer with multiple blocks of the given capacity.
     */
    template <typename T>
    static std::shared_ptr<StorageBuffer> alloc(size_t capacity, Uint32 blocks,
                                                BufferAccess access=BufferAccess::COMPUTE_ALL) {
        std::shared_ptr<StorageBuffer> result = std::make_shared<StorageBuffer>();
        return (result->init<T>(capacity,blocks,access) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated storage buffer with a block of the given capacity and stride.
     *
     * This storage buffer will only support a single block. Unlike a uniform
     * buffer, this buffer is in std430. The value stride is used to indicate
     * the size of a single element. The byte size is the capacity times the
     * stride.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of elements in a block
     * @param stride    The element stride
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    static std::shared_ptr<StorageBuffer> allocWithStride(size_t size, size_t stride,
                                                          BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<StorageBuffer> result = std::make_shared<StorageBuffer>();
        return (result->initWithStride(size,stride,access) ? result : nullptr);
    }

    /**
     * Initializes this storage buffer with blocks of the given size and stride.
     *
     * The value stride is used to indicate the size of a single element. The
     * byte size is capacity times stride.
     *
     * Unlike a uniform buffer, this buffer is in std430. However the blocks
     * must still be aligned, and so this may take significantly more memory
     * than the number of blocks times the block size. If the graphics card
     * cannot support that many blocks, this method will return false.
     *
     * Note that some access types such as {@link BufferAccess#STATIC} or
     * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
     * to load data after the first time will fail.
     *
     * @param size      The maximum number of elements in a block
     * @param stride    The element stride
     * @param blocks    The number of blocks to support
     * @param access    The buffer access
     *
     * @return true if initialization was successful.
     */
    static std::shared_ptr<StorageBuffer> allocWithStride(size_t size, size_t stride, Uint32 blocks,
                                                          BufferAccess access=BufferAccess::DYNAMIC) {
        std::shared_ptr<StorageBuffer> result = std::make_shared<StorageBuffer>();
        return (result->initWithStride(size,stride,blocks,access) ? result : nullptr);
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
     * Returns the access policy of this storage buffer
     *
     * @return the access policy of this storage buffer
     */
    BufferAccess getAccess() const { return _access; }
    
    /**
     * Sets the name of this storage buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is typically the
     * appropriate shader variable name, but this is not necessary for it to
     * function properly.
     *
     * @param name  The name of this storage buffer.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this storage buffer.
     *
     * A name is a user-defined way of identifying a buffer.  It is typically
     * the appropriate shader variable name, but this is not necessary for it
     * to function properly.
     *
     * @return the name of this storage buffer.
     */
    const std::string getName() const { return _name; }
    
    /**
     * Returns the size of a single element in this storage buffer.
     *
     * A storage buffer is essentially an array of elements. All elements must
     * have the same size (in bytes), as specified by this method. Templated
     * methods such as {@link #loadData} should agree with this size.
     *
     * @return the size of a single element in this storage buffer.
     */
    size_t getItemSize() const { return _itemsize; }
    
    /**
     * Returns the capacity of a single block in this storage buffer.
     *
     * The capacity is number of elements that may be stored in a single
     * block. This value is not measured in bytes. To get the value in bytes,
     * mutliply this result by {@link #getItemSize}.
     *
     * @return the capacity of a single block in this storage buffer.
     */
    size_t getCapacity() const { return _blocksize/_itemsize; }

    /**
     * Returns the number of blocks supported by this buffer.
     *
     * A storage buffer can support multiple storage blocks at once.
     *
     * @return the number of blocks supported by this buffer.
     */
    Uint32 getBlockCount() const { return _blockcount; }
    
    /**
     * Returns the stride of a single block in this storage buffer.
     *
     * The stride measures the alignment (in bytes) of a block.  It is at
     * least as large as the block capacity, but may be more due to alignment
     * issues
     *
     * @return the stride of a single block in this storage buffer.
     */
    size_t getBlockStride() const { return _blockstride; }

    
#pragma mark -
#pragma mark Memory Access
    /**
     * Loads the given data in to this buffer
     *
     * This will load memory into the entire buffer, potentially across multiple
     * blocks. It will call {@link #flush} when done, committing the changes
     * to the graphics card.
     *
     * If this buffer spans more than one block, each block must align with
     * {@link #getBlockStride}. That may require padding on the underlying data.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
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
     * This will load the data structure into the storage buffer. It will call
     * {@link #flush} when done, committing the changes to the graphics card.
     *
     * This method is designed for storage buffers with a single block. Due to
     * alignment issues, it does not make sense to use this method on a storage
     * buffer with multiple blocks. Calling this method in that case will fail.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
     *
     * @param data  The element array to load
     * @param size  The number of elements to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadData(const T* data, size_t size) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data);
        return loadData(bytes,size*stride)/stride;
    }
    
    /**
     * Loads the given data in to this buffer
     *
     * This will load the data structure into the storage buffer. It will call
     * {@link #flush} when done, committing the changes to the graphics card.
     *
     * This method is designed for storage buffers with a single block. Due to
     * alignment issues, it does not make sense to use this method on a storage
     * buffer with multiple blocks. Calling this method in that case will fail.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * Note that this method may only be called once for buffers with access
     * {@link BufferAccess#STATIC}, {@link BufferAccess#COMPUTE_ONLY} or
     * {@link BufferAccess#COMPUTE_READ}. Additional attempts to load into
     * such buffers will fail.
     *
     * @param data  The element array to load
     *
     * @return true if the data was successfully loaded
     */
    template <typename T>
    bool loadData(const std::vector<T>& data) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data.data());
        return loadData(bytes,data.size()*stride)/stride;
    }
    
    /**
     * Loads the given data in to the specified block
     *
     * The value amt must be less than or equal to the size of block. Calling
     * this method will update the CPU memory, but not update it on the GPU.
     * You must call {@link #flush} to synchronize the buffer.
     *
     * If the storage buffer has more than one block, this method will fail on
     * any storage buffer that does not have access {@link BufferAccess#STATIC}
     * or {@link BufferAccess#COMPUTE_ALL}. In addition, for buffers without
     * this access, this method can only be called once.
     *
     * @param block     The block to update
     * @param data      The data to load
     * @param amt       The data size in bytes
     *
     * @return the number of bytes loaded
     */
    size_t loadBlock(Uint32 block, const std::byte* data, size_t amt);

    /**
     * Loads the given data in to the specified block
     *
     * The value amt must be less than or equal to the size of block. Calling
     * this method will update the CPU memory, but not update it on the GPU.
     * You must call {@link #flush} to synchronize the buffer.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * If the storage buffer has more than one block, this method will fail on
     * any storage buffer that does not have access {@link BufferAccess#STATIC}
     * or {@link BufferAccess#COMPUTE_ALL}. In addition, for buffers without
     * this access, this method can only be called once.
     *
     * @param block     The block to update
     * @param data      The vertices to load
     * @param size      The number of vertices to load
     *
     * @return the number of bytes loaded
     */
    template <typename T>
    size_t loadBlock(Uint32 block, const T* data, size_t size) {
        size_t stride = sizeof(T);
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data);
        return loadBlock(block, bytes,size*stride)/stride;
    }

    /**
     * Loads the given data in to the specified block
     *
     * The number of elements in data must be less than or equal to the size of
     * block. Calling this method will update the CPU memory, but not update it
     * on the GPU. You must call {@link #flush} to synchronize the buffer.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * If the storage buffer has more than one block, this method will fail on
     * any storage buffer that does not have access {@link BufferAccess#STATIC}
     * or {@link BufferAccess#COMPUTE_ALL}. In addition, for buffers without
     * this access, this method can only be called once.
     *
     * @param block     The block to update
     * @param data      The vertices to load
     *
     * @return the number of bytes loaded
     */
    template <typename T>
    size_t loadBlock(Uint32 block, const std::vector<T>& data) {
        size_t stride = sizeof(T);
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");
        const std::byte* bytes = reinterpret_cast<const std::byte*>(data.data());
        return loadBlock(block, bytes,data.size()*stride)/stride;
    }
    
    /**
     * Reads from the buffer into the given data array.
     *
     * This method allows us to extract the elements of a storage buffer into
     * any array on the CPU side. It is generally only useful for buffers of
     * type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * This version will read from all of the blocks in the buffer. If this
     * buffer spans more than one block, each block will align with
     * {@link #getBlockStride}.
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
     * Reads from the buffer into the given array.
     *
     * This method allows us to extract the elements of a storage buffer into
     * any array on the CPU side. It is generally only useful for buffers of
     * type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * This method is designed for storage buffers with a single block. Due to
     * alignment issues, it does not make sense to use this method on a storage
     * buffer with multiple blocks. Calling this method in that case will fail.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * @param data  The array to receive the data
     * @param size  The number of elements to read
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readData(T* data, size_t size) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");
        std::byte* bytes = reinterpret_cast<std::byte*>(data);
        size_t amount = readData(bytes,size*stride);
        return amount/stride;
    }
    
    /**
     * Reads from the buffer into the given array.
     *
     * This method allows us to extract the elements of a storage buffer into
     * any array on the CPU side. It is generally only useful for buffers of
     * type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * This method is designed for storage buffers with a single block. Due to
     * alignment issues, it does not make sense to use this method on a storage
     * buffer with multiple blocks. Calling this method in that case will fail.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * @param data  The vector to receive the data
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readData(std::vector<T>& data) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");

        size_t size  = data.size();
        size_t avail = _blocksize/_itemsize;
        data.resize(size+avail);
        std::byte* bytes = reinterpret_cast<std::byte*>(data.data());
        bytes += size*stride;
        
        size_t amount = readData(bytes,_blocksize);
        amount /= stride;
        if (amount < avail) {
            data.resize(size+amount);
        }
        return amount;
    }
    
    /**
     * Reads the specified block into the given array
     *
     * This method allows us to extract the elements of a storage buffer block
     * into any array on the CPU side. It is generally only useful for buffers
     * of type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * @param block The block to read from
     * @param data  The array to receive the data
     * @param amt   The number of bytes to read
     *
     * @return the number of bytes read
     */
    size_t readBlock(Uint32 block, std::byte* data, size_t amt);
    
    /**
     * Reads the specified block into the given array
     *
     * This method allows us to extract the elements of a storage buffer block
     * into any array on the CPU side. It is generally only useful for buffers
     * of type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * @param block The block to read from
     * @param data  The array to receive the data
     * @param amt   The number of elements to read
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readBlock(Uint32 block, T* data, size_t amt) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");
        std::byte* bytes = reinterpret_cast<std::byte*>(data);
        size_t amount = readBlock(block,bytes,amt*stride);
        return amount/stride;
    }
    
    /**
     * Reads the specified block into the given vector
     *
     * This method allows us to extract the elements of a storage buffer block
     * into any array on the CPU side. It is generally only useful for buffers
     * of type {@link BufferAccess#COMPUTE_READ} or {@link BufferAccess#COMPUTE_ALL}
     * so that we can inspect the results of a compute shader.
     *
     * The type T should have size {@link #getItemSize}. If not, the results of
     * this method are undefined.
     *
     * @param block The block to read from
     * @param data  The vector to receive the data
     *
     * @return the number of elements read
     */
    template <typename T>
    size_t readBlock(Uint32 block, std::vector<T>& data) {
        size_t stride = sizeof(T);
        CUAssertLog(_blockcount == 1, "Storage buffer has more than one block");
        CUAssertLog(stride == _itemsize, "Data type T does not have the correct size");

        size_t size  = data.size();
        size_t avail = _blocksize/_itemsize;
        data.resize(size+avail);
        std::byte* bytes = reinterpret_cast<std::byte*>(data.data());
        bytes += size*stride;
        
        size_t amount = readBlock(block,bytes,_blocksize);
        amount /= stride;
        if (amount < avail) {
            data.resize(size+amount);
        }
        return amount;
    }

    /**
     * Flushes any changes to the graphics card.
     *
     * For any method other than {@link #loadData}, changes to this storage
     * buffer are only applied to a backing buffer in CPU memory, and are not
     * reflected on the graphics card. This method synchronizes these two
     * memory regions.
     */
    void flush();   
};

    }
}

#endif /* __CU_STORAGE_BUFFER_H__ */

