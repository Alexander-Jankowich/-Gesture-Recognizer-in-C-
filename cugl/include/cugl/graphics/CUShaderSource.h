//
//  CUShaderSource.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a flexible representation of shader source code. As
//  GLSL is different for both OpenGL and Vulkan (and Vulkan shaders must be
//  converted to bytecode), we wanted a way to support both types of shaders
//  in a game so that students can support a wider variety of platforms.
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
#ifndef __CU_SHADER_SOURCE_H__
#define __CU_SHADER_SOURCE_H__
#include <string>
#include <vector>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/assets/CUJsonValue.h>

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

/**
 * This class represents source code for a GLSL shader.
 *
 * This class provides us with a flexible way to support both OpenGL and Vulkan
 * shaders. In particular, it stores the source code for both -- OpenGL shaders
 * as a string and Vulkan shaders as byte code. Note that these shaders have
 * to be loaded separately, as Vulkan GLSL is not the same as OpenGL GLSL.
 *
 * Note that CUGL OpenGL shaders should *not* start with a \#version directive.
 * That information will be appended by this class, as the correct version
 * depends on the platform (OpenGL or OpenGLES).
 *
 * CUGL assumes that Vulkan shaders have already been coverted to bytecode via
 * a SpirV compiler offline. CUGL does not include a SpirV compiler, as that
 * can add up to 1 GB to the application footprint.
 *
 * ShaderModules do not check source for validity. That is done at compilation
 * time for either {@link GraphicsShader} or {@link ComputeShader}.
 *
 * Note that while this class can theortically represent any stage in a graphics
 * pipeline, CUGL only support the shader stages VERTEX, FRAGMENT, and COMPUTE.
 */
class ShaderSource {
private:
    /** The shader stage associated with this source */
    ShaderStage _stage;
    /** The OpenGL shader source */
    std::string _opengl;
    /** The Vulkan shader bytecode */
    std::vector<std::byte> _vulkan;

public:
#pragma mark Constructors
    /**
     * Creates a empty shader module with no source code
     *
     * The stage is undefined. You must initialize this shader module to use it.
     */
    ShaderSource() {
        _stage = ShaderStage::UNDEFINED;
    }
    
    /**
     * Deletes this shader module, disposing all resources.
     */
    ~ShaderSource() { dispose(); }
    
    /**
     * Disposes the shader module, freeing all resources.
     *
     * You must reinitialize the shader module to use it.
     */
    void dispose() {
        _vulkan.clear();
        _opengl.clear();
        _stage = ShaderStage::UNDEFINED;
    }

    /**
     * Initializes a shader module with the given stage
     *
     * The shader has no source. It expects that source to be loaded separately.
     *
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool init(ShaderStage stage);

    /**
     * Initializes an OpenGL shader module with the given source
     *
     * The string should be the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).
     *
     * The Vulkan shader will be blank until it is set.
     *
     * @param source    The shader source
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initOpenGL(const std::string source, ShaderStage stage);
    
    /**
     * Initializes an OpenGL shader module from the given file.
     *
     * The file should contain the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).

     * The Vulkan shader will be blank until it is set.
     *
     * @param file      The file with the shader source
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initOpenGLFromFile(const std::string file, ShaderStage stage);

    /**
     * Initializes a Vulkan shader module with the given bytecode
     *
     * The OpenGL shader will be blank until it is set.
     *
     * @param bytes     The shader bytecode
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initVulkan(const std::vector<std::byte>& bytes, ShaderStage stage);
    
    /**
     * Initializes a Vulkan shader module with the given bytecode
     *
     * This method will take ownership of the given bytecode. The OpenGL
     * shader will be blank until it is set.
     *
     * @param bytes     The shader bytecode
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initVulkan(std::vector<std::byte>&& bytes, ShaderStage stage);

    /**
     * Initializes a Vulkan shader module with the given source
     *
     * The string for the Vulkan shader is assumed to be a Base 64 encoding of
     * the byte code
     *
     * https://en.wikipedia.org/wiki/Base64
     *
     * The OpenGL shader will be blank until it is set.
     *
     * @param source    The shader source in Base 64
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initVulkan(std::string source, ShaderStage stage);

    /**
     * Initializes a Vulkan shader module from the given file.
     *
     * The file should contain the bytecode of a Vulkan shader in binary format.
     * The OpenGL shader will be blank until it is set.
     *
     * @param file      The file with the shader source
     * @param stage     The shader stage
     *
     * @return true if initialization was successful
     */
    bool initVulkanFromFile(const std::string file, ShaderStage stage);
    
