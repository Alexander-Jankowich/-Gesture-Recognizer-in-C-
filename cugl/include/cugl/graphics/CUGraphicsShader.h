//
//  CUGraphicsShader.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a general purpose shader class for GLSL shaders.
//  It supports compilations and has diagnostic tools for errors. The
//  shader is general enough that it typically not need to be subclassed.
//  When we do subclass a shader, it is generally for validation purposes.
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
#ifndef __CU_GRAPHICS_SHADER_H__
#define __CU_GRAPHICS_SHADER_H__
#include <cugl/core/CUBase.h>
#include <cugl/core/math/cu_math.h>
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/graphics/CUShaderSource.h>
#include <string>
#include <vector>
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
    
/**
 * The classes needed implement Vulkan graphics.
 *
 * Unlike OpenGL, Vulkan is not ready to go "out of the box". The code
 * can be quite complicated, and we need some support classes to make it
 * manageable in CUGL That is the point of this package.
 */
namespace vulkan {
    class VulkanBuffer;
}

// Opaque classes for the graphics API
class GraphicsShaderData;
class ResourceData;
// Forward reference
class RenderPass;
class FrameBuffer;
    
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
 * This class defines a GLSL shader.
 *
 * This class compiles and links any well-defined GLSL shader. It also has
 * methods for querying and binding the shader. The class is written to be
 * agnostic about whether we are using OpenGL, OpenGLES, or Vulkan. It is
 * reasonably lightweight enough to be general purpose. However, it only
 * supports basic vertex buffers and 2d textures. If you need anything more
 * than this, you may need to create your own shader class (which will be
 * specific to your platform). For example, if you wish to separate textures
 * from their samplers,
 *
 * Because of difference in how OpenGL and Vulkan define uniforms, we actually
 * have two types of uniforms. Uniforms refer to values that are changed
 * directly in the shader without any intermediate data structure; in Vulkan
 * these are referred to as push constants. Resource, on the other hand, include
 * uniforms that require additional data structures, such as textures, texture
 * arrays, and uniform buffers. While OpenGL allows for an unlimited number of
 * uniforms without the use of uniform buffers, Vulkan typically only allows 192
 * bytes for push constants (generally enough to set the camera), and all other
 * uniforms must be handled by uniform buffers.
 *
 * Because of the limitations of OpenGLES, this class only supports vertex and
 * fragment shaders. It does not support tesselation or geometry shaders.
 * Furthermore, keep in mind that Apple has deprecated OpenGL. MacOS devices are
 * stuck at OpenGL 4.1 and iOS devices are stuck at OpenGLES 3.1. So it is not
 * safe to use any shader more recent than version 330 on any platform.
 *
 * In addition, this shader does not support indirect drawing. This is due to
 * a limitation of MoltenVK, where indirect draws are always converted into a
 * an explicit CPU loop anyway.
 */
class GraphicsShader : public std::enable_shared_from_this<GraphicsShader> {
#pragma mark Values
protected:
    /** The active shader (if any) */
    static std::shared_ptr<GraphicsShader> g_shader;
    
    /** The graphics API implementation of this shader */
    GraphicsShaderData* _data;
    /** The source for the vertex shader */
    ShaderSource _vertModule;
    /** The source for the fragment shader */
    ShaderSource _fragModule;

    /** The number of color attachments */
    size_t _attachments;
    /** The attribute definitions for this shader */
    std::unordered_map<std::string, AttributeDef> _attributes;
    /** The (manually defined) stride for each attribute group */
    std::vector<Uint32> _attrStride;
    /** The uniform definitions of this shader */
    std::unordered_map<std::string, UniformDef> _uniforms;
    /** The resource definitions of this shader */
    std::unordered_map<std::string, ResourceDef> _resources;
    /** The instanced attribute groups */
    std::unordered_set<Uint32> _instances;
    
    /** The initial attribute group */
    Uint32 _initialGroup;
    /** The currently attached vertex buffers */
    std::vector<std::shared_ptr<VertexBuffer>> _vertices;
    /** The currently attached index buffers */
    std::shared_ptr<IndexBuffer> _indices;
    /** The currently attached resources */
    std::unordered_map<std::string, ResourceData*> _values;

    /** The render pass for this shader */
    std::shared_ptr<RenderPass> _renderpass;
    /** The current active framebuffer (nullptr to draw to display) */
    std::shared_ptr<FrameBuffer> _framebuffer;
    
    /**
     * Validates the graphics shader prior to compilation
     *
     * One of the things that we enforce is that attribute groups be continguous.
     * If there is an attribute group for n and n+2, but not n+1, then this
     * method will fail.
     *
     * @return true if validation was successful.
     */
    bool validate();

