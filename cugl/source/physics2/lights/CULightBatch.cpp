//
//  CULightBatch.h
//  Cornell University Game Library (CUGL)
//
//  This module is part of a reimagining of Kalle Hameleinen's box2dLights
//  (version 1.5). Box2dLights is distributed under the Apache license. For the
//  original source code, refer to
//
//    https://github.com/libgdx/box2dlights
//
//  This code has been redesigned to solve architectural issues in that library,'
//  particularly with the way it couples geometry and rendering. This version is
//  compatible with our CUGL's rendering backend (either OpenGL or Vulkan).
//  Future plans are to offload the raycasting to a compute shader.
//
//  This specific module is represents a vertex buffer batch and shader for
//  compositing the lights. It is like a SpriteBatch for the first pass. In the
//  original implementation of box2lights, each light is its own mesh drawn
//  separately (actually with two draw calls). This allows us to update and
//  draw all lights as a single call and is more efficient.
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
//  Version: 1/8/26
//
#include <cugl/physics2/lights/CULightBatch.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/graphics/CUVertexBuffer.h>
#include <cugl/graphics/CUIndexBuffer.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2;
using namespace cugl::physics2::lights;

/**
 * Light batch fragment shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderFrag =
#include "shaders/LightShader.frag"
;

/**
 * Light batch vertex shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderVert =
#include "shaders/LightShader.vert"
;

/**
 * The default number of lights supported.
 *
 * This value assumes the lights all have the default number of rays. If
 * the lights have more rays, then the capacity is less. If this batch ever
 * exceeds its capacity, it will resize the buffers before calling
 * {@link #flush}.
 */
Uint32 LightBatch::DEFAULT_CAPACITY = 16;

#pragma mark Constructors

/**
 * Creates a degenerate light batch with no buffers.
 *
 * You must initialize the batch before using it.
 */
LightBatch::LightBatch() :
_active(false),
_gamma(false),
_premult(true),
_capacity(0),
_vertData(nullptr),
_vertMax(0),
_vertSize(0),
_indxData(nullptr),
_indxMax(0),
_indxSize(0) {
}

/**
 * Deletes the vertex buffers and resets all attributes.
 *
 * You must reinitialize the light batch to use it.
 */
void LightBatch::dispose() {
    _camera = nullptr;
    _shader = nullptr;
    _vertices = nullptr;
    _indices  = nullptr;
    
    if (_vertData) {
        free(_vertData);
        _vertData = nullptr;
        _vertMax = 0;
        _vertSize = 0;
    }

    if (_indxData) {
        free(_indxData);
        _indxData = nullptr;
        _indxMax = 0;
        _indxSize = 0;
    }
    _active = false;
    _capacity = 0;
    _bounds.set(0,0,0,0);
}
 
/**
 * Initializes a light batch with the given physics units and capacity
 *
 * The light batch will assume that the physics bounds are the size of the
 * display divided by the physics unit.
 *
 * The capacity is measured in terms of lights supported, and assumes that
 * the they use the default settings of 256 rays. If the lights passed to
 * the batch have more rays, then less lights are supported.  If the mesh
 * ever exceeds this capacity it will be dynamically resized before drawing
 * (e.g. a call to {@link #flush}.
 *
 * @param units     The physics units (pixels per box2d unit)
 * @param capacity  The number of standard lights supported
 *
 * @return true if initialization was successful.
 */
bool LightBatch::init(float units, Uint32 capacity) {
    if (units <= 0) {
        CUAssertLog(false, "Physics units must be positive");
        return false;
    }
    
    Rect bounds = Display::get()->getViewBounds();
    bounds /= units;
    return initWithBounds(bounds, capacity);
}
 
/**
 * Initializes a light batch with the given bounds, physics units, and capacity.
 *
 * The capacity is measured in terms of lights supported, and assumes that
 * the they use the default settings of 256 rays. If the lights passed to
 * the batch have more rays, then less lights are supported.  If the mesh
 * ever exceeds this capacity it will be dynamically resized before drawing
 * (e.g. a call to {@link #flush}.
 *
 * @param bounds    The bounds of the physics world
 * @param capacity  The number of standard lights supported
 *
 * @return true if initialization was successful.
 */
