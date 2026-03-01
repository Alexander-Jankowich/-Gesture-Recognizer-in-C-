//
//  CUShaderModule.h
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
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUShaderSource.h>
#include <cugl/core/io/CUBinaryReader.h>
#include <cugl/core/io/CUTextReader.h>
#include <cugl/core/util/CUFiletools.h>
#include <cugl/core/util/CUHashtools.h>
#include <algorithm>

using namespace cugl;
using namespace cugl::graphics;

#ifdef CU_MOBILE
    #define SHADER(A) ("#version 300 es\n#define CUGLES 1\n"+A)
#else
    #define SHADER(A) ("#version 330\n"+A)
#endif


/**
 * Initializes a shader module with the given stage
 *
 * The shader has no source. It expects that source to be loaded separately.
 *
 * @param stage     The shader stage
 *
 * @return true if initialization was successful
 */
bool ShaderSource::init(ShaderStage stage) {
    _stage = stage;
    return true;
}

/**
 * Initializes an OpenGL shader module with the given source
 *
 * The string should be the source of a GLSL shader, but it should not
 * start with a #version directive. That information will be appened to
 * the string as it depends on the platform (OpenGL or OpenGLES).
 *
 * The Vulkan shader will be blank until it is set.
 *
 * @param source    The shader source
 * @param stage     The shader stage
 *
 * @return true if initialization was successful
 */
bool ShaderSource::initOpenGL(const std::string source, ShaderStage stage) {
    _stage = stage;
    setOpenGL(source);
    return true;
}

/**
 * Initializes an OpenGL shader module from the given file.
 *
 * The file should contain the source of a GLSL shader, but it should not
 * start with a #version directive. That information will be appened to
 * the string as it depends on the platform (OpenGL or OpenGLES).

 * The Vulkan shader will be blank until it is set.
 *
 * @param file      The file with the shader source
 * @param stage     The shader stage
 *
 * @return true if initialization was successful
 */
bool ShaderSource::initOpenGLFromFile(const std::string file, ShaderStage stage) {
    _stage = stage;
    return loadOpenGL(file);
}

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
bool ShaderSource::initVulkan(const std::vector<std::byte>& bytes, ShaderStage stage) {
    _vulkan = bytes;
    _stage = stage;
    return true;
}

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
bool ShaderSource::initVulkan(std::vector<std::byte>&& bytes, ShaderStage stage) {
    _vulkan = std::move(bytes);
    _stage = stage;
    return true;
}

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
bool ShaderSource::initVulkan(std::string source, ShaderStage stage) {
    setVulkan(source);
    _stage = stage;
    return true;
}

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
bool ShaderSource::initVulkanFromFile(const std::string file, ShaderStage stage) {
    _stage = stage;
    return loadVulkan(file);
}

/**
 * Initializes a shader module with the given JSON specificaton.
 *
 * This initializer is designed to allow a shader module to load both an
 * OpenGL shader and a Vulkan shader. It is the responsibility of the
 * developer to ensure these shaders are compatible.
 *
 * This initializer is typically passed to the object via an
 * {@link AssetDirectory}. This JSON format supports the following
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
bool ShaderSource::initFromSource(const std::shared_ptr<JsonValue>& source) {
    if (source->has("stage")) {
        std::string stage = source->getString("stage");
        std::transform(stage.begin(), stage.end(), stage.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (stage == "vertex") {
            _stage = ShaderStage::VERTEX;
        } else if (stage == "fragment") {
            _stage = ShaderStage::FRAGMENT;
        } else if (stage == "compute") {
            _stage = ShaderStage::COMPUTE;
        } else {
            _stage = ShaderStage::UNDEFINED;
        }
    } else {
        _stage = ShaderStage::UNDEFINED;
    }
    
    bool success = true;
    if (source->has("opengl")) {
        JsonValue* child = source->get("opengl").get();
        if (child->has("file")) {
            success = loadOpenGL(child->getString("file")) && success;
        } else if (child->has("source")) {
            setOpenGL(child->getString("source"));
        }
    }
    
    if (source->has("vulkan")) {
        JsonValue* child = source->get("vulkan").get();
        if (child->has("file")) {
            success = loadVulkan(child->getString("file")) && success;
        } else if (child->has("source")) {
            setVulkan(child->getString("source"));
        }
    }
    return true;
}

#pragma mark -
#pragma mark Sources
/**
 * Sets the OpenGL source for this shader module
 *
 * The string should be the source of a GLSL shader, but it should not
 * start with a #version directive. That information will be appened to
 * the string as it depends on the platform (OpenGL or OpenGLES).
 *
 * @param source    The shader source
 */
void ShaderSource::setOpenGL(const std::string source) {
    _opengl = SHADER(source);
}

/**
 * Loads an OpenGL shader from the given file.
 *
 * The file should contain the source of a GLSL shader, but it should not
 * start with a #version directive. That information will be appened to
 * the string as it depends on the platform (OpenGL or OpenGLES).
 *
 * @param file      The file with the shader source
 *
 * @return true if the file could be read
 */
bool ShaderSource::loadOpenGL(const std::string file) {
    TextReader reader;
    if (filetool::is_absolute(file)) {
        reader.init(file);
    } else {
        reader.initWithAsset(file);
    }
    
    if (!reader.ready()) {
        return false;
    }
    
    _opengl = SHADER(reader.readAll());
    reader.close();
    return true;
}

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
void ShaderSource::setVulkan(const std::string source) {
    _vulkan = hashtool::b64_decode(source);
}

/**
 * Sets the Vulkan bytecode for this shader module
 *
 * @param bytes The shader bytecode
 */
void ShaderSource::setVulkan(const std::vector<std::byte>& bytes) {
    _vulkan = bytes;
}

/**
 * Sets the Vulkan bytecode for this shader module
 *
 * This method will take ownership of the given bytecode.
 *
 * @param bytes The shader bytecode
 */
void ShaderSource::setVulkan(std::vector<std::byte>&& bytes) {
    _vulkan = std::move(bytes);
}

/**
 * Loads a Vulkan shader from the given file.
 *
 * The file should contain the bytecode of a Vulkan shader in binary format.
 *
 * @param file      The file with the shader source
 *
 * @return true if the file could be read
 */
bool ShaderSource::loadVulkan(const std::string file) {
    BinaryReader reader;
    if (filetool::is_absolute(file)) {
        reader.init(file);
    } else {
        reader.initWithAsset(file);
    }
    
    if (!reader.ready()) {
        return false;
    }
    
    _vulkan = reader.readBytes();
    reader.close();
    return true;
}