    /**
     * Returns true if this framebuffer is compatible with this shader
     *
     * While our graphics shaders do have the ability to dynamically change
     * render passes, they still need to be compatible. In particular, the
     * number of color attachments must match.
     *
     * @param framebuffer   The framebuffer to check
     *
     * @return true if this framebuffer is compatible with this shader
     */
    bool isCompatible(const std::shared_ptr<FrameBuffer>& framebuffer);

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized shader with no source.
     *
     * You must initialize the shader to add a source and compile it.
     */
    GraphicsShader() : _attachments(1), _data(nullptr), _initialGroup(0) {};
    
    /**
     * Deletes this shader, disposing all resources.
     */
    ~GraphicsShader() { dispose(); }
    
    /**
     * Deletes the shader and resets all attributes.
     *
     * You must reinitialize the shader to use it.
     */
    virtual void dispose();
    
    /**
     * Initializes this shader with the given vertex and fragment source.
     *
     * This method will allocate resources for a shader, but it will not
     * compile it. You must all the {@link compile} method to compile the
     * shader.
     *
     * @param vsource   The source for the vertex shader.
     * @param fsource   The source for the fragment shader.
     *
     * @return true if initialization was successful.
     */
    virtual bool init(const ShaderSource& vsource, const ShaderSource& fsource);
    
    /**
     * Returns a new shader with the given vertex and fragment source.
     *
     * This method will allocate resources for a shader, but it will not
     * compile it. You must all the {@link compile} method to compile the
     * shader.
     *
     * @param vsource   The source for the vertex shader.
     * @param fsource   The source for the fragment shader.
     *
     * @return a new shader with the given vertex and fragment source.
     */
    static std::shared_ptr<GraphicsShader> alloc(const ShaderSource& vsource, ShaderSource& fsource) {
        std::shared_ptr<GraphicsShader> result = std::make_shared<GraphicsShader>();
        return (result->init(vsource, fsource) ? result : nullptr);
    }
    
    
#pragma mark -
#pragma mark Compilation
    /**
     * Declares a shader attribute.
     *
     * In order to be used, each attribute must be declared before the graphics
     * shader is compiled via {@link #compile}. The declaration links a name to
     * an attribute variables not declared cannot be accesses, and in some
     * cases (e.g. Vulkan) prevent the attribute from being accessed.
     *
     * This method can fail if the attribute has already been declared.
     *
     * @param name  The attribute name
     * @param def   The attribute type, size, and location
     *
     * @return true if the declaration was successful
     */
    bool declareAttribute(const std::string name, const AttributeDef& def);
    
    /**
     * Returns the attribute definition for the given variable name.
     *
     * If there is no attribute definition for the given name, this method
     * returns nullptr.
     *
     * @param name  The attribute name
     *
     * @return the attribute definition for the given variable name.
     */
    const AttributeDef* getAttributeDef(const std::string name) const;

    /**
     * Sets the given attribute group as instanced.
     *
     * Instancing must be applied to a group, as it affects all vertices
     * in a single {@link VertexBuffer}. Hence it is essentially a property
     * of the buffer itself. An instanced vertex buffer processes vertices
     * at once per instance instead of once per vertex.
     *
     * This can be called either before or after {@link #declareAttribute}
     * defines any attributes for this group. However, if there are no
     * attributes declared for this group at the time of {@link #compile},
     * this will have no effect.
     *
     * @param group     The attribute group
     * @param instanced Whether to instance the group
     */
    void setInstanced(Uint32 group, bool instanced);

    /**
     * Sets the stride for the given attribute group
     *
     * By default, the attribute stride is inferred from the declarations.
     * However, sometimes we wish to use a {@link VertexBuffer} that contains
     * more attributes than what is used by this shader. In that case, we
     * need explicitly set the stride of the attribute group.
     *
     * @param group     The attribute group
     * @param stride    The stride of the attribute group
     */
    void setAttributeStride(Uint32 group, size_t stride);

    /**
     * Declares a shader push-constant uniform.
     *
     * In order to be used, each push-constant must be declared before the
     * graphics shader is compiled via {@link #compile}. The declaration links
     * a name to a uniform definition. We can use this name to push data to the
     * graphics shader via an of the push methods.
     *
     * Note that in OpenGL, a push constant is just any uniform that is not
     * associated with a uniform buffer (e.g. a traditional uniform).
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
     * Declares a shader resource uniform.
     *
     * Resource uniforms include uniform buffers and textures. In order to be
     * used, each resource must be declared before the graphics shader is
     * compiled via {@link #compile}. The declaration links a name to a
     * resource definition. We can use this name to load data via methods such
     * as {@link #setUniformBuffer} and {@link #setTexture}.
     *
     * Note that resource arrays are only permitted in Vulkan. In OpenGL,
     * each resource is a single object.
     *
     * This method can fail if the resource has already been declared.
     *
     * @param name  The descriptor name
     * @param def   The descriptor type, size, and location
     *
     * @return true if the declaration was successful
     */
    bool declareResource(const std::string name, const ResourceDef& def);

    /**
     * Returns the resource definition for the given variable name.
     *
     * If there is no resource definition for the given name, this method
     * returns nullptr.
     *
     * @param name  The attribute name
     *
     * @return the resource definition for the given variable name.
     */
    const ResourceDef* getResourceDef(const std::string name) const;

