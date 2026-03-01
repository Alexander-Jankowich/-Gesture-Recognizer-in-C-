//
//  CUIndexBuffer.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a buffer for providing index data to a shader. In
//  previous versions of CUGL we combined this with the vertex buffer,
//  representing the data as as single unit. However, this did not play well
//  with either instancing or Vulkan shaders, so we factored out this
//  functionality. This headless version will store (and retrieve) data, but
//  it will not communicate with the graphics card.
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
#include <cugl/core/util/CUDebug.h>
#include <cugl/graphics/CUIndexBuffer.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/**
 * Creates an uninitialized index buffer.
 *
 * You must initialize the index buffer to allocate buffer memory.
 */
IndexBuffer::IndexBuffer() :
_data(nullptr),
_capacity(0),
_short(false),
_size(0) {
    _access = BufferAccess::STATIC;
}
    
/**
 * Deletes the index buffer, freeing all resources.
 *
 * You must reinitialize the index buffer to use it.
 */
void IndexBuffer::dispose() {
    if (_data != nullptr) {
        _data->dispose();
        delete _data;
        _data = nullptr;
    }
    _capacity = 0;
    _size = 0;
    _access = BufferAccess::STATIC;
}

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
bool IndexBuffer::init(size_t size, BufferAccess access) {
    _short = false;
    _access = access;
    _capacity = size;
    setup(sizeof(Uint32));
    return _data != nullptr;
}

/**
 * Initializes this index buffer to support 16-bit integers
 *
 * The size given is the maximum number of short integers to store in this
 * buffer, not the number of bytes.
 **
 * Note that some access types such as {@link BufferAccess#STATIC} or
 * {@link BufferAccess#COMPUTE_ONLY} can only be updated once. Any attempts
 * to load data after the first time will fail.
 *
 * @param size      The maximum number of integers in this buffer
 * @param access    The buffer access
 *
 * @return true if initialization was successful.
 */
bool IndexBuffer::initShort(size_t size, BufferAccess access) {
    _short = true;
    _access = access;
    _capacity = size;
    setup(sizeof(Uint16));
    return _data != nullptr;
}

/**
 * Allocates the initial buffers for a index buffer
 *
 * @param size  The index byte size
 */
void IndexBuffer::setup(size_t size) {
    _data = new BufferData();
    if (!_data->init(_capacity*size, _access)) {
        delete _data;
        _data = nullptr;
        return;
    }
}


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
bool IndexBuffer::loadData(const Uint32* data, size_t size) {
    if (_size > 0 && (_access != BufferAccess::DYNAMIC && _access != BufferAccess::COMPUTE_ALL)) {
        return false;
    }
    
    _size = size < _capacity ? size : _capacity;
    return _data->loadData(reinterpret_cast<const void*>(data), _size*sizeof(Uint32));
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
bool IndexBuffer::loadData(const Uint16* data, size_t size) {
    if (_size > 0 && (_access != BufferAccess::DYNAMIC && _access != BufferAccess::COMPUTE_ALL)) {
        return false;
    }

    _size = size < _capacity ? size : _capacity;
    return _data->loadData(reinterpret_cast<const Uint8*>(data), _size*sizeof(Uint16));
}
