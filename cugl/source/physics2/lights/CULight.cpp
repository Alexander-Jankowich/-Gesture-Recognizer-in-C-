//
//  CULight.cpp
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
#include <cugl/physics2/lights/CULight.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2;
using namespace cugl::physics2::lights;


/** The minimum supported number of rays */
const int Light::MIN_RAYS = 3;
/** The default number of rays to use */
int Light::DEFAULT_RAYS = 256;
/** The default distance for a light (in box2d units) */
float Light::DEFAULT_REACH = 15.0f;
/** The default shadow softness (in box2d units) */
float Light::DEFAULT_SOFTNESS = 0.1f;

/** The global filter for all lights **/
b2Filter Light::_globalFilter;

/**
 * Returns true if the ray from src to dst overlaps the given rectangle.
 *
 * This function is used for culling rays that are out of bounds.
 *
 * @param bounds    The test rectangle
 * @param src       The ray start
 * @param dst       The ray end
 *
 * @return true if the ray from src to dst overlaps the given rectangle.
 */
static bool overlaps_ray(const Rect& bounds, const b2Vec2& src, const b2Vec2& dst) {
    if (src.x < bounds.origin.x && dst.x < bounds.origin.x) {
        return false;
    } else if (src.x > bounds.origin.x+bounds.size.width &&
               dst.x > bounds.origin.x+bounds.size.width) {
        return false;
    }
    if (src.y < bounds.origin.y && dst.y < bounds.origin.y) {
        return false;
    } else if (src.y > bounds.origin.y+bounds.size.height &&
               dst.y > bounds.origin.y+bounds.size.height) {
        return false;
    }
    return true;
}

#pragma mark Constructors
/**
 * Creates a light with no mesh or drawing information.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
Light::Light(void) :
_rayNum(0),
_gamma(1.0f),
_angle(0),
_reach(DEFAULT_REACH),
_shadowFactor(DEFAULT_SOFTNESS),
_active(false),
_dirty(false),
_soft(true),
_xray(false),
_staticLight(false),
_body(NULL),
_bodyAngle(0),
_units(1.0f),
_ignoreBody(false),
_debug(false) {
}

/**
 * Deletes the light, freeing all resources.
 *
 * You must reinitialize the light to use it.
 */
void Light::dispose() {
    _lightMesh.clear();
    _debugMesh.clear();
    _rayNum = 0;
    _gamma = 1.0f;
    _angle = 0;
    _reach = DEFAULT_REACH;
    _shadowFactor = DEFAULT_SOFTNESS;
    _active = false;
    _dirty = false;
    _soft = true;
    _xray = false;
    _staticLight = false;
    _body = NULL;
    _bodyAngle = 0;
    _units = 1.0f;
    _ignoreBody = false;
    _debug = false;
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
bool Light::initWithPositionReach(const Vec2& pos, float reach, Uint32 rays) {

    if (rays < MIN_RAYS) {
        rays = MIN_RAYS;
    }
    _position = pos;
    setReach(reach);
    setSoftness(DEFAULT_SOFTNESS);
    setRayNum(rays);
    setColor(Color4(192,192,192,192));
    
    // Create the callback function
    _raycast.onQuery = [this](b2Fixture* fixture, const Vec2& point,
                              const Vec2&normal, float fraction) {
        
        if (this->filters(this->_contactFilter,fixture)) {
            return -1.0f;
        }

        if (this->filters(this->_globalFilter,fixture)) {
            return -1.0f;
        }

        if (this->_ignoreBody && this->_body != NULL && fixture->GetBody() == _body) {
            return -1.0f;
        }

        if (fixture->IsSensor()) {
            return -1.0f;
        }

        Uint32 idx = this->_raycast.index;
        this->_endPoints[2*idx+1] = point;
        this->_fractions[idx] = fraction;

        return fraction;
    };
    
    _active = true;
    return true;
}

#pragma mark -
#pragma mark Physics
/**
 * Sets the number of rays for this light.
 *
 * This is an internal method to update the associated mesh.
 *
 * @param rays    The number of rays for this light
 */
void Light::setRayNum(Uint32 rays) {
    if (rays < MIN_RAYS) {
        rays = MIN_RAYS;
    }

    _rayNum = rays;
    initRays();
}

/**
 * Resets the rays to the current orientation.
 *
 * This extends all rays to their full length. It should be called just
 * before ray casting.
 */
void Light::resetRays() {
    Vec2 temp;
    for(int ii = 0; ii < _rayNum; ii++) {
        _endPoints[2*ii  ] = _lightRays[2*ii  ]+_position;
        _endPoints[2*ii+1] = _lightRays[2*ii+1]+_position;
        _fractions[ii] = 1.0f;
    }
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
void Light::attachBody(b2Body* body, const Vec2& offset, float radians) {
    _body = body;
    _bodyOffset = offset;
    _bodyAngle  = radians;
    if (_staticLight) _dirty = true;
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
bool Light::filters(const b2Filter& filter, b2Fixture* fixture) {
    b2Filter filterB = fixture->GetFilterData();

    if (filter.groupIndex != 0 &&
        filter.groupIndex == filterB.groupIndex) {
        return filter.groupIndex < 0;
    }

    return  (filter.maskBits & filterB.categoryBits) == 0 ||
            (filter.categoryBits & filterB.maskBits) == 0;
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
void Light::update(b2World* world, const Rect& bounds) {
    if (_staticLight && !_dirty) {
        return;
    }

    if (_body != NULL) {
        b2Vec2 p = _body->GetPosition();
        _position.set(p.x,p.y);
        _position += _bodyOffset;
        _angle = 180.0f*_body->GetAngle()/M_PI + _bodyAngle;
    }
    resetRays();

    if (world != NULL && !_xray) {
        b2Vec2 src, dst;
        for(int ii = 0; ii < _rayNum; ii++) {
            _raycast.index = ii;
            src.x = _endPoints[2*ii  ].x;
            src.y = _endPoints[2*ii  ].y;
            dst.x = _endPoints[2*ii+1].x;
            dst.y = _endPoints[2*ii+1].y;
            if (overlaps_ray(bounds, src, dst)) {
                world->RayCast(&_raycast, src, dst);
            }
        }
    }

    updateMesh();
    _dirty = false;
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
const Affine2 Light::getGraphicsTransform() const {
    Affine2 transform;
    transform.rotate(getAngle());
    transform.translate(getPosition()*_units);
    return transform;
}
