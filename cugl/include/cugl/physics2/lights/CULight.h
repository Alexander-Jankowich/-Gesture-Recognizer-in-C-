//
//  CULight.h
//  Cornell University Game Library (CUGL)
//
//  This module is part of a reimagining of Kalle Hameleinen's box2dLights
//  (version 1.5). Box2dLights is distributed under the Apache license. For the
//  original source code, refer to
//
//    https://github.com/libgdx/box2dlights
//
//  Our goal has been to remove the circular dependencies, providing a better
//  separation of geometry and rendering. In addition, this version is
//  compatible with our CUGL's rendering backend (either OpenGL or Vulkan).
//  Future plans are to offload the raycasting to a compute shader.
//
//  This specific module is the base class for a light object. As it is an
//  abstract class, it has no allocators and is not intended to be instantiated.
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
#ifndef __CU_LIGHT_H__
#define __CU_LIGHT_H__
#include <cugl/core/math/CUColor4.h>
#include <cugl/core/math/CUVec2.h>
#include <cugl/graphics/CUMesh.h>
#include <cugl/graphics/CUSpriteVertex.h>
#include <cugl/physics2/CUWorldObject.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>

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
 * This class/struct is rendering information for a a light.
 *
 * The class is intended to be used as a struct. This struct has the basic
 * rendering information required by a {@link LightMap} for rendering. As
 * lights are built on top of box2d, all measurements are in meters.
 */
class LightVertex {
public:
    /** The vertex position */
    Vec2   position;
    /** The vertex color */
    Color4 color;
    /** The vertex scale */
    float  scale;
    
    /**
     * Creates a new LightVertex
     */
    LightVertex() : scale(1) {}
    
};

#pragma mark -

/**
 * A lightweight b2RayCastCallback proxy.
 *
 * This class allows us to replace the listener class with a modern closure.
 */
class LightRaycast : public b2RayCastCallback {
public:
    /** The current light index */
    Uint32 index;

    /**
     * Creates a new raycast proxy
     */
    LightRaycast() { onQuery = nullptr; }
    
    /**
     * Called for each fixture found in the query.
     *
     * This callback controls how the ray cast proceeds by returning a float.
     * If -1, it ignores this fixture and continues.  If 0, it terminates the
     * ray cast.  If 1, it does not clip the ray and continues.  Finally, for
     * any fraction, it clips the ray at that point.
     *
     * @param  fixture  the fixture hit by the ray
     * @param  point    the point of initial intersection
     * @param  normal   the normal vector at the point of intersection
     * @param  faction  the fraction to return
     *
     * @return -1 to filter, 0 to terminate, fraction to clip the ray, 1 to continue
     */
    std::function<float(b2Fixture* fixture, const Vec2& point,
                        const Vec2&normal, float fraction)> onQuery;
    
    /**
     * Called for each fixture found in the query.
     *
     * This callback controls how the ray cast proceeds by returning a float.
     * If -1, it ignores this fixture and continues.  If 0, it terminates the
     * ray cast.  If 1, it does not clip the ray and continues.  Finally, for
     * any fraction, it clips the ray at that point.
     *
     * @param  fixture  the fixture hit by the ray
     * @param  point    the point of initial intersection
     * @param  normal   the normal vector at the point of intersection
     * @param  fraction the fraction to return
     *
     * @return -1 to filter, 0 to terminate, fraction to clip the ray, 1 to continue
     */
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
                        const b2Vec2& normal, float fraction) override {
        if (onQuery != nullptr) {
            return onQuery(fixture,Vec2(point.x,point.y),Vec2(normal.x,normal.y),fraction);
        }
        return -1;
    }
};
        
#pragma mark -
#pragma mark Light

/**
 * This is an abstract class for all light objects.
 *
 * This is a fairly minimal class that exists primarily for polymorphic reasons.
 * As a general rule, you will want one of the subclasses, such as
 * {@link PointLight}, {@link ConeLight}, or {@link DirectionalLight}.
 *
 * While a light can be attached to a box2d body, it does not own the body.
 * That body should be constructed elsewhere such as in an {@link Obstacle}
 * object. This class should use the {@link WorldObject} interface to be
 * notified when the underlying world has been released.
 *
 * Note that lights have no methods for rendering. That is done via the classes
 * {@link LightBatch}. The methods {@link #getLightMesh} and {@link #getDebugMesh}
 * extract meshes to be used in LightBatch or {@link graphics::SpriteBatch},
 * respectively.
 *
 * As an abstract class there are no static constructors.
 */
