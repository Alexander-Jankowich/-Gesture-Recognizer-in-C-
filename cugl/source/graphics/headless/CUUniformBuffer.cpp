//
//  CUUniformBuffer.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a uniform buffer for sending batched uniforms to a
//  shader (graphics or compute). Uniforms buffers are necessary for any
//  uniforms that are too large to be expressed as push constants (in Vulkan
//  most platforms only allow 196 bytes of push constants -- enough for three
//  matrices). This headless version stores buffer data but does not communicate
//  with the graphics card.
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
#include <cugl/graphics/CUUniformBuffer.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/** The byte position of an invalid offset */
const size_t UniformBuffer::INVALID_OFFSET =  ((size_t)-1);

#pragma mark Constructors
/**
 * Creates an uninitialized uniform buffer.
 *
 * You must initialize the uniform buffer to allocate memory.
 */
UniformBuffer::UniformBuffer() :
_data(nullptr),
_blockcount(0),
_blocksize(0),
_blockstride(0),
_stream(true),
_dirty(false),
_filled(false) {
}

/**
 * Deletes the uniform buffer, freeing all resources.
 *
 * You must reinitialize the uniform buffer to use it.
 */
void UniformBuffer::dispose() {
    if (_data != nullptr) {
        _data->dispose();
        delete _data;
        _data = nullptr;
    }
    _blocksize = 0;
    _blockstride = 0;
    _blockcount = 0;
    _stream = false;
    _filled = false;
    _dirty = false;
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
 * either {@link #flush} or {@link #load} will fail.
 *
 * @param capacity  The block capacity in bytes
 * @param blocks    The number of blocks to support
 * @param stream    Whether data can be streamed to the buffer
 *
 * @return true if initialization was successful.
 */
bool UniformBuffer::init(size_t capacity, Uint32 blocks, bool stream) {
    _blockcount = blocks;
    _blocksize = capacity;
    _blockstride = _blocksize;
    _stream = stream;
    _filled = false;

    _data = new BufferData();
    if (stream) {
        if (!_data->init(_blockstride*_blockcount,BufferAccess::DYNAMIC)) {
            delete _data;
            _data = nullptr;
            return false;
        }
    } else {
        if (!_data->init(_blockstride*_blockcount,BufferAccess::STATIC)) {
            delete _data;
            _data = nullptr;
            return false;
        }
    }
    _data->blocks = blocks;
    return true;
}
    
#pragma mark -
#pragma mark Block Specification
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
void UniformBuffer::setOffset(const std::string name, size_t offset) {
    _offsets[name] = offset;
}
    
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
size_t UniformBuffer::getOffset(const std::string name) const {
    auto elt = _offsets.find(name);
    if (elt == _offsets.end()) {
        return INVALID_OFFSET;
    }
    return elt->second;
}
    
/**
 * Returns the offsets defined for this buffer
 *
 * The vector returned will include the name of every variable set by
 * the method {@link #setOffset}.
 *
 * @return the offsets defined for this buffer
 */
std::vector<std::string> UniformBuffer::getOffsets() const {
    std::vector<std::string> result;
    for(auto it = _offsets.begin(); it != _offsets.end(); ++it) {
        result.push_back(it->first);
    }
    return result;
}
 
#pragma mark -
#pragma mark Memory Access
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
size_t UniformBuffer::loadData(const std::byte* data, size_t amt) {
    if (!_stream && _filled) {
        return 0;
    }
    
    size_t size = amt <= _blockstride*_blockcount ? amt :  _blockstride*_blockcount;
    size_t result = _data->loadData(reinterpret_cast<const void*>(data), size);
    _filled = true;
    return result;
}

/**
 * Loads the given data in to the specified block
 *
 * The value amt must be less than or equal to the size of block. Calling
 * this method will update the CPU memory, but not update it on the GPU.
 * You must call {@link #flush} to synchronize the buffer.
 *
 * If there is more than one block, this method will fail on any uniform
 * buffer that is not streaming. If there is only one block, this method
 * can only be called once.
 *
 * @param block     The block to update
 * @param data      The data to load
 * @param amt       The data size in bytes
 *
 * @return the number of bytes loaded
 */
size_t UniformBuffer::loadBlock(Uint32 block, const std::byte* data, size_t amt) {
    CUAssertLog(block < _blockcount, "Block %d is out of range [%d]",block,_blockcount);
    if (!_stream && (_filled || _blockcount > 0)) {
        return 0;
    }
    
    size_t size = amt <= _blocksize ? amt : _blocksize;

    if (!_stream) {
        size_t result = _data->loadData(reinterpret_cast<const void*>(data), size);
        _filled = true;
        return result;
    }

    _data->setData(data, block*_blockstride, size);
    _dirty = true;
    return size;
}
    
/**
 * Flushes any changes to the graphics card.
 *
 * For any method other than {@link #loadData}, changes to this uniform
 * buffer are only applied to a backing buffer in CPU memory, and are not
 * reflected on the graphics card. This method synchronizes these two
 * memory regions.
 */
void UniformBuffer::flush() {
    // No-op in headless
}

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
void UniformBuffer::pushInt(Sint64 block, const std::string name, Sint32 value) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushInt(block,offset,value);
    }
 }

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
void UniformBuffer::pushInt(Sint64 block, size_t offset, Sint32 value) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&value), sizeof(Sint32));
}


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
void UniformBuffer::pushUInt(Sint64 block, const std::string name, Uint32 value) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushUInt(block,offset,value);
    }
 }

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
void UniformBuffer::pushUInt(Sint64 block, size_t offset, Uint32 value) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&value), sizeof(Uint32));
}


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
void UniformBuffer::pushFloat(Sint64 block, const std::string name, float value) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushFloat(block,offset,value);
    }
 }

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
void UniformBuffer::pushFloat(Sint64 block, size_t offset, float value) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&value), sizeof(float));
}

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
void UniformBuffer::pushVec2(Sint64 block, const std::string name, const Vec2& vec) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec2(block,offset,vec);
    }
 }

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
void UniformBuffer::pushVec2(Sint64 block, size_t offset, const Vec2& vec) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec2));
}

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
void UniformBuffer::pushVec2(Sint64 block, const std::string name, float x, float y) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec2(block,offset,x,y);
    }
 }

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
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void UniformBuffer::pushVec2(Sint64 block, size_t offset, float x, float y) {
    Vec2 vec(x,y);
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec2));
}

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
void UniformBuffer::pushVec3(Sint64 block, const std::string name, const Vec3& vec) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec3(block,offset,vec);
    }
 }

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
void UniformBuffer::pushVec3(Sint64 block, size_t offset, const Vec3& vec) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec3));

}

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
void UniformBuffer::pushVec3(Sint64 block, const std::string name, float x, float y, float z) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec3(block,offset,x,y,z);
    }
 }

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
void UniformBuffer::pushVec3(Sint64 block, size_t offset, float x, float y, float z) {
    Vec3 vec(x,y,z);
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec3));
}

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
void UniformBuffer::pushVec4(Sint64 block, const std::string name, const Vec4& vec) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec4(block,offset,vec);
    }
 }

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
void UniformBuffer::pushVec4(Sint64 block, size_t offset, const Vec4& vec) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec4));
}

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
void UniformBuffer::pushVec4(Sint64 block, const std::string name, const Quaternion& quat) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec4(block,offset,quat);
    }
 }

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
void UniformBuffer::pushVec4(Sint64 block, size_t offset, const Quaternion& quat) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&quat), sizeof(Quaternion));
}

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
void UniformBuffer::pushVec4(Sint64 block, const std::string name, float x, float y, float z, float w) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushVec4(block,offset,x,y,z,w);
    }
 }

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
void UniformBuffer::pushVec4(Sint64 block, size_t offset, float x, float y, float z, float w) {
    Vec4 vec(x,y,z,w);
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&vec), sizeof(Vec4));
}

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
void UniformBuffer::pushIVec2(Sint64 block, const std::string name, int x, int y) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushIVec2(block,offset,x,y);
    }
 }

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
void UniformBuffer::pushIVec2(Sint64 block, size_t offset, Sint32 x, Sint32 y) {
    Sint32 array[2];
    array[0] = x;
    array[1] = y;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 2*sizeof(Sint32));
}

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
void UniformBuffer::pushIVec3(Sint64 block, const std::string name, int x, int y, int z) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushIVec3(block,offset,x,y,z);
    }
 }

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
void UniformBuffer::pushIVec3(Sint64 block, size_t offset, int x, int y, int z) {
    Sint32 array[3];
    array[0] = x;
    array[1] = y;
    array[2] = z;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 3*sizeof(Sint32));
}


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
void UniformBuffer::pushIVec4(Sint64 block, const std::string name, int x, int y, int z, int w) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushIVec4(block,offset,x,y,z,w);
    }
 }

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
void UniformBuffer::pushIVec4(Sint64 block, size_t offset, int x, int y, int z, int w) {
    Sint32 array[4];
    array[0] = x;
    array[1] = y;
    array[2] = z;
    array[3] = w;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 4*sizeof(Sint32));
}


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
void UniformBuffer::pushUVec2(Sint64 block, const std::string name, Uint32 x, Uint32 y) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushUVec2(block,offset,x,y);
    }
 }

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
void UniformBuffer::pushUVec2(Sint64 block, size_t offset, Uint32 x, Uint32 y) {
    Uint32 array[2];
    array[0] = x;
    array[1] = y;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 2*sizeof(Uint32));
}


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
void UniformBuffer::pushUVec3(Sint64 block, const std::string name, Uint32 x, Uint32 y, Uint32 z) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushUVec3(block,offset,x,y,z);
    }
 }

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
void UniformBuffer::pushUVec3(Sint64 block, size_t offset, Uint32 x, Uint32 y, Uint32 z) {
    Uint32 array[3];
    array[0] = x;
    array[1] = y;
    array[2] = z;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 3*sizeof(Uint32));
}

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
void UniformBuffer::pushUVec4(Sint64 block, const std::string name, Uint32 x, Uint32 y, Uint32 z, Uint32 w) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushUVec4(block,offset,x,y,z,w);
    }
 }

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
void UniformBuffer::pushUVec4(Sint64 block, size_t offset, Uint32 x, Uint32 y, Uint32 z, Uint32 w) {
    Uint32 array[4];
    array[0] = x;
    array[1] = y;
    array[2] = z;
    array[3] = w;
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 4*sizeof(Uint32));
}
    
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
void UniformBuffer::pushColor4f(Sint64 block, const std::string name, const Color4f& color) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushColor4f(block,offset,color);
    }
 }

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
void UniformBuffer::pushColor4f(Sint64 block, size_t offset, const Color4f& color) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&color), sizeof(Color4f));
}

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
void UniformBuffer::pushColor4f(Sint64 block, const std::string name, float red, float green, float blue, float alpha) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushColor4f(block,offset,red,green,blue,alpha);
    }
 }

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
void UniformBuffer::pushColor4f(Sint64 block, size_t offset, float red,
                                float green, float blue, float alpha) {
    Color4f color(red,green,blue,alpha);
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&color), sizeof(Color4f));
}

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
void UniformBuffer::pushColor4(Sint64 block, const std::string name, const Color4 color) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushColor4(block,offset,color);
    }
 }

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
void UniformBuffer::pushColor4(Sint64 block, size_t offset, const Color4 color) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&color), sizeof(Color4));
}

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
void UniformBuffer::pushColor4(Sint64 block, const std::string name, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushColor4(block,offset,red,green,blue,alpha);
    }
 }

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
void UniformBuffer::pushColor4(Sint64 block, size_t offset, Uint8 red,
                               Uint8 green, Uint8 blue, Uint8 alpha) {
    Color4 color(red,green,blue,alpha);
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(&color), sizeof(Color4));
}

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
void UniformBuffer::pushMat2(Sint64 block, const std::string name, const Affine2& mat) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat2(block,offset,mat);
    }
 }

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
void UniformBuffer::pushMat2(Sint64 block, size_t offset, const Affine2& mat) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(mat.m), 4*sizeof(float));
}

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
void UniformBuffer::pushMat2(Sint64 block, const std::string name, const float* array) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat2(block,offset,array);
    }
 }

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
void UniformBuffer::pushMat2(Sint64 block, size_t offset, const float* array) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 4*sizeof(float));
}

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
void UniformBuffer::pushMat3(Sint64 block, const std::string name, const Affine2& mat) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat3(block,offset,mat);
    }
 }

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
void UniformBuffer::pushMat3(Sint64 block, size_t offset, const Affine2& mat) {
    float array[9];
    array[0] = mat.m[0];
    array[1] = mat.m[1];
    array[2] = 0;
    array[3] = mat.m[2];
    array[4] = mat.m[3];
    array[5] = 0;
    array[6] = mat.m[4];
    array[7] = mat.m[5];
    array[8] = 1;
    pushMat3(block,offset,array);
}

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
void UniformBuffer::pushMat3(Sint64 block, const std::string name, const float* array) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat3(block,offset,array);
    }
 }

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
void UniformBuffer::pushMat3(Sint64 block, size_t offset, const float* array) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 9*sizeof(float));
}

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
void UniformBuffer::pushMat4(Sint64 block, const std::string name, const Mat4& mat) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat4(block,offset,mat);
    }
 }

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
void UniformBuffer::pushMat4(Sint64 block, size_t offset, const Mat4& mat) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(mat.m), 16*sizeof(float));
}

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
void UniformBuffer::pushMat4(Sint64 block, const std::string name, const float* array) {
    size_t offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        pushMat4(block,offset,array);
    }
 }

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
void UniformBuffer::pushMat4(Sint64 block, size_t offset, const float* array) {
    pushBytes(block, offset, reinterpret_cast<const Uint8*>(array), 16*sizeof(float));
}


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
 *
 * @return the number of bytes loaded
 */
void UniformBuffer::pushBytes(Sint64 block, size_t offset, const Uint8* data, size_t amt) {
    CUAssertLog(block < _blockcount, "Block %lld is out of range [%d]",block,_blockcount);
    CUAssertLog(offset < _blocksize, "Offset %zu is out of range [%zu]",offset,_blocksize);
    if (!_stream) {
        return;
    }
    size_t size = amt+offset < _blocksize ? amt : _blocksize-offset;
    if (block >= 0) {
        size_t position = block*_blockstride+offset;
        _data->setData(reinterpret_cast<const std::byte*>(data), position, size);
    } else {
        for(int block = 0; block < _blockcount; block++) {
            size_t position = block*_blockstride+offset;
            _data->setData(reinterpret_cast<const std::byte*>(data), position, size);
        }
    }
    _dirty = true;
}
