//
//  CUObstacleSelector.h
//  Cornell University Game Library (CUGL)
//
//  This class implements a selection tool for dragging physics objects with a
//  mouse. It is essentially an instance of b2MouseJoint, but with an API that
//  makes it a lot easier to use. As with all instances of b2MouseJoint, there
//  will be some lag in the drag (though this is true on touch devices in general).
//  You can adjust the degree of this lag by adjusting the force.  However,
//  larger forces can cause artifacts when dragging an obstacle through other
//  obstacles.
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
//  This file is based on the CS 3152 PhysicsDemo Lab by Don Holden, 2007
//
//  Author: Walker White
//  Version: 12/2/25 (SDL3 Integration)
//
#ifndef __CU_OBSTACLE_SELECTOR_H__
#define __CU_OBSTACLE_SELECTOR_H__
#include <cugl/physics2/CUWorldObject.h>
#include <box2d/b2_mouse_joint.h>
#include <box2d/b2_fixture.h>

/** The default size of the mouse selector */
#define DEFAULT_MSIZE        0.2f
/** The default joint stiffness */
#define DEFAULT_STIFFNESS 1000.0f
/** The default damping force of the joint */
#define DEFAULT_DAMPING      0.7f
/** The default force multiplier of the selector */
#define DEFAULT_FORCE     1000.0f

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

// Forward reference to the ObstacleWorld
class ObstacleWorld;
    
#pragma mark -
#pragma mark Obstacle Selector
/**
 * Selection tool to move and drag physics obstacles
 *
 * This class is essentially an instance of b2MouseJoint, but with an API that 
 * makes it a lot easier to use. It must be attached to a WorldController on 
 * creation, and this controller can never change. If you want a selector for
 * a different ObstacleWorld, make a new instance.
 *
 * As with all instances of b2MouseJoint, there will be some lag in the drag 
 * (though this is true on touch devices in general). You can adjust the degree
 * of this lag by adjusting the force. However, larger forces can cause
 * artifacts when dragging an obstacle through other obstacles.
 */
class ObstacleSelector : public WorldObject {
protected:
    /** The ObstacleWorld associated with this selection */
    std::shared_ptr<ObstacleWorld> _controller;
    
    /** The location in world space of this selector */
    Vec2 _position;
    /** The size of the selection region (for accuracy) */
    Size _size;
    /** The amount to multiply by the mass to move the object */
    float _force;
    
    /** The current fixture selected by this tool (may be nullptr) */
    b2Fixture*  _selection;
    /** A default body used as the other half of the mouse joint */
    b2Body*     _ground;
    
    /** A reusable definition for creating a mouse joint */
    b2MouseJointDef   _jointDef;
    /** The current mouse joint, if an item is selected */
    b2MouseJoint*   _mouseJoint;
    
    /** A sprite mesh for debugging purposes */
    graphics::Mesh<graphics::SpriteVertex> _mesh;
    /** The physics units for this selector */
    float _units;

#pragma mark -
#pragma mark Graphics Internals
    /**
     * Creates the outline of the selector as a wireframe mesh.
     *
     * The debug mesh is use to outline the fixtures attached to this
     * object. This is very useful when the fixtures have a very different
     * shape than the texture (e.g. a circular shape attached to a square
     * texture). This mesh can then be passed to a {@link graphics::SpriteBatch}
     * for drawing.
     */
    void resetMesh();
    
    /**
     * Repositions the tail of the debug wireframe
     *
     * The last vertex in the debug wireframe is the offset from the selector
     * position to the selected target
     *
     * @param obstacle  The obstacle to match
     */
    void updateTarget(Obstacle* obstacle);
   
public:
#pragma mark Constructors
    /**
     * Creates a new ObstacleSelector
     *
     * The selector created is not usable.  This constructor only initializes 
     * default values.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    ObstacleSelector() :
    _controller(nullptr),
    _selection(nullptr),
    _mouseJoint(nullptr),
    _ground(nullptr),
    _units(1) {
    }
    
    /**
     * Disposes of this selector, releasing all resources.
     */
    ~ObstacleSelector() { dispose(); }
    
    /**
     * Disposes all of the resources used by this selector.
     *
     * A disposed selector can be safely reinitialized.
     */
    void dispose();
    
