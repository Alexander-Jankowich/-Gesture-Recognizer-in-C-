//
//  CUGraphicsShader.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a general purpose shader class for GLSL shaders. As
//  the headless version, this does not actually do any drawing. It also does
//  not performing any validation checking. It is only to guarantee that CUGL
//  code still compiles when either OpenGL or Vulkan is absent.
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
#include <cugl/core/CUDisplay.h>
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
#include <cugl/graphics/CUFrameBuffer.h>
#include <cugl/graphics/CURenderPass.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include "CUHLOpaques.h"

using namespace cugl;
using namespace cugl::graphics;

/** The active shader (if any) */
std::shared_ptr<GraphicsShader> GraphicsShader::g_shader = nullptr;

/**
 * Returns the data stride for the given GLSL type
 *
 * @param type  The GLSL type
 *
 * @return the data stride for the given GLSL type
 */
static Uint32 attribute_stride(GLSLType type) {
    switch (type) {
        case GLSLType::NONE:
            CUAssertLog(false, "Attributes cannot have type 'void'");
            return 0;
        case GLSLType::INT:
            return sizeof(Sint32);
        case GLSLType::UINT:
            return sizeof(Uint32);
        case GLSLType::FLOAT:
            return sizeof(float);
        case GLSLType::VEC2:
            return sizeof(float)*2;
        case GLSLType::VEC3:
            return sizeof(float)*3;
        case GLSLType::VEC4:
            return sizeof(float)*4;
        case GLSLType::IVEC2:
            return sizeof(Sint32)*2;
        case GLSLType::IVEC3:
            return sizeof(Sint32)*3;
        case GLSLType::IVEC4:
            return sizeof(Sint32)*4;
        case GLSLType::UVEC2:
            return sizeof(Uint32)*2;
        case GLSLType::UVEC3:
            return sizeof(Uint32)*3;
        case GLSLType::UVEC4:
            return sizeof(Uint32)*4;
        case GLSLType::MAT2:
            return sizeof(float)*4;
        case GLSLType::MAT3:
            return sizeof(float)*9;
        case GLSLType::MAT4:
            return sizeof(float)*16;
        case GLSLType::COLOR:
            return sizeof(float)*4;
        case GLSLType::UCOLOR:
            return sizeof(Uint32);
        case GLSLType::BLOCK:
            CUAssertLog(false, "Attributes cannot have a custom type");
            return 0;
    }
    return 0;
}

#pragma mark -
#pragma mark Constructor
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
bool GraphicsShader::init(const ShaderSource& vsource, const ShaderSource& fsource) {
    _vertModule = vsource;
    _fragModule = fsource;
    _data = new GraphicsShaderData();
    return true;
}

/**
 * Deletes the shader and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void GraphicsShader::dispose() {
    if (_data != nullptr) {
        delete _data;
        _data = nullptr;
        for(auto it = _values.begin(); it != _values.end(); ++it) {
            delete it->second;
            it->second = nullptr;
        }
        _values.clear();
        _attributes.clear();
        _uniforms.clear();
        _resources.clear();
        _instances.clear();
        _vertices.clear();
        _indices = nullptr;
        _framebuffer = nullptr;
    }
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
bool GraphicsShader::declareAttribute(const std::string name, const AttributeDef& def) {
    CUAssertLog(!_data->linked, "Shader has already been compiled");
    auto it = _attributes.find(name);
    if (it != _attributes.end()) {
        CUAssertLog(false, "Attribute %s is already declared", name.c_str());
        return false;
    }
    _attributes[name] = def;
    return true;
}
    
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
const AttributeDef* GraphicsShader::getAttributeDef(const std::string name) const {
    auto it = _attributes.find(name);
    if (it != _attributes.end()) {
        return &(it->second);
    }
    return nullptr;
}

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
 * @param group The attribute group
 * @param group Whether to instance the group
 */
