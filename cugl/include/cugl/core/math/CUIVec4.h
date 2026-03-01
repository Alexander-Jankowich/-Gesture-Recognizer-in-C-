//
//  CUIVec4.h
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
#ifndef __CU_IVEC4_H__
#define __CU_IVEC4_H__
#include <cugl/core/math/CUMathBase.h>
#include <cmath>
#include <string>
#include <functional>

namespace cugl {

// Forward reference for conversions
class Vec4;
class IVec2;
class IVec3;
    
/**
 * This class defines a 4-element integer vector.
 *
 * This class is in standard layout with fields of uniform type. This means
 * that it is safe to reinterpret_cast objects to int arrays.
 */
class IVec4 {

#pragma mark Values
public:
    /** The x-coordinate. */
    int x;
    /** The y-coordinate. */
    int y;
    /** The z-coordinate. */
    int z;
    /** The w-coordinate. */
    int w;
    
    /** The zero vector Vec4(0,0,0,0) */
    static const IVec4 ZERO;
    /** The ones vector Vec4(1,1,1,1) */
    static const IVec4 ONE;
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Constructs a new vector initialized to all zeros.
     */
    IVec4() : x(0), y(0), z(0), w(0) {}
    
    /**
     * Constructs a new vector initialized to the specified values.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     * @param z The z coordinate.
     * @param w The w coordinate.
     */
    IVec4(int x, int y, int z, int w) {
        this->x = x; this->y = y; this->z = z; this->w = w;
    }
    
    /**
     * Constructs a new vector from the values in the specified array.
     *
     * The elements of the arra are in the order x, y, z, and w.
     *
     * @param array An array containing the elements of the vector.
     */
    IVec4(const int* array) {
        std::memcpy(this, array, 4*sizeof(float));
    }
    
    /**
     * Constructs a vector that describes the direction between the specified points.
     *
     * @param p1 The first point.
     * @param p2 The second point.
     */
    IVec4(const IVec4& p1, const IVec4& p2) {
        x = p2.x-p1.x; y = p2.y-p1.y; z = p2.z-p1.z; w = p2.w-p1.w;
    }
    
    
#pragma mark -
#pragma mark Setters
    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * The elements of the array are in the order x, y, z, and w.
     *
     * @param array An array containing the elements of the vector.
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator=(const int* array) {
        return set(array);
    }
    
    /**
     * Sets the elements of this vector to the specified values.
     *
     * @param x The new x coordinate.
     * @param y The new y coordinate.
     * @param z The new z coordinate.
     * @param w The new w coordinate.
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& set(int x, int y, int z, int w) {
        this->x = x; this->y = y; this->z = z; this->w = w;
        return *this;
    }
    
    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * The elements of the arra are in the order x, y, z, and w.
     *
     * @param array An array containing the elements of the vector.
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& set(const int* array) {
        std::memcpy(this, array, 4*sizeof(float));
        return *this;
    }
    
    /**
     * Sets the elements of this vector to those in the specified vector.
     *
     * @param v The vector to copy.
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec4& set(const IVec4& v) {
        std::memcpy(this, &v, 4*sizeof(int));
        return *this;
    }
    
    /**
     * Sets this vector to the directional vector between the specified points.
     *
     * @param p1 The initial point of the vector.
     * @param p2 The terminal point of the vector.
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec4& set(const IVec4& p1, const IVec4& p2) {
        x = p2.x-p1.x; y = p2.y-p1.y; z = p2.z-p1.z; w = p2.w-p1.w;
        return *this;
    }
    
    /**
     * Sets the elements of this vector to zero.
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec4& setZero() {
        std::memset(this,0,4*sizeof(int));
        return *this;
    }
    
    
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
    static IVec4* clamp(const IVec4& v, const IVec4& min, const IVec4& max, IVec4* dst);
    
    /**
     * Adds the specified vectors and stores the result in dst.
     *
     * @param v1    The first vector.
     * @param v2    The second vector.
     * @param dst   A vector to store the result in
     *
     * @return A reference to dst for chaining
     */
    static IVec4* add(const IVec4& v1, const IVec4& v2, IVec4* dst);
    
    /**
     * Subtracts the specified vectors and stores the result in dst.
     *
     * The resulting vector is computed as (v1 - v2).
     *
     * @param v1    The first vector.
     * @param v2    The second vector.
     * @param dst   The destination vector.
     *
     * @return A reference to dst for chaining
     */
    static IVec4* subtract(const IVec4& v1, const IVec4& v2, IVec4* dst);

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
    static IVec4* scale(const IVec4& v, int s, IVec4* dst);
    
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
    static IVec4* scale(const IVec4& v1, const IVec4& v2, IVec4* dst);
    
    /**
     * Negates the specified vector and stores the result in dst.
     *
     * @param v     The vector to negate.
     * @param dst   The destination vector.
     *
     * @return A reference to dst for chaining
     */
    static IVec4* negate(const IVec4& v, IVec4* dst);
    
    
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
    IVec4& clamp(const IVec4& min, const IVec4& max);
    
