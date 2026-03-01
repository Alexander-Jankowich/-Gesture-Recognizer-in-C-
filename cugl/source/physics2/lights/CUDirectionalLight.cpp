//
//  CUDirectionalLight.cpp
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
#include <cugl/physics2/lights/CUDirectionalLight.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2::lights;

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
bool DirectionalLight::initWithPositionReach(const Vec2& pos, float reach, Uint32 rays) {
    _angle = 0;
    _size.set(2*reach,2*reach);
    return Light::initWithPositionReach(pos,reach,rays);
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
bool DirectionalLight::initWithBoundsDirection(const Rect& bounds, float angle, Uint32 rays) {
    _angle = 0;
    _position = bounds.origin+bounds.size/2;
    _size = bounds.size;
    return Light::initWithPositionReach(bounds.origin+bounds.size/2,
                                        std::min(_size.width,_size.height)/2, rays);

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
void DirectionalLight::initRays() {
    _lightRays.clear();
    _lightRays.resize(2*_rayNum);
    _endPoints.clear();
    _endPoints.resize(2*_rayNum);
    _directions.clear();
    _directions.resize(_rayNum);
    _fractions.clear();
    _fractions.resize(_rayNum);
    
    Vec2 vec(cosf(_angle),sinf(_angle));
    _reach = (abs(vec.x)*_size.width+abs(vec.y)*_size.height)/2.0f;

    // Compute the light span and use for spacing
    float span = abs(vec.y)*_size.width+abs(vec.x)*_size.height;
    float space = span/(_rayNum-1);

    // The template is centered at the origin
    Vec2 perp(vec.y,-vec.x);
    Vec2 p = perp*span/2.0f;
    for (int ii = 0; ii < _rayNum; ii++) {
        _directions[ii] = vec;
        _lightRays[2*ii  ] = p-vec*(_reach);
        _lightRays[2*ii+1] = p+vec*(_reach);
        p -= perp*space;
    }
}

/**
 * Updates the mesh(es) associated with this light.
 *
 * This will always update the light mesh. It will only update the debug
 * mesh if {@link #getDebug} is true.
 */
void DirectionalLight::updateMesh() {
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
    vertex.position = _endPoints[1];
    _lightMesh.vertices.push_back(vertex);
    for(int ii = 1; ii < _rayNum; ii++) {
        vertex.position = _endPoints[2*ii  ];
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(2*ii-2);
        _lightMesh.indices.push_back(2*ii-1);
        _lightMesh.indices.push_back(2*ii  );
        vertex.position = _endPoints[2*ii+1];
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(2*ii-1);
        _lightMesh.indices.push_back(2*ii  );
        _lightMesh.indices.push_back(2*ii+1);
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
        sprite.color = Color4::WHITE;
        sprite.texcoord.setZero();
        sprite.gradcoord.setZero();
        
        for(int ii = 0; ii < _rayNum; ii++) {
            sprite.position = transform.transform(_endPoints[2*ii  ]);
            sprite.texcoord.x  = 0;
            sprite.gradcoord.x = 0;
            _debugMesh.vertices.push_back(sprite);
            
            sprite.position = transform.transform(_endPoints[2*ii+1]);
            sprite.texcoord.x  = 1;
            sprite.gradcoord.x = 1;
            _debugMesh.vertices.push_back(sprite);
            
            _debugMesh.indices.push_back(2*ii);
            _debugMesh.indices.push_back(2*ii+1);
            
            sprite.texcoord.y += 1/(_rayNum-1);
            sprite.gradcoord.y = sprite.texcoord.y;
        }
    }
    
    
    if (!_soft || _xray) {
        return;
    }

    Color4 clear = _color;
    clear.a = 0;

    // Add the shadow mesh
    // Unroll first part of loop as we are not ready for indices
    Uint32 vcount = (Uint32)_lightMesh.vertices.size();
    vertex.position = _endPoints[1];
    vertex.color = _color;
    _lightMesh.vertices.push_back(vertex);

    float shadowLength = _shadowFactor*_reach;
    vertex.position += _directions[0]*shadowLength;
    vertex.color = clear;
    _lightMesh.vertices.push_back(vertex);

    for (int ii = 1; ii < _rayNum; ii++) {
        vertex.position = _endPoints[2*ii+1];
        vertex.color = _color;
        
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(vcount+2*ii-2);
        _lightMesh.indices.push_back(vcount+2*ii-1);
        _lightMesh.indices.push_back(vcount+2*ii  );

        vertex.position += _directions[ii]*shadowLength;
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
bool DirectionalLight::contains(const Vec2& p) {
    // odd-even rule, run around the boundary
    bool oddNodes = false;
    Vec2 p1;
    Vec2 p2 = _endPoints[0];
    for (int ii = 0; ii <= _rayNum; ++ii) {
        p1 = _endPoints[2*ii+1];
        if (((p1.y < p.y)  && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y <  p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }
    for (int ii = _rayNum-1; ii >= 0; ii--) {
        p1 = _endPoints[2*ii];
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
