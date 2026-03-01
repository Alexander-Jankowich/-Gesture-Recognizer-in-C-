//
//  CUVertexBuffer.cpp
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
//  This headless version will store (and retrieve) data, but it will not
//  communicate with the graphics card.
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
#include <cugl/graphics/CUVertexBuffer.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/graphics/CUTexture.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

#pragma mark Constructors
/**
 * Creates an uninitialized vertex buffer.
 *
 * You must initialize the vertex buffer to allocate buffer memory.
 */
VertexBuffer::VertexBuffer() :
_data(nullptr),
_capacity(0),
_stride(0),
_size(0) {
    _access = BufferAccess::STATIC;
}
    
/**
 * Deletes the vertex buffer, freeing all resources.
 *
 * You must reinitialize the vertex buffer to use it.
 */
void VertexBuffer::dispose() {
    if (_data != nullptr) {
        _data->dispose();
        delete _data;
        _data = nullptr;
    }
    _capacity = 0;
    _stride = 0;
    _size = 0;
    _access = BufferAccess::STATIC;
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
bool VertexBuffer::init(size_t size, size_t stride, BufferAccess access) {
    _stride = stride;
    _capacity = _stride > 0 ? size*_stride : size;
    _access = access;
    
    _data = new BufferData();
    if (!_data->init(_capacity, access)) {
        delete _data;
        _data = nullptr;
        return false;
    }

    return true;
}

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
bool VertexBuffer::loadData(const std::byte* data, size_t size) {
    CUAssertLog(_data != nullptr, "Buffer has not been properly allocated");
    if (_size > 0 && (_access != BufferAccess::DYNAMIC && _access != BufferAccess::COMPUTE_ALL)) {
        return false;
    }
    
    _size = size < _capacity ? size : _capacity;
    return _data->loadData(reinterpret_cast<const void*>(data),_size);
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
size_t VertexBuffer::readData(std::byte* data, size_t size) {
    CUAssertLog(false,"OpenGL does not support compute buffers");
    return 0;
}