void GraphicsShader::setInstanced(Uint32 group, bool instanced) {
    CUAssertLog(!_data->linked, "Shader has already been compiled");
    auto it = _instances.find(group);
    if (it == _instances.end() && instanced) {
        _instances.insert(group);
    } else if (it != _instances.end() && !instanced) {
        _instances.erase(it);
    }
}

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
void GraphicsShader::setAttributeStride(Uint32 group, size_t stride) {
    if (_attrStride.size() <= group) {
        _attrStride.resize(group+1, 0);
    }
    _attrStride[group] = static_cast<Uint32>(stride);
}
    
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
bool GraphicsShader::declareUniform(const std::string name, const UniformDef& def) {
    CUAssertLog(!_data->linked, "Shader has already been compiled");
    auto it = _uniforms.find(name);
    if (it != _uniforms.end()) {
        CUAssertLog(false, "Attribute %s is already declared", name.c_str());
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
const UniformDef* GraphicsShader::getUniformDef(const std::string name) const {
    auto it = _uniforms.find(name);
    if (it != _uniforms.end()) {
        return &(it->second);
    }
    return nullptr;
}

/**
 * Declares a shader resource uniform.
 *
 * Resource uniforms include uniform buffers and textures. In order to be
 * used, each resource must be declared before the graphics shader is
 * compiled via {@link #compile}. The declaration links a name to a
 * resource definition. We can use this name to load data via methods such
 * as {@link #setUniformBuffer} and {@link #setTexture}.
 *
 * This method can fail if the resource has already been declared.
 *
 * @param name  The descriptor name
 * @param def   The descriptor type, size, and location
 *
 * @return true if the declaration was successful
 */
bool GraphicsShader::declareResource(const std::string name, const ResourceDef& def) {
    CUAssertLog(!_data->linked, "Shader has already been compiled");
    auto it = _resources.find(name);
    if (it != _resources.end()) {
        CUAssertLog(false, "Attribute %s is already declared", name.c_str());
        return false;
    }
    _resources[name] = def;
    return true;
}

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
const ResourceDef* GraphicsShader::getResourceDef(const std::string name) const {
    auto it = _resources.find(name);
    if (it != _resources.end()) {
        return &(it->second);
    }
    return nullptr;
}

/**
 * Validates the graphics shader prior to compilation
 *
 * One of the things that we enforce is that attribute groups be continguous.
 * If there is an attribute group for n and n+2, but not n+1, then this
 * method will fail.
 *
 * @return true if validation was successful.
 */
bool GraphicsShader::validate() {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    if (_data->linked) {
        CULogError("Attempt to recompile an existing shader");
        return false;
    }
    
    // Determine the attributes groups
    Uint32 groupSize = 0;
    _initialGroup = -1;
    for(auto it = _attributes.begin(); it != _attributes.end(); ++it) {
        if (it->second.group >= groupSize) {
            groupSize = it->second.group+1;
        }
        if (it->second.group < _initialGroup) {
            _initialGroup = it->second.group;
        }
    }

    Uint32* stride = nullptr;
    if (groupSize > 0) {
        size_t size = groupSize-_initialGroup;
        stride = new Uint32[size];
        _attrStride.resize(size, 0);
        std::memset(stride, 0, sizeof(size_t)*size);

        size_t pos = 0;
        for(auto it = _attributes.begin(); it != _attributes.end(); ++it) {
            pos = it->second.group-_initialGroup;
            if (_attrStride[pos] == 0) {
                stride[pos] += attribute_stride(it->second.type);
            }
        }

        for(pos = 0; pos < size; pos++) {
            if (_attrStride[pos] == 0) {
                _attrStride[pos] = stride[pos];
            }
        }
    }
    
    if (stride != nullptr) {
        delete stride;
    }
    
    if (_attachments == 0) {
        CUAssertLog(false,"Shader has no attachments");
        return false;
    }
    _vertices.resize(groupSize);

    // Verify the resources;
    for(auto it = _resources.begin(); it != _resources.end(); ++it) {
        switch (it->second.type) {
            case ResourceType::IMAGE:
                CUAssertLog(false,"OpenGL does not support images without samplers");
                return false;
            case ResourceType::IMAGE_ARRAY:
                CUAssertLog(false,"OpenGL does not support image arrays without samplers");
                return false;
            case ResourceType::SAMPLER:
                CUAssertLog(false,"OpenGL does not support samplers without images");
                return false;
            case ResourceType::MONO_STORAGE:
            case ResourceType::MULTI_STORAGE:
                CUAssertLog(false,"OpenGL does not support storage buffers");
                return false;
            default:
                break;
        }
    }
    return true;
}

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
bool GraphicsShader::compile() {
    if (!validate()) {
        return false;
    }
    // Set any defaults
    if (_renderpass == nullptr || _renderpass.get() == RenderPass::getDisplay().get()) {
        _renderpass = nullptr;
        _attachments = 1;
    } else {
        _attachments = _renderpass->getColorAttachments();
    }
    
    _data->linked = true;
    for(auto it = _resources.begin(); it != _resources.end(); ++it) {
        switch (it->second.type) {
            case ResourceType::TEXTURE:
                _values[it->first] = new ResourceTexture();
                break;
            case ResourceType::TEXTURE_ARRAY:
                _values[it->first] = new ResourceTextureArray();
                break;
            case ResourceType::SAMPLER:
                _values[it->first] = new ResourceSampler();
                break;
            case ResourceType::MONO_BUFFER:
                _values[it->first] = new ResourceUniform();
                break;
            case ResourceType::MULTI_BUFFER:
            {
                auto udata = new ResourceUniform();
                udata->stride = it->second.blocksize;
                _values[it->first] = udata;
            }
                break;
            default:
                break;
        }
        _values[it->first]->type = it->second.type;
    }

    return true;
}


/**
 * Returns true if this shader has been compiled and is ready for use.
 *
 * @return true if this shader has been compiled and is ready for use.
 */
bool GraphicsShader::isReady() const {
    return (_data != nullptr && _data->linked);
}
    
    
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
void GraphicsShader::enableViewScissor(bool enable) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->scissorTest = enable;
}

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
bool GraphicsShader::usesViewScissor() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->scissorTest;
}

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
void GraphicsShader::setViewScissor(Uint32 x, Uint32 y, Uint32 w, Uint32 h) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->scissor[0] = x;
    _data->scissor[1] = y;
    _data->scissor[2] = w;
    _data->scissor[3] = h;
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
Rect GraphicsShader::getViewScissor() {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return Rect(_data->scissor[0],_data->scissor[1],
                _data->scissor[2],_data->scissor[3]);
}

