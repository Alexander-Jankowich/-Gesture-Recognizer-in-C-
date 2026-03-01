//
//  CUParticleShader.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a lightweight subclass of GraphicsShader the simplifies
//  shader configutation and compilation. In general, it is a model for our
//  narrow-purpose shaders.
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
//  Version: 12/2/25 (SDL3 Integration)
//
#include <cugl/graphics/CUParticleShader.h>
#include <cugl/graphics/CUParticleSystem.h>

using namespace cugl;
using namespace cugl::graphics;


/** Billboard fragment shader */
const std::string partShaderFrag =
#include "shaders/ParticleShader.frag"
;

/** Billboard vertex shader */
const std::string partShaderVert =
#include "shaders/ParticleShader.vert"
;

#pragma mark Constructors
/**
 * Creates an uninitialized shader with no source.
 *
 * You must initialize the shader for it to be compiled.
 */
ParticleShader::ParticleShader() : GraphicsShader() {
}

/**
 * Deletes the shader program and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void ParticleShader::dispose() {
    GraphicsShader::dispose();
}

/**
 * Initializes this shader with the standard vertex and fragment source.
 *
 * The shader will compile the vertex and fragment sources and link
 * them together. When compilation is complete, the shader will be
 * bound and active. In addition, all uniforms will be validated.
 *
 * @return true if initialization was successful.
 */
bool ParticleShader::init() {
    ShaderSource vsource;
    ShaderSource fsource;
    
    vsource.initOpenGL(partShaderVert, ShaderStage::VERTEX);
    fsource.initOpenGL(partShaderFrag, ShaderStage::FRAGMENT);

    return init(vsource,fsource);
}

/**
 * Initializes this shader with the given vertex and fragment source.
 *
 * The shader will compile the vertex and fragment sources and link
 * them together. When compilation is complete, the shader will be
 * bound and active. In addition, all uniforms will be validated.
 *
 * @param vsource   The source for the vertex shader.
 * @param fsource   The source for the fragment shader.
 *
 * @return true if initialization was successful.
 */
bool ParticleShader::init(const ShaderSource& vsource, const ShaderSource& fsource) {
    if (!GraphicsShader::init(vsource, fsource)) {
        return false;
    }
    
    // Set up the uniforms
    UniformDef cameraRightDef;
    cameraRightDef.type = GLSLType::VEC3;
    cameraRightDef.size = sizeof(Vec3);
    cameraRightDef.location = 0;
    cameraRightDef.stage = ShaderStage::VERTEX;
    declareUniform("uCameraRight", cameraRightDef);

    UniformDef cameraUpDef;
    cameraUpDef.type = GLSLType::VEC3;
    cameraUpDef.size = sizeof(Vec3);
    cameraUpDef.location = 1;
    cameraUpDef.stage = ShaderStage::VERTEX;
    declareUniform("uCameraUp", cameraUpDef);

    UniformDef perspectiveDef;
    perspectiveDef.type = GLSLType::MAT4;
    perspectiveDef.size = sizeof(Mat4);
    perspectiveDef.location = 2;
    perspectiveDef.stage = ShaderStage::VERTEX;
    declareUniform("uPerspective", perspectiveDef);

    UniformDef modelMatrixDef;
    modelMatrixDef.type = GLSLType::MAT4;
    modelMatrixDef.size = sizeof(Mat4);
    modelMatrixDef.location = 3;
    modelMatrixDef.stage = ShaderStage::VERTEX;
    declareUniform("uModelMatrix", modelMatrixDef);

    ResourceDef textureDef;
    textureDef.type = ResourceType::TEXTURE;
    textureDef.set = 0;
    textureDef.location = 0;
    textureDef.stage = ShaderStage::FRAGMENT;
    declareResource("uTexture", textureDef);
    
    // Now the attributes
    setAttributeStride(0, sizeof(ParticleVertex));
    setAttributeStride(1, sizeof(ParticleInstance));
    setInstanced(1, true);
    
    AttributeDef positionDef;
    positionDef.type = GLSLType::VEC3;
    positionDef.location = 0;
    positionDef.group = 0;
    positionDef.offset = offsetof(ParticleVertex,position);
    declareAttribute("aPosition", positionDef);

    AttributeDef texCoordDef;
    texCoordDef.type = GLSLType::VEC2;
    texCoordDef.location = 1;
    texCoordDef.group = 0;
    texCoordDef.offset = offsetof(ParticleVertex,texCoord);
    declareAttribute("aTexCoord", texCoordDef);

    AttributeDef centerDef;
    centerDef.type = GLSLType::VEC4;
    centerDef.location = 2;
    centerDef.group = 1;
    centerDef.offset = offsetof(ParticleInstance,position);
    declareAttribute("aCenter", centerDef);

    AttributeDef colorDef;
    colorDef.type = GLSLType::UCOLOR;
    colorDef.location = 3;
    colorDef.group = 1;
    colorDef.offset = offsetof(ParticleInstance,color);
    declareAttribute("aColor", colorDef);

    AttributeDef offsetDef;
    offsetDef.type = GLSLType::VEC2;
    offsetDef.location = 3;
    offsetDef.group = 1;
    offsetDef.offset = offsetof(ParticleInstance,texOffset);
    declareAttribute("aOffset", offsetDef);

    // Compile it
    return compile();
}

/**
 * Sets the texture for this shader.
 *
 * This method will only succeed if the shader is in an active draw pass.
 *
 * @param texture   The texture to use
 */
void ParticleShader::setTexture(const std::shared_ptr<Texture>& texture) {
    if (texture != nullptr) {
        GraphicsShader::setTexture("uTexture", texture);
    } else {
        GraphicsShader::setTexture("uTexture", Texture::getBlank());
    }
}
