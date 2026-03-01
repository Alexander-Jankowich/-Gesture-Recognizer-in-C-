//
//  CUSpriteVertex.h
//  Cornell University Game Library (CUGL)
//
//  This module provides the basic struct for the sprite batch pipeline.
//  These this is is meant to be passed by value, so we have no methods for
//  shared pointers.
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
//  Version: 12/1/25 (Vulkan Integration)
//
#ifndef __CU_SPRITE_VERTEX_H__
#define __CU_SPRITE_VERTEX_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUVec2.h>
#include <cugl/core/math/CUVec3.h>
#include <cugl/core/math/CUVec4.h>

namespace cugl {

class JsonValue;

    /**
     * The classes and functions needed to construct a graphics pipeline.
     *
     * Initially these were part of the core CUGL module. However, in the 
     * transition to Vulkan, we discovered that we need to factor this out.
     * This allows us to provide options for OpenGL, Vulkan, and even headless
     * clients.
     */
    namespace graphics {

/**
 * This class/struct is rendering information for a 2d sprite batch vertex.
 *
 * The class is intended to be used as a struct. This struct has the basic
 * rendering information required by a {@link SpriteBatch} for rendering.
 *
 * Note that not all attributes of a sprite vertex are rendered. In particular,
 * gradient coordinates are ignored if there is no gradient being applied, and
 * texture coordinates are ignored if there is no texture being applied. See
 * {@link SpriteBatch} for how to control these individual elements.
 */
class SpriteVertex {
public:
    /** The vertex position */
    cugl::Vec2    position;
    /** The vertex color */
    Color4        color;
    /** The vertex texture coordinate */
    cugl::Vec2    texcoord;
    /** The vertex gradient coordinate */
    cugl::Vec2    gradcoord;
    
    /**
     * Creates a new SpriteVertex
     *
     * The values of this vertex will all be zeroed. That means that the color
     * will be completely transparent.
     */
    SpriteVertex() {
        color = Color4::CLEAR;
    }
    
    /**
     * Creates a new SpriteVertex from the given JSON value.
     *
     * A sprite vertex can be described as an array of floats or a JSON object.
     * If it is a JSON object, then it supports the following attributes.
     *
     *     "position":  An array of float arrays of length two
     *     "color":     Either a four-element integer array (values 0..255) or a string.
     *                  Any string should be a web color or a Tkinter color name.
     *     "texcoord":  An array of float arrays of length two
     *     "gradcoord": An array of float arrays of length two
     *
     * Again, all attributes are optional. The default color is 'white' and all
     * other values resolve to the origin.
     *
     * If the sprite vertex is represented as an array, then it should be an
     * array of length no more than 10. These float are assigned to the
     * attributes position (2), color (4), texCoord (2) and gradCoord (2) in
     * order. Missing values are replaced with a 0 (or 255 in the case of the
     * color attributes).
     *
     * @param json  The JSON object specifying the vertex
     */
    SpriteVertex(const std::shared_ptr<JsonValue>& json);
    
    /**
     * Sets this SpriteVertex to have the data in the given JSON value.
     *
     * A sprite vertex can be described as an array of floats or a JSON object.
     * If it is a JSON object, then it supports the following attributes.
     *
     *     "position":  An array of float arrays of length two
     *     "color":     Either a four-element integer array (values 0..255) or a string.
     *                  Any string should be a web color or a Tkinter color name.
     *     "texcoord":  An array of float arrays of length two
     *     "gradcoord": An array of float arrays of length two
     *
     * Again, all attributes are optional. The default color is 'white' and all
     * other values resolve to the origin.
     *
     * If the sprite vertex is represented as an array, then it should be an
     * array of length no more than 10. These float are assigned to the
     * attributes position (2), color (4), texCoord (2) and gradCoord (2) in
     * order. Missing values are replaced with a 0 (or 255 in the case of the
     * color attributes).
     *
     * @param json  The JSON object specifying the vertex
     *
     * @return a reference to this sprite vertex for chaining
     */
    SpriteVertex& set(const std::shared_ptr<JsonValue>& json);
    
};

    }
}

#endif /* __CU_SPRITE_VERTEX_H__ */