/**
 * Sets whether color blending is enabled.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param enable    Whether to enable color blendings
 */
void GraphicsShader::enableBlending(bool enable) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->enableBlend = enable;
}
    
/**
 * Returns whether color blending is currently enabled in this shader.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return whether color blending is currently enabled in this shader.
 */
bool GraphicsShader::usesBlending() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->enableBlend;
}

/**
 * Sets whether depth testing is enabled.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param enable    Whether to enable depth testing
 */
void GraphicsShader::enableDepthTest(bool enable) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->depthTest = enable;
}
    
/**
 * Returns whether depth testing is currently enabled in this shader.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return whether depth testing  is currently enabled in this shader.
 */
bool GraphicsShader::usesDepthTest() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->depthTest;
}

/**
 * Sets whether depth writing is enabled.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param enable    Whether to enable depth writing
 */
void GraphicsShader::enableDepthWrite(bool enable) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->depthWrite = enable;
}
    
/**
 * Returns whether depth writing is currently enabled in this shader.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return whether depth writing  is currently enabled in this shader.
 */
bool GraphicsShader::usesDepthWrite() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->depthWrite;
}
    
/**
 * Sets whether stencil testing is enabled.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param enable    Whether to enable stencil testing
 */
void GraphicsShader::enableStencilTest(bool enable) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->stencilTest = enable;
}

/**
 * Returns whether stencil testing is currently enabled in this shader.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return whether stencil testing  is currently enabled in this shader.
 */
bool GraphicsShader::usesStencilTest() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->stencilTest;
}

/**
 * Sets the draw mode for this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param mode  The shader draw mode
 */
void GraphicsShader::setDrawMode(DrawMode mode) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->drawMode = mode;
}

/**
 * Returns the active draw mode of this shader.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return the active draw mode of this shader.
 */
DrawMode GraphicsShader::getDrawMode() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->drawMode;
}

/**
 * Sets the front-facing orientation for triangles
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param face  The front-facing orientation for triangles
 */
void GraphicsShader::setFrontFace(FrontFace face) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->frontFace = face;
}

/**
 * Returns the active front-facing orientation for triangles
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return the active front-facing orientation for triangles
 */
FrontFace GraphicsShader::getFrontFace() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->frontFace;
}

/**
 * Sets the triangle culling mode for this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param mode  The triangle culling mode
 */
void GraphicsShader::setCullMode(CullMode mode) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->cullMode = mode;
}

/**
 * Returns the active triangle culling mode for this shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return the active triangle culling mode for this shader
 */
CullMode GraphicsShader::getCullMode() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->cullMode;
}

/**
 * Sets the depth comparions function for this graphics shader
 *
 * If depth testing is enabled, the graphics shader will discard any
 * fragments that fail this comparison.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @parm func   The depth comparions function
 */
void GraphicsShader::setDepthFunc(CompareOp func) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->depthFunc = func;
}

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
CompareOp GraphicsShader::getDepthFunc() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->depthFunc;
}

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
void GraphicsShader::setStencilState(const StencilState& stencil) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->stencilFront = stencil;
    _data->stencilBack  = stencil;
}

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
void GraphicsShader::setFrontStencilState(const StencilState& stencil) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->stencilFront = stencil;
}

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
void GraphicsShader::setBackStencilState(const StencilState& stencil) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->stencilBack = stencil;
}

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
const StencilState& GraphicsShader::getFrontStencilState() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->stencilFront;
}

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
const StencilState& GraphicsShader::getBackStencilState() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->stencilBack;
}

/**
 * Sets the color mask for stencils in this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @parm mask   The stencil color mask
 */
void GraphicsShader::setColorMask(Uint8 mask) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->colorMask = mask;
}

/**
 * Returns the color mask for stencils in this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return the color mask for stencils in this graphics shader
 */
