//
//  CUComputeShader.h
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
//  Version: 5/8/25 (Vulkan Integration)
//
#ifndef __CU_COMPUTE_SHADER_H__
#define __CU_COMPUTE_SHADER_H__
#include <cugl/core/CUBase.h>
#include <cugl/core/math/cu_math.h>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUShaderSource.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#pragma mark -
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

// Opaque classes for the graphics API
class ComputeShaderData;
class ResourceData;
class ImageData;

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

/**
 * This class defines a GLSL compute shader.
 *
 * VULKAN ONLY: This class compiles any well-defined GLSL compute shader. Note
 * that CUGL only supports compute shaders in Vulkan (due to the OpenGL
 * limitations of Apple platforms). Attempts to allocate an object of this type
 * in OpenGL will fail.
 *
 * Compute shaders only have a single shader module. For input, they support
 * push constants, and any {@link ResourceType}. For output, we only support
 * {@link StorageBuffer}, {@link VertexBuffer}, {@link IndexBuffer} and
 * {@link Image} objects with compute access.
 *
 * Note that it is never safe for input and output to be the same resources.
 * If you need to update a data-set, such as a storage buffer containing
 * particles, you should set up a ping-pong buffer to guarantee safety.
 *
 * While Vulkan is multithreaded, CUGL shader objects can only be used on the
 * main thread (due to synchronization constraints). Using them outside of the
 * main application loop is undefined. In addition, compute shaders may not
 * be used in the {@link Application#draw} method. They can only be used in
 * one of the update methods such as {@link Application#update},
 * {@link Application#preUpdate}, {@link Application#fixedUpdate} or 
 * {@link Application#postUpdate}.
 */
class ComputeShader : public std::enable_shared_from_this<ComputeShader> {
#pragma mark Values
protected:
    /** The active shader (if any) */
    static std::shared_ptr<ComputeShader> g_shader;
    
    /** The graphics API implementation of this shader */
    ComputeShaderData* _data;
    /** The source for the shader module */
    ShaderSource _compModule;
    
    /** The work group dimension */
    IVec3 _dimension;

    /** The specialization constant declarations */
    std::unordered_map<std::string,GLSLConstant> _constants;
    /** The push-constant definitions of this shader */
    std::unordered_map<std::string, UniformDef> _uniforms;
    /** The resource input definitions of this shader */
    std::unordered_map<std::string, ResourceDef> _inputs;
    /** The resource output definitions of this shader */
    std::unordered_map<std::string, ResourceDef> _outputs;
    /** The images that need to be transitioned */
	std::vector<ImageData*> _transitions;
    
	/**
     * The specialization constant values
     *
     * This is the data for the specialization constants. The offsets
     * can be determined by the declarations, which are separate.
     */
    std::vector<uint8_t> _constvalues;

    /** The currently attached input resources */
    std::unordered_map<std::string, ResourceData*> _invalues;
    /** The currently attached output resources */
    std::unordered_map<std::string, ResourceData*> _outvalues;

    /**
     * Create the input data structures for the given definition
     *
     * @param name  The resource name
     * @param def   The resource definition
     */
    void createInputs(const std::string name, ResourceDef def);

    /**
     * Create the output data structures for the given definition
     *
     * @param name  The resource name
     * @param def   The resource definition
     */
    void createOutputs(const std::string name, ResourceDef def);

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized compute shader with no source.
     *
     * You must initialize the shader to add a source and compile it.
     */
    ComputeShader() : _data(nullptr) {};
    
    /**
     * Deletes this shader, disposing all resources.
     */
    ~ComputeShader() { dispose(); }
    
    /**
     * Deletes the shader and resets all attributes.
     *
     * You must reinitialize the shader to use it.
     */
    virtual void dispose();
    
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
    virtual bool init(const ShaderSource& source);
    
