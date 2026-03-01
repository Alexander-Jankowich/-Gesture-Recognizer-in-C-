//
//  CUIVec4.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a 4d vector of integers. It has less
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
#include <cugl/core/math/CUVec4.h>
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
IVec4* IVec4::clamp(const IVec4& v, const IVec4& min, const IVec4& max, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = std::clamp(v.x,min.x,max.x);
    dst->y = std::clamp(v.y,min.y,max.y);
    dst->z = std::clamp(v.z,min.z,max.z);
    dst->w = std::clamp(v.w,min.w,max.w);
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
IVec4* IVec4::add(const IVec4& v1, const IVec4& v2, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x+v2.x;
    dst->y = v1.y+v2.y;
    dst->z = v1.z+v2.z;
    dst->w = v1.w+v2.w;
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
IVec4* IVec4::subtract(const IVec4& v1, const IVec4& v2, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x-v2.x;
    dst->y = v1.y-v2.y;
    dst->z = v1.z-v2.z;
    dst->w = v1.w-v2.w;
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
IVec4* IVec4::scale(const IVec4& v, int s, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v.x*s; dst->y = v.y*s; dst->z = v.z*s; dst->w = v.w*s;
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
IVec4* IVec4::scale(const IVec4& v1, const IVec4& v2, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = v1.x*v2.x; dst->y = v1.y*v2.y; dst->z = v1.z*v2.z; dst->w = v1.w*v2.w;
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
IVec4* IVec4::negate(const IVec4& v, IVec4* dst) {
    CUAssertLog(dst, "Assignment vector is null");
    dst->x = -v.x; dst->y = -v.y; dst->z = -v.z; dst->w = -v.w;
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
 * @return A reference to this (modified) Vec4 for chaining.
 */
IVec4& IVec4::clamp(const IVec4& min, const IVec4& max) {
    x = std::clamp(x, min.x, max.x);
    y = std::clamp(y, min.y, max.y);
    z = std::clamp(z, min.z, max.z);
    w = std::clamp(w, min.w, max.w);
    return *this;
}

#pragma mark -
#pragma mark Comparisons
/**
 * Returns true if this vector is less than the given vector.
 *
 * This comparison uses the lexicographical order.  To test if all
 * components in this vector are less than those of v, use the method
 * under().
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is less than the given vector.
 */
bool IVec4::operator<(const IVec4& v) const {
    return (x == v.x ? (y == v.y ? (z == v.z ? w < v.w : z < v.z) : y < v.y) : x < v.x);
}

/**
 * Returns true if this vector is less than or equal the given vector.
 *
 * This comparison uses the lexicographical order.  To test if all
 * components in this vector are less than those of v, use the method
 * under().
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is less than or equal the given vector.
 */
bool IVec4::operator<=(const IVec4& v) const {
    return (x == v.x ? (y == v.y ? (z == v.z ? w <= v.w : z <= v.z) : y <= v.y) : x <= v.x);
}

/**
 * Determines if this vector is greater than the given vector.
 *
 * This comparison uses the lexicographical order.  To test if all
 * components in this vector are greater than those of v, use the method
 * over().
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is greater than the given vector.
 */
bool IVec4::operator>(const IVec4& v) const {
    return (x == v.x ? (y == v.y ? (z == v.z ? w > v.w : z > v.z) : y > v.y) : x > v.x);
}

/**
 * Determines if this vector is greater than or equal the given vector.
 *
 * This comparison uses the lexicographical order.  To test if all
 * components in this vector are greater than those of v, use the method
 * over().
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is greater than or equal the given vector.
 */
bool IVec4::operator>=(const IVec4& v) const {
    return (x == v.x ? (y == v.y ? (z == v.z ? w >= v.w : z >= v.z) : y >= v.y) : x >= v.x);
}

/**
 * Returns true if this vector is equal to the given vector.
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is equal to the given vector.
 */
bool IVec4::operator==(const IVec4& v) const {
    return x == v.x && y == v.y && z == v.z && w == v.w;
}

/**
 * Returns true if this vector is not equal to the given vector.
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is not equal to the given vector.
 */
bool IVec4::operator!=(const IVec4& v) const {
    return x != v.x || y != v.y || z != v.z || w != v.w;
}

/**
 * Returns true if this vector is dominated by the given vector.
 *
 * Domination means that all components of the given vector are greater than
 * or equal to the components of this one.
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is dominated by the given vector.
 */
bool IVec4::under(const IVec4& v) const {
    return x <= v.x && y <= v.y && z <= v.z && w <= v.w;
}
    
/**
 * Returns true if this vector dominates the given vector.
 *
 * Domination means that all components of this vector are greater than
 * or equal to the components of the given vector.
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is dominated by the given vector.
 */
bool IVec4::over(const IVec4& v) const {
    return x >= v.x && y >= v.y && z >= v.z && w >= v.w;
}

/**
 * Returns true if this vector is not equal to the given vector.
 *
 * @param v The vector to compare against.
 *
 * @return True if this vector is not equal to the given vector.
 */
bool IVec4::equals(const IVec4& v) const {
    return *this == v;
}
    
    
/**
 * Returns true this vector contains all zeros.
 *
 * @return true if this vector contains all zeros, false otherwise.
 */
bool IVec4::isZero() const {
    return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f;
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
std::string IVec4::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::IVec4(" : "(");
    ss << x;
    ss << ",";
    ss << y;
    ss << ",";
    ss << z;
    ss << ",";
    ss << w;
    ss << ")";
    return ss.str();
}

/** Cast from IVec4 to Vec4. */
IVec4::operator Vec4() const {
    return Vec4(x,y,z,w);
}

/**
 * Creates a vector from the given floating point vector.
 *
 * The float values are truncated.
 *
 * @param v The vector to convert
 */
IVec4::IVec4(const Vec4& v) {
    x = v.x; y = v.y; z = v.z; w = v.w;
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
IVec4& IVec4::operator= (const Vec4& v) {
    x = v.x; y = v.y; z = v.z; w = v.w;
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
IVec4& IVec4::operator+=(const Vec4& v) {
    x += (int)v.x; y += (int)v.y; z += (int)v.z; w += (int)v.w;
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
IVec4& IVec4::operator-=(const Vec4& v) {
    x -= (int)v.x; y -= (int)v.y; z -= (int)v.z; w -= (int)v.w;
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
IVec4& IVec4::operator*=(const Vec4& v) {
    x *= (int)v.x; y *= (int)v.y; z *= (int)v.z; w *= (int)v.w;
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
const IVec4 IVec4::operator+(const Vec4& v) {
    return IVec4(x+(int)v.x, y+(int)v.y, z+(int)v.z, w+(int)v.w);
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
const IVec4 IVec4::operator-(const Vec4& v) {
    return IVec4(x-(int)v.x, y-(int)v.y, z-(int)v.z, w-(int)v.w);
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
const IVec4 IVec4::operator*(const Vec4& v) {
    return IVec4(x*(int)v.x, y*(int)v.y, z*(int)v.z, w*(int)v.w);
}

/**
 * Casts from a IVec2 to IVec4.
 *
 * The z and w coordinates are dropped.
 */
IVec4::operator IVec2() const {
    return IVec2(x,y);
}

/**
 * Creates a 4d vector from the given 2d one.
 *
 * The z and w coordinates are set to 0.
 *
 * @param v The vector to convert
 */
IVec4::IVec4(const IVec2& v) {
    x = v.x; y = v.y; z = 0; w = 0;
}

/**
 * Creates a 4d vector from the given 2d one
 *
 * The z and w coordinates are set to the given values.
 *
 * @param v The vector to convert
 * @param z The z-coordinate
 * @param w The w-coordinate
 */
IVec4::IVec4(const IVec2& v, int z, int w) {
    x = v.x; y = v.y; this->z = z; this->w = w;

}

/**
 * Sets the coordinates of this vector to those of the given 2d vector.
 *
 * The z and w coordinates are set to 0.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) Vec4 for chaining.
 */
IVec4& IVec4::operator= (const IVec2& v) {
    x = v.x; y = v.y; z = 0; w = 0;
    return *this;
}

/**
 * Casts from Vec4 to Vec3.
 *
 * The w coordinate is dropped
 */
IVec4::operator IVec3() const {
    return IVec3(x,y,z);
}

/**
 * Creates a homogenous vector from the given 3d one.
 *
 * The w-coordinate is set to 0
 *
 * @param v The vector to convert
 */
IVec4::IVec4(const IVec3& v) {
    x = v.x; y = v.y; z = v.z; w = 0;
}

/**
 * Creates a 4d vector from the given 3d one
 *
 * The w-coordinate is set to the given value.
 *
 * @param v The vector to convert
 * @param w The w-coordinate
 */
IVec4::IVec4(const IVec3& v, int w) {
    x = v.x; y = v.y; z = v.z; this->w = 0;
}

/**
 * Sets the coordinates of this vector to those of the given 3d vector.
 *
 * The w-coordinate is set to 0
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) Vec4 for chaining.
 */
IVec4& IVec4::operator= (const IVec3& v) {
    x = v.x; y = v.y; z = v.z; w = 0;
    return *this;
}

/**
 * Sets the coordinates of this vector to those of the given 3d vector.
 *
 * The w-coordinate is set to the given value.
 *
 * @param v The vector to convert
 * @param w The w-coordinate
 *
 * @return A reference to this (modified) Vec4 for chaining.
 */
IVec4& IVec4::set(const IVec3& v, int w) {
    x = v.x; y = v.y; z = v.z; this->w = w;
    return *this;
}

#pragma mark -
#pragma mark Constants

/** The zero vector Vec4(0,0,0,0) */
const IVec4 IVec4::ZERO(0,0,0,0);
/** The ones vector Vec4(1,1,1,1) */
const IVec4 IVec4::ONE(1,1,1,1);

