//
//  CUConeLight.h
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
//  This specific module is the class for a light object that is focused
//  in a cone pointed in a specific direction. This class uses our standard
//  shared-pointer architecture.
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
#ifndef __CU_CONE_LIGHT_H__
#define __CU_CONE_LIGHT_H__
#include <cugl/physics2/lights/CULight.h>


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
 * This class is a light shaped as a circle's sector with given angle
 *
 * As with {@link PointLight}, the rays have an position and extend from that
 * origin. The light rays are separated by an angle of raynum/(2*halfarc),
 * measure in radians.
 */
class ConeLight : public Light {
public:
    /** The default cone arc size */
    static const float DEFAULT_ARC;
    
protected:
    /** Half the arc angle of this cone in radians */
    float _halfArc;

    /**
     * Creates the ray template and associated data structures
     *
     * This method should be called after any action that would alter the ray
     * template. Such examples include changing the distance or the number of
     * rays.
     *
     * This method is specific to each light type
     */
    virtual void initRays() override;
    
    /**
     * Updates the mesh(es) associated with this light.
     *
     * This will always update the light mesh. It will only update the debug
     * mesh if {@link #getDebug} is true.
     */
    virtual void updateMesh() override;

#pragma mark Constructors
public:
    /**
     * Creates a cone light with no mesh or rays.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    ConeLight(void) : Light() {
        _halfArc = DEFAULT_ARC/2.0f;
    }
        
    /**
     * Deletes this cone light, disposing all resources.
     */
    ~ConeLight() { dispose(); }
    
    /**
     * Initializes a cone-shaped light with the given number of rays.
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All attributes will have their default values. The light will be placed
     * at the world origin, and will have the default reach. The light color
     * will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param arc   The cone arc in radians
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool init(float arc, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(Vec2::ZERO, DEFAULT_REACH, arc, rays);
    }

    /**
     * Initializes a cone-shaped light with the given number of rays and reach
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light will be
     * placed at the world origin. The light color will be a transluscent light
     * grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param radius    The light reach
     * @param arc       The cone arc in radians
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithReach(float radius, float arc, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(Vec2::ZERO, radius, arc, rays);
    }
   
    /**
     * Initializes a cone shaped light with the given rays and position
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light will have
     * the default reach. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos       The light position
     * @param arc       The cone arc in radians
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPosition(const Vec2& pos, float arc, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(pos, DEFAULT_REACH, arc, rays);
    }

    /**
     * Initializes a cone shaped light with the given rays, position, and reach
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light color will
     * be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos       The light position
     * @param radius    The light reach
     * @param arc       The cone arc in radians
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    bool initWithPositionReach(const Vec2& pos, float radius, float arc, Uint32 rays=DEFAULT_RAYS);
    
    /**
     * Returns a newly allocated cone-shaped light with the given number of rays.
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All attributes will have their default values. The light will be placed
     * at the world origin, and will have the default reach. The light color
     * will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param arc       The cone arc in radians
     * @param rays      The number of rays
     *
     * @return a newly allocated cone-shaped light with the given number of rays.
     */
    static std::shared_ptr<ConeLight> alloc(float arc, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ConeLight> result = std::make_shared<ConeLight>();
        return (result->init(arc, rays) ? result : nullptr);

    }

    /**
     * Returns a newly allocated cone-shaped light with the given reach
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light will be
     * placed at the world origin. The light color will be a transluscent light
     * grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param radius    The light reach
     * @param arc       The cone arc in radians
     * @param rays      The number of rays
     *
     * @return a newly allocated cone-shaped light with the given reach
     */

    static std::shared_ptr<ConeLight> allocWithReach(float radius, float arc, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ConeLight> result = std::make_shared<ConeLight>();
        return (result->initWithReach(radius,arc, rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated cone-shaped light with the given position
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light will have
     * the default reach. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos       The light position
     * @param arc       The size of the cone arc in radians
     * @param rays      The number of rays
     *
     * @return  a newly allocated cone-shaped light with the given position
     */
    static std::shared_ptr<ConeLight> allocWithPosition(const Vec2& pos, float arc, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ConeLight> result = std::make_shared<ConeLight>();
        return (result->initWithPosition(pos, arc,rays) ? result : nullptr);
    }

    /**
     * Returns a newly allocated cone-shaped light with the given position and reach
     *
     * The arc defines the section of the circle swept by the light. This arc
     * is centered on the light direction, which is 0 by default.
     *
     * All other attributes will have their default values. The light color will
     * be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos       The light reach
     * @param radius    The light radius
     * @param arc       The size of the cone arc in radians
     * @param rays      The number of rays
     *
     * @return a newly allocated cone-shaped light with the given position and reach
     */
    static std::shared_ptr<ConeLight> allocWithArc(const Vec2& pos, float radius, float arc,
                                                   Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ConeLight> result = std::make_shared<ConeLight>();
        return (result->initWithPositionReach(pos, radius, arc,rays) ? result : nullptr);
    }
    
#pragma mark Attributes
    /**
     * Returns the cone arc angle of this light
     *
     * The cone is centered over the direction angle. This means that half of
     * this angle is covered by the cone on each side of the direction vector.
     *
     * @return the cone angle of this light
     */
    float getArc() const { return 2*_halfArc; }

    /**
     * Sets the cone arc angle of this light
     *
     *
     * The cone is centered over the direction angle. This means that half of
     * this angle is covered by the cone on each side of the direction vector.
     * Note that actual recalculations will be done only on an {@link #update()}
     * call.
     *
     * @param arc   The cone angle in radians
     */
    void setArc(float arc);
    
    /**
     * Returns true if given point is inside of this light area
     *
     * @param p the point in world coordinates
     *
     * @return true if given point is inside of this light area
     */
    virtual bool contains(const Vec2& p) override;
    
};

        }
    }
}

#endif /* __CU_CONE_LIGHT_H__ */