bool LightBatch::initWithBounds(const Rect& bounds, Uint32 capacity) {
    if (capacity == 0) {
        CUAssertLog(false, "Capacity must be non-zero");
        return false;
    }
    _bounds = bounds;
    _capacity = capacity;
    
    Uint32 verts = capacity*2*Light::DEFAULT_RAYS;
    _vertData = (LightVertex*)malloc(sizeof(LightVertex)*verts);
    std::memset(_vertData,0,sizeof(LightVertex)*verts);
    _indxData = (Uint32*)malloc(sizeof(Uint32)*verts);
    std::memset(_indxData,0,sizeof(Uint32)*verts);
    _vertMax = verts;
    _indxMax = verts;
    
    _camera = OrthographicCamera::allocOffset(_bounds);
    _vertices = VertexBuffer::alloc(verts,sizeof(LightVertex));
    _indices  = IndexBuffer::alloc(verts);
    
    // Time to compile the shader
    ShaderSource vsource;
    vsource.initOpenGL(oglShaderVert, ShaderStage::VERTEX);
    ShaderSource fsource;
    fsource.initOpenGL(oglShaderFrag, ShaderStage::FRAGMENT);
    _shader = GraphicsShader::alloc(vsource, fsource);
    
    AttributeDef adef;
    adef.type = GLSLType::VEC2;
    adef.group = 0;
    adef.location = 0;
    adef.offset = offsetof(LightVertex,position);
    _shader->declareAttribute("aPosition", adef);

    adef.type = GLSLType::UCOLOR;
    adef.location = 1;
    adef.offset = offsetof(LightVertex,color);
    _shader->declareAttribute("aColor", adef);

    adef.type = GLSLType::FLOAT;
    adef.location = 2;
    adef.offset = offsetof(LightVertex,scale);
    _shader->declareAttribute("aScale", adef);
    
    UniformDef udef;
    udef.type = GLSLType::MAT4;
    udef.location = 0;
    udef.size = sizeof(Mat4);
    udef.stage = ShaderStage::VERTEX;
    _shader->declareUniform("uProjection", udef);

    udef.type = GLSLType::INT;
    udef.location = 1;
    udef.size = sizeof(int);
    _shader->declareUniform("uPremultiplied", udef);

    udef.location = 2;
    udef.stage = ShaderStage::FRAGMENT;
    _shader->declareUniform("uGamma", udef);
    
    return _shader->compile();
}

#pragma mark -
#pragma mark Attributes
/**
 * Resizes the mesh to support a larger capacity of lights
 *
 * @param capacity  The new capacity
 */
void LightBatch::resize(Uint32 capacity) {
    Uint32 verts = capacity*2*Light::DEFAULT_RAYS;
    _capacity = capacity;
    
    if (verts < _vertMax) {
        _vertMax = verts;
        _indxMax = verts;
        return;
    }
    
    
    if (_vertData) {
        LightVertex* newData = (LightVertex*)malloc(sizeof(LightVertex)*verts);
        std::memcpy(newData, _vertData, sizeof(LightVertex)*_vertSize);
        std::memset(newData+_vertSize,0,sizeof(LightVertex)*(verts-_vertSize));
        free(_vertData);
        _vertData = newData;
    } else {
        _vertData = (LightVertex*)malloc(sizeof(LightVertex)*verts);
        std::memset(_vertData,0,sizeof(LightVertex)*verts);
    }
    
    if (_indxData) {
        Uint32* newData = (Uint32*)malloc(sizeof(Uint32)*verts);
        std::memcpy(newData, _indxData, sizeof(Uint32)*_indxSize);
        std::memset(newData+_indxSize,0,sizeof(Uint32)*(verts-_indxSize));
        free(_indxData);
        _indxData = newData;
    } else {
        _indxData = (Uint32*)malloc(sizeof(Uint32)*verts);
        std::memset(_indxData,0,sizeof(Uint32)*verts);
    }
    
    _vertMax = verts;
    _indxMax = verts;
    
    _vertices = VertexBuffer::alloc(verts,sizeof(LightVertex));
    _indices  = IndexBuffer::alloc(verts);
}


/**
 * Sets the supported capacity of this light batch
 *
 * The capacity is measured in number of lights supported. It is assumed
 * that all lights use the same number of rays, which is provided as a
 * second argument. Lights with more rays will reduce the stated capacity,
 * while lights with less rays will increase it.
 *
 * Note that this value can change if too many lights are added to the
 * mesh. In that case the batch will resize automatically to support the
 * new number of lights. In addition, the value {@link #getCapacity} will
 * return a value indicating 256 ray lights, even if the number of rays
 * here is less.
 *
 * @param capacity  The capacity measured in lights
 * @param rays      The expected number of light rays
 */
