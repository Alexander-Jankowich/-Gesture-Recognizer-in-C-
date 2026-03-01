//
//  CUDirectionalLight.h
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
//  This specific module is the class for a light object that is positioned
//  infinitely far away (and so only has a direction). This class uses our
//  standard shared-pointer architecture.
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
#ifndef __CU_DIRECTIONAL_LIGHT_H__
#define __CU_DIRECTIONAL_LIGHT_H__
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
 * This class is light source which is placed at infinite distance away.
 *
 * While technically such a light does not have a position, we do define this
 * light as a bounding box, and the position is the origin of this bounding
 * box. Therefore, attaching this light to a body can cause the bounding box
 * to move.
 *
 * When defining the bounding box, you provide a width and height. The origin
 * of this box is in the center, and rotated to match the light angle.
 */
class DirectionalLight : public Light {
protected:
    /** The size of the bounding box */
    Size _size;
    
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
     * Creates a directional light with no mesh or rays.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    DirectionalLight(void) : Light() {}
        
    /**
     * Deletes this directional light, disposing all resources.
     */
    ~DirectionalLight() { dispose(); }
    
    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the default light reach on all
     * sides. This box will be centered on the light position, which is the
     * world origin.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool init(Uint32 rays=DEFAULT_RAYS) override {
        return initWithPositionReach(Vec2::ZERO,DEFAULT_REACH,rays);
    }

    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the given light reach on all
     * sides. This box will be centered on the light position, which is the
     * world origin.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithReach(float reach, Uint32 rays=DEFAULT_RAYS) override {
        return initWithPositionReach(Vec2::ZERO,reach,rays);
    }
    
    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the default light reach on all
     * sides. This box will be centered on the given light position.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos   The light position
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPosition(const Vec2& pos, Uint32 rays=DEFAULT_RAYS) override {
        return initWithPositionReach(pos,DEFAULT_REACH,rays);
    }

    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the given light reach on all
     * sides. This box will be centered on the given light position.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
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
    virtual bool initWithPositionReach(const Vec2& pos, float reach, Uint32 rays=DEFAULT_RAYS) override;
    
    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to the given bounding box. The light will be positioned at
     * the center of this box.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param bounds    The bounding box
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    bool initWithBounds(const Rect& bounds, Uint32 rays=DEFAULT_RAYS) {
        return initWithBoundsDirection(bounds,0,rays);
    }

    /**
     * Initializes a directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to the given bounding box. The light will be positioned at
     * the center of this box. The light rays will have the given angle passing
     * through this center.
     *
     * All other attributes will have their default values. The light color
     * will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param bounds    The bounding box
     * @param angle     The light angle
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    bool initWithBoundsDirection(const Rect& bounds, float angle, Uint32 rays=DEFAULT_RAYS);
    
    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the default light reach on all
     * sides. This box will be centered on the light position, which is the
     * world origin.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param rays  The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> alloc(Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->init(rays) ? result : nullptr);
    }

    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the given light reach on all
     * sides. This box will be centered on the light position, which is the
     * world origin.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> allocWithReach(float reach, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->initWithReach(reach, rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the default light reach on all
     * sides. This box will be centered on the given light position.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos   The light position
     * @param rays  The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> allocWithPosition(const Vec2& pos, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->initWithPosition(pos,rays) ? result : nullptr);
    }

    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to a bounding box of twice the given light reach on all
     * sides. This box will be centered on the given light position.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param pos   The light position
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> allocWithPositionReach(const Vec2& pos, float reach, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->initWithPositionReach(pos, reach, rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to the given bounding box. The light will be positioned at
     * the center of this box.
     *
     * All other attributes will have their default values. The angle of the
     * light will be 0 (aligned with the x-axis), and so the light distance
     * will be the width. The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param bounds    The bounding box
     * @param rays      The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> allocWithBounds(const Rect& bounds, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->initWithBounds(bounds, rays) ? result : nullptr);
    }

    /**
     * Returns a newly allocated directional light whose source is at an infinite distance
     *
     * The direction and intensity is same everywhere. However, the rays will
     * be restricted to the given bounding box. The light will be positioned at
     * the center of this box. The light rays will have the given angle passing
     * through this center.
     *
     * All other attributes will have their default values. The light color
     * will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param bounds    The bounding box
     * @param angle     The light angle
     * @param rays      The number of rays
     *
     * @return a newly allocated directional light whose source is at an infinite distance
     */
    static std::shared_ptr<DirectionalLight> allocWithBoundsDirection(const Rect& bounds, float angle,
                                                                      Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<DirectionalLight> result = std::make_shared<DirectionalLight>();
        return (result->initWithBoundsDirection(bounds, angle, rays) ? result : nullptr);
    }
    
#pragma mark Attributes
    /**
     * Returns the size of the light bounding box.
     *
     * The position of the light corresponds to the center of the bounding
     * box. The light distance will be such that a ray through the light
     * position fits inside the bounding box.
     *
     * @return the size of the light bounding box.
     */
    const Size& getSize() const { return _size; }

    /**
     * Sets the size of the light bounding box.
     *
     * The position of the light corresponds to the center of the bounding
     * box. The light distance will be such that a ray through the light
     * position fits inside the bounding box.
     *
     * @param size  The size of the bounding box
     */
    void setSize(const Size& size) {
        _size = size;
        // We actually need to redo the template in this case
        setRayNum(_rayNum);
    }
    
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

#endif /* __CU_DIRECTIONAL_LIGHT_H__ */