    /**
     * Sets the default render pass for this shader
     *
     * The render pass determines what {@link FrameBuffer} objects can be used
     * with this graphics shader. The {@link RenderPass} of the framebuffer must
     * be compatible with this renderpass as determined by
     * {@link RenderPass#isCompatible}.
     *
     * If this value is not set, or is nullptr, then graphics shader is compiled
     * to be used with the display. It can still be used with any compatible
     * {@link FrameBuffer}, which is any framebuffer with one color attachment
     * and a depth buffer.
     *
     * @param pass  The default render pass for this shader
     */
    void setRenderPass(const std::shared_ptr<RenderPass>& pass) { _renderpass = pass; }

    /**
     * Returns the default render pass for this shader
     *
     * The render pass determines what {@link FrameBuffer} objects can be used
     * with this graphics shader. The {@link RenderPass} of the framebuffer must
     * be compatible with this renderpass as determined by
     * {@link RenderPass#isCompatible}.
     *
     * If this value is not set, or is nullptr, then graphics shader is compiled
     * to be used with the display. It can still be used with any compatible
     * {@link FrameBuffer}, which is any framebuffer with one color attachment
     * and a depth buffer.
     *
     * @return the default render pass for this shader
     */
    const std::shared_ptr<RenderPass>& getRenderPass() const { return _renderpass; }
    
    /**
     * Compiles this shader for use
     *
     * All variables (attributes, uniforms, and resources) should be declared
     * before compilation. If the shader is designed for a non-traditional
     * renderpass with multiple framebuffers, that should be set as well. Once
     * compilation is successful, this shader should be ready for use.
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
#pragma mark Pipeline State
    /**
     * Sets whether the view scissor is enabled in this graphics shader.
     *
     * This is a drawing primitive that is distinct from the {@link Scissor}
     * class. That is a rotational scissor supported by {@link SpriteBatch}.
     *
     * Changing this value has no immediate effect if it is called in the
     * middle of a render pass. Instead, it will be applies at the next call
     * to {@link #begin}.
     *
     * @param enable    Whether to enable the view scissor
     */
    void enableViewScissor(bool enable);
    
    /**
     * Returns true if the view scissor is enabled in this graphics shader.
     *
     * This is a drawing primitive that is distinct from the {@link Scissor}
     * class. That is a rotational scissor supported by {@link SpriteBatch}.
     *
     * Changing this value has no immediate effect if it is called in the
     * middle of a render pass. Instead, it will be applies at the next call
     * to {@link #begin}.
     *
     * @return true if the view scissor is enabled in this graphics shader.
     */
    bool usesViewScissor() const;
    
    /**
     * Sets the AABB scissor value of this graphics shader
     *
     * This is a drawing primitive that is distinct from the {@link Scissor}
     * class. That is a rotational scissor supported by {@link SpriteBatch}.
     *
     * Changing this value has no immediate effect if it is called in the
     * middle of a render pass. Instead, it will be applies at the next call
     * to {@link #begin}.
     *
     * @param x The x-coordinate of the scissor origin
     * @param y The y-coordinate of the scissor origin
     * @param w The scissor width
     * @param h The scissor height
     */
    void setViewScissor(Uint32 x, Uint32 y, Uint32 w, Uint32 h);

    /**
     * Sets the AABB scissor value of this graphics shader
     *
     * This is a drawing primitive that is distinct from the {@link Scissor}
     * class. That is a rotational scissor supported by {@link SpriteBatch}.
     *
     * Changing this value has no immediate effect if it is called in the
     * middle of a render pass. Instead, it will be applies at the next call
     * to {@link #begin}.
     *
     * @param scissor   The graphics shader scissor
     */
    void setViewScissor(const Rect& scissor) {
        setViewScissor(scissor.origin.x, scissor.origin.y, scissor.size.width, scissor.size.height);
    }

    /**
     * Returns the AABB scissor value of this graphics shader
     *
     * This is a drawing primitive that is distinct from the {@link Scissor}
     * class. That is a rotational scissor supported by {@link SpriteBatch}.
     *
     * Changing this value has no immediate effect if it is called in the
     * middle of a render pass. Instead, it will be applies at the next call
     * to {@link #begin}.
     *
     * @return the scissor value of this graphics shader
     */
    Rect getViewScissor();

    /**
     * Sets whether color blending is enabled.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param enable    Whether to enable color blendings
     */
    void enableBlending(bool enable);
    
    /**
     * Returns whether color blending is currently enabled in this shader.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return whether color blending is currently enabled in this shader.
     */
    bool usesBlending() const;
    
    /**
     * Sets whether depth testing is enabled.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param enable    Whether to enable depth testing
     */
    void enableDepthTest(bool enable);
    
    /**
     * Returns whether depth testing is currently enabled in this shader.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return whether depth testing  is currently enabled in this shader.
     */
    bool usesDepthTest() const;
    