Uint8 GraphicsShader::getColorMask() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->colorMask;
}

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
void GraphicsShader::setStencilMode(StencilMode mode) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->stencilFront.set(mode);
    _data->stencilBack.set(mode);
    switch (mode) {
        case StencilMode::CLEAR:
        case StencilMode::STAMP:
        case StencilMode::STAMP_EVENODD:
        case StencilMode::STAMP_NONZERO:
            _data->colorMask = 0;
            break;
        case StencilMode::CLIP:
        case StencilMode::CLIP_CLEAR:
        case StencilMode::MASK:
        case StencilMode::MASK_CLEAR:
        case StencilMode::CLAMP:
            _data->colorMask = 0xf;
        default:
            break;
    }
}

/**
 * Sets the color blend state for this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @param blend The color blend state
 */
void GraphicsShader::setBlendState(const BlendState& blend) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->colorBlend = blend;
}

/**
 * Returns the color blend state for this graphics shader
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @return the color blend state for this graphics shader
 */
const BlendState& GraphicsShader::getBlendState() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->colorBlend;
}

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
void GraphicsShader::setBlendMode(BlendMode mode) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->colorBlend.set(mode);
}


/**
 * Sets the line width for this graphics shader
 *
 * VULKAN ONLY: This feature is not supported in OpenGL. In Vulkan this
 * defines the pixel width of a line.
 *
 * It is possible to set this value *before* the call to {@ #compile}.
 * In that case, the value set is the default for this graphics shader.
 *
 * @parm width  The line width
 */
void GraphicsShader::setLineWidth(float width) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _data->lineWidth = width;
}


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
float GraphicsShader::getLineWidth() const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    return _data->lineWidth;
}

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
 * @param buffer    The vertex buffer
 */
void GraphicsShader::setVertices(Uint32 group, const std::shared_ptr<VertexBuffer>& vertices) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    if (group < _vertices.size()) {
        _vertices[group] = vertices;
    }
}

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
const std::shared_ptr<VertexBuffer> GraphicsShader::getVertices(Uint32 group) const {
    if (group >= _vertices.size()) {
        return nullptr;
    }
    return _vertices[group];
}

/**
 * Sets the indices for the shader
 *
 * These indices define the drawing topology and are applied to **all**
 * attached vertex buffers that are not members of an instanced group.
 * They define the behavior of the methods {@link #drawIndexed} and
 * {@link #drawInstancesIndexed}.
 *
 * @param buffer    The index buffer
 */
void GraphicsShader::setIndices(const std::shared_ptr<IndexBuffer>& indices) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    _indices = indices;
}

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
const std::shared_ptr<IndexBuffer> GraphicsShader::getIndices() const {
    return _indices;
}

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
void GraphicsShader::setUniformBuffer(const std::string name, const std::shared_ptr<UniformBuffer>& buffer) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(buffer != nullptr, "Cannot assign an empty uniform buffer");
    
    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    
    ResourceUniform* resource = reinterpret_cast<ResourceUniform*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(buffer);
    }
}

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
void GraphicsShader::setUniformBuffers(const std::string name, const std::vector<std::shared_ptr<UniformBuffer>>& buffers) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!buffers.empty(), "Cannot assign an empty uniform buffer");

    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    
    ResourceUniform* resource = reinterpret_cast<ResourceUniform*>(_values[name]);
    resource->sources = buffers;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, buffers.back());
    }
}

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
const std::shared_ptr<UniformBuffer> GraphicsShader::getUniformBuffer(const std::string name) const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        return nullptr;
    }

    ResourceUniform* resource = reinterpret_cast<ResourceUniform*>(_values.at(name));
    return resource->sources.empty() ? nullptr : resource->sources.front();
}
 
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
const std::vector<std::shared_ptr<UniformBuffer>> GraphicsShader::getUniformBuffers(const std::string name) const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        return std::vector<std::shared_ptr<UniformBuffer>>();
    }

    ResourceUniform* resource = reinterpret_cast<ResourceUniform*>(_values.at(name));
    return resource->sources;
}

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
void GraphicsShader::setStorageBuffer(const std::string name, const std::shared_ptr<StorageBuffer>& buffer) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(buffer != nullptr, "Cannot assign an empty storage buffer");

    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_STORAGE &&
                                   it->second.type != ResourceType::MULTI_STORAGE)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    
    ResourceStorage* resource = reinterpret_cast<ResourceStorage*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(buffer);
    }
}

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
void GraphicsShader::setStorageBuffers(const std::string name, const std::vector<std::shared_ptr<StorageBuffer>>& buffers) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!buffers.empty(), "Cannot assign an empty storage buffer");

    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_STORAGE &&
                                   it->second.type != ResourceType::MULTI_STORAGE)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    
    ResourceStorage* resource = reinterpret_cast<ResourceStorage*>(_values[name]);
    resource->sources = buffers;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, buffers.back());
    }
}

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
const std::shared_ptr<StorageBuffer> GraphicsShader::getStorageBuffer(const std::string name) const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        return nullptr;
    }

    ResourceStorage* resource = reinterpret_cast<ResourceStorage*>(_values.at(name));
    return resource->sources.empty() ? nullptr : resource->sources.front();
}

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
const std::vector<std::shared_ptr<StorageBuffer>> GraphicsShader::getStorageBuffers(const std::string name) const {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _resources.find(name);
    if (it == _resources.end() || (it->second.type != ResourceType::MONO_BUFFER &&
                                   it->second.type != ResourceType::MULTI_BUFFER)) {
        return std::vector<std::shared_ptr<StorageBuffer>>();
    }

    ResourceStorage* resource = reinterpret_cast<ResourceStorage*>(_values.at(name));
    return resource->sources;
}

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
void GraphicsShader::setBlock(const std::string name, Uint32 block) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _values.find(name);
    if (it == _values.end()) {
        CUAssertLog(false,"Variable %s is not a dynamic buffer",name.c_str());
        return;
    }
    
    if (it->second->type == ResourceType::MULTI_BUFFER) {
        ResourceUniform* uniform = reinterpret_cast<ResourceUniform*>(it->second);
        uniform->block = block;
    } else if (it->second->type == ResourceType::MULTI_STORAGE) {
        ResourceStorage* storage = reinterpret_cast<ResourceStorage*>(it->second);
        storage->block = block;
    } else {
        CUAssertLog(false,"Variable %s is not a dynamic buffer",name.c_str());
    }
}