    /**
     * Initializes a new selector for the given ObstacleWorld
     *
     * This controller can never change.  If you want a selector for a different
     * ObstacleWorld, make a new instance.
     *
     * This initializer uses the default mouse size.
     *
     * @param world     the physics controller
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<ObstacleWorld>& world) {
        return init(world, Size(DEFAULT_MSIZE, DEFAULT_MSIZE));
    }
    
    /**
     * Initializes a new selector for the given ObstacleWorld and mouse size.
     *
     * This controller can never change.  If you want a selector for a different
     * ObstacleWorld, make a new instance.  However, the mouse size can be 
     * changed at any time.
     *
     * @param world     the physics controller
     * @param mouseSize the mouse size
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<ObstacleWorld>& world, const Size& mouseSize);
    
    /**
     * Detatches the given obstacle world from this object.
     *
     * This method ensures safe clean-up.
     *
     * @param world The world to detatch.
     */
    virtual void detachWorld(const ObstacleWorld* world) override;

#pragma mark Static Constructors
    /**
     * Returns a newly allocated ObstacleSelector for the given ObstacleWorld
     *
     * This controller can never change.  If you want a selector for a different
     * ObstacleWorld, make a new instance.
     *
     * This constructor uses the default mouse size.
     *
     * @param world     the physics controller
     *
     * @return a newly allocated ObstacleSelector for the given ObstacleWorld
     */
    static std::shared_ptr<ObstacleSelector> alloc(const std::shared_ptr<ObstacleWorld>& world) {
        std::shared_ptr<ObstacleSelector> result = std::make_shared<ObstacleSelector>();
        return (result->init(world) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ObstacleSelector for the given world and mouse size.
     *
     * This controller can never change.  If you want a selector for a different
     * ObstacleWorld, make a new instance.  However, the mouse size can be changed
     * at any time.
     *
     * @param world     the physics controller
     * @param mouseSize the mouse size
     *
     * @return an autoreleased selector object
     */
    static std::shared_ptr<ObstacleSelector> alloc(const std::shared_ptr<ObstacleWorld>& world,
                                                   const Size& mouseSize) {
        std::shared_ptr<ObstacleSelector> result = std::make_shared<ObstacleSelector>();
        return (result->init(world,mouseSize) ? result : nullptr);
    }

#pragma mark Positional Methods
    /**
     * Returns the current position of this selector (in World space)
     *
     * @return the current position of this selector (in World space)
     */
    const Vec2& getPosition() const { return _position; }

    /**
     * Sets the current position of this selector (in World space)
     *
     * @param x  the x-coordinate of the selector
     * @param y  the y-coordinate of the selector
     */
    void setPosition(float x, float y);

    /**
     * Sets the current position of this selector (in World space)
     *
     * @param pos the position of the selector
     */
    void setPosition(const Vec2& pos) { setPosition(pos.x,pos.y); }

    
    /**
     * Returns the current position of this selector in pixels.
     *
     * This value is the position multiplied by {@link #getPhysicsUnits}.
     *
     * @return the current position of this selector in pixels.
     */
    const Vec2 getWindowPosition() const { return _position*_units; }

    /**
     * Sets the current position of this selector in pixels.
     *
     * This value is the position multiplied by {@link #getPhysicsUnits}.
     *
     * @param x  the x-coordinate of the selector
     * @param y  the y-coordinate of the selector
     */
    void setWindowPosition(float x, float y) {
        setPosition(x/_units,y/_units);
    }

    /**
     * Sets the current position of this selector in pixels.
     *
     * This value is the position multiplied by {@link #getPhysicsUnits}.
     *
     * @param pos the position of the selector
     */
    void setWindowPosition(const Vec2& pos) {
        setWindowPosition(pos.x,pos.y);
    }
    
#pragma mark Selection Methods
    /**
     * Returns true if a physics body is currently selected
     *
     * @return true if a physics body is currently selected
     */
    bool isSelected() const { return _selection != nullptr; }
    
    /**
     * Returns true if a physics body was selected at the current position.
     *
     * This method contructs and AABB the size of the mouse pointer, centered at
     * the current position.  If any part of the AABB overlaps a fixture, it is
     * selected.
     *
     * @return true if a physics body was selected at the given position.
     */
    bool select();
    
    /**
     * Deselects the physics body, discontinuing any mouse movement.
     *
     * The body may still continue to move of its own accord.
     */
    void deselect();
    
    /**
     * Returns a (weak) reference to the Obstacle selected (if any)
     *
     * Just because a physics body was selected does not mean that an Obstacle
     * was selected.  The body could be a basic Box2d body generated by other
     * means. If the body is not an Obstacle, this method returns nullptr.
     *
     * @return a (weak) reference to the Obstacle selected (if any)
     */
    Obstacle* getObstacle();
    
    /**
     * Callback function for mouse selection.
     *
     * This is the callback function used by the method queryAABB to select a
     * physics body at the current mouse location.
     *
     * @param  fixture  the fixture selected
     *
     * @return false to terminate the query.
     */
    bool onQuery(b2Fixture* fixture);
    
    
#pragma mark Attribute Properties
    /**
     * Returns the stiffness of the mouse joint
     *
     * See the documentation of b2JointDef for more information on the response
     * speed.
     *
     * @return the response speed of the mouse joint
     */
    float getStiffness() const { return _jointDef.stiffness; }
    
    /**
     * Sets the stiffness of the mouse joint
     *
     * See the documentation of b2JointDef for more information on the response
     * speed.
     *
     * @param  stiffness    the stiffness of the mouse joint
     */
    void setStiffness(float stiffness) { _jointDef.stiffness = stiffness; }
    
    /**
     * Returns the damping ratio of the mouse joint
     *
     * See the documentation of b2JointDef for more information on the damping
     * ratio.
     *
     * @return the damping ratio of the mouse joint
     */
    float getDamping() const { return _jointDef.damping; }
    
    /**
     * Sets the damping ratio of the mouse joint
     *
     * See the documentation of b2JointDef for more information on the damping
     * ratio.
     *
     * @param  ratio   the damping ratio of the mouse joint
     */
    void setDamping(float ratio) { _jointDef.damping = ratio; }
    
    /**
     * Returns the force multiplier of the mouse joint
     *
     * The mouse joint will move the attached fixture with a force of this value
     * times the object mass.
     *
     * @return the force multiplier of the mouse joint
     */
    float getForce() const { return _force; }
    
    /**
     * Sets the force multiplier of the mouse joint
     *
     * The mouse joint will move the attached fixture with a force of this value
     * times the object mass.
     *
     * @param  force    the force multiplier of the mouse joint
     */
    void setForce(float force) { _force = force; }
    
    /**
     * Returns the size of the mouse pointer
     *
     * When a selection is made, this selector will create an axis-aligned 
     * bounding box centered at the mouse position.  Any fixture overlapping 
     * this box will be selected.  The size of this box is determined by this 
     * value.
     *
     * @return the size of the mouse pointer
     */
    const Size& getMouseSize() const { return _size; }
    
    /**
     * Sets the size of the mouse pointer
     *
     * When a selection is made, this selector will create an axis-aligned
     * bounding box centered at the mouse position.  Any fixture overlapping
     * this box will be selected.  The size of this box is determined by this
     * value.
     *
     * @param  size the size of the mouse pointer
     */
    void setMouseSize(const Size& size) {
        _size = size;
        resetMesh();
    }
    
    
#pragma mark -
#pragma mark Graphics Support
    /**
     * Returns the physics units for this obstacle selector
     *
     * Physics units are the number of pixels per box2d unit. These values are
     * used to create the debug wireframe. They also allow us to use this
     * selector to pick an object in screen coordinates.
     *
     * @return the physics units for this obstacle selector
     */
    float getPhysicsUnits() const {
        return _units;
    }

    /**
     * Sets the physics units for this obstacle selector
     *
     * Physics units are the number of pixels per box2d unit. These values are
     * used to create the debug wireframe. However, they are also useful for
     * any other object that you wish to attach to this obstacle.
     *
     * @param units The physics units for this obstacle selector
     */
    void setPhysicsUnits(float units);

    /**
     * Returns a wireframe mesh representing this selector.
     *
     * This mesh can be drawn by a {@link graphics::SpriteBatch} to help with
     * debugging. The wireframe will consist of the selector AABB (as a
     * cross-hair) plus a line from the selector to the selected body (if any).
     *
     * @return a wireframe mesh representing this selector.
     */
    const graphics::Mesh<graphics::SpriteVertex>& getDebugMesh();
    
    /**
     * Returns a transform for displaying graphics attached to this obstacle.
     *
     * Obstacles are generally used to transform a 2d graphics object on the
     * screen. This includes {@link #getDebugMesh}. Passing this transform to
     * a {@link graphics::SpriteBatch} will display the object in its proper
     * location.
     *
     * @return a transform for displaying graphics attached to this obstacle.
     */
    const Affine2 getGraphicsTransform();
        
};
	}
}
#endif /* __CU_OBSTACLE_SELECTOR_H__ */