class Light : public WorldObject {
public:
    /** The minimum supported number of rays */
    static const int MIN_RAYS;
    /** The default number of rays to use */
    static int DEFAULT_RAYS;
    /** The default reach for a light (in box2d units) */
    static float DEFAULT_REACH;
    /** The default shadow softness (in box2d units) */
    static float DEFAULT_SOFTNESS;

protected:
    /** The number of light rays */
    Uint32 _rayNum;
    /** The light color */
    Color4 _color;
    /** The gamma correction factor */
    float _gamma;
    
    /** The current light position */
    Vec2 _position;
    /** The current light angle in radians */
    float _angle;
    /** The light reach (e.g. how far the light extends) */
    float _reach;
    /** The percentage of the reach to make as soft shadows */
    float _shadowFactor;
    
    /** Whether this light is active (inactive lights do not update or render) */
    bool _active;
    /** Whether this light requires an update */
    bool _dirty;
    /** Whether this light has soft tips */
    bool _soft;
    /** Whether this light shines through obstacles */
    bool _xray;
    /** Whether this light ignores dynamic game elements */
    bool _staticLight;
    
    /** The attached body (if any) */
    b2Body* _body;
    /** The positional offset from the body */
    Vec2 _bodyOffset;
    /** The angle offset of the light from the body */
    float _bodyAngle;
    /** The physics units for this light */
    float _units;
    /** Whether this light ignores the fixtures of the attached body */
    bool _ignoreBody;

    /** The global filter for all lights **/
    static b2Filter _globalFilter;
    /** The light specific filter **/
    b2Filter _contactFilter;

    /** The unmodified light rays for this light */
    std::vector<Vec2> _lightRays;
    /** The current ray endpoints */
    std::vector<Vec2> _endPoints;
    /** The normalized direction of each ray */
    std::vector<Vec2>  _directions;
    /** The fraction of each ray that is illuminated */
    std::vector<float> _fractions;
    /** The raycasting function for this light */
    LightRaycast _raycast;

    /** Whether to compute the debug mesh */
    bool _debug;
    /** The mesh for rendering the light*/
    graphics::Mesh<LightVertex> _lightMesh;
    /** The mesh for debugging (generated on demand) */
    graphics::Mesh<graphics::SpriteVertex> _debugMesh;
        
    /**
     * Creates the ray template and associated data structures
     *
     * This method should be called after any action that would alter the ray
     * template. Such examples include changing the reach or the number of
     * rays.
     *
     * This method is specific to each light type
     */
    virtual void initRays() = 0;
    
    /**
     * Resets the rays to the current orientation.
     *
     * This extends all rays to their full lemgth. It should be called just
     * before ray casting.
     */
    void resetRays();

    /**
     * Updates the mesh(es) associated with this light.
     *
     * This will always update the light mesh. It will only update the debug
     * mesh if {@link #getDebug} is true.
     */
    virtual void updateMesh() = 0;

public:
    /**
     * Creates a light with no mesh or drawing information.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    Light(void);
    
    /**
     * Deletes this light, disposing all resources.
     */
    ~Light() { dispose(); }
    
    /**
     * Deletes the light, freeing all resources.
     *
     * You must reinitialize the light to use it.
     */
    virtual void dispose();