/**
 * Returns the storage block for the given resource variable
 *
 * VULKAN ONLY: If the variable does not refer to a storage buffer, then
 * this method will return 0.
 *
 * Note that if the resource variable refers to an array of buffers, all
 * buffers must have the same block. Any block assigned to a name is
 * stateful, in that it will persist across render passes.
 *
 * @param name  The name of the resource
 *
 * @return the storage block for the given resource variable
 */
Uint32 GraphicsShader::getBlock(const std::string name) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    auto it = _values.find(name);
    if (it == _values.end()) {
        CUAssertLog(false,"Invalid access for variable %s",name.c_str());
        return 0;
    }

    if (it->second->type == ResourceType::MULTI_BUFFER) {
        ResourceUniform* uniform = reinterpret_cast<ResourceUniform*>(it->second);
        return uniform->block;
    } else if (it->second->type == ResourceType::MULTI_STORAGE) {
        ResourceStorage* storage = reinterpret_cast<ResourceStorage*>(it->second);
        return storage->block;
    }
    return 0;
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
 * Setting this value is stateful, in that it will persist across render
 * passes.
 *
 * @param name      The name of the resource
 * @param texture   The texture for the resource
 */
void GraphicsShader::setTexture(const std::string name, const std::shared_ptr<Texture>& texture) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(texture != nullptr, "Cannot assign an empty texture");

    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::TEXTURE) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceTexture* resource = reinterpret_cast<ResourceTexture*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(texture);
    }
}

/**
 * Sets the textures for the given resource variable
 *
 * If the variable does not refer to a texture (a sampler2D in GLSL), then
 * this method will fail. If the vector is larger than the number of
 * textures referenced by this variable, then the textures past the limit
 * are ignored. If the texture is smaller, then the missing positions will
 * be replaced by a blank texture.
 *
 * Setting this value is stateful, in that it will persist across render
 * passes.
 *
 * @param name      The name of the resource
 * @param textures  The textures for the resource
 */
void GraphicsShader::setTextures(const std::string name, const std::vector<std::shared_ptr<Texture>>& textures) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!textures.empty(), "Cannot assign an empty texture");
    
    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::TEXTURE) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceTexture* resource = reinterpret_cast<ResourceTexture*>(_values[name]);
    resource->sources = textures;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, textures.back());
    }
}

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
const std::shared_ptr<Texture> GraphicsShader::getTexture(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::TEXTURE) {
        return nullptr;
    }
    ResourceTexture* resource = reinterpret_cast<ResourceTexture*>(it->second);
    return resource->sources.empty() ? nullptr : resource->sources[0];
}

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
const std::vector<std::shared_ptr<Texture>> GraphicsShader::getTextures(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::TEXTURE) {
        return std::vector<std::shared_ptr<Texture>>();
    }
    ResourceTexture* resource = reinterpret_cast<ResourceTexture*>(it->second);
    return resource->sources;
}

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
void GraphicsShader::setTextureArray(const std::string name, const std::shared_ptr<TextureArray>& array) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(array != nullptr, "Cannot assign an empty texture array");

    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::TEXTURE_ARRAY) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceTextureArray* resource = reinterpret_cast<ResourceTextureArray*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(array);
    }
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
 * Setting this value is stateful, in that it will persist across render
 * passes.
 *
 * @param name      The name of the resource
 * @param arrays    The texture arrays for the resource
 */
