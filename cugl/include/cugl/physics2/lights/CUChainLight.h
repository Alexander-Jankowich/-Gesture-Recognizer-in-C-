//
//  CUChainLight.h
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
//  This specific module is the class for a light whose rays are evenly
//  distributed along a chain of vertices (i.e. a path). This class uses our
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
#ifndef __CU_CHAIN_LIGHT_H__
#define __CU_CHAIN_LIGHT_H__
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
 * This class is a light whose ray starting points are evenly distributed along a path.
 *
 * The directions of the rays are perpendicular to the path at the ends, and
 * are interpolated in-between by means of a {@link Spinor}.
 */
class ChainLight : public Light {
protected:
    /** The chain path */
    Path2 _path;
    /** True if lights are to the right of (outside) the path */
    bool _right;
    /** The offset of the start of each ray from the path */
    float _rayOffset;
    /** The bounding box for this light */
    Rect _bounds;
    
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
    
    /**
     * Recomputes the bounding box for this light.
     *
     * Note that this is a conservative bounding box that does not take into
     * account any rays being blocked.
     */
    void computeBounds();
    
    /**
     * Returns true if given point is inside of this light area
     *
     * This method assumes that the path is open.
     *
     * @param p the point in world coordinates
     *
     * @return true if given point is inside of this light area
     */
    bool containsOpen(const Vec2& p);

    /**
     * Returns true if given point is inside of this light area
     *
     * This method assumes that the path is closed.
     *
     * @param p the point in world coordinates
     *
     * @return true if given point is inside of this light area
     */
    bool containsClosed(const Vec2& p);

#pragma mark Constructors
public:
    /**
     * Creates a chain light with no mesh or rays.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    ChainLight(void) :
    Light(),
    _right(true),
    _rayOffset(0) {
    }
        
    /**
     * Deletes this chain light, disposing all resources.
     */
    ~ChainLight() { dispose(); }
    
    /**
     * Deletes the chain light, freeing all resources.
     *
     * You must reinitialize the light to use it.
     */
    virtual void dispose() override;
    
    /**
     * Initializes a chain light with the given path
     *
     * All attributes will have their default values. The position of the light
     * will be the world origin, regardless of how far away the path is from
     * that origin. The light rays will be to the right of the path, which is
     * considered the outside for a traditional counter-clockwise path. The
     * rays will have the default reach. The light color will be a transluscent
     * light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool init(const Path2& path, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(path,Vec2::ZERO,DEFAULT_REACH,rays);
    }

    /**
     * Initializes a chain light with the given path and reach
     *
     * All other attributes will have their default values. The position of the
     * light will be the world origin, regardless of how far away the path is
     * from that origin. The light rays will be to the right of the path, which is
     * considered the outside for a traditional counter-clockwise path. The
     * light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param reach     The light reach
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithReach(const Path2& path, float reach, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(path,Vec2::ZERO,reach,rays);
    }
    
    /**
     * Initializes a chain light with the given path and position
     *
     * The position does not have to be within the bounds of the path. All
     * other attributes will all have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * traditional counter-clockwise path. The rays will have the default reach.
     * The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param pos   The light position
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPosition(const Path2& path, const Vec2& pos, Uint32 rays=DEFAULT_RAYS) {
        return initWithPositionReach(path,pos,DEFAULT_REACH,rays);
    }
    
    /**
     * Initializes a chain light with the given path, position, and reach.
     *
     * The position does not have to be within the bounds of the path. All
     * other attributes will all have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * traditional counter-clockwise path. The light color will be a transluscent
     * light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param pos   The light position
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return true if initialization is successful
     */
    virtual bool initWithPositionReach(const Path2& path, const Vec2& pos, float reach,
                                       Uint32 rays=DEFAULT_RAYS);
    
    /**
     * Initializes a chain light with the given path and anchor
     *
     * The anchor is used to determine the position of the chain light relative
     * to the path vertices. It is interpreted as a percentage of the bounding
     * box of the path. So an anchor of (0.5,0.5) would be in the center of the
     * bounding box, while (1,1) would be in the top right corner.
     *
     * All other attributes will have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * standard counter-clockwise path. The rays will have the default reach.
     * The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param anchor    The position anchor
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    bool initWithAnchor(const Path2& path, const Vec2& anchor, Uint32 rays=DEFAULT_RAYS) {
        return initWithAnchorReach(path,anchor,DEFAULT_REACH,rays);
    }
    
    /**
     * Initializes a chain light with the given path, anchor, and reach
     *
     * The anchor is used to determine the position of the chain light relative
     * to the path vertices. It is interpreted as a percentage of the bounding
     * box of the path. So an anchor of (0.5,0.5) would be in the center of the
     * bounding box, while (1,1) would be in the top right corner.
     *
     * All other attributes will have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * standard counter-clockwise path. The light color will be a transluscent
     * light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param anchor    The position anchor
     * @param reach     The light reach
     * @param rays      The number of rays
     *
     * @return true if initialization is successful
     */
    bool initWithAnchorReach(const Path2& path, const Vec2& anchor, float reach,
                             Uint32 rays=DEFAULT_RAYS);
    