void LightBatch::setCapacity(Uint32 capacity, Uint32 rays) {
    CUAssertLog(capacity > 0, "Capacity cannot be 0");
    Uint32 cap = capacity*Light::DEFAULT_RAYS/rays;
    resize(cap);
}
   
/**
 * Sets the bounds of this light batch in physics units.
 *
 * This value and {@link #getPhysicsUnits} is used to determine the
 * {@link OrthogonalCamera} for this batch.
 *
 * @param bounds    The bounds of this light batch in physics units.
 */
void LightBatch::setBounds(const Rect& bounds) {
    _bounds = bounds;
    _camera = OrthographicCamera::allocOffset(_bounds);
}

#pragma mark Rendering
/**
 * Starts a drawing pass for the current bounds and physics units.
 *
 * This call will disable depth buffer writing. It enables blending and
 * texturing. You must call either {@link #flush} or {@link #end} to
 * complete drawing.
 */
void LightBatch::begin() {
    begin(FrameBuffer::getDisplay());
}

/**
 * Starts drawing with the given frame buffer.
 *
 * This call will disable depth buffer writing. It enables blending and
 * texturing. You must call either {@link #flush} or {@link #end} to
 * complete drawing.
 *
 * This method allows the sprite batch to draw to an offscreen framebuffer.
 * You must call {@link #end} to resume drawing to the primary display.
 *
 * @param framebuffer   The offscreen framebuffer to draw to
 */
void LightBatch::begin(const std::shared_ptr<FrameBuffer>& framebuffer) {
    auto fb = framebuffer;
    if (!fb) {
        fb = FrameBuffer::getDisplay();
    }
    
    _shader->begin(fb);
    _shader->setCullMode(CullMode::NONE);
    _shader->enableDepthTest(false);
    _shader->enableBlending(true);
    _shader->setDrawMode(DrawMode::TRIANGLES);
    if (_premult) {
        _shader->setBlendMode(BlendMode::PREMULT);
    } else {
        _shader->setBlendMode(BlendMode::ALPHA);
    }

    _shader->pushMat4("uProjection", _camera->getCombined());
    _shader->pushInt("uGamma", _gamma ? 1 : 0);
    _shader->pushInt("uPremultipled", _premult ? 1 : 0);
    _active = true;
    _vertSize = 0;
    _indxSize = 0;
}

/**
 * Completes the drawing pass for this sprite batch, flushing the buffer.
 *
 * This method will unbind the associated shader. It must always be called
 * after a call to {@link #begin}.
 */
void LightBatch::end() {
    flush();
    _shader->end();
    _active = false;
}

/**
 * Flushes the current mesh to the underlying shader.
 *
 * This methods converts all of the batched drawing commands into low-level
 * drawing operations in the associated graphics API. This method is useful
 * in OpenGL in that it allows us to recycle internal data structures,
 * reducing the memory footprint. However, it is rarely useful in Vulkan. It
 * is provided simply to keep the interface uniform between graphics APIs.
 */
void LightBatch::flush() {
    CUAssertLog(_active,"There is no active draw pass");
    if (_vertSize == 0 || _indxSize == 0) {
        return;
    }
    
    _vertices->loadData(_vertData, sizeof(LightVertex)*_vertSize);
    _indices->loadData(_indxData, _indxSize);
    
    _shader->setVertices(0,_vertices);
    _shader->setIndices(_indices);
    _shader->drawIndexed(_indxSize);
    
    _vertSize = 0;
    _indxSize = 0;
}

/**
 * Adds the given light to the mesh for drawing.
 *
 * If adding this light would exceed the mesh capacity, then this method
 * will resize the mesh to fit. Note, however, that resizing is a very
 * heavyweight operation and should be avoided.
 *
 * @param light The light to draw
 */
void LightBatch::draw(const std::shared_ptr<Light>& light) {
    CUAssertLog(_active,"There is no active draw pass");
    auto mesh = light->getLightMesh();
    if (mesh.vertices.size()+_vertSize >= _vertMax ||
        mesh.indices.size()+_indxSize >= _indxMax) {
        resize(2*_capacity);
    }
    
    std::memcpy(_vertData+_vertSize, mesh.vertices.data(),
                sizeof(LightVertex)*mesh.vertices.size());
    
    Uint32* indices = _indxData+_indxSize;
    for(int ii = 0; ii < mesh.indices.size(); ii++) {
        indices[ii] = mesh.indices[ii]+_vertSize;
    }
    _vertSize += mesh.vertices.size();
    _indxSize += mesh.indices.size();
}