void GraphicsShader::setTextureArrays(const std::string name,
                                      const std::vector<std::shared_ptr<TextureArray>>& arrays) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!arrays.empty(), "Cannot assign an empty texture array");
    
    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::TEXTURE_ARRAY) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceTextureArray* resource = reinterpret_cast<ResourceTextureArray*>(_values[name]);
    resource->sources = arrays;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, arrays.back());
    }
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
 * across render passes.
 *
 * @param name      The name of the resource
 *
 * @return the texture array for the given resource variable
 */
const std::shared_ptr<TextureArray> GraphicsShader::getTextureArray(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::TEXTURE_ARRAY) {
        return nullptr;
    }
    ResourceTextureArray* resource = reinterpret_cast<ResourceTextureArray*>(it->second);
    return resource->sources.empty() ? nullptr : resource->sources[0];
}

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
const std::vector<std::shared_ptr<TextureArray>> GraphicsShader::getTextureArrays(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::TEXTURE_ARRAY) {
        return std::vector<std::shared_ptr<TextureArray>>();
    }
    ResourceTextureArray* resource = reinterpret_cast<ResourceTextureArray*>(it->second);
    return resource->sources;
}

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
void GraphicsShader::setImage(const std::string name,
                              const std::shared_ptr<Image>& image) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(image != nullptr, "Cannot assign an empty image");
    
    auto it = _resources.find(name);
    if (it == _resources.end() ||
        (it->second.type != ResourceType::IMAGE && it->second.type != ResourceType::COMPUTE_IMAGE)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceImage* resource = reinterpret_cast<ResourceImage*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(image);
    }
}

/**
 * Sets the images for the given resource variable
 *
 * VULKAN ONLY: This method will fail is the variable does not refer to
 * an image (a texture2D in GLSL), or if the value is nullptr. If the
 * vector is larger than the number of images referenced by this variable,
 * then the images past the limit are ignored. If the vector is smaller,
 * then the missing positions will be replaced by nullptr (potentially
 * causing the pipeline to crash).
 *
 * Setting this value is stateful, in that it will persist across render
 * passes.
 *
 * @param name      The name of the resource
 * @param images    The images for the resource
 */
void GraphicsShader::setImages(const std::string name,
                               const std::vector<std::shared_ptr<Image>>& images) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!images.empty(), "Cannot assign an empty image");

    auto it = _resources.find(name);
    if (it == _resources.end() ||
        (it->second.type != ResourceType::IMAGE && it->second.type != ResourceType::COMPUTE_IMAGE)) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceImage* resource = reinterpret_cast<ResourceImage*>(_values[name]);
    resource->sources = images;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, images.back());
    }
}

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
const std::shared_ptr<Image> GraphicsShader::getImage(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() ||
        (it->second->type != ResourceType::IMAGE && it->second->type != ResourceType::COMPUTE_IMAGE)) {
        return nullptr;
    }
    ResourceImage* resource = reinterpret_cast<ResourceImage*>(it->second);
    return resource->sources.empty() ? nullptr : resource->sources[0];
}

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
const std::vector<std::shared_ptr<Image>> GraphicsShader::getImages(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() ||
        (it->second->type != ResourceType::IMAGE && it->second->type != ResourceType::COMPUTE_IMAGE)) {
        return std::vector<std::shared_ptr<Image>>();
    }
    ResourceImage* resource = reinterpret_cast<ResourceImage*>(it->second);
    return resource->sources;
}

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
void GraphicsShader::setImageArray(const std::string name, const std::shared_ptr<ImageArray>& array) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(array != nullptr, "Cannot assign an empty image array");

    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::IMAGE_ARRAY) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceImageArray* resource = reinterpret_cast<ResourceImageArray*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(array);
    }
}

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
void GraphicsShader::setImageArrays(const std::string name,
                                    const std::vector<std::shared_ptr<ImageArray>>& arrays) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!arrays.empty(), "Cannot assign an empty image array");
    
    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::IMAGE_ARRAY) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceImageArray* resource = reinterpret_cast<ResourceImageArray*>(_values[name]);
    resource->sources = arrays;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, arrays.back());
    }
}
    
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
const std::shared_ptr<ImageArray> GraphicsShader::getImageArray(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::IMAGE_ARRAY) {
        return nullptr;
    }
    ResourceImageArray* resource = reinterpret_cast<ResourceImageArray*>(it->second);
    return resource->sources.empty() ? nullptr : resource->sources[0];
}

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
const std::vector<std::shared_ptr<ImageArray>> GraphicsShader::getImageArrays(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::IMAGE_ARRAY) {
        return std::vector<std::shared_ptr<ImageArray>>();
    }
    ResourceImageArray* resource = reinterpret_cast<ResourceImageArray*>(it->second);
    return resource->sources;
}