    /**
     * Returns a copy of this vector clamped within the given range.
     *
     * Note: this does not modify this vector.
     *
     * @param min The minimum value.
     * @param max The maximum value.
     *
     * @return A copy of this vector clamped within the given range.
     */
    IVec4 getClamp(const IVec4& min, const IVec4& max) const {
        return IVec4(std::clamp(x,min.x,max.x),
                     std::clamp(y,min.y,max.y),
                     std::clamp(z,min.z,max.z),
                     std::clamp(w,min.w,max.w));
    }
    
    /**
     * Adds the given vector to this one in place.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& add(const IVec4& v) {
        x += v.x; y += v.y; z += v.z;  w += v.w;
        return *this;
    }
    
    /**
     * Adds the given values to this vector.
     *
     * @param x The x coordinate to add.
     * @param y The y coordinate to add.
     * @param z The z coordinate to add.
     * @param w The w coordinate to add.
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& add(int x, int y, int z, int w) {
        this->x += x; this->y += y; this->z += z; this->w += w;
        return *this;
    }
    
    /**
     * Subtracts the given vector from this one in place.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& subtract(const IVec4& v) {
        x -= v.x; y -= v.y; z -= v.z;  w -= v.w;
        return *this;
    }
    
    /**
     * Subtracts the given values from this vector.
     *
     * @param x The x coordinate to subtract.
     * @param y The y coordinate to subtract.
     * @param z The z coordinate to subtract.
     * @param w The w coordinate to subtract.
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& subtract(int x, int y, int z, int w) {
        this->x -= x; this->y -= y; this->z -= z; this->w -= w;
        return *this;
    }
    
    /**
     * Scales this vector in place by the given factor.
     *
     * @param s The scalar to multiply by
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& scale(int s) {
        x *= s; y *= s; z *= s; w *= s;
        return *this;
    }
    
    /**
     * Scales this vector nonuniformly by the given factors.
     *
     * @param sx The scalar to multiply the x-axis
     * @param sy The scalar to multiply the y-axis
     * @param sz The scalar to multiply the z-axis
     * @param sw The scalar to divide the w-axis
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& scale(int sx, int sy, int sz, int sw) {
        x *= sx; y *= sy; z *= sz; w *= sw;
        return *this;
    }
    
    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& scale(const IVec4& v) {
        x *= v.x; y *= v.y; z *= v.z; w *= v.w;
        return *this;
    }
    
    /**
     * Negates this vector in place.
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& negate() {
        x = -x; y = -y; z = -z; w = -w;
        return *this;
    }

    /**
     * Returns a negated copy of this vector.
     *
     * Note: This method does not modify the vector
     *
     * @return a negated copy of this vector.
     */
    IVec4 getNegation() const {
        IVec4 result(*this);
        return result.negate();
    }
    
    /**
     * Maps the given function to the vector coordinates in place.
     *
     * This method supports any function that has the signature float func(float);
     * This includes many of the functions in math.h.
     *
     * @param func The function to map on the coordinates.
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& map(std::function<int(int)> func) {
        x = func(x); y = func(y); z = func(z); w = func(w);
        return *this;
    }
    
    /**
     * Returns a copy of this vector with func applied to each component.
     *
     * This method supports any function that has the signature float func(float);
     * This includes many of the functions in math.h.
     *
     * @param func The function to map on the coordinates.
     *
     * @return A copy of this vector with func applied to each component.
     */
    IVec4 getMap(std::function<int(int)> func) const {
        return IVec4(func(x), func(y), func(z), func(w));
    }
    
    
#pragma mark -
#pragma mark Operators
    /**
     * Adds the given vector to this one in place.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator+=(const IVec4& v) {
        return add(v);
    }
    
    /**
     * Subtracts the given vector from this one in place.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator-=(const IVec4& v) {
        return subtract(v);
    }
    
    /**
     * Scales this vector in place by the given factor.
     *
     * @param s The value to scale by
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator*=(int s) {
        return scale(s);
    }
    
    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator*=(const IVec4& v) {
        return scale(v);
    }
    
    /**
     * Returns the sum of this vector with the given vector.
     *
     * Note: this does not modify this vector.
     *
     * @param v The vector to add.
     *
     * @return The sum of this vector with the given vector.
     */
    const IVec4 operator+(const IVec4& v) const {
        IVec4 result(*this);
        return result.add(v);
    }
    
    /**
     * Returns the difference of this vector with the given vector.
     *
     * Note: this does not modify this vector.
     *
     * @param v The vector to subtract.
     *
     * @return The difference of this vector with the given vector.
     */
    const IVec4 operator-(const IVec4& v) const  {
        IVec4 result(*this);
        return result.subtract(v);
    }
    
    /**
     * Returns the negation of this vector.
     *
     * Note: this does not modify this vector.
     *
     * @return The negation of this vector.
     */
    const IVec4 operator-() const {
        IVec4 result(*this);
        return result.negate();
    }
    
    /**
     * Returns the scalar product of this vector with the given value.
     *
     * Note: this does not modify this vector.
     *
     * @param s The value to scale by.
     *
     * @return The scalar product of this vector with the given value.
     */
    const IVec4 operator*(int s) const {
        IVec4 result(*this);
        return result.scale(s);
    }
    