    /**
     * Returns a newly allocated chain light with the given path
     *
     * All attributes will have their default values. The position of the light
     * will be the world origin, regardless of how far away the path is from
     * that origin. The light rays will be to the right of the path, which is
     * considered the outside for a traditional counter-clockwise path. The
     * light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param rays  The number of rays
     *
     * @return a newly allocated chain light with the given path
     */
    static std::shared_ptr<ChainLight> alloc(const Path2& path, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->init(path,rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated chain light with the given path and reach
     *
     * All other attributes will have their default values. The position of the
     * light will be the world origin, regardless of how far away the path is
     * from that origin. The light rays will be to the right of the path, which is
     * considered the outside for a traditional counter-clockwise path. The
     * light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param reach     The light reach
     * @param rays      The number of rays
     *
     * @return a newly allocated chain light with the given path and reach
     */
    static std::shared_ptr<ChainLight> allocWithReach(const Path2& path, float reach, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->initWithReach(path,reach,rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated chain light with the given path and position
     *
     * The position does not have to be within the bounds of the path. All
     * other attributes will all have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * traditional counter-clockwise path. The rays will have the default reach.
     * The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param pos   The light position
     * @param rays  The number of rays
     *
     * @return a newly allocated chain light with the given path and position
     */
    static std::shared_ptr<ChainLight> allocWithPosition(const Path2& path, const Vec2& pos, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->initWithPosition(path,pos,rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated chain light with the given path, position, and reach.
     *
     * The position does not have to be within the bounds of the path. All
     * other attributes will all have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * traditional counter-clockwise path. The light color will be a transluscent
     * light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path  The path of vertices
     * @param pos   The light position
     * @param reach The light reach
     * @param rays  The number of rays
     *
     * @return a newly allocated chain light with the given path, position, and reach.
     */
    static std::shared_ptr<ChainLight> allocWithPositionReach(const Path2& path, const Vec2& pos,
                                                              float reach,
                                                              Uint32 rays=DEFAULT_RAYS){
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->initWithPositionReach(path,pos,reach,rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated chain light with the given path and anchor
     *
     * The anchor is used to determine the position of the chain light relative
     * to the path vertices. It is interpreted as a percentage of the bounding
     * box of the path. So an anchor of (0.5,0.5) would be in the center of the
     * bounding box, while (1,1) would be in the top right corner.
     *
     * All other attributes will have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * standard counter-clockwise path. The rays will have the default reach.
     * The light color will be a transluscent light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param anchor    The position anchor
     * @param rays      The number of rays
     *
     * @return a newly allocated chain light with the given path and anchor
     */
    static std::shared_ptr<ChainLight> allocWithAnchor(const Path2& path,
                                                       const Vec2& anchor,
                                                       Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->initWithAnchor(path,anchor,rays) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated chain light with the given path, anchor, and distance
     *
     * The anchor is used to determine the position of the chain light relative
     * to the path vertices. It is interpreted as a percentage of the bounding
     * box of the path. So an anchor of (0.5,0.5) would be in the center of the
     * bounding box, while (1,1) would be in the top right corner.
     *
     * All other attributes will have their default values. The light rays will
     * be to the right of the path, which is considered the outside for a
     * standard counter-clockwise path. The light color will be a transluscent
     * light grey.
     *
     * The number of rays cannot be less than {@link #MIN_RAYS}. More rays make
     * the light look more realistic but will decrease performance.
     *
     * @param path      The path of vertices
     * @param anchor    The position anchor
     * @param distance  The light distance
     * @param rays      The number of rays
     *
     * @return a newly allocated chain light with the given path, anchor, and distance
     */
    static std::shared_ptr<ChainLight> allocWithAnchorReach(const Path2& path, const Vec2& anchor,
                                                       float distance, Uint32 rays=DEFAULT_RAYS) {
        std::shared_ptr<ChainLight> result = std::make_shared<ChainLight>();
        return (result->initWithAnchorReach(path,anchor,distance,rays) ? result : nullptr);
    }
    
#pragma mark Attributes
    /**
     * Returns the path of this chain light.
     *
     * @return the path of this chain light.
     */
    const Path2& getPath() const { return _path; }

    /**
     * Sets the path of this chain light.
     *
     * @param path  The path of this chain light.
     */
    void setPath(const Path2& path) {
        _path = path;
        // We actually need to redo the template in this case
        setRayNum(_rayNum);
    }
    
    /**
     * Returns whether the lights are to the right of the path.
     *
     * As proper paths wind counter-clockwise, the right side of a path is
     * defined to be the outside. If this is false, the rays will be generated
     * on the left side, or inside. This value is true by default.
     *
     * @return whether the lights are to the right of the path.
     */
    bool isRightward() const { return _right; }

    /**
     * Sets whether the lights are to the right of the path.
     *
     * As proper paths wind counter-clockwise, the right side of a path is
     * defined to be the outside. If this is false, the rays will be generated
     * on the left side, or inside. This value is true by default.
     *
     * @param right Whether the lights are to the right of the path.
     */
    void setRightward(bool right) {
        _right = right;
        // We actually need to redo the template in this case
        setRayNum(_rayNum);
    }

    /**
     * Returns the offset distance of the rays from the path.
     *
     * If this value is 0, then the rays will start touching the path. This
     * allows us to adjust that position in the direction of the ray. The
     * value is measured in box2d units.
     *
     * @return the offset distance of the rays from the path.
     */
    float getRayOffset() const { return _right; }

    /**
     * Sets the offset distance of the rays from the path.
     *
     * If this value is 0, then the rays will start touching the path. This
     * allows us to adjust that position in the direction of the ray. The
     * value is measured in box2d units.
     *
     * @param offset    The offset distance of the rays from the path.
     */
    void setRayOffset(float offset) {
        _rayOffset = offset;
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

#endif /* __CU_CHAIN_LIGHT_H__ */
