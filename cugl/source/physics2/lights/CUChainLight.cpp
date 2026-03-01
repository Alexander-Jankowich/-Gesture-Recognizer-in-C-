//
//  CUChainLight.cpp
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
#include <cugl/physics2/lights/CUChainLight.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2::lights;


/**
 * Returns the cast of x unless it is within epsilon of an int.
 *
 * It if it is within epsilon of an int, it snaps to that int value
 *
 * @param x     The value to cast
 * @param eps   The error epsilon
 *
 * @return the cast of x unless it is within epsilon of an int.
 */
static int cast_with_epsilon(float x, float eps = 1e-4f) {
    int i = static_cast<int>(x);
    //if (x > 0 && x + eps >= i + 1) {
    //    return i + 1;
    //}
    return i;
}

/** The default offset for each ray */
#define RAY_OFFSET  0.001f
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
bool ChainLight::initWithPositionReach(const Path2& path, const Vec2& pos,
                                       float reach, Uint32 rays) {
    _path = path;
    return Light::initWithPositionReach(pos,reach,rays);
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
bool ChainLight::initWithAnchorReach(const Path2& path, const Vec2& anchor,
                                     float reach, Uint32 rays){
    _path = path;
    Rect bounds = path.getBounds();
    Vec2 pos = bounds.origin+anchor*bounds.size;
    return Light::initWithPositionReach(pos,reach,rays);
}

/**
 * Deletes the chain light, freeing all resources.
 *
 * You must reinitialize the light to use it.
 */
void ChainLight::dispose() {
    _path.clear();
    Light::dispose();
}

/**
 * Returns true if given point is inside of this light area
 *
 * @param p the point in world coordinates
 *
 * @return true if given point is inside of this light area
 */
bool ChainLight::contains(const Vec2& p) {
    // fast fail
    if (!_bounds.contains(p)) {
        return false;
    }

    if (_path.closed) {
        return containsClosed(p);
    } else {
        return containsOpen(p);
    }
}

#pragma mark -
#pragma mark Internals
/**
 * Creates the ray template and associated data structures
 *
 * This method should be called after any action that would alter the ray
 * template. Such examples include changing the distance or the number of
 * rays.
 *
 * This method is specific to each light type
 */
void ChainLight::initRays() {
    _lightRays.clear();
    _lightRays.resize(2*_rayNum);
    _endPoints.clear();
    _endPoints.resize(2*_rayNum);
    _directions.clear();
    _directions.resize(_rayNum);
    _fractions.clear();
    _fractions.resize(_rayNum);

    Uint32 segmentCount = (Uint32)(_path.closed ? _path.size() : _path.size() - 1);

    std::vector<float> segmentAngles;
    std::vector<float> segmentLengths;
    float totalLength = 0;

    Vec2 vec1, vec2;
    Vec2 segStart, segDir;
    for (int ii = 0; ii < _path.size()-1; ii++) {
        vec1  = _path.vertices[ii+1];
        vec1 -= _path.vertices[ii  ];

        float len = vec1.length();
        vec2 = _right ? vec1.getRPerp() : vec1.getPerp();
        float angle = vec2.getAngle();
        segmentLengths.push_back(len);
        segmentAngles.push_back(angle);
        totalLength += len;
    }
    
    if (_path.closed) {
        Uint32 ii = (Uint32)_path.size()-1;
        vec1  = _path.vertices[0 ];
        vec1 -= _path.vertices[ii];
        
        float len = vec1.length();
        vec2 = _right ? vec1.getRPerp() : vec1.getPerp();
        float angle = vec2.getAngle();
        segmentLengths.push_back(len);
        segmentAngles.push_back(angle);
        totalLength += len;
    }
    Uint32 totalRays = _path.closed ? _rayNum : _rayNum -1;
    float raySpacing = totalLength / totalRays;
    float rayOffset = 0; // Offset from current segment start
    
    Spinor prevAngle, currAngle;
    Spinor nextAngle, rayAngle;
    Uint32 rayPos = 0;
    for (int ii = 0; ii < segmentCount; ii++) {
        // get this and adjacent segment angles
        if (ii == 0) {
            prevAngle = _path.closed ? segmentAngles[segmentAngles.size()-1] : segmentAngles[0];
        } else {
            prevAngle = segmentAngles[ii - 1];
        }
        currAngle = segmentAngles[ii];
        if (ii == segmentAngles.size() - 1) {
            nextAngle = _path.closed ? segmentAngles[0] : segmentAngles[ii];
        } else {
            nextAngle = segmentAngles[ii+1];
        }

        // interpolate to find actual start and end angles
        prevAngle.slerp(currAngle, 0.5f);
        rayAngle = currAngle;
        rayAngle.slerp(nextAngle, 0.5f);
        nextAngle = rayAngle;

        segStart =  _path.vertices[ii];
        if (ii >= _path.size()-1) {
            segDir = _path.vertices[0]-segStart;
            segDir.normalize();
        } else {
            segDir = _path.vertices[ii+1]-segStart;
            segDir.normalize();
        }
        
        // TODO: Unsure why rayPos < _rayNum necessary
        while(rayOffset < segmentLengths[ii] && rayPos < _rayNum) {
            // interpolate ray angle based on position within segment
            rayAngle = prevAngle;
            rayAngle.slerp( nextAngle, rayOffset / segmentLengths[ii]);
            float angle = rayAngle.getAngle();

            vec2.set(_rayOffset, 0).rotate(angle);
            vec1 = segDir*rayOffset + segStart + vec2;
            vec2.set(_reach, 0).rotate(angle);
            vec2 += vec1;

            _lightRays[2*rayPos  ] = vec1;
            _lightRays[2*rayPos+1] = vec2;

            vec2 -= vec1;
            vec2.normalize();
            _directions[rayPos] = vec2;
            rayPos += 1;
            rayOffset += raySpacing;
        }

        rayOffset -= segmentLengths[ii];
    }
    
    // Puts last ray on path endpoint
    if (!_path.closed) {
        float position = segmentLengths.back();
        rayAngle = prevAngle;
        rayAngle.slerp( nextAngle, 1);
        float angle = rayAngle.getAngle();

        vec2.set(_rayOffset, 0).rotate(angle);
        vec1 = segDir*position + segStart + vec2;
        vec2.set(_reach, 0).rotate(angle);
        vec2 += vec1;

        _lightRays[2*rayPos  ] = vec1;
        _lightRays[2*rayPos+1] = vec2;

        vec2 -= vec1;
        vec2.normalize();
        _directions[rayPos] = vec2;
    }
}


/**
 * Updates the mesh(es) associated with this light.
 *
 * This will always update the light mesh. It will only update the debug
 * mesh if {@link #getDebug} is true.
 */
void ChainLight::updateMesh() {
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
    vertex.scale = (1 - _fractions[0]);
    _lightMesh.vertices.push_back(vertex);
    for(int ii = 1; ii < _rayNum; ii++) {
        vertex.position = _endPoints[2*ii  ];
        vertex.scale = 1.0f;
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(2*ii-2);
        _lightMesh.indices.push_back(2*ii-1);
        _lightMesh.indices.push_back(2*ii  );
        
        vertex.position = _endPoints[2*ii+1];
        vertex.scale = (1 - _fractions[ii]);
        _lightMesh.vertices.push_back(vertex);
        _lightMesh.indices.push_back(2*ii-1);
        _lightMesh.indices.push_back(2*ii  );
        _lightMesh.indices.push_back(2*ii+1);
    }
    if (_path.closed) {
        _lightMesh.indices.push_back(2*_rayNum-2);
        _lightMesh.indices.push_back(2*_rayNum-1);
        _lightMesh.indices.push_back(0);
        _lightMesh.indices.push_back(2*_rayNum-1);
        _lightMesh.indices.push_back(0);
        _lightMesh.indices.push_back(1);
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
        for(int ii = 0; ii < _rayNum; ii++) {
            Vec2 p = _endPoints[2*ii  ];
            sprite.position = transform.transform(p);
            sprite.texcoord.x  = (p.x-_bounds.origin.x)/_bounds.size.width;
            sprite.texcoord.y  = (p.y-_bounds.origin.y)/_bounds.size.height;
            sprite.gradcoord = sprite.texcoord;
            _debugMesh.vertices.push_back(sprite);

            p = _endPoints[2*ii+1];
            sprite.position = transform.transform(p);
            sprite.texcoord.x  = (p.x-_bounds.origin.x)/_bounds.size.width;
            sprite.texcoord.y  = (p.y-_bounds.origin.y)/_bounds.size.height;
            sprite.gradcoord = sprite.texcoord;
            _debugMesh.vertices.push_back(sprite);

            _debugMesh.indices.push_back(2*ii  );
            _debugMesh.indices.push_back(2*ii+1);
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
    
    if (_path.closed) {
        _lightMesh.indices.push_back(vcount+2*_rayNum-2);
        _lightMesh.indices.push_back(vcount+2*_rayNum-1);
        _lightMesh.indices.push_back(vcount            );
        _lightMesh.indices.push_back(vcount+2*_rayNum-1);
        _lightMesh.indices.push_back(vcount  );
        _lightMesh.indices.push_back(vcount+1);
    }
    computeBounds();
}

/**
 * Recomputes the bounding box for this light.
 *
 * Note that this is a conservative bounding box that does not take into
 * account any rays being blocked.
 */
void ChainLight::computeBounds() {
    float maxX = _lightRays[0].x;
    float minX = maxX;
    float maxY = _lightRays[0].y;
    float minY = maxY;

    for (int ii = 0; ii < _rayNum; ii++) {
        maxX = std::max( _endPoints[2*ii  ].x, maxX);
        maxX = std::max( _endPoints[2*ii+1].x, maxX);
        minX = std::min( _endPoints[2*ii  ].x, minX);
        minX = std::min( _endPoints[2*ii+1].x, minX);

        maxY = std::max( _endPoints[2*ii  ].y, maxY);
        maxY = std::max( _endPoints[2*ii+1].y, maxY);
        minY = std::min( _endPoints[2*ii  ].y, minY);
        minY = std::min( _endPoints[2*ii+1].y, minY);
    }

    _bounds.set(minX, minY, maxX-minX, maxY-minY);
}

/**
 * Returns true if given point is inside of this light area
 *
 * This method assumes that the path is open.
 *
 * @param p the point in world coordinates
 *
 * @return true if given point is inside of this light area
 */
bool ChainLight::containsOpen(const Vec2& p) {
    // even-odd rule
    bool oddNodes = false;
    Vec2 p2 = _endPoints[0];
    Vec2 p1;

    // If chain is open, there is a single boundary
    for (int ii = 0; ii < _rayNum; ++ii) {
        p1 = _endPoints[2*ii+1];
        if (((p1.y < p.y) && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y < p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }
    for (int ii = _rayNum-1; ii >= 0; --ii) {
        p1 = _endPoints[2*ii  ];
        if (((p1.y < p.y) && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y < p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }

    return oddNodes;
}

/**
 * Returns true if given point is inside of this light area
 *
 * This method assumes that the path is closed.
 *
 * @param p the point in world coordinates
 *
 * @return true if given point is inside of this light area
 */
bool ChainLight::containsClosed(const Vec2& p) {
    // even-odd rule
    bool oddNodes = false;
    Vec2 p2 = _endPoints[1];
    Vec2 p1;

    // Counter-clockwise outer boundary
    for (int ii = 1; ii <= _rayNum; ++ii) {
        if (ii < _rayNum) {
            p1 = _endPoints[2*ii+1];
        } else {
            p1 = _endPoints[1];
        }
        if (((p1.y < p.y) && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y < p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }

    // Clockwise interior boundary
    p2 = _endPoints[2*_rayNum-2];
    for (int ii = _rayNum-2; ii >= -1; ii--) {
        if (ii >= 0) {
            p1 = _endPoints[2*ii  ];
        } else {
            p1 = _endPoints[2*_rayNum-2];
        }
        if (((p1.y < p.y) && (p2.y >= p.y)) ||
            ((p1.y >= p.y) && (p2.y < p.y))) {
            if ((p.y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) < (p.x - p1.x)) {
                oddNodes = !oddNodes;
            }
        }
        p2 = p1;
    }

    return oddNodes;
}
