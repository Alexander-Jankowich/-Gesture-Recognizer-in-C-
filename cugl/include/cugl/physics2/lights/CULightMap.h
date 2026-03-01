//
//  CULightMap.h
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
//  This specific module is a layer of composited lights. While it is typically
//  attached to a LightBatch, it has additional shaders (and framebuffers) for
//  the compositing pass. The map can either be rendered directly to the screen
//  or it can be drawn to a texture for later use in a SpriteBatch.
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
//  Version: 1/4/26
//
#ifndef __CU_LIGHT_MAP_H__
#define __CU_LIGHT_MAP_H__
#include <cugl/physics2/lights/CULightBatch.h>
#include <cugl/graphics/CUSpriteBatch.h>
#include <unordered_map>
#include <vector>

namespace cugl {

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
 * An class for compositing 2d lights.
 *
 * This class takes a collection of {@link Light} objects and uses them to
 * generate a shadow layer. This shadow layer is typically drawn on top of
 * a scene to produce fog-of-war or other obscuring effects. This layer can
 * either be drawn directly to the screen via {@link #render} or it can be
 * saved as a texture accessible via {@link #getTexture}.
 */
class LightMap  : public WorldObject {
protected:
    /** The world to use in this light map */
	std::shared_ptr<ObstacleWorld> _world;
    /** The bounds of the world in box2d coordinates */
    Rect _bounds;
    /**
     * The number of pixels per box2d unit
     *
     * This value is used to compute the size of the framebuffer(s). By default
     * it is 1/4 the ratio of the display and the box2d bounds (creating a
     * framebuffer 1/4 the size of the display.
     */
    float _units;
    /** Whether to use gamma correction */
    bool _gamma;
    /** Whether to use premultiplied alpha */
    bool _premult;
    /** Whether to render offscreen to a texture */
    bool _offscreen;
    /** The number of blur passes to use */
    Uint32 _blurpass;
    /** The ambient color */
    Color4 _ambient;
    
    /** The lights added to this map*/
    std::unordered_map<size_t,std::shared_ptr<Light>> _lights;
    /** The light batch for drawing */
    std::shared_ptr<LightBatch> _batch;
    
    /** The first ping-pong framebuffer */
    std::shared_ptr<graphics::FrameBuffer> _framebuffer1;
    /** The second ping-pong framebuffer */
    std::shared_ptr<graphics::FrameBuffer> _framebuffer2;
    /** The orthographic camera for drawing */
    std::shared_ptr<OrthographicCamera> _camera;
    /** The gaussian shader */
    std::shared_ptr<graphics::GraphicsShader> _gaussianShader;
    /** The shadow shader */
    std::shared_ptr<graphics::GraphicsShader> _shadowShader;
    /** The full screen quad vertex buffer */
    std::shared_ptr<graphics::VertexBuffer> _quadVertices;
	
public:
    /**
     * Creates a degenerate light map with no buffers.
     *
     * You must initialize the map before using it.
     */
    LightMap();
    
    /**
     * Deletes the light map, disposing all resources
     */
    ~LightMap() { dispose(); }
    
    /**
     * Deletes the internal shaders and resets all attributes.
     *
     * You must reinitialize the light map to use it.
     */
    void dispose();

    /**
     * Initializes a new light map using the given physics units
     *
     * The physics units are the number of pixels per box2d unit. This method
     * sets the bounds of the light map to be the size of the display divided
     * by the physics units. That way, the physics world takes up the entire
     * display.
     *
     * If this map uses offscreen rendering, then the framebuffer is guaranteed
     * to be the same size as the display.
     *
     * @param units The physics units
     *
     * @return true if initialization was successful.
     */
    bool init(float units);

    /**
     * Initializes a new light map with the given bounds.
     *
     * The physics units are the number of pixels per box2d unit. They are
     * computed by dividing the size of the display by the box2d bounds and
     * taking the smallest dimension.
     *
     * If this map uses offscreen rendering, then the framebuffer is guaranteed
     * to be the same size as the display.
     *
     * @param bounds    The bounds of the physics world
     *
     * @return true if initialization was successful.
     */
    bool initWithBounds(const Rect& bounds);

    /**
     * Initializes a new light map with the given bounds and physics units.
     *
     * The physics units are the number of pixels per box2d unit. This map will
     * draw to a region the size of bounds times the physics units, which may
     * or may not be the entire display
     *
     * If this map uses offscreen rendering, then the framebuffer will also be
     * the size of bounds times the physics units.
     *
     * @param bounds    The bounds of the physics world
     * @param units     The physics units
     *
     * @return true if initialization was successful.
     */
    bool initWithBounds(const Rect& bounds, float units);

    /**
     * Returns a newly allocated light map using the given physics units
     *
     * The physics units are the number of pixels per box2d unit. This method
     * sets the bounds of the light map to be the size of the display divided
     * by the physics units. That way, the physics world takes up the entire
     * display.
     *
     * If this map uses offscreen rendering, then the framebuffer is guaranteed
     * to be the same size as the display.
     *
     * @param units The physics units
     *
     * @return a newly allocated light map using the given physics units
     */
    static std::shared_ptr<LightMap> alloc(float units) {
        std::shared_ptr<LightMap> result = std::make_shared<LightMap>();
        return (result->init(units) ? result : nullptr);
    }

    /**
     * Returns a newly allocated light map with the given bounds.
     *
     * The physics units are the number of pixels per box2d unit. They are
     * computed by dividing the size of the display by the box2d bounds and
     * taking the smallest dimension.
     *
     * If this map uses offscreen rendering, then the framebuffer is guaranteed
     * to be the same size as the display.
     *
     * @param bounds    The bounds of the physics world
     *
     * @return a newly allocated light map with the given bounds.
     */
    static std::shared_ptr<LightMap> allocWithBounds(const Rect& bounds) {
        std::shared_ptr<LightMap> result = std::make_shared<LightMap>();
        return (result->initWithBounds(bounds) ? result : nullptr);
    }

    /**
     * Returns a newly allocated light map with the given bounds and physics units.
     *
     * The physics units are the number of pixels per box2d unit. This map will
     * draw to a region the size of bounds times the physics units, which may
     * or may not be the entire display
     *
     * If this map uses offscreen rendering, then the framebuffer will also be
     * the size of bounds times the physics units.
     *
     * @param bounds    The bounds of the physics world
     * @param units     The physics units
     *
     * @return a newly allocated light map with the given bounds and physics units.
     */
    static std::shared_ptr<LightMap> allocWithBounds(const Rect& bounds, float units) {
        std::shared_ptr<LightMap> result = std::make_shared<LightMap>();
        return (result->initWithBounds(bounds,units) ? result : nullptr);
    }
    
#pragma mark Attributes
    /**
     * Returns the bounds of this light batch in physics units.
     *
     * This value and {@link #getPhysicsUnits} is used to determine the
     * {@link OrthographicCamera} for this batch.
     *
     * @return the bounds of this light batch in physics units.
     */
    const Rect& getBounds() const { return _bounds; }
    
    /**
     * Sets the bounds of this light batch in physics units.
     *
     * This value and {@link #getPhysicsUnits} is used to determine the
     * {@link OrthographicCamera} for this batch.
     *
     * @param bounds    The bounds of this light batch in physics units.
     */
    void setBounds(const Rect& bounds);

    /**
     * Returns the ambient color of this light map.
     *
     * The ambient color is the color of shadows. By default, it is pure black,
     * representing the fact that nothing is visible in the shadows. Decreasing
     * the alpha of the ambient color has the affect of making the scene lighter.
     *
     * This color should use premultipled alpha if {@link #isPremultiplied}
     * is true.
     *
     * @return the ambient color of this light map.
     */
    const Color4 getAmbient() const { return _ambient; }

    /**
     * Sets the ambient color of this light map.
     *
     * The ambient color is the color of shadows. By default, it is pure black,
     * representing the fact that nothing is visible in the shadows. Decreasing
     * the alpha of the ambient color has the affect of making the scene lighter.
     *
     * This color should use premultipled alpha if {@link #isPremultiplied}
     * is true.
     *
     * @param color The ambient color of this light map.
     */
    void setAmbient(const Color4 color) { _ambient = color; }

    /**
     * Returns true if the lightmap uses gamma correction.
     *
     * @return true if the lightmap uses gamma correction.
     */
    const bool getGamma() const { return _gamma; }
    
    /**
     * Sets whether the lightmap uses gamma correction.
     *
     * @param value Whether the lightmap uses gamma correction.
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

    /**
     * Returns the number of blur passes to apply when compositing
     *
     * If this value is non-zero, a Gaussian blur will be applied to the scene
     * to soften the lights. Each value above 1 is an additional pass. Blurring
     * can be a pretty heavy weight operation, but values of 1-3 should be safe.
     *
     * The default value is 1. Setting this value to 0 will disable blurring.
     *
     * @return the number of blur passes to apply when compositing
     */
    Uint32 getBlurPasses() const { return _blurpass; }
    
    /**
     * Sets the number of blur passes to apply when compositing
     *
     * If this value is non-zero, a Gaussian blur will be applied to the scene
     * to soften the lights. Each value above 1 is an additional pass. Blurring
     * can be a pretty heavy weight operation, but values of 1-3 should be safe.
     *
     * The default value is 1. Setting this value to 0 will disable blurring.
     *
     * @param passes    The number of blur passes to apply when compositing
     */
    void setBlurPasses(Uint32 passes) { _blurpass = passes; }
    
#pragma mark Light Management
    /**
     * Adds a light to this map
     *
     * Adding a light will associate it with the {@link #getWorld} and assign
     * the physics units to match this map. However, a light will only be
     * drawn if it is active.
     *
     * @param light The light to add
     */
    void addLight(const std::shared_ptr<Light>& light);

    /**
     * Removes a light from this map
     *
     * Adding a light will disassociate it from the {@link #getWorld}.
     *
     * @param light The light to remove
     *
     * @return true if the light was successfully removed
     */
    bool removeLight(const std::shared_ptr<Light>& light);
    
    /**
     * Returns the list of lights associated with this map
     *
     * @return the list of lights associated with this map
     */
    const std::vector<std::shared_ptr<Light>> getLights() const;

    /**
     * Removes all lights from this map.
     */
    void clearLights();
    
#pragma mark Physics
    /**
     * Returns the physics units for this light.
     *
     * Physics units are the number of pixels per box2d unit. This value is
     * used to create the internal framebuffers.
     *
     * @return the physics units for this light
     */
    float getPhysicsUnits() const { return _units; }

    /**
     * Sets the physics units for this light.
     *
     * Physics units are the number of pixels per box2d unit. This value is
     * used to create the internal framebuffers.
     *
     * @param units The physics units for this light
     */
    void setPhysicsUnits(float units);
    
    /**
     * Returns the obstacle world associated with this light map.
     *
     * @return the obstacle world associated with this light map.
     */
    const std::shared_ptr<ObstacleWorld>& getWorld() const { return _world; }
    
    /**
     * Detatches the given obstacle world from this map.
     *
     * This method sets the world for use in calls to {@link #update}.
     *
     * @param world The world to attach.
     */
    void attachWorld(const std::shared_ptr<ObstacleWorld>& world);

    /**
     * Detatches the given obstacle world from this object.
     *
     * This method ensures safe clean-up.
     *
     * @param world The world to detatch.
     */
    virtual void detachWorld(const ObstacleWorld* world) override;
    
    /**
     * Updates all of the active lights in the scene
     *
     * This method uses raycasting on the world provided by {@link #attachWorld}.
     * If there is no world provided, this method does nothing.
     */
    void update();
    
#pragma mark Rendering
    /**
     * Returns true if the lightmap renders to an offscreen buffer.
     *
     * If this value is true, calls to {@link #render} to no draw to the display.
     * Instead they render to an offscreen buffer than can be accessed via
     * the method {@link #getTexture}.
     *
     * @return true if the lightmap renders to an offscreen buffer.
     */
    const bool isOffscreen() const { return _offscreen; }
    
    /**
     * Sets whether the lightmap renders to an offscreen buffer.
     *
     * If this value is true, calls to {@link #render} to no draw to the display.
     * Instead they render to an offscreen buffer than can be accessed via
     * the method {@link #getTexture}.
     *
     * @param value Whether the lightmap renders to an offscreen buffer.
     */
    void setOffscreen(bool value) { _offscreen = value; }

    /**
     * Returns the texture for the offscreen rendering buffer
     *
     * If {@link #isOffscreen} is false, this method will return nullptr.
     * Otherwise, it will return a texture containing the results of the
     * most recent call to {@link #render}
     */
    const std::shared_ptr<graphics::Texture> getTexture() const;
    
    /**
     * Draws the lights in the scene.
     *
     * This method draws a shadow layer created by the most recent call to
     * {@link #update}. Areas not covered by lights will use {@link #getAmbient}
     * for their color.
     *
     * If {@link #isOffscreen} is false, this method will draw directly to the
     * screen. Otherwise, it will draw the results to an offscreen buffer that
     * can be accessed with {@link #getTexture}.
     */
    void render();

    /**
     * Draws the lights within the given bounds.
     *
     * This method draws a shadow layer created by the most recent call to
     * {@link #update}. Areas not covered by lights will use {@link #getAmbient}
     * for their color.
     *
     * The bounds is used to restrict the portion of the display or framebuffer
     * that this method uses when drawing (e.g. it will apply a view scissor).
     * The bounds window is measured in box2d units, and it should fit within
     * the bounds of the light map. The origin of this window is the bottom
     * left.
     *
     * If {@link #isOffscreen} is false, this method will draw directly to the
     * screen. Otherwise, it will draw the results to an offscreen buffer that
     * can be accessed with {@link #getTexture}.
     *
     * @param bounds    The scissor region to draw to
     */
    void render(const Rect& bounds) {
        render(bounds.origin.x,bounds.origin.y,bounds.size.width,bounds.size.height);
    }

    /**
     * Draws the lights within the given bounds.
     *
     * This method draws a shadow layer created by the most recent call to
     * {@link #update}. Areas not covered by lights will use {@link #getAmbient}
     * for their color.
     *
     * The bounds is used to restrict the portion of the display or framebuffer
     * that this method uses when drawing (e.g. it will apply a view scissor).
     * The bounds window is measured in box2d units, and it should fit within
     * the bounds of the light map. The origin of this window is the bottom
     * left.
     *
     * If {@link #isOffscreen} is false, this method will draw directly to the
     * screen. Otherwise, it will draw the results to an offscreen buffer that
     * can be accessed with {@link #getTexture}.
     *
     * @param x         The x-coordinate of the scissor origin
     * @param y         The x-coordinate of the scissor origin
     * @param width     The scissor width
     * @param height    The scissor height
     */
    void render(float x, float y, float width, float height);


};

        }
    }
}

#endif /* __CU_LIGHT_MAP_H__ */