    /**
     * Sets whether depth writing is enabled.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param enable    Whether to enable depth writing
     */
    void enableDepthWrite(bool enable);
        
    /**
     * Returns whether depth writing is currently enabled in this shader.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return whether depth writing  is currently enabled in this shader.
     */
    bool usesDepthWrite() const;
     
	/**
     * Sets whether stencil testing is enabled.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param enable    Whether to enable stencil testing
     */
    void enableStencilTest(bool enable);
       
    /**
     * Returns whether stencil testing is currently enabled in this shader.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return whether stencil testing  is currently enabled in this shader.
     */
    bool usesStencilTest() const;
    
    /**
     * Sets the draw mode for this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param mode  The shader draw mode
     */
    void setDrawMode(DrawMode mode);
    
    /**
     * Returns the active draw mode of this shader.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the active draw mode of this shader.
     */
    DrawMode getDrawMode() const;
    
    /**
     * Sets the front-facing orientation for triangles
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param face  The front-facing orientation for triangles
     */
    void setFrontFace(FrontFace face);
    
    /**
     * Returns the active front-facing orientation for triangles
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the active front-facing orientation for triangles
     */
    FrontFace getFrontFace() const;
    
    /**
     * Sets the triangle culling mode for this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param mode  The triangle culling mode
     */
    void setCullMode(CullMode mode);
    
    /**
     * Returns the active triangle culling mode for this shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the active triangle culling mode for this shader
     */
    CullMode getCullMode() const;
    
    /**
     * Sets the depth comparions function for this graphics shader
     *
     * If depth testing is enabled, the graphics shader will discard any
     * fragments that fail this comparison.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param func   The depth comparions function
     */
    void setDepthFunc(CompareOp func);
    
    /**
     * Returns the depth comparions function for this graphics shader
     *
     * If depth testing is enabled, the graphics shader will discard any
     * fragments that fail this comparison.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the depth comparions function for this graphics shader
     */
    CompareOp getDepthFunc() const;
    
    /**
     * Sets the stencil state (both front and back) for this graphics shader
     *
     * This method will change both the front and back stencil states to the
     * given value. You can access the individual settings with the methods
     * {@link #getFrontStencilState} and {@link #getBackStencilState}.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param stencil   The (front and back) stencil state
     */
    void setStencilState(const StencilState& stencil);
    
    /**
     * Sets the font-facing stencil state for this graphics shader
     *
     * This setting will only apply to triangles that are front-facing. Use
     * {@link #setBackStencilState} to affect back-facing triangles.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param stencil   The font-facing stencil state
     */
    void setFrontStencilState(const StencilState& stencil);
    
    /**
     * Sets the back-facing stencil state for this graphics shader
     *
     * This setting will only apply to triangles that are back-facing. Use
     * {@link #setFrontStencilState} to affect front-facing triangles.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param stencil   The back-facing stencil state
     */
    void setBackStencilState(const StencilState& stencil);
    
    /**
     * Returns the font-facing stencil state for this graphics shader
     *
     * This setting will only apply to triangles that are front-facing. Use
     * {@link #getBackStencilState} to see the setting for back-facing
     * triangles.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the font-facing stencil state for this graphics shader
     */
    const StencilState& getFrontStencilState() const;
    
    /**
     * Returns the back-facing stencil state for this graphics shader
     *
     * This setting will only apply to triangles that are back-facing. Use
     * {@link #getFrontStencilState} to see the setting for front-facing
     * triangles.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the back-facing stencil state for this graphics shader
     */
    const StencilState& getBackStencilState() const;
    
    /**
     * Sets the color mask for stencils in this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param mask   The stencil color mask
     */
    void setColorMask(Uint8 mask);
    
    /**
     * Returns the color mask for stencils in this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the color mask for stencils in this graphics shader
     */
    Uint8 getColorMask() const;
    
    /**
     * Sets the stencil mode for this graphics shader
     *
     * A stencil mode is a predefined stencil setting that applies to both
     * front-facing and back-facing triangles. You cannot determine the stencil
     * mode for a shader after it is set. You can only get the stencil state
     * for that mode with the methods {@link #getFrontStencilState} and
     * {@link #getBackStencilState}.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param mode  The stencil mode for this shader
     */
    void setStencilMode(StencilMode mode);
   
    /**
     * Sets the color blend state for this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param blend The color blend state
     */
    void setBlendState(const BlendState& blend);

    /**
     * Returns the color blend state for this graphics shader
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the color blend state for this graphics shader
     */
    const BlendState& getBlendState() const;

    /**
     * Sets the blending mode for this graphics shader
     *
     * A blending mode is a predefined color blend setting that sets the color
     * blend state. You cannot determine the blend mode for a shader after it
     * is set. You can only get the color blend state for that mode with the
     * method {@link #getBlendState}.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param mode  The blending mode for this shader
     */
    void setBlendMode(BlendMode mode);