    /**
     * Returns the scalar product of this vector with the given vector.
     *
     * This method is provided to support non-uniform scaling.
     * Note: this does not modify this vector.
     *
     * @param v The vector to scale by.
     *
     * @return The scalar product of this vector with the given vector.
     */
    const IVec4 operator*(const IVec4& v) const {
        IVec4 result(*this);
        return result.scale(v);
    }
    
#pragma mark -
#pragma mark Comparisons
    /**
     * Returns true if this vector is less than the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are less than those of v, use the method
     * under().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is less than the given vector.
     */
    bool operator<(const IVec4& v) const;
    
    /**
     * Returns true if this vector is less than or equal the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are less than those of v, use the method
     * under().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is less than or equal the given vector.
     */
    bool operator<=(const IVec4& v) const;
    
    /**
     * Determines if this vector is greater than the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are greater than those of v, use the method
     * over().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is greater than the given vector.
     */
    bool operator>(const IVec4& v) const;
    
    /**
     * Determines if this vector is greater than or equal the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are greater than those of v, use the method
     * over().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is greater than or equal the given vector.
     */
    bool operator>=(const IVec4& v) const;
    
    /**
     * Returns true if this vector is equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is equal to the given vector.
     */
    bool operator==(const IVec4& v) const;
    
    /**
     * Returns true if this vector is not equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is not equal to the given vector.
     */
    bool operator!=(const IVec4& v) const;
    
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
    bool under(const IVec4& v) const;
    
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
    bool over(const IVec4& v) const;
    
    /**
     * Returns true if this vector is equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is equal to the given vector.
     */
    bool equals(const IVec4& v) const;
    
    /**
     * Returns true this vector contains all zeros.
     *
     * @return true if this vector contains all zeros, false otherwise.
     */
    bool isZero() const;
    
#pragma mark -
#pragma mark Conversion Methods
public:
    /**
     * Returns a string representation of this vector for debugging purposes.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this vector for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Cast from IVec4 to a string. */
    operator std::string() const { return toString(); }
    
    /** Cast from IVec4 to a Vec4. */
    operator Vec4() const;
    
    /**
     * Creates a vector from the given floating point vector.
     *
     * The float values are truncated.
     *
     * @param v The vector to convert
     */
    explicit IVec4(const Vec4& v);
 
    /**
     * Sets this vector to be the given floating point vector.
     *
     * The float values are truncated.
     *
     * @param v The vector to convert
     *
     * @return A reference to this (modified) IVec4 for chaining.
     */
    IVec4& operator= (const Vec4& v);
    
    /**
     * Adds the given vector to this one in place.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec4& operator+=(const Vec4& v);

    /**
     * Subtracts the given vector from this one in place.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec4& operator-=(const Vec4& v);

    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec4& operator*=(const Vec4& v);
    
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
    const IVec4 operator+(const Vec4& v);

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
    const IVec4 operator-(const Vec4& v);

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
    const IVec4 operator*(const Vec4& v);
    
    /**
     * Casts from a IVec2 to IVec4.
     *
     * The z and w coordinates are dropped.
     */
    operator IVec2() const;
    
    /**
     * Creates a 4d vector from the given 2d one.
     *
     * The z and w coordinates are set to 0.
     *
     * @param v The vector to convert
     */
    explicit IVec4(const IVec2& v);
    
    /**
     * Creates a 4d vector from the given 2d one
     *
     * The z and w coordinates are set to the given values.
     *
     * @param v The vector to convert
     * @param z The z-coordinate
     * @param w The w-coordinate
     */
    IVec4(const IVec2& v, int z, int w);
    
    /**
     * Sets the coordinates of this vector to those of the given 2d vector.
     *
     * The z and w coordinates are set to 0.
     *
     * @param size The vector to convert
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& operator= (const IVec2& size);
    
    /**
     * Casts from Vec4 to Vec3.
     *
     * The w coordinate is dropped
     */
    operator IVec3() const;
    
    /**
     * Creates a homogenous vector from the given 3d one.
     *
     * The w-coordinate is set to 0
     *
     * @param v The vector to convert
     */
    explicit IVec4(const IVec3& v);

    /**
     * Creates a 4d vector from the given 3d one
     *
     * The w-coordinate is set to the given value.
     *
     * @param v The vector to convert
     * @param w The w-coordinate
     */
    IVec4(const IVec3& v, int w);

    /**
     * Sets the coordinates of this vector to those of the given 3d vector.
     *
     * The w-coordinate is set to 0
     *
     * @param v The vector to convert
     *
     * @return A reference to this (modified) Vec4 for chaining.
     */
    IVec4& operator= (const IVec3& v);
    
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
    IVec4& set(const IVec3& v, int w);
};
    
    
#pragma mark -
#pragma mark Friend Operations
/**
 * Returns the scalar product of the given vector with the given value.
 *
 * @param x The value to scale by.
 * @param v The vector to scale.
 *
 * @return The scaled vector.
 */
inline const IVec4 operator*(int x, const IVec4& v) {
    IVec4 result(v);
    return result.scale(x);
}

}

#endif /* __CU_IVEC4_H__ */
