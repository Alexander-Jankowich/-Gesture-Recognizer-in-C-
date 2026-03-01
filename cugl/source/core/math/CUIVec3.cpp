//
//  CUIVec3.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a 3d vector of integers. It has less
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
#include <cugl/core/math/CUVec3.h>
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
IVec3* IVec3::clamp(const IVec3& v, const IVec3& min, const IVec3& max, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = std::clamp(v.x,min.x,max.x);
    dst->y = std::clamp(v.y,min.y,max.y);
    dst->z = std::clamp(v.z,min.z,max.z);
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
IVec3* IVec3::add(const IVec3& v1, const IVec3& v2, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x+v2.x;
    dst->y = v1.y+v2.y;
    dst->z = v1.z+v2.z;
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
IVec3* IVec3::subtract(const IVec3& v1, const IVec3& v2, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x-v2.x;
    dst->y = v1.y-v2.y;
    dst->z = v1.z-v2.z;
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
IVec3* IVec3::scale(const IVec3& v, int s, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v.x*s;
    dst->y = v.y*s;
    dst->z = v.z*s;
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
IVec3* IVec3::scale(const IVec3& v1, const IVec3& v2, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x*v2.x;
    dst->y = v1.y*v2.y;
    dst->z = v1.z*v2.z;
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
IVec3* IVec3::negate(const IVec3& v, IVec3* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = -v.x; dst->y = -v.y; dst->z = -v.z;
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
 * @return A reference to this (modified) Vec3 for chaining.
 */
IVec3& IVec3::clamp(const IVec3& min, const IVec3& max) {
    x = std::clamp(x, min.x, max.x);
    y = std::clamp(y, min.y, max.y);
    z = std::clamp(z, min.z, max.z);
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
std::string IVec3::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::IVec3(" : "(");
    ss << x;
    ss << ",";
    ss << y;
    ss << ",";
    ss << z;
    ss << ")";
    return ss.str();
}

/** Cast from IVec3 to Vec3. */
IVec3::operator Vec3() const {
    return Vec3(x,y,z);
}

/**
 * Creates a vector from the given floating point vector.
 *
 * The float values are truncated.
 *
 * @param v The vector to convert
 */
IVec3::IVec3(const Vec3& v) {
    x = v.x; y = v.y; z = v.z;
}

/**
 * Sets this vector to be the given floating point vector.
 *
 * The float values are truncated.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) IVec3 for chaining.
 */
IVec3& IVec3::operator= (const Vec3& v) {
    x = v.x; y = v.y; z = v.z;
    return *this;
}
 
/**
 * Adds the given vector to this one in place.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to add
 *
 * @return A reference to this (modified) IVec3 for chaining.
 */
IVec3& IVec3::operator+=(const Vec3& v) {
    x += (int)v.x; y += (int)v.y; z += (int)v.z;
    return *this;
}

/**
 * Subtracts the given vector from this one in place.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to subtract
 *
 * @return A reference to this (modified) IVec3 for chaining.
 */
IVec3& IVec3::operator-=(const Vec3& v) {
    x -= (int)v.x; y -= (int)v.y; z -= (int)v.z;
    return *this;
}


/**
 * Scales this vector nonuniformly by the given vector.
 *
 * The float values of v are truncated.
 *
 * @param v The vector to scale by
 *
 * @return A reference to this (modified) IVec3 for chaining.
 */
IVec3& IVec3::operator*=(const Vec3& v) {
    x *= (int)v.x; y *= (int)v.y; z *= (int)v.z;
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
const IVec3 IVec3::operator+(const Vec3& v) {
    return IVec3(x+(int)v.x, y+(int)v.y, z+(int)v.z);
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
const IVec3 IVec3::operator-(const Vec3& v) {
    return IVec3(x-(int)v.x, y-(int)v.y, z-(int)v.z);
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
const IVec3 IVec3::operator*(const Vec3& v) {
    return IVec3(x*(int)v.x, y*(int)v.y, z*(int)v.z);
}

/**
 * Casts from IVec2 to IVec3.
 *
 * The z-value is dropped.
 */
IVec3::operator IVec2() const {
    return IVec2(x,y);
}

/**
 * Creates a 3d vector from the given 2d one.
 *
 * The z-value is set to 0.
 *
 * @param v The vector to convert
 */
IVec3::IVec3(const IVec2& v) {
    x = v.x; y = v.y; z = 0;
}

/**
 * Creates a 3d vector from the given 2d one.
 *
 * The z-coordinate is given the appropriate value
 *
 * @param v The vector to convert
 * @param z The z-coordinate
 */
IVec3::IVec3(const IVec2& v, int z) {
    x = v.x; y = v.y; this->z = z;
}


/**
 * Sets the coordinates of this vector to those of the given 3d vector.
 *
 * The z-value is set to 0.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) Vec3 for chaining.
 */
IVec3& IVec3::operator= (const IVec2& v) {
    x = v.x; y = v.y; z = 0;
    return *this;
}

/**
 * Casts from Vec3 to Vec4.
 *
 * The w-coordinate is set to 0.
 */
IVec3::operator IVec4() const {
    return IVec4(x,y,z,0);
}

/**
 * Creates a 2d vector from the given 4d one.
 *
 * The w-coordinate is dropped.
 *
 * @param v The vector to convert
 */
IVec3::IVec3(const IVec4& v) {
    x = v.x; y = v.y; z = v.z;
}

/**
 * Sets the coordinates of this vector to those of the given 4d vector.
 *
 * The w-coordinate is dropped.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) Vec3 for chaining.
 */
IVec3& IVec3::operator= (const IVec4& v) {
    x = v.x; y = v.y; z = v.z;
    return *this;
}

#pragma mark -
#pragma mark Constants

/** The zero vector Vec3(0,0,0) */
const IVec3 IVec3::ZERO(0,0,0);
/** The unit vector Vec3(1,1,1) */
const IVec3 IVec3::ONE(1,1,1);
