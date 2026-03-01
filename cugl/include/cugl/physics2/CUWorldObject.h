//
//  CUWorldObject.h
//  Cornell University Game Library (CUGL)
//
//  To make box2d simpler for students, we often wrap bodies and fixtures into
//  a single entity called an Obstacle. But sometimes we want box2d bodies with
//  no fixtures. But without the wrapper, ObstacleWorld may not know to do
//  clean up the object on deletion. That is the purpose of this abstract class.
//  It is an interface to listen for world deletions to clean up properly.
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
//  This file is based on the CS 3152 PhysicsDemo Lab by Don Holden, 2007
//
//  Author: Walker White
//  Version: 12/2/25 (SDL3 Integration)
//
#ifndef __CU_WORLD_OBJECT_H__
#define __CU_WORLD_OBJECT_H__

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

class ObstacleWorld;
    
/**
 * An interface for box2d objects that require clean-up.
 *
 * If a {@link ObstacleWorld} is deleted, then all box2d values are deleted,
 * not just the obstacles and joints. This interface is for other box2d
 * features that need cleanup in this case.
 */
class WorldObject : public std::enable_shared_from_this<WorldObject> {
public:
    /**
     * Detatches the given obstacle world from this object.
     *
     * This method ensures safe clean-up.
     *
     * @param world The world to detatch.
     */
    virtual void detachWorld(const ObstacleWorld* world) {}
};
    
    }
}

#endif /* __CU_WORLD_OBJECT_H__ */
