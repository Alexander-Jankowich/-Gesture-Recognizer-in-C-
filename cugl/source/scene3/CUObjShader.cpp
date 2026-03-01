//
//  CUObjShader.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is a lightweight subclass of Shader that caches the uniform
//  locations, making it a little quicker to update their values.
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
#include <cugl/scene3/CUObjShader.h>
#include <cugl/scene3/CUObjModel.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::scene3;

// TODO: Will need to be moved in the refactor
/** OBJ fragment shader */
const std::string objShaderFrag =
#include "shaders/OBJShader.frag"
;

/** OBJ vertex shader */
const std::string objShaderVert =
#include "shaders/OBJShader.vert"
;

#pragma mark Constructors
/**
 * Creates an uninitialized shader with no source.
 *
 * You must initialize the shader for it to be compiled.
 */
ObjShader::ObjShader() : GraphicsShader() {
    // This is here only so that we can add caches later
}

/**
 * Deletes the shader program and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void ObjShader::dispose() {
    // This is here only so that we can add caches later
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
bool ObjShader::init() {
    ShaderSource vsource;
    ShaderSource fsource;
    vsource.initOpenGL(objShaderVert, ShaderStage::VERTEX);
    fsource.initOpenGL(objShaderFrag, ShaderStage::FRAGMENT);
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
bool ObjShader::init(const ShaderSource& vsource, const ShaderSource& fsource) {
    if (!GraphicsShader::init(vsource, fsource)) {
        return false;
    }
    
    setAttributeStride(0, sizeof(OBJVertex));

    // Now set up everything
    AttributeDef adef;
    adef.location = 0;
    adef.type = GLSLType::VEC3;
    adef.group = 0;
    adef.offset = offsetof(OBJVertex,position);
    declareAttribute("aPosition", adef);

    adef.location = 1;
    adef.type = GLSLType::VEC2;
    adef.offset = offsetof(OBJVertex,texcoord);
    declareAttribute("aTexCoord", adef);

    adef.location = 2;
    adef.type = GLSLType::VEC3;
    adef.offset = offsetof(OBJVertex,normal);
    declareAttribute("aNormal", adef);

    adef.location = 3;
    adef.type = GLSLType::VEC3;
    adef.offset = offsetof(OBJVertex,tangent);
    declareAttribute("aTangent", adef);

    UniformDef udef;
    udef.location = 0;
    udef.size = sizeof(Mat4);
    udef.type = GLSLType::MAT4;
    udef.stage = ShaderStage::VERTEX;
    declareUniform("uPerspective", udef);

    udef.location = 1;
    declareUniform("uModelMatrix", udef);

    udef.location = 2;
    declareUniform("uNormalMatrix", udef);

    udef.location = 3;
    udef.size = sizeof(Vec3);
    udef.type = GLSLType::VEC3;
    declareUniform("uLightPos", udef);

    udef.location = 4;
    udef.size = sizeof(int);
    udef.type = GLSLType::INT;
    udef.stage = ShaderStage::FRAGMENT;
    declareUniform("uIllum", udef);

    udef.location = 5;
    declareUniform("uHasKa", udef);

    udef.location = 6;
    declareUniform("uHasKd", udef);

    udef.location = 7;
    declareUniform("uHasKs", udef);

    udef.location = 8;
    declareUniform("uHasKn", udef);

    udef.location = 9;
    udef.size = sizeof(Vec4);
    udef.type = GLSLType::VEC4;
    declareUniform("uKa", udef);

    udef.location = 10;
    declareUniform("uKd", udef);

    udef.location = 11;
    declareUniform("uKs", udef);

    udef.location = 12;
    udef.size = sizeof(float);
    udef.type = GLSLType::FLOAT;
    declareUniform("uNs", udef);

    ResourceDef rdef;
    rdef.set = 0;   // This is the bindpoint in OpenGL
    rdef.location = 1;
    rdef.type = ResourceType::TEXTURE;
    rdef.arraysize = 1;
    rdef.blocksize = 0;
    rdef.unbounded = true;
    rdef.stage = ShaderStage::FRAGMENT;
    declareResource("uMapKa", rdef);

    rdef.set = 1;
    rdef.location = 2;
    declareResource("uMapKd", rdef);

    rdef.set = 2;
    rdef.location = 3;
    declareResource("uMapKs", rdef);

    rdef.set = 3;
    rdef.location = 4;
    declareResource("uMapKn", rdef);

    return compile();
}

/**
 * Sets the ambient texture for this shader.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param texture   The ambient texture
 */
void ObjShader::setAmbientTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == nullptr) {
        pushInt("uHasKa", 0);
        setTexture("uMapKa", Texture::getBlank());
    } else {
        pushInt("uHasKa", 1);
        setTexture("uMapKa", texture);
    }
}

/**
 * Sets the diffuse texture for this shader.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param texture   The diffuse texture
 */
void ObjShader::setDiffuseTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == nullptr) {
        pushInt("uHasKd", 0);
        setTexture("uMapKd", Texture::getBlank());
    } else {
        pushInt("uHasKd", 1);
        setTexture("uMapKd", texture);
    }
}

/**
 * Sets the specular texture for this shader.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param texture   The specular texture
 */
void ObjShader::setSpecularTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == nullptr) {
        pushInt("uHasKs", 0);
        setTexture("uMapKs", Texture::getBlank());
    } else {
        pushInt("uHasKs", 1);
        setTexture("uMapKs", texture);
    }
}

/**
 * Sets the normal (bump) texture for this shader.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param texture   The normal texture
 */
void ObjShader::setNormalTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == nullptr) {
        pushInt("uHasKn", 0);
        setTexture("uMapKn", Texture::getBlank());
    } else {
        pushInt("uHasKn", 1);
        setTexture("uMapKn", texture);
    }
}