    /**
     * Returns a new compute shader with the given shader source.
     *
     * This method will allocate resources for a shader, but it will not
     * compile it. You must all the {@link compile} method to compile the
     * shader.
     *
     * @param source    The source for the compute shader.
     *
     * @return a new compute shader with the given vertex and fragment source.
     */
    static std::shared_ptr<ComputeShader> alloc(const ShaderSource& source) {
        std::shared_ptr<ComputeShader> result = std::make_shared<ComputeShader>();
        return (result->init(source) ? result : nullptr);
    }
    
    
#pragma mark -
#pragma mark Compilation
    /**
     * Returns the local group dimension of this compute shader
     *
     * Setting this value has no effect on the compute shader. The dimension of
     * a compute shader is entirely deteremined by its shader source. However,
     * setting this value is useful for debugging.
     *
     * @return the local group dimension of this compute shader
     */
    IVec3 getDimension() const { return _dimension; }

    /**
     * Sets the local group dimension of this compute shader
     *
     * Setting this value has no effect on the compute shader. The dimension of
     * a compute shader is entirely deteremined by its shader source. However,
     * setting this value is useful for debugging.
     *
     * @param dimension The local group dimension
     */
    void setDimension(const IVec3 dimension) {
    	_dimension = dimension;
    }
    
    /**
     * Sets the local group dimension of this compute shader
     *
     * Setting this value has no effect on the compute shader. The dimension of
     * a compute shader is entirely deteremined by its shader source. However,
     * setting this value is useful for debugging.
     *
     * @param x     The local group size in the x-dimension
     * @param y     The local group size in the y-dimension
     * @param z     The local group size in the z-dimension
     */
    void setDimension(Uint32 x, Uint32 y, Uint32 z) {
    	_dimension.set(x,y,z);
    }
    
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
    bool declareUniform(const std::string name, const UniformDef& def);

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
    const UniformDef* getUniformDef(const std::string name) const;
    
    /**
     * Declares and sets a specialization constant
     *
     * Specialization constants are assigned to the compute shader at compile
     * time. This value can be reassigned any time before a call to 
     * {@link #compile}, but cannot be changed once the compute shader is 
     * complete.
     * 
     * @param name  The constant name
     * @param spec  The specialization constant
     */
    void setConstant(const std::string name, const GLSLConstant& spec);
    
    /**
     * Returns the information for a specialization constant
     *
     * Specialization constants are assigned to the compute shader at compile
     * time. This value can be reassigned any time before a call to 
     * {@link #compile}, but cannot be changed once the compute shader is
     * complete.
     *
     * If there is no constant associated with the given name, this method
     * returns nullptr.
     *
     * @param name  The constant name
     *
     * @return the information for a specialization constant
     */
    const GLSLConstant* getConstant(const std::string name) const;
    
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
    bool declareInput(const std::string name, const ResourceDef& def);

    /**
     * Declares an shader ouput uniform.
     *
     * Output uniforms are resources that can be written to by a compute shader.
     * Currently we only support {@link Image}, {@link VertexBuffer},
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
    bool declareOutput(const std::string name, const ResourceDef& def);
    
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
    const ResourceDef* getResourceDef(const std::string name) const;
    
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
    bool isWriteable(const std::string name) const;
    
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
    bool compile();
    
    /**
     * Returns true if this shader has been compiled and is ready for use.
     *
     * @return true if this shader has been compiled and is ready for use.
     */
    bool isReady() const;
    
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
    void setUniformBuffer(const std::string name, const std::shared_ptr<UniformBuffer>& buffer);

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
    void setUniformBuffers(const std::string name, const std::vector<std::shared_ptr<UniformBuffer>>& buffers);

