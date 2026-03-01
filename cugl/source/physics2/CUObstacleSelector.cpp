//
//  CUObstacleSelector.cpp
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
#include <cugl/physics2/CUObstacle.h>
#include <cugl/physics2/CUObstacleWorld.h>
#include <cugl/physics2/CUObstacleSelector.h>
#include <cugl/core/math/polygon/CUPolyFactory.h>
#include <box2d/b2_world.h>
#include <box2d/b2_circle_shape.h>

using namespace cugl;
using namespace cugl::physics2;
using namespace cugl::graphics;


#pragma mark -
#pragma mark Constructors

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
bool ObstacleSelector::init(const std::shared_ptr<ObstacleWorld>& world, const Size& mouseSize) {
    _controller = world;
    
    // Define the size and create a polygon
    _size = mouseSize;
    
    _jointDef.stiffness = DEFAULT_STIFFNESS;
    _jointDef.damping   = DEFAULT_DAMPING;
    _force = DEFAULT_FORCE;
    _controller->addObject(shared_from_this());

    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    b2CircleShape groundShape;
    groundShape.m_radius = _size.width;
    
    _ground = _controller->getWorld()->CreateBody(&groundDef);
    if (_ground != nullptr) {
        b2FixtureDef groundFixture;
        groundFixture.shape = &groundShape;
        return _ground->CreateFixture(&groundFixture) != nullptr;
    }
    
    return false;
}

/**
 * Disposes all of the resources used by this selector.
 *
 * A disposed selector can be safely reinitialized.
 */
void ObstacleSelector::dispose(void) {
    if (_controller) {
        _controller->removeObject(shared_from_this());
    }
    detachWorld(_controller.get());
}

/**
 * Detatches the given obstacle world from this object.
 *
 * This method ensures safe clean-up.
 *
 * @param world The world to detatch.
 */
void ObstacleSelector::detachWorld(const ObstacleWorld* obj) {
    if (_controller) {
        if (_mouseJoint != nullptr) {
            _controller->getWorld()->DestroyJoint(_mouseJoint);
            _mouseJoint = nullptr;
        }
        if (_ground != nullptr) {
            _controller->getWorld()->DestroyBody(_ground);
            _mouseJoint = nullptr;
        }
        _controller = nullptr;
    }
}



#pragma mark -
#pragma mark Selection Methods

/**
 * Sets the current position of this selector (in World space)
 *
 * @param x  the x-coordinate of the selector
 * @param y  the y-coordinate of the selector
 */
void ObstacleSelector::setPosition(float x, float y) {
    _position.set(x,y);
    if (_mouseJoint) {
        _mouseJoint->SetTarget(b2Vec2(x,y));
    }
}

/**
 * Returns true if a physics body was selected at the given position.
 *
 * This method contructs and AABB the size of the mouse pointer, centered at the
 * given position. If any part of the AABB overlaps a fixture, it is selected.
 *
 * @return true if a physics body was selected at the given position.
 */
bool ObstacleSelector::select() {
    if (!_controller) {
        return false;
    }
    
    std::function<bool(b2Fixture* fixture)> callback = [this](b2Fixture* fixture) {
        return onQuery(fixture);
    };
    
    Rect pointer(_position.x-_size.width/2.0f, _position.y-_size.height/2.0f,
                 _size.width,_size.height);
    _controller->queryAABB(callback,pointer);
    if (_selection != nullptr) {
        b2Body* body = _selection->GetBody();
        _jointDef.bodyA = _ground;  // Generally ignored
        _jointDef.bodyB = body;
        _jointDef.maxForce = _force * body->GetMass();
        _jointDef.target.Set(_position.x,_position.y);
        _mouseJoint = (b2MouseJoint*)_controller->getWorld()->CreateJoint(&_jointDef);
        body->SetAwake(true);
        
        Obstacle* obs = getObstacle();
        if (obs) {
            obs->setListener([this](Obstacle* obs) { updateTarget(obs); } );
        }
    } else {
        updateTarget(nullptr);
    }
    
    return _selection != nullptr;
}

/**
 * Deselects the physics body, discontinuing any mouse movement.
 *
 * The body may still continue to move of its own accord.
 */
void ObstacleSelector::deselect() {
    if (!_controller) {
        return;
    }

    if (_selection != nullptr) {
        Obstacle* obs = getObstacle();
        if (obs) {
            obs->setListener(nullptr);
        }
        updateTarget(nullptr);
        _controller->getWorld()->DestroyJoint(_mouseJoint);
        _selection = nullptr;
        _mouseJoint = nullptr;
    }
}

/**
 * Returns a (weak) reference to the Obstacle selected (if any)
 *
 * Just because a physics body was selected does not mean that an Obstacle
 * was selected.  The body could be a basic Box2d body generated by other
 * means. If the body is not an Obstacle, this method returns nullptr.
 *
 * @return a (weak) reference to the Obstacle selected (if any)
 */
