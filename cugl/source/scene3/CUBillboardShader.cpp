//
//  CUBillboardShader.hpp
//  Cornell University Game Library (CUGL)
//
//  This module is a lightweight subclass of Shader that caches the uniform
//  locations, making it a little quicker to update their values.
//
//  Note that this shader has been moved to the scene3 folder. Normally, we
//  put shaders in the render folder.  However, we want to restrict render to
//  our core graphic elements. Anything unique to scene graph like this module
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
#include <cugl/scene3/CUBillboardShader.h>
#include <cugl/graphics/CUSpriteVertex.h>

using namespace cugl;
using namespace cugl::scene3;
using namespace cugl::graphics;

// TODO: Will need to be moved in the refactor
/** Billboard fragment shader */
const std::string billShaderFrag =
#include "shaders/BillboardShader.frag"
;

/** Billboard vertex shader */
const std::string billShaderVert =
#include "shaders/BillboardShader.vert"
;

#pragma mark Constructors
/**
 * Creates an uninitialized shader with no source.
 *
 * You must initialize the shader for it to be compiled.
 */
BillboardShader::BillboardShader() : GraphicsShader() {
    std::memset(_cache,0,sizeof(float)*BILL_GRAD_CACHE);
}

/**
 * Deletes the shader program and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void BillboardShader::dispose() {
    // In case we want other attributes in the refactor
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
bool BillboardShader::init() {
    ShaderSource vsource;
    ShaderSource fsource;
    vsource.initOpenGL(billShaderVert, ShaderStage::VERTEX);
    fsource.initOpenGL(billShaderFrag, ShaderStage::FRAGMENT);
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
bool BillboardShader::init(const ShaderSource& vsource, const ShaderSource& fsource) {
    if (!GraphicsShader::init(vsource, fsource)) {
        return false;
    }

    setAttributeStride(0, sizeof(SpriteVertex));
    
    // Set up the shader
    AttributeDef adef;
    adef.type = GLSLType::VEC2;
    adef.group = 0;
    adef.location = 0;
    adef.offset = offsetof(SpriteVertex,position);
    declareAttribute("aPosition", adef);

    adef.type = GLSLType::UCOLOR;
    adef.location = 1;
    adef.offset = offsetof(SpriteVertex,color);
    declareAttribute("aColor", adef);

    adef.type = GLSLType::VEC2;
    adef.location = 2;
    adef.offset = offsetof(SpriteVertex,texcoord);
    declareAttribute("aTexCoord", adef);
    
    adef.location = 3;
    adef.offset = offsetof(SpriteVertex,gradcoord);
    declareAttribute("aGradCoord", adef);
    
    UniformDef udef;
    udef.location = 0;
    udef.size = sizeof(Vec3);
    udef.type = GLSLType::VEC3;
    udef.stage = ShaderStage::VERTEX;
    declareUniform("uCameraRight", udef);

    udef.location = 2;
    declareUniform("uCameraUp", udef);

    udef.location = 3;
    udef.size = sizeof(Mat4);
    udef.type = GLSLType::MAT4;
    declareUniform("uPerspective", udef);

    udef.location = 4;
    declareUniform("uModelMatrix", udef);

    udef.location = 5;
    udef.size = sizeof(Vec2);
    udef.type = GLSLType::VEC2;
    declareUniform("uTexOffset", udef);
    
    udef.location = 6;
    udef.size = sizeof(float)*9;
    udef.type = GLSLType::MAT3;
    udef.stage = ShaderStage::FRAGMENT;
    declareUniform("uGradientMatrix", udef);

    udef.location = 7;
    udef.size = sizeof(Vec4);
    udef.type = GLSLType::VEC4;
    declareUniform("uGradientInner", udef);

    udef.location = 8;
    declareUniform("uGradientOuter", udef);

    udef.location = 9;
    udef.size = sizeof(Vec2);
    udef.type = GLSLType::VEC2;
    declareUniform("uGradientExtent", udef);

    udef.location = 10;
    udef.size = sizeof(float);
    udef.type = GLSLType::FLOAT;
    declareUniform("uGradientRadius", udef);

    udef.location = 11;
    declareUniform("uGradientFeathr", udef);

    udef.location = 12;
    udef.size = sizeof(int);
    udef.type = GLSLType::INT;
    declareUniform("uType", udef);

    ResourceDef rdef;
    rdef.set = 0;
    rdef.location = 1;
    rdef.type = ResourceType::TEXTURE;
    rdef.arraysize = 1;
    rdef.blocksize = 0;
    rdef.unbounded = true;
    rdef.stage = ShaderStage::FRAGMENT;
    declareResource("uTexture", rdef);
    

    return compile();
}

#pragma mark Attributes
/**
 * Sets the texture for this shader.
 *
 * This method will only succeed if the shader is actively bound. In
 * addition, it does not actually bind the texture. That must be done
 * separately.
 *
 * @param texture   The texture to bind
 */
void BillboardShader::setTexture(const std::shared_ptr<graphics::Texture>& texture) {
    GraphicsShader::setTexture("uTexture",texture);
}

/**
 * Sets the gradient uniforms for this shader.
 *
 * If the gradient is nullptr, this will 0 all gradient uniforms.
 *
 * This method will only succeed if the shader is in an active drawing pass.
 *
 * @param grad The gradient object
 */
void BillboardShader::setGradient(const std::shared_ptr<Gradient>& grad) {
    if (grad != nullptr) {
        grad->pack(_cache);
    } else {
        std::memset(_cache,0,sizeof(float)*BILL_GRAD_CACHE);
    }
    
    pushMat3("uGradientMatrix", _cache);
    pushVec4("uGradientInner", _cache[ 9],_cache[10],_cache[11],_cache[12]);
    pushVec4("uGradientOuter",  _cache[13],_cache[14],_cache[15],_cache[16]);
    pushVec2("uGradientExtent", _cache[17],_cache[18]);
    pushFloat("uGradientRadius", _cache[19]);
    pushFloat("uGradientFeathr", _cache[20]);
}