    /**
     * Returns the uniform buffer for the given resource variable
     *
     * If the variable does not refer to a uniform buffer then this method will
     * return nullptr. If more than one buffer is assigned to this attribute,
     * then this method will return the first one.
     *
     * Any buffer assigned to a name is stateful, in that it will persist across
     * compute passes. Note that uniform buffers are always read-only in CUGL
     * compute shaders.
     *
     * @param name      The name of the resource
     *
     * @return the uniform buffer for the given resource variable
     */
    const std::shared_ptr<UniformBuffer> getUniformBuffer(const std::string name) const;
    
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
    const std::vector<std::shared_ptr<UniformBuffer>> getUniformBuffers(const std::string name) const;
    
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
    void setStorageBuffer(const std::string name, const std::shared_ptr<StorageBuffer>& buffer);
    
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
    void setStorageBuffers(const std::string name, const std::vector<std::shared_ptr<StorageBuffer>>& buffers);
    
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
    const std::shared_ptr<StorageBuffer> getStorageBuffer(const std::string name) const;

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
    const std::vector<std::shared_ptr<StorageBuffer>> getStorageBuffers(const std::string name) const;
    
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
    void setBlock(const std::string name, Uint32 block);
    
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
    Uint32 getBlock(const std::string name) const;

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
    void setVertexBuffer(const std::string name, const std::shared_ptr<VertexBuffer>& buffer);

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
    const std::shared_ptr<VertexBuffer> getVertexBuffer(const std::string name) const;
    
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
    void setIndexBuffer(const std::string name, const std::shared_ptr<IndexBuffer>& buffer);

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
    const std::shared_ptr<IndexBuffer> getIndexBuffer(const std::string name) const;
    
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
    void setTexture(const std::string name, const std::shared_ptr<Texture>& texture);

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
    void setTextures(const std::string name, const std::vector<std::shared_ptr<Texture>>& textures);
        
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
    const std::shared_ptr<Texture> getTexture(const std::string name) const;

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
    const std::vector<std::shared_ptr<Texture>> getTextures(const std::string name) const;

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
    void setTextureArray(const std::string name, const std::shared_ptr<TextureArray>& array);

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
    void setTextureArrays(const std::string name, const std::vector<std::shared_ptr<TextureArray>>& arrays);
        
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
    const std::shared_ptr<TextureArray> getTextureArray(const std::string name) const;

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
    const std::vector<std::shared_ptr<TextureArray>> getTextureArrays(const std::string name) const;
    
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
    void setImage(const std::string name, const std::shared_ptr<Image>& image);

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
    void setImages(const std::string name, const std::vector<std::shared_ptr<Image>>& images);

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
    const std::shared_ptr<Image> getImage(const std::string name);

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
    const std::vector<std::shared_ptr<Image>> getImages(const std::string name) const;

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
    void setImageArray(const std::string name, const std::shared_ptr<ImageArray>& array);

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
    void setImageArrays(const std::string name, const std::vector<std::shared_ptr<ImageArray>>& arrays);

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
    const std::shared_ptr<ImageArray> getImageArray(const std::string name) const;

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
    const std::vector<std::shared_ptr<ImageArray>> getImageArrays(const std::string name) const;

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
    void setSampler(const std::string name, const std::shared_ptr<Sampler>& sampler);

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
     * Setting this value is stateful, in that it will persist across compute
     * passes. Note that samplers are read-only in compute shaders.
     *
     * @param name      The name of the resource
     * @param samplers  The samplers for the resource
     */
    void setSamplers(const std::string name, const std::vector<std::shared_ptr<Sampler>>& samplers);