    /**
     * Sets the line width for this graphics shader
     *
     * VULKAN ONLY: This feature is not supported in OpenGL. In Vulkan this
     * defines the pixel width of a line.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @param width  The line width
     */
    void setLineWidth(float width);
    
    /**
     * Returns the line width for this graphics shader
     *
     * VULKAN ONLY: This feature is not supported in OpenGL. In Vulkan this
     * defines the pixel width of a line.
     *
     * It is possible to set this value *before* the call to {@ #compile}.
     * In that case, the value set is the default for this graphics shader.
     *
     * @return the line width for this graphics shader
     */
    float getLineWidth() const;
    
#pragma mark -
#pragma mark Data Buffers
    /**
     * Sets the vertex buffer for an attribute group
     *
     * Attribute groups must be defined in the {@link AttributeDef} provided
     * at compile time. All attributes in the vertex buffer must share the
     * same binding group.
     *
     * Elements in the vertex buffer correspond to vertices. If the attribute
     * group is instanced, these vertices will be indexed by the drawing
     * instance. Otherwise, they will be indexed by the {@link IndexBuffer}.
     *
     * @param group     The attribute group
     * @param vertices  The vertex buffer
     */
    void setVertices(Uint32 group, const std::shared_ptr<VertexBuffer>& vertices);

    /**
     * Returns the vertex buffer for an attribute group
     *
     * Attribute groups must be defined in the {@link AttributeDef} provided
     * at compile time. All attributes in the vertex buffer must share the
     * same binding group.
     *
     * Elements in the vertex buffer correspond to vertices. If the attribute
     * group is instanced, these vertices will be indexed by the drawing
     * instance. Otherwise, they will be indexed by the {@link IndexBuffer}.
     *
     * @param group     The attribute group
     *
     * @return the vertex buffer for an attribute group
     */
    const std::shared_ptr<VertexBuffer> getVertices(Uint32 group) const;
    
    /**
     * Sets the indices for the shader
     *
     * These indices define the drawing topology and are applied to **all**
     * attached vertex buffers that are not members of an instanced group.
     * They define the behavior of the methods {@link #drawIndexed} and
     * {@link #drawInstancesIndexed}.
     *
     * @param indices   The index buffer
     */
    void setIndices(const std::shared_ptr<IndexBuffer>& indices);

    /**
     * Returns the indices assigned to the shader
     *
     * These indices define the drawing topology and are applied to **all**
     * attached vertex buffers that are not members of an instanced group.
     * They define the behavior of the methods {@link #drawIndexed} and
     * {@link #drawInstancesIndexed}.
     *
     * @return the indices assigned to the shader
     */
    const std::shared_ptr<IndexBuffer> getIndices() const;

    /**
     * Sets the uniform buffer for the given resource variable
     *
     * This method will fail if the variable does not refer to a uniform buffer
     * (e.g. a std140 buffer in OpenGL, or a uniform in Vulkan) or if the value
     * is nullptr. If the resource variable refers to an array of uniform
     * buffers, then this buffer will be assigned to all positions in the
     * array.
     *
     * If the uniform buffer is composed of more than one block (e.g. it has
     * type {@link ResourceType#MULTI_BUFFER}), the shader will initially
     * start at block 0. To switch the block, use {@link #setBlock}.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param buffer    The uniform buffer for the resource
     */
    void setUniformBuffer(const std::string name, const std::shared_ptr<UniformBuffer>& buffer);

    /**
     * Sets the uniform buffer array for the given resource variable
     *
     * This method will fail if the variable does not refer to a uniform buffer
     * (e.g. a std140 buffer in OpenGL, or a uniform in Vulkan) or if the vector 
     * is empty. If the vector is larger than the number of buffers referenced 
     * by this variable, then the buffers past the limit are ignored. If the 
     * vector is smaller, then the missing positions will be replaced by the 
     * last buffer.
     *
     * If the uniform buffers are composed of more than one block (e.g. they
     * have type {@link ResourceType#MULTI_BUFFER}), the shader will initially
     * start at block 0. To switch the block, use {@link #setBlock}. Note that all
     * buffers in an array must use the same block.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param buffers   The uniform buffers for the resource
     */
    void setUniformBuffers(const std::string name, const std::vector<std::shared_ptr<UniformBuffer>>& buffers);
    
    /**
     * Returns the uniform buffer for the given resource variable
     *
     * If the variable does not refer to a uniform buffer (e.g. a std140 buffer
     * in OpenGL, or a uniform in Vulkan), then this method will return nullptr.
     * If more than one buffer is assigned to this attribute, then this method
     * will return the first one.
     *
     * Any buffer assigned to a name is stateful, in that it will persist
     * across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the uniform buffer for the given resource variable
     */
    const std::shared_ptr<UniformBuffer> getUniformBuffer(const std::string name) const;
    
