//
//  CUIVec2.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a 2d vector of integers. It has less
//  functionality than Vec2, and is primarily used for data grouping.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty. In no event will the authors be held liable for any damages
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
//  Version: 5/15/25 (Vulkan Integration)
//
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/util/CUStringTools.h>
#include <cugl/core/math/CUIVec2.h>
#include <cugl/core/math/CUIVec3.h>
#include <cugl/core/math/CUIVec4.h>
#include <cugl/core/math/CUVec2.h>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace cugl;

#pragma mark -
#pragma mark Static Arithmetic

/**
 * Clamps the specified vector within the given range and returns it in dst.
 *
 * @param v     The vector to clamp.
 * @param min   The minimum value.
 * @param max   The maximum value.
 * @param dst   A vector to store the result in.
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::clamp(const IVec2& v, const IVec2& min, const IVec2& max, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = std::clamp(v.x,min.x,max.x);
    dst->y = std::clamp(v.y,min.y,max.y);
    return dst;
}

/**
 * Adds the specified vectors and stores the result in dst.
 *
 * @param v1    The first vector.
 * @param v2    The second vector.
 * @param dst   A vector to store the result in
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::add(const IVec2& v1, const IVec2& v2, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x+v2.x;
    dst->y = v1.y+v2.y;
    return dst;

}

/**
 * Subtracts the specified vectors and stores the result in dst.
 * The resulting vector is computed as (v1 - v2).
 *
 * @param v1    The first vector.
 * @param v2    The second vector.
 * @param dst   The destination vector.
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::subtract(const IVec2& v1, const IVec2& v2, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x-v2.x;
    dst->y = v1.y-v2.y;
    return dst;
}

/**
 * Scales the specified vector and stores the result in dst.
 *
 * The scale is uniform by the constant s.
 *
 * @param v     The vector to scale.
 * @param s     The uniform scaling factor.
 * @param dst   The destination vector.
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::scale(const IVec2& v, int s, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v.x*s;
    dst->y = v.y*s;
    return dst;
}

/**
 * Scales the specified vector and stores the result in dst.
 *
 * The scale is nonuniform by the attributes in v2.
 *
 * @param v1    The vector to scale.
 * @param v2    The nonuniform scaling factor.
 * @param dst   The destination vector.
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::scale(const IVec2& v1, const IVec2& v2, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x*v2.x;
    dst->y = v1.y*v2.y;
    return dst;
}

/**
 * Negates the specified vector and stores the result in dst.
 *
 * @param v     The vector to negate.
 * @param dst   The destination vector.
 *
 * @return A reference to dst for chaining
 */
IVec2* IVec2::negate(const IVec2& v, IVec2* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = -v.x; dst->y = -v.y;
    return dst;
}

#pragma mark -
#pragma mark Arithmetic
/**
 * Clamps this vector within the given range.
 *
 * @param min The minimum value.
 * @param max The maximum value.
 *
 * @return A reference to this (modified) Vec2 for chaining.
 */
IVec2& IVec2::clamp(const IVec2& min, const IVec2& max) {
    x = std::clamp(x, min.x, max.x);
    y = std::clamp(y, min.y, max.y);
    return *this;
}


#pragma mark -
#pragma mark Conversion Methods
/**
 * Returns a string representation of this vector for debuggging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this vector for debuggging purposes.
 */
std::string IVec2::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::IVec2(" : "(");
    ss << x;
    ss << ",";
    ss << y;
    ss << ")";
    return ss.str();
}

/** Cast from IVec2 to Vec2. */
IVec2::operator Vec2() const {
    return Vec2(x,y);
}

/**
 * Creates a vector from the given floating point vector.
 *
 * The float values are truncated.
 *
 * @param v The vector to convert
 */
IVec2::IVec2(const Vec2& v) {
    x = v.x; y = v.y;
}

/**
 * Sets this vector to be the given floating point vector.
 *
 * The float values are truncated.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) IVec2 for chaining.
 */
IVec2& IVec2::operator= (const Vec2& v) {
    x = v.x; y = v.y;
    return *this;
}
 
/**
 * Adds the given vector to this one in place.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to add
 *
 * @return A reference to this (modified) IVec2 for chaining.
 */
IVec2& IVec2::operator+=(const Vec2& v) {
    x += (int)v.x; y += (int)v.y;
    return *this;
}

/**
 * Subtracts the given vector from this one in place.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to subtract
 *
 * @return A reference to this (modified) IVec2 for chaining.
 */
IVec2& IVec2::operator-=(const Vec2& v) {
    x -= (int)v.x; y -= (int)v.y;
    return *this;
}


/**
 * Scales this vector nonuniformly by the given vector.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to scale by
 *
 * @return A reference to this (modified) Vec2 for chaining.
 */
IVec2& IVec2::operator*=(const Vec2& v) {
    x *= (int)v.x; y *= (int)v.y;
    return *this;
}

/**
 * Returns the sum of this vector with the given vector.
 *
 * The float values of v are truncated.
 *
 * Note: this does not modify this vector.
 *
 * @param v The vector to add.
 *
 * @return The sum of this vector with the given vector.
 */
const IVec2 IVec2::operator+(const Vec2& v) {
    return IVec2(x+(int)v.x, y+(int)v.y);
}

/**
 * Returns the difference of this vector with the given vector.
 *
 * The float values of v are truncated.
 *
 * Note: this does not modify this vector.
 *
 * @param v The vector to subtract.
 *
 * @return The difference of this vector with the given vector.
 */
const IVec2 IVec2::operator-(const Vec2& v) {
    return IVec2(x-(int)v.x, y-(int)v.y);
}

/**
 * Returns the scalar product of this vector with the given vector.
 *
 * The float values of v are truncated.
 *
 * Note: this does not modify this vector.
 *
 * @param v The vector to scale by.
 *
 * @return The scalar product of this vector with the given vector.
 */
const IVec2 IVec2::operator*(const Vec2& v) {
    return IVec2(x*(int)v.x, y*(int)v.y);
}

/**
 * Casts from IVec2 to IVec3.
 *
 * The z-value is set to 0.
 */
IVec2::operator IVec3() const {
    return IVec3(x,y,0);
}

/**
 * Creates a 2d vector from the given 3d one.
 *
 * The z-value is dropped.
 *
 * @param v The vector to convert
 */
IVec2::IVec2(const IVec3& v) {
    x = v.x; y = v.y;
}

/**
 * Sets the coordinates of this vector to those of the given 3d vector.
 *
 * The z-value is dropped.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) IVec2 for chaining.
 */
IVec2& IVec2::operator= (const IVec3& v) {
    x = v.x; y = v.y;
    return *this;
}

/**
 * Casts from IVec2 to IVec4.
 *
 * The z and w coordinates are set to 0.
 */
IVec2::operator IVec4() const {
    return IVec4(x,y,0,0);
}

/**
 * Creates a 2d vector from the given 4d one.
 *
 * The z and w coordinates are dropped.
 *
 * @param v The vector to convert
 */
IVec2::IVec2(const IVec4& v) {
    x = v.x; y = v.y;
}

/**
 * Sets the coordinates of this vector to those of the given 4d vector.
 *
 * The z and w coordinates are dropped.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) IVec2 for chaining.
 */
IVec2& IVec2::operator= (const IVec4& v) {
    x = v.x; y = v.y;
    return *this;
}


#pragma mark -
#pragma mark Constants

/** The zero vector Vec2(0,0) */
const IVec2 IVec2::ZERO(0.0f, 0.0f);
/** The unit vector Vec2(1,1) */
const IVec2 IVec2::ONE(1.0f, 1.0f);
