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
#ifndef __CU_LIGHT_BATCH_H__
#define __CU_LIGHT_BATCH_H__
#include <cugl/physics2/lights/CULight.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/graphics/CUFrameBuffer.h>

namespace cugl {

// Forward declaration
class OrthographicCamera;

    /**
     * The classes to represent 2-d physics.
     *
     * For 2-d physics, CUGL uses the venerable box2d. For the most part, we
     * do not need anything more than that. However, box2d does involve a lot
     * of boilerplate code in setting up bodies and fixtures. Students have
     * found that they like the "training wheel" classes in this package.
     */
    namespace physics2 {
        /**
         * A C++ implementation of box2dlights
         *
         * Kalle Hameleinen's box2dlights is a relatively easy-to-use lighting
         * library made popular by LibGDX. This package is a reimagining of this
         * library for C++.
         */
        namespace lights {

/**
 * A batched mesh shader for rendering lights.
 *
 * This is the primary shader for creating composting lights. However, it does
 * not render any shadows and actually renders the lights inverted (so lights
 * are opaque and shadows are transparent). That is because this class is
 * never intended to be use directly. Instead, it is part of {@link LightMap}
 * which is the proper class for rendering lights.
 */
class LightBatch {
protected:
    /** The number of lights supported by this batch */
    Uint32 _capacity;
    /** The bounds of the world in box2d coordinates */
    Rect _bounds;
    /** Whether this light batch is currently active */
    bool _active;
    /** Whether to use gamma correction in the lights */
    bool _gamma;
    /** Whether to use premultiplied alpha */
    bool _premult;

    /** The sprite batch vertex mesh */
    LightVertex* _vertData;
    /** The vertex capacity of the mesh */
    Uint32 _vertMax;
    /** The number of vertices in the current mesh */
    Uint32 _vertSize;
    
    /** The indices for the vertex mesh */
    Uint32*  _indxData;
    /** The index capacity of the mesh */
    Uint32 _indxMax;
    /** The number of indices in the current mesh */
    Uint32 _indxSize;
    
    /** The orthogonal camera (in physics units) */
    std::shared_ptr<OrthographicCamera> _camera;
    /** The underlying graphics shader */
    std::shared_ptr<graphics::GraphicsShader> _shader;
    /** The vertex buffer */
    std::shared_ptr<graphics::VertexBuffer> _vertices;
    /** The index buffer */
    std::shared_ptr<graphics::IndexBuffer> _indices;
    
    /**
     * Resizes the mesh to support a larger capacity of lights
     *
     * @param capacity  The new capacity
     */
    void resize(Uint32 capacity);
    
public:
    /**
     * The default number of lights supported.
     *
     * This value assumes the lights all have the default number of rays. If
     * the lights have more rays, then the capacity is less. If this batch ever
     * exceeds its capacity, it will resize the buffers before calling
     * {@link #flush}.
     */
    static Uint32 DEFAULT_CAPACITY;
    
#pragma mark Constructors
    /**
     * Creates a degenerate light batch with no buffers.
     *
     * You must initialize the batch before using it.
     */
    LightBatch();
    
    /**
     * Deletes the light batch, disposing all resources
     */
    ~LightBatch() { dispose(); }
    
    /**
     * Deletes the vertex buffers and resets all attributes.
     *
     * You must reinitialize the light batch to use it.
     */
    void dispose();
    
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
    bool init(float units, Uint32 capacity=DEFAULT_CAPACITY);
    
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
    bool initWithBounds(const Rect& bounds, Uint32 capacity=DEFAULT_CAPACITY);
    
