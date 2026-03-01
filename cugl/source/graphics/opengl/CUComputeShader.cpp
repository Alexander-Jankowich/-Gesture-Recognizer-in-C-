//
//  CUComputeShader.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a general purpose shader class for compute shaders in
//  Vulkan. Because of limitations with macOS/iOS, CUGL does not support
//  compute shaders in OpenGL. Attempts to allocated objects of this class
//  will fail unless Vulkan is the active backend (or it is headless).
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
//      warranty. In no event will the authors be held liable for any damages
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
#include <cugl/core/CUApplication.h>
#include <cugl/core/util/CUDebug.h>

#include <cugl/graphics/CUSampler.h>
#include <cugl/graphics/CUImage.h>
#include <cugl/graphics/CUImageArray.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CUTextureArray.h>
#include <cugl/graphics/CUVertexBuffer.h>
#include <cugl/graphics/CUIndexBuffer.h>
#include <cugl/graphics/CUUniformBuffer.h>
#include <cugl/graphics/CUStorageBuffer.h>
#include <cugl/graphics/CUComputeShader.h>
#include "CUGLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/** The active shader (if any) */
std::shared_ptr<ComputeShader> ComputeShader::g_shader = nullptr;

// We allow compute shaders to be set-up, but not compiled  

#pragma mark Constructor
/**
 * Initializes this shader with the given shader source
 *
 * This method will allocate resources for a shader, but it will not
 * compile it. You must all the {@link compile} method to compile the
 * shader.
 *
 * @param source    The source for the compute shader.
 *
 * @return true if initialization was successful.
 */
bool ComputeShader::init(const ShaderSource& source) {
    _compModule = source;
    _data = nullptr;
    _constvalues.reserve(256);
    _dimension.set(1,1,1);
    return true;
}

/**
 * Deletes the shader and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void ComputeShader::dispose() {
    for(auto it = _invalues.begin(); it != _invalues.end(); ++it) {
        delete it->second;
        it->second = nullptr;
    }
    for(auto it = _outvalues.begin(); it != _outvalues.end(); ++it) {
        delete it->second;
        it->second = nullptr;
    }
    _uniforms.clear();
    _invalues.clear();
    _outvalues.clear();
    _inputs.clear();
    _outputs.clear();
    _constvalues.clear();
    _constants.clear();
    _transitions.clear();
}
    
#pragma mark -
#pragma mark Compilation
/**
 * Declares a shader push-constant uniform.
 *
 * In order to be used, each push-constant must be declared before the
 * graphics shader is compiled via {@link #compile}. The declaration links
 * a name to a uniform definition. We can use this name to push data to the
 * compute shader via any of the push methods.
 *
 * This method can fail if the uniform has already been declared.
 *
 * @param name  The uniform name
 * @param def   The uniform type, size, and location
 *
 * @return true if the declaration was successful
 */
bool ComputeShader::declareUniform(const std::string name, const UniformDef& def) {
    auto it = _uniforms.find(name);
    if (it != _uniforms.end()) {
        CUAssertLog(false, "Uniform %s is already declared", name.c_str());
        return false;
    }
    _uniforms[name] = def;
    return true;
}

/**
 * Returns the uniform definition for the given variable name.
 *
 * If there is no uniform definition for the given name, this method
 * returns nullptr.
 *
 * @param name  The attribute name
 *
 * @return the uniform definition for the given variable name.
 */
const UniformDef* ComputeShader::getUniformDef(const std::string name) const {
    auto it = _uniforms.find(name);
    if (it != _uniforms.end()) {
        return &(it->second);
    }
    return nullptr;
}

/**
 * Declares and sets a specialization constant
 *
 * Specialization constants are assigned to the compute shader at compile
 * time. This value can be reassigned any time before a call to
 * {@link #build}, but cannot be changed once the compute shader is
 * complete.
 *
 * @param name  The constant name
 * @param spec  The specialization constant
 */
void ComputeShader::setConstant(const std::string name, const GLSLConstant& spec) {
    auto it = _constants.find(name);
    if (it != _constants.end() && it->second.size == spec.size) {
        it->second.type = spec.type;
        it->second.location = spec.location;
        std::memcpy(it->second.data,spec.data,spec.size);
        return;
    }
    
    size_t limit = _constvalues.size();
    uint8_t* orig = _constvalues.data();
    _constvalues.resize(limit+spec.size, 0);
    uint8_t* targ = _constvalues.data();
    if (orig != targ) {
        for(auto& spec : _constants) {
            spec.second.data = (spec.second.data-orig)+targ;
        }
    }

    uint8_t* dst = _constvalues.data()+limit;
    std::memcpy(dst,spec.data,spec.size);
    _constants[name] = spec;
    _constants[name].data = dst;

    return;
}

