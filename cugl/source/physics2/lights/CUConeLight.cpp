//
//  CUConeLight.cpp
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
#include <cugl/physics2/lights/CUConeLight.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2::lights;

/** The default cone arc size */
const float ConeLight::DEFAULT_ARC = M_PI/3;

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
bool ConeLight::initWithPositionReach(const Vec2& pos, float radius, float arc, Uint32 rays) {
    if (arc < 0) {
        CUAssertLog(false, "Arc angle cannot be negative: %g",arc);
        return false;
    }
    _halfArc = arc/2.0f;
    return Light::initWithPositionReach(pos,radius,rays);
}

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
void ConeLight::setArc(float arc) {
    CUAssertLog(arc < 0, "Arc angle cannot be negative: %g",arc);
    _halfArc = arc/2.0f;
    if (_staticLight) _dirty = true;
}

/**
 * Creates the ray template and associated data structures
 *
 * This method should be called after any action that would alter the ray
 * template. Such examples include changing the distance or the number of
 * rays.
 *
 * This method is specific to each light type
 */
void ConeLight::initRays() {
    _lightRays.clear();
    _lightRays.resize(2*_rayNum);
    _endPoints.clear();
    _endPoints.resize(2*_rayNum);
    _directions.clear();
    _directions.resize(_rayNum);
    _fractions.clear();
    _fractions.resize(_rayNum);
    
    for (int ii = 0; ii < _rayNum; ii++) {
        float a = _halfArc - 2.0f * _halfArc * ii / (_rayNum - 1.0f);
        a += _angle;
        float c = cosf(a);
        float s = sinf(a);

        _directions[ii].set(c,s);
        _lightRays[2*ii  ].setZero();
        _lightRays[2*ii+1].set(_reach*c,_reach*s);
    }
}

/**
 * Updates the mesh(es) associated with this light.
 *
 * This will always update the light mesh. It will only update the debug
 * mesh if {@link #getDebug} is true.
 */
void ConeLight::updateMesh() {
    _lightMesh.clear();
    _debugMesh.clear();
    
    // Unlike box2dlights, we use TRIANGLES topology for composability
    _lightMesh.command = DrawMode::TRIANGLES;
    LightVertex vertex;
    vertex.color = _color;
    vertex.scale = 1.0f;
    
    // Define the core light
    vertex.position = _endPoints[0];
    _lightMesh.vertices.push_back(vertex);
    for(int ii = 0; ii < _rayNum; ii++) {
        vertex.position = _endPoints[2*ii+1];
        vertex.scale = (1 - _fractions[ii]);
        _lightMesh.vertices.push_back(vertex);
        if (ii < _rayNum-1) {
            _lightMesh.indices.push_back(0);
            _lightMesh.indices.push_back(ii+1);
            _lightMesh.indices.push_back(ii+2);
        }
    }
    
    // Debug is a wireframe of the rays
    if (_debug) {
        // Needs to be in object coordinate frame
        Affine2 transform;
        transform.translate(-_position);
        transform.rotate(-_angle);
        transform.scale(_units);
        
        _debugMesh.command = DrawMode::LINES;
        SpriteVertex sprite;
        sprite.position = transform.transform(_endPoints[0]);
        sprite.color = Color4::WHITE;
        sprite.texcoord.set(0.5,0.5);
        sprite.gradcoord.set(0.5,0.5);
        _debugMesh.vertices.push_back(sprite);
        
        Vec2 center(0.5,0.5);
        for(int ii = 0; ii < _rayNum-1; ii++) {
            sprite.position = transform.transform(_endPoints[2*ii+1]);
            sprite.texcoord = 0.5*_directions[ii]+center;
            sprite.gradcoord = sprite.texcoord;
            
            _debugMesh.vertices.push_back(sprite);
            _debugMesh.indices.push_back(0);
            _debugMesh.indices.push_back(ii+1);
        }
    }
    
    if (!_soft || _xray) {
        return;
    }

    Color4 clear = _color;
    clear.a = 0;

    // Add the shadow mesh
    // Unroll first part of loop as we are not ready for indices
    float s = (1 - _fractions[0]);
    Uint32 vcount = (Uint32)_lightMesh.vertices.size();
    vertex.position = _endPoints[1];
    vertex.color = _color;
    vertex.scale = s;
    _lightMesh.vertices.push_back(vertex);

    s *= _shadowFactor*_reach;
    vertex.position += _directions[0]*s;
    vertex.color = clear;
    vertex.scale = 0;
    _lightMesh.vertices.push_back(vertex);

    for (int ii = 1; ii < _rayNum; ii++) {
        float s = (1 - _fractions[ii]);
        vertex.position = _endPoints[2*ii+1];
        vertex.color = _color;
        vertex.scale = s;
        
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(vcount+2*ii-2);
        _lightMesh.indices.push_back(vcount+2*ii-1);
        _lightMesh.indices.push_back(vcount+2*ii  );

        s *= _shadowFactor*_reach;
        vertex.position += _directions[ii]*s;
        vertex.color = clear;
        vertex.scale = 0;

        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(vcount+2*ii-1);
        _lightMesh.indices.push_back(vcount+2*ii  );
        _lightMesh.indices.push_back(vcount+2*ii+1);
    }
}

/**
 * Returns true if given point is inside of this light area
 *
 * @param p the point in world coordinates
 *
 * @return true if given point is inside of this light area
 */
bool ConeLight::contains(const Vec2& p) {
    // fast fail
    Vec2 delta = _position-p;
    float dst2 = delta.lengthSquared();
    if (_reach * _reach <= dst2) {
        return false;
    }

    // odd-even rule
    bool oddNodes = false;
    Vec2 p1;
    Vec2 p2 = _position;
    for (int ii = 0; ii <= _rayNum; ++ii) {
        if (ii < _rayNum) {
            p1 = _endPoints[2*ii+1];
        } else {
            p1 = _endPoints[1];
        }
        if (((p1.y < p.y)  && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y <  p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }
    return oddNodes;
}