    /**
     * Returns a newly allocated light batch with the given physics units and capacity
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
     * @return a newly allocated light batch with the given physics units and capacity
     */
    static std::shared_ptr<LightBatch> alloc(float units, Uint32 capacity=DEFAULT_CAPACITY) {
        std::shared_ptr<LightBatch> result = std::make_shared<LightBatch>();
        return (result->init(units,capacity) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated light batch with the given bounds, physics units, and capacity.
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
     * @return a newly allocated light batch with the given bounds, physics units, and capacity.
     */
    static std::shared_ptr<LightBatch> allocWithBounds(const Rect& bounds,
                                                       Uint32 capacity=DEFAULT_CAPACITY) {
        std::shared_ptr<LightBatch> result = std::make_shared<LightBatch>();
        return (result->initWithBounds(bounds,capacity) ? result : nullptr);
    }

#pragma mark Attributes
    
    /**
     * Returns the supported capacity of this light batch
     *
     * The capacity is measured in number of lights supported. It is assumed
     * that each light supports the default 256 number of rays. Lights with
     * more rays will reduce the stated capacity, while lights with less rays
     * will increase it.
     *
     * Note that this value can change if too many lights are added to the
     * mesh. In that case the batch will resize automatically to support the
     * new number of lights.
     *
     * @return the supported capacity of this light batch
     */
    Uint32 getCapacity() const { return _capacity; }
    
    /**
     * Sets the supported capacity of this light batch
     *
     * The capacity is measured in number of lights supported. It is assumed
     * that each light supports the default 256 number of rays. Lights with
     * more rays will reduce the stated capacity, while lights with less rays
     * will increase it.
     *
     * Note that this value can change if too many lights are added to the
     * mesh. In that case the batch will resize automatically to support the
     * new number of lights.
     *
     * @param capacity  The capacity measured in standard lights.
     */
    void setCapacity(Uint32 capacity) {
        setCapacity(capacity,Light::DEFAULT_RAYS);
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
    void setCapacity(Uint32 capacity, Uint32 rays);
    
    /**
     * Returns the bounds of this light batch in physics units.
     *
     * This value is used to determine the {@link OrthographicCamera} for this
     * batch.
     *
     * @return the bounds of this light batch in physics units.
     */
    const Rect& getBounds() const { return _bounds; }
    
    /**
     * Sets the bounds of this light batch in physics units.
     *
     * This value is used to determine the {@link OrthographicCamera} for this
     * batch.
     *
     * @param bounds    The bounds of this light batch in physics units.
     */
    void setBounds(const Rect& bounds);

    /**
     * Returns true if the light batch shader uses gamma correction.
     *
     * Changing this value in the middle of a draw pass has no effect.
     *
     * @return true if the light batch shader uses gamma correction.
     */
    const bool getGamma() const { return _gamma; }
    
    /**
     * Sets whether the light batch shader uses gamma correction.
     *
     * Changing this value in the middle of a draw pass has no effect.
     *
     * @param value Whether the light batch shader uses gamma correction.
     */
    void setGamma(bool value) { _gamma = value; }
    
    /**
     * Returns true if light composition uses premultipled alpha.
     *
     * If this value is false, the light back will use (non-premultiplied)
     * alpha compositing instead.
     *
     * This value is true by default. {@link graphics::SpriteBatch} uses
     * premultiplied alpha in texture compositing due to how SDL3 reads image
     * files. Therefore, if this lighting is to be combined with a sprite batch,
     * it is important for the blending to be consistent.
     *
     * @return true if light composition uses premultipled alpha.
     */
    bool isPremultiplied() const { return _premult; }

    /**
     * Sets whether light composition uses premultipled alpha.
     *
     * If this value is false, the light back will use (non-premultiplied)
     * alpha compositing instead.
     *
     * This value is true by default. {@link graphics::SpriteBatch} uses
     * premultiplied alpha in texture compositing due to how SDL3 reads image
     * files. Therefore, if this lighting is to be combined with a sprite batch,
     * it is important for the blending to be consistent.
     *
     * @param value Whether light composition uses premultipled alpha.
     */
    void setPremultiplied(bool value) { _premult = value; }

#pragma mark Rendering
    /**
     * Starts a drawing pass for the current bounds and physics units.
     *
     * This call will disable depth buffer writing. It enables blending and
     * texturing. You must call either {@link #flush} or {@link #end} to
     * complete drawing.
     */
    void begin();

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
    void begin(const std::shared_ptr<graphics::FrameBuffer>& framebuffer);

    /**
     * Completes the drawing pass for this sprite batch, flushing the buffer.
     *
     * This method will unbind the associated shader. It must always be called
     * after a call to {@link #begin}.
     */
    void end();
    
    /**
     * Flushes the current mesh to the underlying shader.
     *
     * This methods converts all of the batched drawing commands into low-level
     * drawing operations in the associated graphics API. This method is useful
     * in OpenGL in that it allows us to recycle internal data structures,
     * reducing the memory footprint. However, it is rarely useful in Vulkan. It
     * is provided simply to keep the interface uniform between graphics APIs.
     */
    void flush();
    
    /**
     * Adds the given light to the mesh for drawing.
     *
     * If adding this light would exceed the mesh capacity, then this method
     * will resize the mesh to fit. Note, however, that resizing is a very
     * heavyweight operation and should be avoided.
     *
     * @param light The light to draw
     */
    void draw(const std::shared_ptr<Light>& light);

};

        }
    }
}
#endif /* __CU_LIGHT_BATCH_H__ */