Obstacle* ObstacleSelector::getObstacle() {
    if (!_controller) {
        return nullptr;
    }
    
    if (_selection != nullptr) {
        b2BodyUserData data = _selection->GetBody()->GetUserData();
        return reinterpret_cast<Obstacle*>(data.pointer);
    }
    return nullptr;
}

/**
 * Callback function for mouse selection.
 *
 * This is the callback function used by the method queryAABB to select a physics
 * body at the current mouse location.
 *
 * @param  fixture  the fixture selected
 *
 * @return false to terminate the query.
 */
bool ObstacleSelector::onQuery(b2Fixture* fixture) {
    bool cn = fixture->TestPoint(b2Vec2(_position.x,_position.y));
    bool tl = fixture->TestPoint(b2Vec2(_position.x-_size.width/2,_position.y+_size.height/2));
    bool bl = fixture->TestPoint(b2Vec2(_position.x-_size.width/2,_position.y-_size.height/2));
    bool tr = fixture->TestPoint(b2Vec2(_position.x+_size.width/2,_position.y+_size.height/2));
    bool br = fixture->TestPoint(b2Vec2(_position.x+_size.width/2,_position.y-_size.height/2));
    if (cn || tl || bl || tr || br) {
        _selection = fixture;
        return _selection == nullptr;
    }
    return true;
}


#pragma mark -
#pragma mark Graphics Support
/**
 * Sets the physics units for this light.
 *
 * Physics units are the number of pixels per box2d unit. These values are
 * used to create the debug wireframe. However, they are also useful for
 * any other object that you wish to attach to this obstacle.
 *
 * @param units The physics units for this light
 */
void ObstacleSelector::setPhysicsUnits(float units) {
    _units = units;
    resetMesh();
}

/**
 * Returns a wireframe mesh representing this selector.
 *
 * This mesh can be drawn by a {@link SpriteBatch} to help with debugging.
 * The wireframe will consist of the selector AABB (as a cross-hair) plus
 * a line from the selector to the selected body (if any)
 *
 * @return a wireframe mesh representing this selector.
 */
const graphics::Mesh<graphics::SpriteVertex>& ObstacleSelector::getDebugMesh() {
    if (_mesh.vertices.empty()) {
        resetMesh();
    }
    return _mesh;
}

/**
 * Returns a transform for displaying graphics attached to this obstacle.
 *
 * Obstacles are generally used to transform a 2d graphics object on the
 * screen. This includes {@link #getDebugMesh}. Passing this transform to
 * a {@link SpriteBatch} will display the object in its proper location.
 *
 * @return a transform for displaying graphics attached to this obstacle.
 */
const Affine2 ObstacleSelector::getGraphicsTransform() {
    Affine2 transform;
    transform.translate(getPosition()*_units);
    return transform;
}

/**
 * Repositions the debug node so that it agrees with the physics object.
 *
 * @param obstacle  The obstacle to match
 */
void ObstacleSelector::updateTarget(Obstacle* obstacle) {
    if (obstacle) {
        Vec2 pos = obstacle->getPosition()-getPosition();
        _mesh.vertices.back().position = pos*_units;
    } else {
        _mesh.vertices.back().position.setZero();
    }
}

/**
 * Creates the outline of the selector as a wireframe mesh.
 *
 * The debug mesh is use to outline the fixtures attached to this
 * object. This is very useful when the fixtures have a very different
 * shape than the texture (e.g. a circular shape attached to a square
 * texture). This mesh can then be passed to a {@link SpriteBatch} for
 * drawing.
 */
void ObstacleSelector::resetMesh() {
    Size size = _size*_units;
    
    // Add the four corners
    _mesh.clear();
    SpriteVertex v;
    v.color = Color4::WHITE;
    v.position.x = -size.width/2.0f;
    v.position.y = -size.height/2.0f;
    v.texcoord.setZero();
    v.gradcoord.setZero();
    _mesh.vertices.push_back(v);
    
    v.position.x = -size.width/2.0f;
    v.position.y =  size.height/2.0f;
    v.texcoord.y = 1;
    v.gradcoord.y = 1;
    _mesh.vertices.push_back(v);
    
    v.position.x =  size.width/2.0f;
    v.position.y =  size.height/2.0f;
    v.texcoord.x = 1;
    v.gradcoord.x = 1;
    _mesh.vertices.push_back(v);
    
    v.position.x =  size.width/2.0f;
    v.position.y = -size.height/2.0f;
    v.texcoord.y = 0;
    v.gradcoord.y = 0;
    _mesh.vertices.push_back(v);
    
    // Add the attachment line
    v.position.setZero();
    v.texcoord.set(0.5,0.5);
    v.gradcoord = v.texcoord;
    _mesh.vertices.push_back(v);
    _mesh.vertices.push_back(v);
    
    // Now the indices and draw command
    _mesh.indices.push_back(0);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(3);
    _mesh.indices.push_back(3);
    _mesh.indices.push_back(0);
    _mesh.indices.push_back(0);
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(3);
    _mesh.indices.push_back(4);
    _mesh.indices.push_back(5);
    _mesh.command = DrawMode::LINES;
}