/**
 * Returns the information for a specialization constant
 *
 * Specialization constants are assigned to the compute shader at compile
 * time. This value can be reassigned any time before a call to
 * {@link #build}, but cannot be changed once the compute shader is
 * complete.
 *
 * If there is no constant associated with the given name, this method
 * returns nullptr.
 *
 * @param name  The constant name
 *
 * @return the information for a specialization constant
 */
const GLSLConstant* ComputeShader::getConstant(const std::string name) const {
    auto it = _constants.find(name);
    if (it != _constants.end()) {
        return &(it->second);
    }
    return nullptr;

}

/**
 * Declares an shader input uniform.
 *
 * Input uniforms include resources such as uniform buffers, storage buffers,
 * and images. In order to be used, each resource must be declared before
 * the compute shader is compiled via {@link #compile}. The declaration
 * links a name to a resource definition. We can use this name to load data
 * via methods such as {@link #setUniformBuffer} and {@link #setImage}.
 *
 * Input uniforms are always read-only. This method can fail if the resource
 * has already been declared.
 *
 * @param name  The resource name
 * @param def   The resource type, size, and location
 *
 * @return true if the declaration was successful
 */
bool ComputeShader::declareInput(const std::string name, const ResourceDef& def) {
    auto it = _inputs.find(name);
    if (it != _inputs.end()) {
        CUAssertLog(false, "Input resource %s is already declared", name.c_str());
        return false;
    }
    auto jt = _outputs.find(name);
    if (jt != _outputs.end()) {
        CUAssertLog(false, "Variable %s refers to an output resource", name.c_str());
        return false;
    }

    _inputs[name] = def;
    return true;
}

/**
 * Declares an shader ouput uniform.
 *
 * Output uniforms are resources that can be written to by a compute shader.
 * Currently we only support {@link Images}, {@link VertexBuffer},
 * {@link IndexBuffer} and {@link StorageBuffer} objects with access type
 * compute.
 *
 * In order to be used, each resource must be declared before the compute
 * shader is compiled via {@link #compile}. The declaration links a name to
 * a resource definition. We can use this name to load data via methods such
 * as {@link #setUniformBuffer} and {@link #setImage}.
 *
 * This method can fail if the resource has already been declared.
 *
 * @param name  The resource name
 * @param def   The resource type, size, and location
 *
 * @return true if the declaration was successful
 */
bool ComputeShader::declareOutput(const std::string name, const ResourceDef& def) {
    auto it = _outputs.find(name);
    if (it != _outputs.end()) {
        CUAssertLog(false, "Output resource %s is already declared", name.c_str());
        return false;
    }
    auto jt = _inputs.find(name);
    if (jt != _inputs.end()) {
        CUAssertLog(false, "Variable %s refers to an input resource", name.c_str());
        return false;
    }

    _outputs[name] = def;
    return true;
}

/**
 * Returns the resource definition for the given variable name.
 *
 * If there is no resource definition for the given name, or if it does not
 * correspond to an input variable, this method returns nullptr.
 *
 * This method does not distinguish input resources from output resources.
 *
 * @param name  The resource name
 *
 * @return the resource definition for the given variable name.
 */
const ResourceDef* ComputeShader::getResourceDef(const std::string name) const {
    auto it = _inputs.find(name);
    if (it != _inputs.end()) {
        return &(it->second);
    }
    auto jt = _outputs.find(name);
    if (jt != _outputs.end()) {
        return &(jt->second);
    }
    return nullptr;
}

/**
 * Returns true if the resource variable is writeable
 *
 * A writeable resource is one that has been declared by the method
 * {@link #declareOutput}. If the variable does not exist or was declared
 * with another method, this method returns false.
 *
 * @param name  The resource name
 *
 * @return the resource definition for the given variable name.
 */
bool ComputeShader::isWriteable(const std::string name) const {
    auto jt = _outputs.find(name);
    if (jt != _outputs.end()) {
        return true;
    }
    return false;
}

/**
 * Create the input data structures for the given definition
 *
 * @param name  The resource name
 * @param def   The resource definition
 */