    /**
     * Initializes a new light with the given number of rays.
     *
     * All attributes will have their default values. The light will be placed
     * at the world origin, and will have the default reach. The light color
     * will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool init(Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(Vec2::ZERO,DEFAULT_REACH,rays);
    }

    /**
     * Initializes a new light with the given number of rays and reach
     *
     * All other attributes will have their default values. The light will be
     * placed at the world origin. The light color will be a transluscent light
     * grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithReach(float reach, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(Vec2::ZERO,reach,rays);
    }
    
    /**
     * Initializes a new light with the given number of rays and position
     *
     * All other attributes will have their default values. The light will have
     * the default reach. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos   The light position
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPosition(const Vec2& pos, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(pos, DEFAULT_REACH, rays);
    }
    
    /**
     * Initializes a new light with the given number of rays, position, and reach
     *
     * All other attributes will have their default values. The light color will
     * be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos   The light position
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPositionReach(const Vec2& pos, float reach, Uint32 rays=DEFAULT_RAYS);

#pragma mark Attributes
    /**
     * Sets the number of rays for this light.
     *
     * This is an internal method to update the associated mesh.
     *
     * @param rays    The number of rays for this light
     */
    void setRayNum(Uint32 rays);

    /**
     * Returns the number of rays set for this light
     *
     * @return the number of rays set for this light
     */
    int getRayNum() { return _rayNum; }
    
    /**
     * Returns the position of the light
     *
     * All positions are measured in box2d world coordinates. If this light
     * has an attached box2d body, that body will determine this position.
     *
     * @return the position of the light
     */
    const Vec2& getPosition() const {
        return _position;
    }

    /**
     * Returns the horizontal position of the light
     *
     * All positions are measured in box2d world coordinates. If this light
     * has an attached box2d body, that body will determine this position.
     *
     * @return the horizontal starting of the light
     */
    float getX() const {
        return _position.x;
    }

    /**
     * Returns the vertical starting position of light
     *
     * All positions are measured in box2d world coordinates. If this light
     * has an attached box2d body, that body will determine this position.
     *
     * @return the vertical starting position of light
     */
    float getY() const {
        return _position.y;
    }

    /**
     * Sets the light starting position
     *
     * All positions are measured in box2d world coordinates. If this light
     * has an attached box2d body, that body will determine this position.
     * In particular, setting this value is only relevant if there is no
     * attached body. Otherwise this value will be overwritten at the next
     * call to {@link #update}.
     *
     * @param x The x-coordinate of the light
     * @param y The y-coordinate of the light
     */
    void setPosition(float x, float y) {
        _position.set(x,y);
        _dirty = true;
    }

    /**
     * Sets the light starting position
     *
     * All positions are measured in box2d world coordinates. If this light
     * has an attached box2d body, that body will determine this position.
     * In particular, setting this value is only relevant if there is no
     * attached body. Otherwise this value will be overwritten at the next
     * call to {@link #update}.
     *
     * @param position  The light position
     */
    void setPosition(const Vec2& position) {
        _position.set(position);
        if (_staticLight) _dirty = true;
    }
    /**
     * Returns the light reach
     *
     * The reach is the distance that the light extends from its position.
     *
     * NOTE: The minimum value should be capped to 0.1f meter. In addition,
     * this value does not include any gamma correction.
     *
     * @return the light reach
     */
    float getReach() const {
        return _reach;
    }

    /**
     * Sets the light reach
     *
     * The reach is the distance that the light extends from its position.
     *
     * NOTE: The minimum value should be capped to 0.1f meter. In addition,
     * this value does not include any gamma correction.
     *
     * @param dist  The light reach
     */
    void setReach(float dist) {
        _reach = std::max(0.01f,dist);
        // We actually need to redo the template in this case
        setRayNum(_rayNum);
    }

    /**
     * Returns the angle of the light in radians
     *
     * For lights with direction, the angle is typically the "facing" of the
     * light.
     *
     * @return the angle in radians
     */
    float getAngle() const {
        return _angle;
    }

    /**
     * Sets the angle of the light in radians
     *
     * For lights with direction, the angle is typically the "facing" of the
     * light.
     *
     * @param angle The angle in radians
     */
    void setAngle(float angle) {
        _angle = angle;
        // We actually need to redo the template in this case
        setRayNum(_rayNum);
    }
    
    /**
     * Returns the current color of this light
     *
     * The RGB values set the color while the alpha value sets the intensity.
     * Note that you can also use colorless (e.g {@link Color4#BLACK} light
     * with shadows. By default this is a light grey color.
     *
     * @return the current color of this light
     */
    Color4 getColor() const {
        return _color;
    }
    