/**
 * Sets the sampler for the given resource variable
 *
 * VULKAN ONLY: This method will fail is the variable does not refer to
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
void GraphicsShader::setSampler(const std::string name, const std::shared_ptr<Sampler>& sampler) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(sampler != nullptr, "Cannot assign an empty sampler");

    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::SAMPLER) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceSampler* resource = reinterpret_cast<ResourceSampler*>(_values[name]);
    resource->sources.clear();
    for(int ii = 0; ii < it->second.arraysize; ii++) {
        resource->sources.push_back(sampler);
    }
}

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
void GraphicsShader::setSamplers(const std::string name,
                                 const std::vector<std::shared_ptr<Sampler>>& samplers) {
    CUAssertLog(_data != nullptr, "Graphics shader is not initialized");
    CUAssertLog(!samplers.empty(), "Cannot assign an empty sampler");

    auto it = _resources.find(name);
    if (it == _resources.end() || it->second.type != ResourceType::SAMPLER) {
        CUAssertLog(false,"Invalid assignment to variable %s",name.c_str());
        return;
    }
    ResourceSampler* resource = reinterpret_cast<ResourceSampler*>(_values[name]);
    resource->sources = samplers;
    if (resource->sources.size() != it->second.arraysize) {
        resource->sources.resize(it->second.arraysize, samplers.back());
    }
}

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
const std::shared_ptr<Sampler> GraphicsShader::getSampler(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::SAMPLER) {
        return nullptr;
    }
    ResourceSampler* resource = reinterpret_cast<ResourceSampler*>(it->second);
    return resource->sources.empty() ? nullptr : resource->sources[0];
}

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
const std::vector<std::shared_ptr<Sampler>> GraphicsShader::getSamplers(const std::string name) const {
    auto it = _values.find(name);
    if (it == _values.end() || it->second->type != ResourceType::SAMPLER) {
        return std::vector<std::shared_ptr<Sampler>>();
    }
    ResourceSampler* resource = reinterpret_cast<ResourceSampler*>(it->second);
    return resource->sources;
}

#pragma mark -
#pragma mark Uniforms/Push Constants
/**
 * Sets the given uniform to a integer value.
 *
 * This will fail if the uniform is not a GLSL int variable.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void GraphicsShader::pushInt(const std::string name, Sint32 value) {
    // No-op in headless
}

/**
 * Sets the given uniform to an unsigned integer value.
 *
 * This will fail if the uniform is not a GLSL uint variable.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void GraphicsShader::pushUInt(const std::string name, Uint32 value) {
    // No-op in headless
}

/**
 * Sets the given uniform to a float value.
 *
 * This will fail if the uniform is not a GLSL float variable.
 *
 * @param name  The name of the uniform
 * @param value The value for the uniform
 */
void GraphicsShader::pushFloat(const std::string name, float value) {
    // No-op in headless
}

/**
 * Sets the given uniform to a Vector2 value.
 *
 * This will fail if the uniform is not a GLSL vec2 variable.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void GraphicsShader::pushVec2(const std::string name, const Vec2& vec) {
    // No-op in headless
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL vec2 variable.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void GraphicsShader::pushVec2(const std::string name, float x, float y) {
    // No-op in headless
}

/**
 * Sets the given uniform to a Vector3 value.
 *
 * This will fail if the uniform is not a GLSL vec3 variable.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void GraphicsShader::pushVec3(const std::string name, const Vec3& vec) {
    // No-op in headless
}

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
void GraphicsShader::pushVec3(const std::string name, float x, float y, float z) {
    // No-op in headless
}

/**
 * Sets the given uniform to a Vector4 value.
 *
 * This will fail if the uniform is not a GLSL vec4 variable.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void GraphicsShader::pushVec4(const std::string name, const Vec4& vec) {
    // No-op in headless
}

/**
 * Sets the given uniform to a quaternion.
 *
 * This will fail if the uniform is not a GLSL vec4 variable.
 *
 * @param name  The name of the uniform
 * @param quat  The value for the uniform
 */
