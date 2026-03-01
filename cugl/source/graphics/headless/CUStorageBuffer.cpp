//
//  CUStorageBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a storage buffer for use with Vulkan. Storage buffers
//  can be used as the input to a shader, or (in the case of compute shaders)
//  on output. They are not supported in OpenGL. This headless version stores
//  buffer data but does not communicate with the graphics card.
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
#include <cugl/graphics/CUStorageBuffer.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

#pragma mark Constructors
// We just allow them to be set-up, not used.
/**
 * Creates an uninitialized storage buffer.
 *
 * You must initialize the storage buffer to allocate memory.
 */
StorageBuffer::StorageBuffer() :
_blockcount(0),
_blockstride(0),
_blocksize(0),
_itemsize(0),
_data(nullptr),
_filled(false),
_dirty(false) {
    _access = BufferAccess::STATIC;
}

/**
 * Deletes the storage buffer, freeing all resources.
 *
 * You must reinitialize the storage buffer to use it.
 */
void StorageBuffer::dispose() {
    _itemsize = 0;
    _blocksize = 0;
    _blockstride = 0;
    _blockcount = 0;
    _filled = false;
    _dirty  = false;
    _access = BufferAccess::STATIC;
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
bool StorageBuffer::initWithStride(size_t capacity, size_t stride, Uint32 blocks, BufferAccess access) {
    _blocksize = capacity*stride;
    _blockstride = _blocksize;
    _blockcount = blocks;
    _itemsize = stride;
    _access = access;
    return true;
}


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
 * {@link #getBlockSize}. That may require padding on the underlying data.
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
size_t StorageBuffer::loadData(const std::byte* data, size_t amt) {
    CUAssertLog(false, "OpenGL does not support storage buffers");
    return 0;
}

/**
 * Loads the given data in to the specified block
 *
 * The value amt must be less than or equal to the size of block. Calling
 * this method will update the CPU memory, but not update it on the GPU.
 * You must call {@link #flush} to synchronize the buffer.
 *
 * If the storage buffer has more than one block, this method will fail on
 * any storage buffer that does not have access {@link BufferAccess#DYNAMIC}
 * or {@link BufferAccess#COMPUTE_ALL}. In addition, for buffers without
 * this access, this method can only be called once.
 *
 * @param block     The block to update
 * @param data      The data to load
 * @param amt       The data size in bytes
 *
 * @return the number of bytes loaded
 */
size_t StorageBuffer::loadBlock(Uint32 block, const std::byte* data, size_t amt) {
    CUAssertLog(false, "OpenGL does not support storage buffers");
    return 0;
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
 * {@link #getBlockSize}.
 *
 * In this version of the method, size is the number of bytes to read.
 *
 * @param data  The array to receive the data
 * @param size  The number of bytes to read
 *
 * @return the number of bytes read
 */
size_t StorageBuffer::readData(std::byte* data, size_t size) {
    CUAssertLog(false, "OpenGL does not support storage buffers");
    return 0;
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
 * @param size  The number of bytes to read
*
 * @return the number of bytes read
 */
size_t StorageBuffer::readBlock(Uint32 block, std::byte* data, size_t size) {
    CUAssertLog(false, "OpenGL does not support storage buffers");
    return 0;
}

/**
 * Flushes any changes to the graphics card.
 *
 * For any method other than {@link #loadData}, changes to this storage
 * buffer are only applied to a backing buffer in CPU memory, and are not
 * reflected on the graphics card. This method synchronizes these two
 * memory regions.
 */
void StorageBuffer::flush() {
    // No-op
}
