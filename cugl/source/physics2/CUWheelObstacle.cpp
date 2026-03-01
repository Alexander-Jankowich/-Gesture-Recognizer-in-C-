//
//  CUWheelObstacle.cpp
//  Cornell University Game Library (CUGL)
//
//  This class implements a circular Physics object. We do not use it in any of
//  our samples,  but it is included for your education. Note that the shape
//  must be circular, not elliptical. If you want to make an ellipse, you will
//  need to use the PolygonObstacle class.
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
#include <cugl/physics2/CUWheelObstacle.h>
#include <cugl/core/math/polygon/CUPathFactory.h>

using namespace cugl::physics2;
using namespace cugl::graphics;

#pragma mark -
#pragma mark Constructors

/**
 * Initializes a new wheel object of the given dimensions.
 *
 * The scene graph is completely decoupled from the physics system.
 * The node does not have to be the same size as the physics body. We
 * only guarantee that the scene graph node is positioned correctly
 * according to the drawing scale.
 *
 * @param  pos      Initial position in world coordinates
 * @param  radius   The wheel radius
 *
 * @return  true if the obstacle is initialized properly, false otherwise.
 */
bool WheelObstacle::init(const Vec2& pos, float radius) {
    Obstacle::init(pos);
    _geometry = nullptr;
    _shape.m_radius = radius;
    resetMesh();
    return true;
}


#pragma mark -
#pragma mark Graphics Support
/**
 * Creates the outline of the physics fixtures as a wireframe mesh.
 *
 * The debug mesh is use to outline the fixtures attached to this
 * object. This is very useful when the fixtures have a very different
 * shape than the texture (e.g. a circular shape attached to a square
 * texture). This mesh can then be passed to a {@link SpriteBatch} for
 * drawing.
 */
void WheelObstacle::resetMesh() {
    PathFactory factory;
    Path2 path = factory.makeCircle(Vec2::ZERO,getRadius()*_units);

    _mesh.clear();
    Rect bounds = path.getBounds();
    for(auto it = path.vertices.begin(); it != path.vertices.end(); ++it) {
        SpriteVertex v;
        v.color = Color4::WHITE;
        v.position = *it;
        v.texcoord = (v.position-bounds.origin)/bounds.size;
        v.gradcoord = v.texcoord;
        _mesh.vertices.push_back(v);
    }
    
    for(int ii = 0; ii < path.vertices.size()-1; ii++) {
        _mesh.indices.push_back(ii);
        _mesh.indices.push_back(ii+1);
    }
    _mesh.indices.push_back((Uint32)path.vertices.size()-1);
    _mesh.indices.push_back(0);
    _mesh.command = DrawMode::LINES;
}

#pragma mark -
#pragma mark Physics Methods
/**
 * Create new fixtures for this body, defining the shape
 *
 * This is the primary method to override for custom physics objects
 */
void WheelObstacle::createFixtures() {
    if (_body == nullptr) {
        return;
    }
    
    releaseFixtures();
    
    // Create the fixture
    _fixture.shape = &_shape;
    _geometry = _body->CreateFixture(&_fixture);
    markDirty(false);
}

/**
 * Release the fixtures for this body, reseting the shape
 *
 * This is the primary method to override for custom physics objects
 */
void WheelObstacle::releaseFixtures() {
    if (_geometry != nullptr) {
        _body->DestroyFixture(_geometry);
        _geometry = nullptr;
    }
}