    /**
     * Returns the uniform buffers for the given resource variable
     *
     * If the variable does not refer to a uniform buffer (e.g. a std140 buffer
     * in OpenGL, or a uniform in Vulkan), then this method will return an
     * empty vector.
     *
     * Any buffer assigned to a name is stateful, in that it will persist
     * across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the uniform buffers for the given resource variable
     */
    const std::vector<std::shared_ptr<UniformBuffer>> getUniformBuffers(const std::string name) const;
    
    /**
     * Sets the storage buffer for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to a
     * storage buffer or if the value is nullptr. If the resource variable
     * refers to an array of storage buffers, then this buffer will be assigned
     * to all positions in the array.
     *
     * If the storage buffer is composed of more than one block (e.g. it has
     * type {@link ResourceType#MULTI_STORAGE}), the shader will initially start
     * at block 0. To switch the block, use {@link #setBlock}.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param buffer    The storage buffer for the resource
     */
    void setStorageBuffer(const std::string name, const std::shared_ptr<StorageBuffer>& buffer);

    /**
     * Sets the storage buffer array for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to a
     * storage buffer or if the vector is empty. If the vector is larger than
     * the number of buffers referenced by this variable, then the buffers past
     * the limit are ignored. If the vector is smaller, then the missing
     * positions will be replaced by the last buffer.
     *
     * If the storage buffers are composed of more than one block (e.g. they
     * have type {@link ResourceType#MULTI_STORAGE}), the shader will initially
     * start at block 0. To switch the block, use {@link #setBlock}. Note that
     * all storage buffers in an array must have the same block count.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param buffers   The storage buffers for the resource
     */
    void setStorageBuffers(const std::string name, const std::vector<std::shared_ptr<StorageBuffer>>& buffers);
    
    /**
     * Returns the storage buffer for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to a storage buffer, then
     * this method will return nullptr. If more than one buffer is assigned to
     * this attribute, then this method will return the first one.
     *
     * Any buffer assigned to a name is stateful, in that it will persist
     * across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the storage buffer for the given resource variable
     */
    const std::shared_ptr<StorageBuffer> getStorageBuffer(const std::string name) const;

    /**
     * Returns the storage buffers for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to a storage buffer, then
     * this method will return an empty vector.
     *
     * Any buffer assigned to a name is stateful, in that it will persist
     * across render passes.
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
     * draw call.
     *
     * Note that if the resource variable refers to an array of buffers, all
     * buffers must have the same block. Setting this value is stateful, in
     * that it will persist across render passes.
     *
     * @param name  The name of the resource
     * @param block The block to use
     */
    void setBlock(const std::string name, Uint32 block);

    /**
     * Returns the active block of the associated resource buffer
     *
     * If the type of the resource is not {@link ResourceType#MULTI_BUFFER} or
     * {@link ResourceType#MULTI_STORAGE}, this method return 0. This block
     * will be used in the next draw call.
     *
     * @param name  The name of the resource
     *
     * @return the uniform block for the given resource variable
     */
    Uint32 getBlock(const std::string name);

 
#pragma mark -
#pragma mark Textures
    /**
     * Sets the texture for the given resource variable
     *
     * This method will fail if the variable does not refer to a texture (a
     * sampler2D in GLSL), or if the value is nullptr. If the variable refers
     * to an array of textures, all slots will be assigned to this texture.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
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
     * Setting this value is stateful, in that it will persist across render
     * passes.
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
     * across render passes.
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
     * across render passes.
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
     * Setting this value is stateful, in that it will persist across render
     * passes.
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
     * Setting this value is stateful, in that it will persist across render
     * passes.
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
     * across render passes.
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
     * persist across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the texture arrays for the given resource variable
     */
    const std::vector<std::shared_ptr<TextureArray>> getTextureArrays(const std::string name) const;
    
    /**
     * Sets the image for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * an image (a texture2D in GLSL), or if the value is nullptr. If the
     * variable refers to an array of images, all slots will be assigned to
     * this image.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name  The name of the resource
     * @param image The image for the resource
     */
    void setImage(const std::string name, const std::shared_ptr<Image>& image);

    /**
     * Sets the images for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * an image (a texture2D in GLSL), or if the vector is empty. If the
     * vector is larger than the number of images referenced by this variable,
     * then the images past the limit are ignored. If the vector is smaller,
     * then the missing positions will be replaced by the last image.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param images    The images for the resource
     */
    void setImages(const std::string name, const std::vector<std::shared_ptr<Image>>& images);

    /**
     * Returns the image for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to an image (a texture2D in
     * GLSL), then this method will return nullptr. If more than one image is
     * assigned to this attribute, then this method will return the first one.
     *
     * Any image assigned to a name is stateful, in that it will persist across
     * render passes.
     *
     * @param name  The name of the resource
     * 
     * @return the image for the given resource variable
     */
    const std::shared_ptr<Image> getImage(const std::string name) const;

    /**
     * Returns the images for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to an image (a texture2D in
     * GLSL), then this method will return an empty vector.
     *
     * Any images assigned to a name are stateful, in that they will persist
     * across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the images for the given resource variable
     */
    const std::vector<std::shared_ptr<Image>> getImages(const std::string name) const;
    