    /**
     * Returns the sampler for the given resource variable
     *
     * If the variable does not refer to a sampler (a sampler in GLSL), then
     * this method will return nullptr. If more than one sampler is assigned to
     * this attribute, then this method will return the first one.
     *
     * Any sampler assigned to a name is stateful, in that it will persist
     * across compute passes. Note that samplers are read-only in compute
     * shaders.
     *
     * @param name  The name of the resource
     *
     * @return the sampler for the given resource variable
     */
    const std::shared_ptr<Sampler> getSampler(const std::string name);

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
    const std::vector<std::shared_ptr<Sampler>> getSamplers(const std::string name) const;

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
    void pushInt(const std::string name, Sint32 value);

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
    void pushUInt(const std::string name, Uint32 value);

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
    void pushFloat(const std::string name, float value);
    
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
    void pushVec2(const std::string name, const Vec2& vec);

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
    void pushVec2(const std::string name, float x, float y);
    
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
    void pushVec3(const std::string name, const Vec3& vec);

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
    void pushVec3(const std::string name, float x, float y, float z);
    
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
    void pushVec4(const std::string name, const Vec4& vec);

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
    void pushVec4(const std::string name, const Quaternion& quat);
    
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
    void pushVec4(const std::string name, float x, float y, float z, float w);
    
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
    void pushIVec2(const std::string name, Sint32 x, Sint32 y);
    
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
    void pushIVec3(const std::string name, Sint32 x, Sint32 y, Sint32 z);
    
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
    void pushIVec4(const std::string name, Sint32 x, Sint32 y, Sint32 z, Sint32 w);
    
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
    void pushUVec2(const std::string name, Uint32 x, Uint32 y);
    
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
    void pushUVec3(const std::string name, Uint32 x, Uint32 y, Uint32 z);
    
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
    void pushUVec4(const std::string name, Uint32 x, Uint32 y, Uint32 z, Uint32 w);
        
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
    void pushColor4f(const std::string name, const Color4f& color);

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
    void pushColor4f(const std::string name, float red, float green, float blue, float alpha);

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
    void pushColor4(const std::string name, const Color4 color);

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
    void pushColor4(const std::string name, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);
    
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
    void pushMat2(const std::string name, const Affine2& mat);
    
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
    void pushMat2(const std::string name, const float* array);
    
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
    void pushMat3(const std::string name, const Affine2& mat);
    
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
    void pushMat3(const std::string name, const float* array);
    
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
    void pushMat4(const std::string name, const Mat4& mat);
    
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
    void pushMat4(const std::string name, const float* array);
    
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
    void push(const std::string name, Uint8* data);
    
    /**
     * Sets the given uniform to the provided value.
     *
     * The data type should match up with the size of the uniform previously
     * declared. If not, the results of this method are undefined.
     *
     * Note that this method does nothing if it is not invoked during an active
     * {@link #begin}/{@link #end} pair. This method does not cache uniforms
     * for later.
     *
     * @param name  The uniform name
     * @param value The uniform value
     */
    template<typename T>
    void push(const std::string name, T value) {
        Uint8* data = reinterpret_cast<Uint8*>(&value);
        push(name,data);
    }
    
#pragma mark -
#pragma mark Computation
    /**
     * Begins a dispatch pass with this shader for the application.
     *
     * Once this method is called, no other compute shader can perform a
     * dispatch pass. You must complete this dispatch pass with {@link #end}
     * before starting a new pass.
     *
     * Note that it is only safe to call this method within the scope of the
     * {@link Application#update} method, or any of the deterministic update
     * methods such as {@link Application#preUpdate}, {@link Application#fixedUpdate},
     * or {@link Application#postUpdate}. It is not safe to call this method
     * during {@link Application#draw}. This is due to limitations on the
     * implicit rendering queue in CUGL.
     */
    void begin();

    /**
     * Ends the dispatch pass for this shader.
     *
     * Once this method is called, it is safe to start a dispatch pass with
     * another shader. This method has no effect if there is no active
     * dispatch pass for this shader.
     *
     * Note that once this method is called, it is not safe to make futher
     * modifications to any output resources.
     */
    void end();
    
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
    void dispatch(Uint32 x = 1, Uint32 y = 1, Uint32 z = 1);
    
    
protected:
    /**
     * Updates all descriptors and buffers before computation.
     */
    void flush();

};

    }

}

#endif /* __CU_COMPUTE_SHADER_H__ */