void ComputeShader::createInputs(const std::string name, ResourceDef def) {
    switch (def.type) {
        case ResourceType::TEXTURE:
        {
            auto udata = new ResourceTexture();
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::IMAGE:
        {
            auto udata = new ResourceImage();
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::SAMPLER:
        {
            auto udata = new ResourceSampler();
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::MONO_BUFFER:
        {
            auto udata = new ResourceUniform();
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::MULTI_BUFFER:
        {
            auto udata = new ResourceUniform();
            udata->stride = def.blocksize;
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::MONO_STORAGE:
        {
            auto udata = new ResourceStorage();
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        case ResourceType::MULTI_STORAGE:
        {
            auto udata = new ResourceStorage();
            udata->stride = def.blocksize;
            udata->type = def.type;
            _invalues[name] = udata;
        }
            break;
        default:
            CUAssertLog(false,"Unsupported input type %d", def.type);
            return;
    }
}

/**
 * Create the output data structures for the given definition
 *
 * @param name  The resource name
 * @param def   The resource definition
 */
void ComputeShader::createOutputs(const std::string name, ResourceDef def) {
    switch (def.type) {
        case ResourceType::COMPUTE_STORAGE:
        {
            auto udata = new ResourceStorage();
            udata->stride = def.blocksize;
            udata->type = def.type;
            _outvalues[name] = udata;
        }
        case ResourceType::COMPUTE_VERTEX:
        {
            auto udata = new ResourceVertex();
            udata->type = def.type;
            _outvalues[name] = udata;
        }
            break;
        case ResourceType::COMPUTE_INDEX:
        {
            auto udata = new ResourceIndex();
            udata->type = def.type;
            _outvalues[name] = udata;
        }
            break;
        case ResourceType::COMPUTE_IMAGE:
        {
            auto udata = new ResourceImage();
            udata->type = def.type;
            _outvalues[name] = udata;
        }
            break;
        default:
            CUAssertLog(false,"Unsupported output type %d", def.type);
            return;
    }
}


/**
 * Compiles this compute shader for use
 *
 * All variables (uniforms, input resources, and output resources) should
 * be declared before compilation. Once compilation is successful, this
 * shader should be ready for use.
 *
 * If compilation fails, it will display error messages on the log.
 *
 * @return true if compilation was successful.
 */
bool ComputeShader::compile() {
    CUAssertLog(false, "OpenGL does not support compute shaders.");
    return false;
}


/**
 * Returns true if this shader has been compiled and is ready for use.
 *
 * @return true if this shader has been compiled and is ready for use.
 */
bool ComputeShader::isReady() const {
    return false;
}

#pragma mark -
#pragma mark Data Buffers
/**
 * Sets the uniform buffer for the given resource variable
 *
 * This method will fail if the variable does not refer to a uniform buffer
 * or if the value is nullptr. If the resource variable refers to an array
 * of uniform buffers, then this buffer will be assigned to all positions
 * in the array.
 *
 * If the uniform buffer is composed of more than one block (e.g. it has
 * type {@link ResourceType#MULTI_BUFFER}), the shader will initially
 * start at block 0. To switch the block, use {@link #setBlock}.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that uniform buffers are always read-only in CUGL compute
 * shaders.
 *
 * @param name      The name of the resource
 * @param buffer    The uniform buffer for the resource
 */
void ComputeShader::setUniformBuffer(const std::string name, const std::shared_ptr<UniformBuffer>& buffer) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}


/**
 * Sets the uniform buffer array for the given resource variable
 *
 * This method will fail if the variable does not refer to a uniform buffer
 * or if the vector is empty. If the vector is larger than the number of
 * buffers referenced by this variable, then the buffers past the limit are
 * ignored. If the vector is smaller, then the missing positions will be
 * replaced by the last buffer.
 *
 * If the uniform buffers are composed of more than one block (e.g. they
 * have type {@link ResourceType#MULTI_BUFFER}), the shader will initially
 * start at block 0. To switch the block, use {@link #setBlock}. Note that all
 * buffers in an array must use the same block.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that uniform buffers are always read-only in CUGL compute
 * shaders.
 *
 * @param name      The name of the resource
 * @param buffers   The uniform buffers for the resource
 */
void ComputeShader::setUniformBuffers(const std::string name, const std::vector<std::shared_ptr<UniformBuffer>>& buffers) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}


/**
 * Returns the uniform buffer for the given resource variable
 *
 * If the variable does not refer to a non push-constant uniform, then this
 * method will return nullptr. Note that uniform buffers are always
 * read-only in CUGL compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the uniform buffer for the given resource variable
 */
const std::shared_ptr<UniformBuffer> ComputeShader::getUniformBuffer(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Returns the uniform buffers for the given resource variable
 *
 * If the variable does not refer to a uniform buffer then this method will
 * return an empty vector.
 *
 * Any buffer assigned to a name is stateful, in that it will persist across
 * compute passes. Note that uniform buffers are always read-only in CUGL
 * compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the uniform buffers for the given resource variable
 */
const std::vector<std::shared_ptr<UniformBuffer>> ComputeShader::getUniformBuffers(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return std::vector<std::shared_ptr<UniformBuffer>>();
}

/**
 * Sets the storage buffer for the given resource variable
 *
 * If the variable does not refer to a storage buffer, then this method will
 * fail. If the resource variable refers to an array of storage buffers,
 * then this buffer will be assigned to all positions in the array.
 *
 * If the storage buffer is composed of more than one block (e.g. it has
 * type {@link ResourceType#MULTI_STORAGE}), the shader will initially start
 * at block 0. To switch the block, use {@link #setBlock}.
 *
 * Storage buffers may be used either an input our output in a compute
 * shader. Output buffers should have an access type that allows write
 * access by the compute shader. Setting this value is stateful, in that
 * it will persist across compute passes.
 *
 * @param name      The name of the resource
 * @param buffer    The uniform buffer for the resource
 */
void ComputeShader::setStorageBuffer(const std::string name, const std::shared_ptr<StorageBuffer>& buffer) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the storage buffer array for the given resource variable
 *
 * This method will fail if the variable does not refer to a storage buffer
 * or if the vector is empty. If the vector is larger than the number of
 * buffers referenced by this variable, then the buffers past the limit are
 * ignored. If the vector is smaller, then the missing positions will be
 * replaced by the last buffer.
 *
 * If the storage buffers are composed of more than one block (e.g. they
 * have type {@link ResourceType#MULTI_STORAGE}), the shader will initially
 * start at block 0. To switch the block, use {@link #setBlock}. Note that
 * all storage buffers in an array must have the same block count.
 *
 * Storage buffers may be used either an input our output in a compute
 * shader. Output buffers should have an access type that allows write
 * access by the compute shader. Setting this value is stateful, in that
 * it will persist across compute passes.
 *
 * @param name      The name of the resource
 * @param buffers   The storage buffers for the resource
 */
void ComputeShader::setStorageBuffers(const std::string name, const std::vector<std::shared_ptr<StorageBuffer>>& buffers) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the storage buffer for the given resource variable
 *
 * If the variable does not refer to a storage buffer, then this method will
 * return nullptr. If more than one buffer is assigned to this attribute,
 * then this method will return the first one.
 *
 * Storage buffers may be used either an input our output in a compute
 * shader. Output buffers should have an access type that allows write
 * access by the compute shader. Any buffer assigned to a name is stateful,
 * in that it will persist across compute passes.
 *
 * @param name      The name of the resource
 *
 * @return the storage buffer for the given resource variable
 */
const std::shared_ptr<StorageBuffer> ComputeShader::getStorageBuffer(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Returns the storage buffers for the given resource variable
 *
 * If the variable does not refer to a storage buffer, then this method will
 * return an empty vector.
 *
 * Storage buffers may be used either an input our output in a compute
 * shader. Output buffers should have an access type that allows write
 * access by the compute shader. Any buffer assigned to a name is stateful,
 * in that it will persist across compute passes.
 *
 * @param name      The name of the resource
 *
 * @return the storage buffers for the given resource variable
 */
const std::vector<std::shared_ptr<StorageBuffer>> ComputeShader::getStorageBuffers(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return std::vector<std::shared_ptr<StorageBuffer>>();
}

/**
 * Sets the active block of the associated resource buffer
 *
 * If the type of the resource is not {@link ResourceType#MULTI_BUFFER} or
 * {@link ResourceType#MULTI_STORAGE}, this method will fail. Otherwise,
 * it will change the active block. That block will be used in the next
 * call to {@link #dispatch}.
 *
 * @param name      The name of the resource
 * @param block     The resource block to use
 */
void ComputeShader::setBlock(const std::string name, Uint32 block) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}
 
/**
 * Returns the active block of the associated resource buffer
 *
 * If the type of the resource is not {@link ResourceType#MULTI_BUFFER} or
 * {@link ResourceType#MULTI_STORAGE}, this method return 0. This block
 * will be used in the next call to {@link #dispatch}.
 *
 * @param name      The name of the resource
 *
 * @return the active block of the associated resource buffer
 */
Uint32 ComputeShader::getBlock(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return 0;
}

/**
 * Sets the vertex buffer for the given resource variable
 *
 * Compute shaders do not use vertex buffers as inputs, but they can use
 * them as outputs. This is particularly useful for particle systems, where
 * a compute shader is used to fill the contents of the vertex buffer.
 *
 * Any {@link VertexBuffer} set by this method must have compute access.
 *
 * @param name      The name of the resource
 * @param buffer    The vertex buffer for the resource
 */
void ComputeShader::setVertexBuffer(const std::string name, const std::shared_ptr<VertexBuffer>& buffer) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the vertex buffer for the given resource variable
 *
 * Compute shaders do not use vertex buffers as inputs, but they can use
 * them as outputs. This is particularly useful for particle systems, where
 * a compute shader is used to fill the contents of the vertex buffer.
 *
 * Any {@link VertexBuffer} returned by this method must have compute access.
 *
 * @param name      The name of the resource
 *
 * @return the vertex buffer for the given resource variable
 */
const std::shared_ptr<VertexBuffer> ComputeShader::getVertexBuffer(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Sets the index buffer for the given resource variable
 *
 * Compute shaders do not use index buffers as inputs, but they can use
 * them as outputs. This is particularly useful for tesselation, where
 * a compute shader is used to fill the contents of the index buffer.
 *
 * Any {@link IndexBuffer} set by this method must have compute access.
 *
 * @param name      The name of the resource
 * @param buffer    The index buffer for the resource
 */
void ComputeShader::setIndexBuffer(const std::string name, const std::shared_ptr<IndexBuffer>& buffer) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the index buffer for the given resource variable
 *
 * Compute shaders do not use index buffers as inputs, but they can use
 * them as outputs. This is particularly useful for tesselation, where
 * a compute shader is used to fill the contents of the index buffer.
 *
 * Any {@link IndexBuffer} returned by this method must have compute access.
 *
 * @param name      The name of the resource
 *
 * @return the index buffer for the given resource variable
 */
const std::shared_ptr<IndexBuffer> ComputeShader::getIndexBuffer(const std::string name) const {
    CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

#pragma mark -
#pragma mark Textures
/**
 * Sets the texture for the given resource variable
 *
 * This method will fail if the variable does not refer to a texture (a
 * sampler2D in GLSL), or if the value is nullptr. If the variable refers
 * to an array of textures, all slots will be assigned to this texture.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that textures (which include both an image and a sampler)
 * are read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param texture   The texture for the resource
 */
void ComputeShader::setTexture(const std::string name,
                               const std::shared_ptr<Texture>& texture) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the textures for the given resource variable
 *
 * This method will fail if the variable does not refer to a texture (a
 * sampler2D in GLSL), or if the vector is empty. If the vector is larger
 * than the number of textures referenced by this variable, then the
 * textures past the limit are ignored. If the vector is smaller, then the
 * missing positions will be replaced by the last texture.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that textures (which include both an image and a sampler)
 * are read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param textures  The textures for the resource
 */
void ComputeShader::setTextures(const std::string name,
                                const std::vector<std::shared_ptr<Texture>>& textures) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the texture for the given resource variable
 *
 * If the variable does not refer to a texture (a sampler2d in GLSL), then
 * this method will return nullptr. If more than one texture is assigned to
 * this attribute, then this method will return the first one.
 *
 * Any texture assigned to a name is stateful, in that it will persist
 * across dispatch calls. Note that textures (which include both an image
 * and a sampler) are read-only in compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the texture for the given resource variable
 */
const std::shared_ptr<Texture> ComputeShader::getTexture(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Returns the textures for the given resource variable
 *
 * If the variable does not refer to a texture (a sampler2D in GLSL), then
 * this method will return an empty vector.
 *
 * Any textures assigned to a name are stateful, in that they will persist
 * across dispatch calls. Note that textures (which include both an image
 * and a sampler) are read-only in compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the textures for the given resource variable
 */
const std::vector<std::shared_ptr<Texture>> ComputeShader::getTextures(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return std::vector<std::shared_ptr<Texture>>();
}

/**
 * Sets the texture array for the given resource variable
 *
 * This method will fail if the variable does not refer to a texture array
 * (a sampler2DArray in GLSL), or if the value is nullptr. If the variable
 * refers to an array of texture arrays, all slots will be assigned to this
 * texture array.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that texture arrays (which include both an image array and
 * a sampler) are read-only in compute shaders.
 *
 * @param name  The name of the resource
 * @param array The texture array for the resource
 */
void ComputeShader::setTextureArray(const std::string name, const std::shared_ptr<TextureArray>& array)  {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the texture arrays for the given resource variable
 *
 * This method will fail if the variable does not refer to a texture array
 * (a sampler2DArray in GLSL), or if the vector is empty. If the vector is
 * larger than the number of texture arrays referenced by this variable,
 * then the texture arrays past the limit are ignored. If the array is
 * smaller, then the missing positions will be replaced by the last texture
 * array.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that texture arrays (which include both an image array and
 * a sampler) are read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param arrays    The texture arrays for the resource
 */
void ComputeShader::setTextureArrays(const std::string name, const std::vector<std::shared_ptr<TextureArray>>& arrays) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}
 
/**
 * Returns the texture array for the given resource variable
 *
 * If the variable does not refer to a texture array (a sampler2DArray in
 * GLSL), then this method will return nullptr. If more than one texture
 * array is assigned to this attribute, then this method will return the
 * first one.
 *
 * Any texture array assigned to a name is stateful, in that it will persist
 * across compute passes. Note that texture arrays (which include both an
 * image array and a sampler) are read-only in compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the texture array for the given resource variable
 */
const std::shared_ptr<TextureArray> ComputeShader::getTextureArray(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Returns the texture arrays for the given resource variable
 *
 * If the variable does not refer to a texture array (a sampler2DArray in
 * GLSL), then this method will return an empty vector.
 *
 * Any texture arrays assigned to a name are stateful, in that they will
 * persist across compute passes. Note that texture arrays (which include
 * both an image array and a sampler) are read-only in compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the texture arrays for the given resource variable
 */
const std::vector<std::shared_ptr<TextureArray>> ComputeShader::getTextureArrays(const std::string name) const {
    CUAssertLog(false, "OpenGL does not support compute shaders.");
    return std::vector<std::shared_ptr<TextureArray>>();
}

/**
 * Sets the image for the given resource variable
 *
 * This method will fail is the variable does not refer to an image (a
 * texture2D in GLSL), or if the value is nullptr. If the variable refers
 * to an array of images, all slots will be assigned to this image.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that an image can either be an input or an output resource.
 * If name is an output variable, then the image must have compute access
 * or this method will fail.
 *
 * @param name  The name of the resource
 * @param image The image for the resource
 */
void ComputeShader::setImage(const std::string name,
                             const std::shared_ptr<Image>& image) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the images for the given resource variable
 *
 * This method will fail is the variable does not refer to an image (a
 * texture2D in GLSL), or if the value is nullptr. If the vector is larger
 * than the number of images referenced by this variable, then the images
 * past the limit are ignored. If the vector is smaller, then the missing
 * positions will be replaced by nullptr (potentially causing the pipeline
 * to crash).
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that an image can either be an input or an output resource.
 * If name is an output variable, then the image must have compute access
 * or this method will fail.
 *
 * @param name      The name of the resource
 * @param images    The images for the resource
 */
void ComputeShader::setImages(const std::string name,
                              const std::vector<std::shared_ptr<Image>>& images) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}
    
/**
 * Returns the image for the given resource variable
 *
 * If the variable does not refer to an image (a texture2D in GLSL), then
 * this method will return nullptr. If more than one image is assigned to
 * this attribute, then this method will return the first one.
 *
 * Any image assigned to a name is stateful, in that it will persist compute
 * passes. Note that an image can either be an input or an output resource.
 * If name is an output variable, then the image must have compute access
 * or this method will fail.
 *
 * @param name  The name of the resource
 *
 * @return the image for the given resource variable
 */
const std::shared_ptr<Image> ComputeShader::getImage(const std::string name) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return nullptr;
}

/**
 * Returns the images for the given resource variable
 *
 * If the variable does not refer to an image (a texture2D in GLSL), then
 * this method will return an empty vector.
 *
 * Any image assigned to a name is stateful, in that it will persist compute
 * passes. Note that an image can either be an input or an output resource.
 * If name is an output variable, then the image must have compute access
 * or this method will fail.
 *
 * @param name      The name of the resource
 *
 * @return the images for the given resource variable
 */
const std::vector<std::shared_ptr<Image>> ComputeShader::getImages(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
    return std::vector<std::shared_ptr<Image>>();
}

/**
 * Sets the image array for the given resource variable
 *
 * This method will fail if the variable does not refer to an image array
 * (a texture2DArray in GLSL), or if the value is nullptr. If the variable
 * refers to an array of image arrays, all slots will be assigned to this
 * image array.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that image arrays (as opposed to images) are currently
 * read-only in compute shaders.
 *
 * @param name  The name of the resource
 * @param array The image array for the resource
 */
void ComputeShader::setImageArray(const std::string name, const std::shared_ptr<ImageArray>& array) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the image arrays for the given resource variable
 *
 * This method will fail if the variable does not refer to an image array
 * (a texture2DArray in GLSL), or if the vector is empty. If the vector is
 * larger than the number of images referenced by this variable, then the
 * image arrays past the limit are ignored. If the vector is smaller, then
 * the missing positions will be replaced by the last image array.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that image arrays (as opposed to images) are currently
 * read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param arrays    The image arrays for the resource
 */
void ComputeShader::setImageArrays(const std::string name, const std::vector<std::shared_ptr<ImageArray>>& arrays) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the image array for the given resource variable
 *
 * If the variable does not refer to an image array (a texture2DArray in
 * GLSL), then this method will return nullptr. If more than one image
 * array is assigned to this attribute, then this method will return the
 * first one.
 *
 * Any image array assigned to a name is stateful, in that it will persist
 * across compute passes. Note that image arrays (as opposed to images) are
 * currently read-only in compute shaders.
 *
 * @param name  The name of the resource
 *
 * @return the image array for the given resource variable
 */
const std::shared_ptr<ImageArray> ComputeShader::getImageArray(const std::string name) const  {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
	return nullptr;
}

/**
 * Returns the image arrays for the given resource variable
 *
 * If the variable does not refer to an image array (a texture2DArray in
 * GLSL), then this method will return an empty vector.
 *
 * Any image array assigned to a name is stateful, in that it will persist
 * across compute passes. Note that image arrays (as opposed to images) are
 * currently read-only in compute shaders.
 *
 * @param name      The name of the resource
 *
 * @return the image arrays for the given resource variable
 */
const std::vector<std::shared_ptr<ImageArray>> ComputeShader::getImageArrays(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
	return std::vector<std::shared_ptr<ImageArray>>();
}

/**
 * Sets the sampler for the given resource variable
 *
 * This method will fail is the variable does not refer to a sampler (a
 * sampler in GLSL), or if the value is nullptr. If the variable refers to
 * an array of samplers, all slots will be assigned to this sampler.
 *
 * Setting this value is stateful, in that it will persist across compute
 * passes. Note that samplers are read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param sampler   The sampler for the resource
 */
void ComputeShader::setSampler(const std::string name, const std::shared_ptr<Sampler>& sampler) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the samplers for the given resource variable
 *
 * This method will fail is the variable does not refer to a sampler (a
 * sampler in GLSL), or if the value is nullptr. If the vector is larger
 * than the number of samplers referenced by this variable, then the
 * samplers past the limit are ignored. If the vector is smaller, then the
 * missing positions will be replaced by nullptr (potentially causing the
 * pipeline to crash).
 *
 * Setting this value is stateful, in that it will persist across dispatch
 * calls. Note that samplers are read-only in compute shaders.
 *
 * @param name      The name of the resource
 * @param samplers  The samplers for the resource
 */
void ComputeShader::setSamplers(const std::string name,
                                const std::vector<std::shared_ptr<Sampler>>& samplers) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Returns the sampler for the given resource variable
 *
 * If the variable does not refer to a sampler (a sampler in GLSL), then
 * this method will return nullptr. If more than one sampler is assigned to
 * this attribute, then this method will return the first one.
 *
 * Any sampler assigned to a name is stateful, in that it will persist
 * across dispatch calls. Note that samplers are read-only in compute
 * shaders.
 *
 * @param name  The name of the resource
 *
 * @return the sampler for the given resource variable
 */
const std::shared_ptr<Sampler> ComputeShader::getSampler(const std::string name) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
	return nullptr;
}

/**
 * Returns the samplers for the given resource variable
 *
 * If the variable does not refer to a sampler (a sampler in GLSL), then
 * this method will return an empty vector.
 *
 * Any samplers assigned to a name are stateful, in that they will persist
 * across compute passes. Note that samplers are read-only in compute
 * shaders.
 *
 * @param name      The name of the resource
 *
 * @return the samplers for the given resource variable
 */
const std::vector<std::shared_ptr<Sampler>> ComputeShader::getSamplers(const std::string name) const {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
	return std::vector<std::shared_ptr<Sampler>>();
}


#pragma mark -
#pragma mark Uniforms/Push Constants
/**
 * Sets the given uniform to a integer value.
 *
 * This will fail if the uniform is not a GLSL int variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void ComputeShader::pushInt(const std::string name, Sint32 value) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to an unsigned integer value.
 *
 * This will fail if the uniform is not a GLSL uint variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void ComputeShader::pushUInt(const std::string name, Uint32 value) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a float value.
 *
 * This will fail if the uniform is not a GLSL float variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void ComputeShader::pushFloat(const std::string name, float value) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a Vector2 value.
 *
 * This will fail if the uniform is not a GLSL vec2 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void ComputeShader::pushVec2(const std::string name, const Vec2& vec) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL vec2 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void ComputeShader::pushVec2(const std::string name, float x, float y) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a Vector3 value.
 *
 * This will fail if the uniform is not a GLSL vec3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void ComputeShader::pushVec3(const std::string name, const Vec3& vec) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL vec3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 */
void ComputeShader::pushVec3(const std::string name, float x, float y, float z) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a Vector4 value.
 *
 * This will fail if the uniform is not a GLSL vec4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void ComputeShader::pushVec4(const std::string name, const Vec4& vec) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a quaternion.
 *
 * This will fail if the uniform is not a GLSL vec4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param quat  The value for the uniform
 */
void ComputeShader::pushVec4(const std::string name, const Quaternion& quat) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL vec4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 * @param w     The z-value for the vector
 */
void ComputeShader::pushVec4(const std::string name, float x, float y, float z, float w) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec2 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void ComputeShader::pushIVec2(const std::string name, Sint32 x, Sint32 y) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 */
void ComputeShader::pushIVec3(const std::string name, int x, int y, int z) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 * @param w     The z-value for the vector
 */
void ComputeShader::pushIVec4(const std::string name, int x, int y, int z, int w) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec2 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void ComputeShader::pushUVec2(const std::string name, Uint32 x, Uint32 y) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 */
void ComputeShader::pushUVec3(const std::string name, Uint32 x, Uint32 y, Uint32 z) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 * @param z     The z-value for the vector
 * @param w     The z-value for the vector
 */
void ComputeShader::pushUVec4(const std::string name, Uint32 x, Uint32 y, Uint32 z, Uint32 w) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL color variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param color The value for the uniform
 */
void ComputeShader::pushColor4f(const std::string name, const Color4f& color) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL color variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param red   The uniform red value
 * @param green The uniform green value
 * @param blue  The uniform blue value
 * @param alpha The uniform alpha value
 */
void ComputeShader::pushColor4f(const std::string name, float red, float green, float blue, float alpha) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL ucolor variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param color The value for the uniform
 */
void ComputeShader::pushColor4(const std::string name, const Color4 color) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL ucolor variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param red   The uniform red value
 * @param green The uniform green value
 * @param blue  The uniform blue value
 * @param alpha The uniform alpha value
 */
void ComputeShader::pushColor4(const std::string name, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 2x2 matrix.
 *
 * The 2x2 matrix will be the non-translational part of the affine transform.
 * This method will fail if the uniform is not a GLSL mat2 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void ComputeShader::pushMat2(const std::string name, const Affine2& mat) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 2x2 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat2 variable.
 *
 * Note that uniforms can only be pushed during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void ComputeShader::pushMat2(const std::string name, const float* array) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 3x3 matrix.
 *
 * The 3x3 matrix will be affine transform in homoegenous coordinates.
 * This method will fail if the uniform is not a GLSL mat3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void ComputeShader::pushMat3(const std::string name, const Affine2& mat) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 3x3 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat3 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void ComputeShader::pushMat3(const std::string name, const float* array) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 4x4 matrix
 *
 * This will fail if the uniform is not a GLSL mat4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void ComputeShader::pushMat4(const std::string name, const Mat4& mat) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to a 4x4 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat4 variable.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void ComputeShader::pushMat4(const std::string name, const float* array) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Sets the given uniform to the provided array.
 *
 * The array data should have the size of the uniform previously declared.
 * If not, the results of this method are undefined.
 *
 * Note that this method does nothing if it is not invoked during an active
 * {@link #begin}/{@link #end} pair. This method does not cache uniforms
 * for later.
 *
 * @param name  The uniform name
 * @param data  The uniform data
 */
void ComputeShader::push(const std::string name, Uint8* data) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}
    
#pragma mark -
#pragma mark Computation
/**
 * Begins a dispatch pass with this shader for the application.
 *
 * Once this method is called, no other compute shader can perform a
 * computation pass. You must complete this computation pass with
 * {@link #end} before starting a new pass.
 *
 * Note that it is only safe to call this method within the scope of the
 * {@link Application#update} method, or any of the deterministic update
 * methods such as {@link Application#preUpdate}, {@link Application#fixedUpdate},
 * or {@link Application#postUpdate}. It is not safe to call this method
 * during {@link Application#draw}. This is due to limitations on the
 * implicit rendering queue in CUGL.
 */
void ComputeShader::begin() {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Ends the render pass for this shader.
 *
 * Once this method is called, it is safe to start a render pass with
 * another shader. This method has no effect if there is no active
 * render pass for this shader.
 */
void ComputeShader::end() {
	return;
}

/**
 * Dispatches a global workgroup on this compute shader
 *
 * Each workgroup with have {@link #getDimension} many local threads. The
 * dimension must be non-zero along each axis
 *
 * @param x Number of workgroups along the x dimension
 * @param y Number of workgroups along the y dimension
 * @param z Number of workgroups along the z dimension
 */
void ComputeShader::dispatch(Uint32 x, Uint32 y, Uint32 z) {
	CUAssertLog(false, "OpenGL does not support compute shaders.");
}

/**
 * Updates all descriptors and buffers before drawing.
 */
void ComputeShader::flush() {
	return;
}