    /**
     * Sets the image array for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * an image array (a texture2DArray in GLSL), or if the value is nullptr.
     * If the variable refers to an array of image arrays, all slots will be
     * assigned to this image array.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name  The name of the resource
     * @param array The image array for the resource
     */
    void setImageArray(const std::string name, const std::shared_ptr<ImageArray>& array);

    /**
     * Sets the image arrays for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * an image array (a texture2DArray in GLSL), or if the vector is empty.
     * If the vector is larger than the number of images referenced by this
     * variable, then the image arrays past the limit are ignored. If the
     * vector is smaller, then the missing positions will be replaced by the
     * last image array.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param arrays    The image arrays for the resource
     */
    void setImageArrays(const std::string name, const std::vector<std::shared_ptr<ImageArray>>& arrays);

    /**
     * Returns the image array for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to an image array (a
     * texture2DArray in GLSL), then this method will return nullptr. If more
     * than one image array is assigned to this attribute, then this method
     * will return the first one.
     *
     * Any image array assigned to a name is stateful, in that it will persist
     * across render passes.
     *
     * @param name  The name of the resource
     *
     * @return the image array for the given resource variable
     */
    const std::shared_ptr<ImageArray> getImageArray(const std::string name) const;

    /**
     * Returns the image arrays for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to an image array (a
     * texture2DArray in GLSL), then this method will return an empty vector.
     *
     * Any image arrays assigned to a name are stateful, in that they will
     * persist across render passes.
     *
     * @param name      The name of the resource
     *
     * @return the image arrays for the given resource variable
     */
    const std::vector<std::shared_ptr<ImageArray>> getImageArrays(const std::string name) const;

    /**
     * Sets the sampler for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * a sampler (a sampler in GLSL), or if the value is nullptr. If the
     * variable refers to an array of samplers, all slots will be assigned to
     * this sampler.
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param sampler   The sampler for the resource
     */
    void setSampler(const std::string name, const std::shared_ptr<Sampler>& sampler);

    /**
     * Sets the samplers for the given resource variable
     *
     * VULKAN ONLY: This method will fail if the variable does not refer to
     * a sampler (a sampler in GLSL), or if the value is nullptr. If the
     * vector is larger than the number of samplers referenced by this variable,
     * then the samplers past the limit are ignored. If the vector is smaller,
     * then the missing positions will be replaced by nullptr (potentially
     * causing the pipeline to crash).
     *
     * Setting this value is stateful, in that it will persist across render
     * passes.
     *
     * @param name      The name of the resource
     * @param samplers  The samplers for the resource
     */
    void setSamplers(const std::string name, const std::vector<std::shared_ptr<Sampler>>& samplers);

    /**
     * Returns the sampler for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to a sampler (a sampler in
     * GLSL), then this method will return nullptr. If more than one sampler is
     * assigned to this attribute, then this method will return the first one.
     *
     * Any sampler assigned to a name is stateful, in that it will persist
     * across render passes.
     *
     * @param name  The name of the resource
     *
     * @return the sampler for the given resource variable
     */
    const std::shared_ptr<Sampler> getSampler(const std::string name) const;

    /**
     * Returns the samplers for the given resource variable
     *
     * VULKAN ONLY: If the variable does not refer to a sampler (a sampler in
     * GLSL), then this method will return an empty vector.
     *
     * Any samplers assigned to a name are stateful, in that they will persist
     * across render passes.
     *
     * @param name  The name of the resource
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
     * Note that uniforms can only be pushed during an active
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
     * @param name  The name of the uniform
     * @param value The value for the uniform
     */
    void pushUInt(const std::string name, Uint32 value);

    /**
     * Sets the given uniform to a float value.
     *
     * This will fail if the uniform is not a GLSL float variable.
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
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec2(const std::string name, const Vec2& vec);

    /**
     * Sets the given uniform to the given values
     *
     * This will fail if the uniform is not a GLSL vec2 variable.
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
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec3(const std::string name, const Vec3& vec);

    /**
     * Sets the given uniform to the given values
     *
     * This will fail if the uniform is not a GLSL vec3 variable.
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
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void pushVec4(const std::string name, const Vec4& vec);

    /**
     * Sets the given uniform to a quaternion.
     *
     * This will fail if the uniform is not a GLSL vec4 variable.
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
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void pushColor4f(const std::string name, const Color4f& color);

    /**
     * Sets the given uniform to a color value.
     *
     * This will fail if the uniform is not a GLSL color variable.
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
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void pushColor4(const std::string name, const Color4 color);

    /**
     * Sets the given uniform to a color value.
     *
     * This will fail if the uniform is not a GLSL ucolor variable.
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
     * @param name  The name of the uniform
     * @param array The values for the uniform
     */
    void pushMat3(const std::string name, const float* array);
    