    /**
     * Initializes a shader module with the given JSON specificaton.
     *
     * This initializer is designed to allow a shader module to load both an
     * OpenGL shader and a Vulkan shader. It is the responsibility of the
     * developer to ensure these shaders are compatible.
     *
     * This initializer is typically passed to the object via an
     * {@link AssetManager}. This JSON format supports the following
     * attribute values:
     *
     *      "stage":    One of "vertex", "fragment", or "compute"
     *      "opengl":   An OpenGL shader object
     *      "vulkan":   A Vulkan shader object
     *
     * Shader objects have exactly one of the two attributes
     *
     *      "file":     The path to the shader code
     *      "source":   The shader source embedded in the JSON object
     *
     * The shader source will be a string for both OpenGL and Vulkan shaders.
     * In the case of Vulkan, this string will be a Base 64 encoding of the
     * byte code
     *
     * https://en.wikipedia.org/wiki/Base64
     *
     * @param source    The JSON object specifying the shader
     *
     * @return true if initialization was successful.
     */
    bool initFromSource(const std::shared_ptr<JsonValue>& source);
    
    /**
     * Returns a newly allocated shader module with the given stage
     *
     * The shader has no source. It expects that source to be loaded separately.
     *
     * @param stage     The shader stage
     *
     * @return a newly allocated shader module with the given stage
     */
    static std::shared_ptr<ShaderSource> alloc(ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->init(stage) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated OpenGL shader module with the given source
     *
     * The string should be the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).
     *
     * The Vulkan shader will be blank until it is set.
     *
     * @param source    The shader source
     * @param stage     The shader stage
     *
     * @return a newly allocated OpenGL shader module with the given source
     */
    static std::shared_ptr<ShaderSource> allocOpenGL(const std::string source, ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initOpenGL(source,stage) ? result : nullptr);
    }

    /**
     * Returns a newly allocated OpenGL shader module from the given file.
     *
     * The file should contain the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).
     *
     * The Vulkan shader will be blank until it is set.
     *
     * @param file      The shader source
     * @param stage     The shader stage
     *
     * @return a newly allocated OpenGL shader module from the given file.
     */
    static std::shared_ptr<ShaderSource> allocOpenGLFromFile(const std::string file, ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initOpenGLFromFile(file,stage) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated Vulkan shader module with the given bytecode
     *
     * The Vulkan shader will be blank until it is set.
     *
     * @param bytes     The shader bytecode
     * @param stage     The shader stage
     *
     * @return a newly allocated Vulkan shader module with the given bytecode
     */
    static std::shared_ptr<ShaderSource> allocVulkan(const std::vector<std::byte>& bytes, ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initVulkan(bytes,stage) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated Vulkan shader module with the given bytecode
     *
     * This method will take ownership of the given bytecode. The Vulkan
     * shader will be blank until it is set.
     *
     * @param bytes     The shader bytecode
     * @param stage     The shader stage
     *
     * @return a newly allocated Vulkan shader module with the given bytecode
     */
    static std::shared_ptr<ShaderSource> allocVulkan(std::vector<std::byte>&& bytes, ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initVulkan(std::move(bytes),stage) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated Vulkan shader module from the given file.
     *
     * The file should contain the bytecode of a Vulkan shader. The OpenGL
     * shader will be blank until it is set.
     *
     * @param file      The shader source
     * @param stage     The shader stage
     *
     * @return a newly allocated Vulkan shader module from the given file.
     */
    static std::shared_ptr<ShaderSource> allocVulkanFromFile(const std::string file, ShaderStage stage) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initVulkanFromFile(file,stage) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated shader module with the given JSON specificaton.
     *
     * This initializer is designed to allow a shader module to load both an
     * OpenGL shader and a Vulkan shader. It is the responsibility of the
     * developer to ensure these shaders are compatible.
     *
     * This initializer is typically passed to the object via an
     * {@link AssetManager}. This JSON format supports the following
     * attribute values:
     *
     *      "stage":    One of "vertex", "fragment", or "compute"
     *      "opengl":   An OpenGL shader object
     *      "vulkan":   A Vulkan shader object
     *
     * Shader objects have exactly one of the two attributes
     *
     *      "file":     The path to the shader code
     *      "source":   The shader source embedded in the JSON object
     *
     * The shader source will be a string for both OpenGL and Vulkan shaders.
     * In the case of Vulkan, this string will be a Base 64 encoding of the
     * byte code
     *
     * https://en.wikipedia.org/wiki/Base64
     *
     * @param source    The JSON object specifying the shader source
     *
     * @return a newly allocated shader module with the given JSON specificaton.
     */
    static std::shared_ptr<ShaderSource> allocFromSource(const std::shared_ptr<JsonValue>& source) {
        std::shared_ptr<ShaderSource> result = std::make_shared<ShaderSource>();
        return (result->initFromSource(source) ? result : nullptr);
    }
    
#pragma mark Sources
    /**
     * Returns the shader stage for this module.
     *
     * CUGL only supports the stages VERTEX, FRAGMENT, and COMPUTE.
     *
     * @return the shader stage for this module.
     */
    ShaderStage getStage() const { return _stage; }
  
    /**
     * Sets the shader stage for this module.
     *
     * CUGL only supports the stages VERTEX, FRAGMENT, and COMPUTE.
     *
     * @param stage The shader stage for this module.
     */
    void setStage(ShaderStage stage) { _stage = stage; }
    
    /**
     * Returns the OpenGL source for this shader module
     *
     * The returned string will include a \#version directive chosen for the
     * current platform (OpenGL or OpenGLES).
     *
     * @return the OpenGL source for this shader module
     */
    const std::string getOpenGL() const { return _opengl; }
    
    /**
     * Sets the OpenGL source for this shader module
     *
     * The string should be the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).
     *
     * @param source    The shader source
     */
    void setOpenGL(const std::string source);
    
    /**
     * Loads an OpenGL shader from the given file.
     *
     * The file should contain the source of a GLSL shader, but it should not
     * start with a \#version directive. That information will be appened to
     * the string as it depends on the platform (OpenGL or OpenGLES).
     *
     * @param file      The file with the shader source
     *
     * @return true if the file could be read
     */
    bool loadOpenGL(const std::string file);

    /**
     * Returns the Vulkan bytecode for this shader module
     *
     * @return the Vulkan bytecode for this shader module
     */
    const std::vector<std::byte>& getVulkan() const { return _vulkan; }

    /**
     * Sets the Vulkan bytecode for this shader module
     *
     * The string for the Vulkan shader is assumed to be a Base 64 encoding of
     * the byte code
     *
     * https://en.wikipedia.org/wiki/Base64
     *
     * @param source    The shader source in Base 64
     */
    void setVulkan(const std::string source);

    /**
     * Sets the Vulkan bytecode for this shader module
     *
     * @param bytes The shader bytecode
     */
    void setVulkan(const std::vector<std::byte>& bytes);

    /**
     * Sets the Vulkan bytecode for this shader module
     *
     * This method will take ownership of the given bytecode.
     *
     * @param bytes The shader bytecode
     */
    void setVulkan(std::vector<std::byte>&& bytes);

    /**
     * Loads a Vulkan shader from the given file.
     *
     * The file should contain the bytecode of a Vulkan shader in binary format.
     *
     * @param file      The file with the shader source
     *
     * @return true if the file could be read
     */
    bool loadVulkan(const std::string file);
    
};

    }
}


#endif /* __CU_SHADER_SOURCE_H__ */
