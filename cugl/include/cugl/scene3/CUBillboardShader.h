//
//  CUBillboardShader.h
//  Cornell University Game Library (CUGL)
//
//  This module is a lightweight subclass of Shader that caches the uniform
//  locations, making it a little quicker to update their values.
//
//  Note that this shader has been moved to the scene3 folder. Normally, we
//  put shaders in the render folder.  However, we want to restrict render to
//  our core graphic elements. Anything unique to scene graph like this module
//  is pulled out of that folder.
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
// TODO: This class as written has too many push constants for Vulkan
// We can JUST afford the uniforms in the vertex shader. Cache these
// Everything in fragment shader goes to an (internal) uniform buffer
#ifndef __CU_BILLBOARD_SHADER_H__
#define __CU_BILLBOARD_SHADER_H__
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/graphics/CUGradient.h>

namespace  cugl {

    /**
     * The classes to construct a 3-d scene graph.
     *
     * Unlike the scene2 package, the 3-d scene graph classes are quite limited.
     * We only have support for OBJ/MTL files, as well as simple billboarding.
     * There is no support for bone animation or physically based rendering.
     *
     * The reason for this limitation is because this is a student training
     * engine, and we often like to task students at adding those features.
     * In addition, unlike our 2-d scene graph with Figma support, there are
     * a lot of third party libraries out there that handle rendering better
     * for 3-d scenes.
     */
    namespace scene3 {

#define BILL_GRAD_CACHE 21

/**
 * This class is a shader for rendering {@link BillboardNode} objects.
 *
 * This class is a very lighweight subclass of {@link GraphicsShader}. It exists
 * mainly to simplify the compilation process.
 */
class BillboardShader : public graphics::GraphicsShader {
private:
    /** A cache for extracting gradient information */
    float _cache[BILL_GRAD_CACHE];
    
public:
#pragma mark Constructors
    /**
     * Creates an uninitialized shader with no source.
     *
     * You must initialize the shader for it to be compiled.
     */
    BillboardShader();
    
    /**
     * Deletes this shader, disposing all resources.
     */
    ~BillboardShader() { dispose(); }
    
    /**
     * Deletes the shader program and resets all attributes.
     *
     * You must reinitialize the shader to use it.
     */
    void dispose() override;
    
    /**
     * Initializes this shader with the standard vertex and fragment source.
     *
     * The shader will compile the vertex and fragment sources and link
     * them together. When compilation is complete, the shader will be
     * bound and active. In addition, all uniforms will be validated.
     *
     * @return true if initialization was successful.
     */
    bool init();
    
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
    bool init(const graphics::ShaderSource& vsource, const graphics::ShaderSource& fsource) override;
    
    /**
     * Returns a newly allocated shader with the standard vertex and fragment source.
     *
     * The shader will compile the vertex and fragment sources and link
     * them together. When compilation is complete, the shader will be
     * bound and active. In addition, all uniforms will be validated.
     *
     * @return a newly allocated shader with the standard vertex and fragment source.
     */
    static std::shared_ptr<BillboardShader> alloc() {
        std::shared_ptr<BillboardShader> result = std::make_shared<BillboardShader>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated shader with the given vertex and fragment source.
     *
     * The shader will compile the vertex and fragment sources and link
     * them together. When compilation is complete, the shader will be
     * bound and active. In addition, all uniforms will be validated.
     *
     * @param vsource   The source for the vertex shader.
     * @param fsource   The source for the fragment shader.
     *
     * @return a newly allocated shader with the given vertex and fragment source.
     */
    static std::shared_ptr<BillboardShader> alloc(const graphics::ShaderSource& vsource,
                                                  const graphics::ShaderSource& fsource) {
        std::shared_ptr<BillboardShader> result = std::make_shared<BillboardShader>();
        return (result->init(vsource,fsource) ? result : nullptr);
    }
    
#pragma mark Attributes
    
    /**
     * Sets the drawing style for this shader.
     *
     * A value of 0 will omit the texture and/or gradient, and only use colors.
     * A value of 1 or 3 will include the texture. A value of 2 or 3 will
     * include the gradient.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param style The drawing style
     */
    void setStyle(int style) {
        pushInt("uType", style);
    }
    
    /**
     * Sets the perspective matrix for this shader.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param matrix    The perspective matrix
     */
    void setPerspective(const Mat4& matrix) {
        pushMat4("uPerspective",matrix);
    }
    
    /**
     * Sets the model matrix for this shader.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param matrix    The model matrix
     */
    void setModelMatrix(const Mat4& matrix) {
        pushMat4("uModelMatrix",matrix);
    }
    
    /**
     * Sets the right direction of the camera for this shader.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param v The camera right direction
     */
    void setCameraRight(const Vec3& v) {
        pushVec3("uCameraRight",v);
    }
    
    /**
     * Sets the up direction of the camera for this shader.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param v The camera up direction
     */
    void setCameraUp(const Vec3& v) {
        pushVec3("uCameraUp",v);
    }
    
    /**
     * Sets the texture offset for this shader.
     *
     * This value can be used for simple animations. It adjusts the texture
     * coordinates of the sprite mesh by the given amount.
     *
     * This method will only succeed if the shader is in active drawing pass.
     *
     * @param v The texture offset
     */
    void setTextureOffset(const Vec2& v) {
        pushVec2("uTexOffset",v);
    }
    
    /**
     * Sets the texture offset for this shader.
     *
     * This value can be used for simple animations. It adjusts the texture
     * coordinates of the sprite mesh by the given amount.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param x The x-coordinate of the texture offset
     * @param y The y-coordinate of the texture offset
     */
    void setTextureOffset(float x, float y) {
        pushVec2("uTexOffset",x,y);
    }
    
    /**
     * Sets the texture for this shader.
     *
     * This method will only succeed if the shader is actively bound. In
     * addition, it does not actually bind the texture. That must be done
     * separately.
     *
     * @param texture   The texture to bind
     */
    void setTexture(const std::shared_ptr<graphics::Texture>& texture);
    
    /**
     * Sets the gradient uniforms for this shader.
     *
     * If the gradient is nullptr, this will 0 all gradient uniforms.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param grad The gradient object
     */
    void setGradient(const std::shared_ptr<graphics::Gradient>& grad);
    
};

    }
}
#endif /* __CU_BILLBOARD_SHADER_H__ */