    /**
     * Sets the given uniform to a 4x4 matrix
     *
     * This will fail if the uniform is not a GLSL mat4 variable.
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
#pragma mark Drawing
    /**
     * Begins a render pass with this shader for the application display.
     *
     * Once this method is called, no other graphics shader can begin a
     * render pass. You must complete this render pass with {@link #end}
     * before starting a new pass.
     *
     * This method will fail if {@link #getRenderPass} is not compatible
     * with {@link RenderPass::getDisplay()}.
     */
    void begin();

    /**
     * Begins a render pass with this shader for the given framebuffer
     *
     * Once this method is called, no other graphics shader can begin a
     * render pass. You must complete this render pass with {@link #end}
     * before starting a new pass.
     *
     * This method will fail if {@link #getRenderPass} does not agree with
     * that of the framebuffer
     *
     * @param framebuffer   The framebuffer to target this pass
     */
    void begin(const std::shared_ptr<FrameBuffer>& framebuffer);

    /**
     * Ends the render pass for this shader.
     *
     * Once this method is called, it is safe to start a render pass with
     * another shader. This method has no effect if there is no active
     * render pass for this shader.
     */
    void end();
    
    /**
     * Draws to the active render pass
     *
     * This draw command will ignore the index buffer and draw the vertices in
     * the order that they were loaded. Any call to this command will use the
     * current texture and uniforms, as well as the drawing topology set by
     * {@link #setDrawMode}. If any of these values need to be changed, then
     * this draw command will need to be broken up into chunks. Use the vertex
     * offset parameter to chunk up draw calls without having to reload data.
     *
     * Note that any attribute group that is instanced will only be accessed
     * once, no matter how many vertices are drawn.
     *
     * This method will only succeed if this shader is in the middle of a
     * render pass.
     *
     * @param count     The number of vertices to draw
     * @param offset    The vertex offset to start with
     */
    void draw(Uint32 count, Uint32 offset=0);
    
    /**
     * Draws to the active render pass
     *
     * This draw command will use the index buffer to stream the vertices in
     * order. Any call to this command will use the current texture and
     * uniforms, as well as the drawing topology set by {@link #setDrawMode}.
     * If any of these values need to be changed, then this draw command will
     * need to be broken up into chunks. Use the index offset parameter to
     * chunk up draw calls without having to reload data.
     *
     * Note that any attribute group that is instanced will only be accessed
     * once, no matter how many vertices are drawn.
     *
     * This method will only succeed if this shader is in the middle of a
     * render pass.
     *
     * @param count     The number of vertices to draw
     * @param offset    The initial index to start with
     */
    void drawIndexed(Uint32 count,  Uint32 offset=0);
    
    /**
     * Draws to the active render pass for the given number of instances
     *
     * This draw command will create a mesh of `count` vertices from the
     * non-instanced attributes. Like {@link #draw}, this mesh will ignore the
     * index buffer and take the vertices in he order that they were loaded.
     * It will then repeat this mesh `instance` number of times using the
     * data in the instanced attributes. The instance data will be taken from
     * the instanced attribute groups, starting at position `insoff`.
     *
     * Any call to this command will use the current texture and uniforms, as
     * well as the drawing topology set by {@link #setDrawMode}. If any of
     * these values need to be changed, then this draw command will need to be
     * broken up into chunks. Use the vertex offset parameter to chunk up draw
     * calls without having to reload data.
     *
     * This method will only succeed if this shader is in the middle of a
     * render pass.
     *
     * @param count     The number of vertices in the base mesh
     * @param instances The number of mesh instances
     * @param vtxoff    The offset of the initial vertex in the base mesh
     * @param insoff    The offset in the instance data
     */
    void drawInstances(Uint32 count, Uint32 instances, Uint32 vtxoff=0, Uint32 insoff=0);
    
    /**
     * Draws to the active render pass for the given number of instances
     *
     * This draw command will create a mesh of `count` vertices from the
     * non-instanced attributes. Like {@link #drawIndexed}, this mesh will use
     * the index buffer to define its vertices. It will then repeat this mesh
     * `instance` number of times using the data in the instanced attributes.
     * The instance data will be taken from the instanced attribute groups,
     * starting at position `insoff`.
     *
     * Any call to this command will use the current texture and uniforms, as
     * well as the drawing topology set by {@link #setDrawMode}. If any of
     * these values need to be changed, then this draw command will need to be
     * broken up into chunks. Use the vertex offset parameter to chunk up draw
     * calls without having to reload data.
     *
     * This method will only succeed if this shader is in the middle of a
     * render pass.
     *
     * @param count     The number of vertices in the base mesh
     * @param instances The number of mesh instances
     * @param idxoff    The offset of the initial index in the base mesh
     * @param insoff    The offset in the instance data
     */
    void drawInstancesIndexed(Uint32 count, Uint32 instances, Uint32 idxoff=0, Uint32 insoff=0);
    
protected:
    /**
     * Updates all descriptors and buffers before drawing.
     */
    void flush();

};

    }

}

#endif /* __CU_GRAPHICS_SHADER_H__ */