    /**
     * Sets the light color
     *
     * The RGB values set the color while the alpha value sets the intensity.
     * Note that you can also use colorless (e.g {@link Color4#BLACK} light
     * with shadows. By default this is a light grey color.
     *
     * Note that this color will not be committed to mesh until a call to
     * {@link #update}.
     *
     * @param color The light color
     */
    void setColor(Color4 color) {
        _color = color;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Sets the light color
     *
     * The RGB values set the color while the alpha value sets the intensity.
     * Note that you can also use colorless (e.g {@link Color4#BLACK} light
     * with shadows. By default this is a light grey color.
     *
     * Note that this color will not be committed to mesh until a call to
     * {@link #update}.
     *
     * @param r	the red component
     * @param g	the green component
     * @param b	the blue component
     * @param a	the shadow intensity
     */
    void setColor(float r, float g, float b, float a) {
        _color.set(r, g, b, a);
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns the gamma correction factor for this light.
     *
     * This value is used to adjust the light color in the {@link LightBatch#draw}
     * method.
     *
     * @return the gamma correction factor for this light.
     */
    float getGamma() const {
        return _gamma;
    }
    
    /**
     * Sets the gamma correction factor for this light.
     *
     * This value is used to adjust the light color in the {@link LightBatch#draw}
     * method.
     *
     * @param gamma the gamma correction factor for this light.
     */
    void setGamma(float gamma) {
        _gamma = gamma;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns true if this light is active.
     *
     * An enabled light will respond to calls to {@link #update} and
     * {@link LightMap#render}.
     *
     * @return true if this light is active.
     */
    bool isActive() const {
        return _active;
    }
    
    /**
     * Enables/disables this light.
     *
     * An enabled light will respond to calls to {@link #update()} and
     * {@link LightMap#render}.
     *
     * @param active	Whether to enable this light
     */
    void setActive(bool active) {
        _active = active;
    }
    
    /**
     * Returns true if this light beams go through obstacles
     *
     * Enabling this will allow beams to go through obstacles that reduce CPU
     * burden of light about 70%. Use the combination of x-ray and non x-ray
     * lights wisely,
     *
     * @return true if this light beams go through obstacles
     */
    bool isXray() const {
        return _xray;
    }
    
    /**
     * Enables/disables x-ray beams for this light
     *
     * Enabling this will allow beams to go through obstacles that reduce CPU
     * burden of light about 70%. Use the combination of x-ray and non x-ray
     * lights wisely,
     *
     * @param xray	Whether to enable x-ray beams
     */
    void setXray(bool xray) {
        _xray = xray;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns true if this light is static
     *
     * Static lights do not get any automatic updates, but any changes to one
     * of its parameters will update it. Static lights are useful for lights
     * that you want to collide with static geometry but ignore all the dynamic
     * objects. This can reduce the CPU burden of the light by about 90%.
     *
     * @return true if this light is static
     */
    bool isStaticLight() const {
        return _staticLight;
    }
    
    /**
     * Enables/disables light static behavior
     *
     * Static lights do not get any automatic updates, but any changes to one
     * of its parameters will update it. Static lights are useful for lights
     * that you want to collide with static geometry but ignore all the dynamic
     * objects. This can reduce the CPU burden of the light by about 90%.
     *
     * @param staticLight	Whether to make this light static
     */
    void setStaticLight(bool staticLight) {
        _staticLight = staticLight;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns true if the tips of the light beams are soft
     *
     * This value is true by default.
     *
     * @return true if the tips of the light beams are soft
     */
    bool isSoft() const {
        return _soft;
    }
    
    /**
     * Enables/disables softness on tips of the light beams
     *
     * This value is true by default.
     *
     * @param soft 	Whether to enable soft tips
     */
    void setSoft(bool soft) {
        _soft = soft;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns the softness value for the beam tips.
     *
     * This value is multiplied by the reach of the light to get the length of
     * the soft shadows. The default value is {@code 0.1f}
     *
     * @return the softness value for the beam tips.
     */
    float getSoftness() const {
        return _shadowFactor;
    }

    /**
     * Sets the softness value for the beam tips.
     *
     * This value is multiplied by the reach of the light to get the length of
     * the soft shadows. The default value is {@code 0.1f}
     *
     * @param factor    The softness value for the beam tips.
     */
    void setSoftness(float factor) {
        _shadowFactor = factor;
        if (_staticLight) _dirty = true;
    }
    
    /**
     * Returns true if debugging is enabled.
     *
     * If this attribute is true, {@link #getDebugMesh} will return a non-empty
     * mesh for debugging.
     *
     * @return true if debugging is enabled.
     */
    bool getDebug() const {
        return _debug;
    }

    /**
     * Sets whether debugging is enabled.
     *
     * If this attribute is true, {@link #getDebugMesh} will return a non-empty
     * mesh for debugging.
     *
     * @param flag  Whether debugging should be enabled.
     */
    void setDebug(bool flag) {
        _debug = flag;
        if (_staticLight) _dirty = true;
    }

    
#pragma mark Geometry
    /**
     * Returns true if given point is inside of this light area
     *
     * @param x the horizontal position of point in world coordinates
     * @param y	the vertical position of point in world coordinates
     *
     * @return true if given point is inside of this light area
     */
    bool contains(float x, float y) {
        return contains(Vec2(x,y));
    }

    /**
     * Returns true if given point is inside of this light area
     *
     * @param p the point in world coordinates
     *
     * @return true if given point is inside of this light area
     */
    virtual bool contains(const Vec2& p) {
        return false;
    }
    
    /**
     * Returns the light mesh for this light.
     *
     * This light mesh can be passed to a {@link LightBatch} for drawing.
     * Vertices in a light mesh are in absolute worlds coordinates and do not
     * need to be transformed before sending them to the light batch.
     *
     * @return the light mesh for this light.
     */
    const graphics::Mesh<LightVertex>& getLightMesh() const {
        return _lightMesh;
    }
    
    /**
     * Returns the debug mesh for this light.
     *
     * This mesh is a wireframe that can be passed to {@link graphics::SpriteBatch}
     * for drawing, or used in a {@link scene2::MeshNode}. The coordinates of
     * the mesh are in the light coordinate space, meaning that they the origin
     * is the light position. They are scaled by the {@link #getPhysicsUnits}.
     * This means that you should use {@link #getGraphicsTransform} to draw this
     * mesh to a sprite batch.
     *
     * Note that if {@link #getDebug} is not true, this mesh will be empty.
     *
     * @return the debug mesh for this light.
     */
    const graphics::Mesh<graphics::SpriteVertex>& getDebugMesh() const {
        return _debugMesh;
    }
    
    /**
     * Returns a transform for displaying graphics attached to this light.
     *
     * Obstacles are generally used to transform a 2d graphics object on the
     * screen. This includes {@link #getDebugMesh}. Passing this transform to
     * a {@link graphics::SpriteBatch} will display the object in its proper
     * location.
     *
     * @return a transform for displaying graphics attached to this light.
     */
    const Affine2 getGraphicsTransform() const;

#pragma mark Physics
    /**
     * Attaches the light to the specified the body
     *
     * When attached, the light will automatically follow this body unless
     * stated otherwise. Note that the body rotation angle is taken into
     * account for the light offset and direction calculations.
     *
     * Note that lights do not own box2d bodies. The body should created (and
     * deleted) separately, either manually or part of an {@link Obstacle}
     * object.
     *
     * @param body  The body to attach this light to
     */
    void attachBody(b2Body* body) {
        attachBody(body, Vec2::ZERO, 0.0f);
    }

    /**
     * Attaches the light to the specified body with the relative offset
     *
     * When attached, the light will automatically follow this body. Note that
     * the body rotation angle is taken into account for the light offset
     * and direction calculations.
     *
     * Note that lights do not own box2d bodies. The body should created (and
     * deleted) separately, either manually or part of an {@link Obstacle}
     * object.
     *
     * @param body      The body to attach this light to
     * @param offset    The relative offset in world coordinates
     */
    void attachBody(b2Body* body, const Vec2& offset) {
        attachBody(body, offset, 0.0f);
    }

    /**
     * Attaches the light to the specified body with the relative offset and direction
     *
     * When attached, the light will automatically follow this body unless
     * stated otherwise. Note that the body rotation angle is taken into
     * account for the light offset and direction calculations.
     * 
     * Note that lights do not own box2d bodies. The body should created (and
     * deleted) separately, either manually or part of an {@link Obstacle}
     * object.
     *
     * @param body      The body to attach this light to
     * @param offset    The relative offset in world coordinates
     * @param radians   The angle offset offset in radians
     */
    void attachBody(b2Body* body, const Vec2& offset, float radians);

    /**
     * Returns the physics units for this light.
     *
     * Physics units are the number of pixels per box2d unit. These values are
     * used to create the debug mesh.
     *
     * @return the physics units for this light
     */
    float getPhysicsUnits() const {
        return _units;
    }

    /**
     * Sets the physics units for this light.
     *
     * Physics units are the number of pixels per box2d unit. These values are
     * used to create the debug mesh.
     *
     * @param units The physics units for this light
     */
    void setPhysicsUnits(float units) {
        _units = units;
        if (_staticLight) _dirty = true;
    }

    /**
     * Returns true if the attached body fixtures will be ignored during raycasting
     *
     * If the value is {@code true}, all the fixtures of attached body will be
     * ignored and will not create any shadows for this light. By default, this
     * value is set to {@code false}.
     *
     * @return true if the attached body fixtures will be ignored during raycasting
     */
    bool doesIgnoreAttachedBody() const {
        return _ignoreBody;
    }


    /**
     * Sets if the attached body fixtures should be ignored during raycasting
     *
     * If the value is {@code true}, all the fixtures of attached body will be
     * ignored and will not create any shadows for this light. By default, this
     * value is set to {@code false}.
     *
     * @param flag  Whether to ignore the attached body.
     */
    void setIgnoreAttachedBody(bool flag) {
        _ignoreBody = flag;
    }

    /**
     * Returns true if the given filter omits the given fixture.
     *
     * This method is used to eliminate any fixtures that would be filtered
     * out during raycasting.
     *
     * @param filter    The filter to apply
     * @param fixture   The fixture to test
     *
     * @return true if the given filter omits the given fixture.
     */
    static bool filters(const b2Filter& filter, b2Fixture* fixture);
    
    /**
     * Returns the contact filter for this light
     *
     * In raycasting, this filter is combined with {@link #getGlobalContactFilter}
     * to remove any fixtures that should be ignored.
     *
     * @return the contact filter for this light
     */
    const b2Filter& getContactFilter() const {
        return _contactFilter;
    }

    /**
     * Sets the contact filter for this light
     *
     * In raycasting, this filter is combined with {@link #getGlobalContactFilter}
     * to remove any fixtures that should be ignored.
     *
     * @param filter    The contact filter
     */
    void setContactFilter(const b2Filter& filter) {
        _contactFilter = filter;
    }
    
    /**
     * Returns the contact filter for all lights.
     *
     * In raycasting, this filter is combined with {@link #getContactFilter}
     * to remove any fixtures that should be ignored.
     *
     * @return the contact filter for all lights.
     */
    static const b2Filter& getGlobalContactFilter() {
        return _globalFilter;
    }

    /**
     * Sets the contact filter for all lights.
     *
     * In raycasting, this filter is combined with {@link #getContactFilter}
     * to remove any fixtures that should be ignored.
     *
     * @param filter    The contact filter
     */
    static void setGlobalContactFilter(const b2Filter& filter) {
        _globalFilter = filter;
    }

    /**
     * Detatches the given obstacle world from this object.
     *
     * This method ensures safe clean-up.
     *
     * @param world The world to detatch.
     */
    virtual void detachWorld(const ObstacleWorld* world) override {
        _body = NULL;
    }
    
    /**
     * Updates this light according to box2d.
     *
     * The bounds are used for culling. Rays outside of the bounds are not
     * processed and are set to 0.
     *
     * @param world     The box2d world
     * @param bounds    The world bounds
     */
    void update(b2World* world, const Rect& bounds);
    
    
};
    
        }
    }
}

#endif /* __CU_LIGHT_H__ */