void GraphicsShader::pushVec4(const std::string name, const Quaternion& quat) {
    // No-op in headless
}

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
void GraphicsShader::pushVec4(const std::string name, float x, float y, float z, float w) {
    // No-op in headless
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec2 variable.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void GraphicsShader::pushIVec2(const std::string name, Sint32 x, Sint32 y) {
    // No-op in headless
}

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
void GraphicsShader::pushIVec3(const std::string name, int x, int y, int z) {
    // No-op in headless
}


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
void GraphicsShader::pushIVec4(const std::string name, int x, int y, int z, int w) {
    // No-op in headless
}

/**
 * Sets the given uniform to the given values
 *
 * This will fail if the uniform is not a GLSL ivec2 variable.
 *
 * @param name  The name of the uniform
 * @param x     The x-value for the vector
 * @param y     The x-value for the vector
 */
void GraphicsShader::pushUVec2(const std::string name, Uint32 x, Uint32 y) {
    // No-op in headless
}


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
void GraphicsShader::pushUVec3(const std::string name, Uint32 x, Uint32 y, Uint32 z) {
    // No-op in headless
}


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
void GraphicsShader::pushUVec4(const std::string name, Uint32 x, Uint32 y, Uint32 z, Uint32 w) {
    // No-op in headless
}

    
/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL color variable.
 *
 * @param name  The name of the uniform
 * @param color The value for the uniform
 */
void GraphicsShader::pushColor4f(const std::string name, const Color4f& color) {
    // No-op in headless
}


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
void GraphicsShader::pushColor4f(const std::string name, float red, float green, float blue, float alpha) {
    // No-op in headless
}

/**
 * Sets the given uniform to a color value.
 *
 * This will fail if the uniform is not a GLSL ucolor variable.
 *
 * @param name  The name of the uniform
 * @param color The value for the uniform
 */
void GraphicsShader::pushColor4(const std::string name, const Color4 color) {
    // No-op in headless
}

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
void GraphicsShader::pushColor4(const std::string name, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) {
    // No-op in headless
}

/**
 * Sets the given uniform to a 2x2 matrix.
 *
 * The 2x2 matrix will be the non-translational part of the affine transform.
 * This method will fail if the uniform is not a GLSL mat2 variable.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void GraphicsShader::pushMat2(const std::string name, const Affine2& mat) {
    // No-op in headless
}

/**
 * Sets the given uniform to a 2x2 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat2 variable.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void GraphicsShader::pushMat2(const std::string name, const float* array) {
    // No-op in headless
}

/**
 * Sets the given uniform to a 3x3 matrix.
 *
 * The 3x3 matrix will be affine transform in homoegenous coordinates.
 * This method will fail if the uniform is not a GLSL mat3 variable.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void GraphicsShader::pushMat3(const std::string name, const Affine2& mat) {
    // No-op in headless
}

/**
 * Sets the given uniform to a 3x3 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat3 variable.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void GraphicsShader::pushMat3(const std::string name, const float* array) {
    // No-op in headless
}


/**
 * Sets the given uniform to a 4x4 matrix
 *
 * This will fail if the uniform is not a GLSL mat4 variable.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void GraphicsShader::pushMat4(const std::string name, const Mat4& mat) {
    // No-op in headless
}

/**
 * Sets the given uniform to a 4x4 matrix.
 *
 * The matrix will be read from the array in column major order (not row
 * major). This method will fail if the uniform is not a GLSL mat4 variable.
 *
 * @param name  The name of the uniform
 * @param array The values for the uniform
 */
void GraphicsShader::pushMat4(const std::string name, const float* array) {
    // No-op in headless
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
void GraphicsShader::push(const std::string name, Uint8* data) {
    // No-op in headless
}
    
#pragma mark -
#pragma mark Drawing
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
bool GraphicsShader::isCompatible(const std::shared_ptr<FrameBuffer>& framebuffer) {
    if (_renderpass == nullptr) {
        auto imp = framebuffer->getRenderPass()->getImplementation();
        return imp->colorFormats->size() == 1 && *imp->depthStencilFormat == TexelFormat::DEPTH_STENCIL;
    }
    return _renderpass->isCompatible(framebuffer->getRenderPass());
}

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
void GraphicsShader::begin() {
    g_shader = shared_from_this();
    // No-op in headless
}
    
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
void GraphicsShader::begin(const std::shared_ptr<FrameBuffer>& framebuffer) {
    g_shader = shared_from_this();
    _framebuffer = framebuffer;
    // No-op in headless
}

/**
 * Ends the render pass for this shader.
 *
 * Once this method is called, it is safe to start a render pass with
 * another shader. This method has no effect if there is no active
 * render pass for this shader.
 */
void GraphicsShader::end() {
    // No-op in headless
    g_shader = nullptr;
    _framebuffer = nullptr;
}

/**
 * Draws to the active render target
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
void GraphicsShader::draw(Uint32 count, Uint32 offset) {
    // No-op in headless
}

/**
 * Draws to the active render target
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
void GraphicsShader::drawIndexed(Uint32 count,  Uint32 offset) {
    // No-op in headless
}

/**
 * Draws to the active render target for the given number of instances
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
void GraphicsShader::drawInstances(Uint32 count, Uint32 instances, Uint32 vtxoff, Uint32 insoff) {
    // No-op in headless
}

/**
 * Draws to the active render target for the given number of instances
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
void GraphicsShader::drawInstancesIndexed(Uint32 count, Uint32 instances, Uint32 idxoff, Uint32 insoff) {
    // No-op in headless
}

/**
 * Updates all descriptors and buffers before drawing.
 */
void GraphicsShader::flush() {
    // No-op in headless
}
