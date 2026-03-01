//
//  CUVec2.h
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
#ifndef __CU_IVEC2_H__
#define __CU_IVEC2_H__
#include <cugl/core/math/CUMathBase.h>
#include <cmath>
#include <string>
#include <functional>

namespace cugl {

// Forward references for conversions
class Vec2;
class IVec3;
class IVec4;
    
/**
 * This class defines a 2-element integer vector.
 *
 * This class is in standard layout with fields of uniform type. This means
 * that it is safe to reinterpret_cast objects to int arrays.
 */
class IVec2 {
    
#pragma mark Values
public:
    /** The x coordinate. */
    int x;
    /** The x coordinate. */
    int y;
    
    /** The zero vector IVec2(0,0) */
    static const IVec2 ZERO;
    /** The unit vector IVec2(1,1) */
    static const IVec2 ONE;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Constructs a new vector initialized to all zeros.
     */
    IVec2() : x(0), y(0) { }

    /**
     * Constructs a new vector initialized to the specified values.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     */
    IVec2(int x, int y) { this->x = x; this->y = y; }
    
    /**
     * Constructs a new vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y.
     */
    IVec2(const int* array) { x = array[0]; y = array[1]; }
    
    /**
     * Constructs a vector that describes the direction between the specified points.
     *
     * @param p1 The first point
     * @param p2 The second point
     */
    IVec2(const IVec2& p1, const IVec2& p2) {
        x = p2.x-p1.x; y = p2.y-p1.y;
    }
    

#pragma mark -
#pragma mark Setters
    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator=(const int* array) {
        return set(array);
    }
    
    /**
     * Sets the elements of this vector to the specified values.
     *
     * @param x The new x coordinate.
     * @param y The new y coordinate.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& set(int x, int y) {
        this->x = x; this->y = y;
        return *this;
    }
    
    /**
     * Sets the elements of this vector from the values in the specified array.
     *
     * @param array An array containing the elements of the vector in the order x, y.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& set(const int* array) {
        x = array[0]; y = array[1];
        return *this;
    }
    
    /**
     * Sets the elements of this vector to those in the specified vector.
     *
     * @param v The vector to copy.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& set(const IVec2& v) {
        x = v.x; y = v.y;
        return *this;
    }
    
    /**
     * Sets this vector to the directional vector between the specified points.
     *
     * @param p1 The initial point of the vector.
     * @param p2 The terminal point of the vector.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& set(const IVec2& p1, const IVec2& p2) {
        x = p2.x-p1.x; y = p2.y-p1.y;
        return *this;
    }
    
    /**
     * Sets the elements of this vector to zero.
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec2& setZero() {
        x = y = 0;
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
    static IVec2* clamp(const IVec2& v, const IVec2& min, const IVec2& max, IVec2* dst);
    
    /**
     * Adds the specified vectors and stores the result in dst.
     *
     * @param v1    The first vector.
     * @param v2    The second vector.
     * @param dst   A vector to store the result in
     *
     * @return A reference to dst for chaining
     */
    static IVec2* add(const IVec2& v1, const IVec2& v2, IVec2* dst);

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
    static IVec2* subtract(const IVec2& v1, const IVec2& v2, IVec2* dst);

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
    static IVec2* scale(const IVec2& v, int s, IVec2* dst);
    
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
    static IVec2* scale(const IVec2& v1, const IVec2& v2, IVec2* dst);
    
    /**
     * Negates the specified vector and stores the result in dst.
     *
     * @param v     The vector to negate.
     * @param dst   The destination vector.
     *
     * @return A reference to dst for chaining
     */
    static IVec2* negate(const IVec2& v, IVec2* dst);
    
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
    IVec2& clamp(const IVec2& min, const IVec2& max);
    
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
    IVec2 getClamp(const IVec2& min, const IVec2& max) const {
        return IVec2(std::clamp(x,min.x,max.x), 
                     std::clamp(y, min.y, max.y));
    }

    /**
     * Adds the given vector to this one in place.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& add(const IVec2& v) {
        x += v.x; y += v.y;
        return *this;
    }
    
    /**
     * Adds the given values to this vector.
     *
     * @param x The x coordinate to add.
     * @param y The y coordinate to add.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& add(int x, int y) {
        this->x += x; this->y += y;
        return *this;
    }
    
    /**
     * Subtracts the given vector from this one in place.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& subtract(const IVec2& v) {
        x -= v.x; y -= v.y;
        return *this;
    }
    
    /**
     * Subtracts the given values from this vector.
     *
     * @param x The x coordinate to subtract.
     * @param y The y coordinate to subtract.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& subtract(int x, int y) {
        this->x -= x; this->y -= y;
        return *this;
    }
    
    /**
     * Scales this vector in place by the given factor.
     *
     * @param s The scalar to multiply by
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& scale(int s) {
        x *= s; y *= s;
        return *this;
    }
    
    /**
     * Scales this vector nonuniformly by the given factors.
     *
     * @param sx The scalar to multiply the x-axis
     * @param sy The scalar to multiply the y-axis
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& scale(int sx, int sy) {
        x *= sx; y *= sy;
        return *this;
    }
    
    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& scale(const IVec2& v) {
        x *= v.x; y *= v.y;
        return *this;
    }
    
    /**
     * Negates this vector.
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& negate() {
        x = -x; y = -y;
        return *this;
    }
    
    /**
     * Returns a negated copy of this vector.
     *
     * Note: This method does not modify the vector
     *
     * @return a negated copy of this vector.
     */
    IVec2 getNegation() const {
        IVec2 result(*this);
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
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec2& map(std::function<int(int)> func) {
        x = func(x); y = func(y);
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
    IVec2 getMap(std::function<int(int)> func) const {
        return IVec2(func(x), func(y));
    }

    
#pragma mark -
#pragma mark Operators
    /**
     * Adds the given vector to this one in place.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator+=(const IVec2& v) {
        return add(v);
    }

    /**
     * Subtracts the given vector from this one in place.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator-=(const IVec2 v) {
        return subtract(v);
    }

    /**
     * Scales this vector in place by the given factor.
     *
     * @param s The value to scale by
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator*=(int s) {
        return scale(s);
    }

    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec2& operator*=(const IVec2& v) {
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
    const IVec2 operator+(const IVec2 v) const {
        IVec2 result(*this);
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
    const IVec2 operator-(const IVec2& v) const  {
        IVec2 result(*this);
        return result.subtract(v);
    }
    
    /**
     * Returns the negation of this vector.
     *
     * Note: this does not modify this vector.
     *
     * @return The negation of this vector.
     */
    const IVec2 operator-() const {
        IVec2 result(*this);
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
    const IVec2 operator*(int s) const {
        IVec2 result(*this);
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
    const IVec2 operator*(const IVec2& v) const {
        IVec2 result(*this);
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
    bool operator<(const IVec2& v) const {
        return (x == v.x ? y < v.y : x < v.x);
    }

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
    bool operator<=(const IVec2& v) const {
        return (x == v.x ? y <= v.y : x <= v.x);
    }

    /**
     * Returns true if this vector is greater than the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are greater than those of v, use the method
     * over().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is greater than the given vector.
     */
    bool operator>(const IVec2& v) const {
        return (x == v.x ? y > v.y : x > v.x);
    }

    /**
     * Returns true if this vector is greater than or equal the given vector.
     *
     * This comparison uses the lexicographical order. To test if all
     * components in this vector are greater than those of v, use the method
     * over().
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is greater than or equal the given vector.
     */
    bool operator>=(const IVec2& v) const {
        return (x == v.x ? y >= v.y : x >= v.x);
    }

    /**
     * Returns true if this vector is equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is equal to the given vector.
     */
    bool operator==(const IVec2& v) const {
        return x == v.x && y == v.y;
    }
    
    /**
     * Returns true if this vector is not equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is not equal to the given vector.
     */
    bool operator!=(const IVec2& v) const {
        return x != v.x || y != v.y;
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
    bool under(const IVec2& v) const {
        return x <= v.x && y <= v.y;
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
    bool over(const IVec2& v) const {
        return x >= v.x && y >= v.y;
    }
    
    /**
     * Returns true if this vector is equal to the given vector.
     *
     * @param v The vector to compare against.
     *
     * @return True if this vector is equal to the given vector.
     */
    bool equals(const IVec2& v) const {
        return *this == v;
    }

    /**
     * Returns true this vector contains all zeros.
     *
     * @return true if this vector contains all zeros, false otherwise.
     */
    bool isZero() const {
        return x == 0.0f && y == 0.0f;
    }
    
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
     * @return a string representation of this vector for debuggging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Cast from IVec2 to a string. */
    operator std::string() const { return toString(); }
    
    /** Cast from IVec2 to Vec2. */
    operator Vec2() const;

    /**
     * Creates a vector from the given floating point vector.
     *
     * The float values are truncated.
     *
     * @param v The vector to convert
     */
    explicit IVec2(const Vec2& v);
    
	/**
     * Sets this vector to be the given floating point vector.
     *
     * The float values are truncated.
     *
     * @param v The vector to convert
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator= (const Vec2& v);
 
    /**
     * Adds the given vector to this one in place.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to add
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator+=(const Vec2& v);

    /**
     * Subtracts the given vector from this one in place.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to subtract
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator-=(const Vec2& v);

    /**
     * Scales this vector nonuniformly by the given vector.
     *
     * The float values of v are truncated.
     *
     * @param v The vector to scale by
     *
     * @return A reference to this (modified) Vec2 for chaining.
     */
    IVec2& operator*=(const Vec2& v);
    
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
    const IVec2 operator+(const Vec2& v);

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
    const IVec2 operator-(const Vec2& v);

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
    const IVec2 operator*(const Vec2& v);
    
    /**
     * Casts from IVec2 to IVec2.
     *
     * The z-value is set to 0.
     */
    operator IVec3() const;
    
    /**
     * Creates a 2d vector from the given 3d one.
     *
     * The z-value is dropped.
     *
     * @param v The vector to convert
     */
    explicit IVec2(const IVec3& v);
    
    /**
     * Sets the coordinates of this vector to those of the given 3d vector.
     *
     * The z-value is dropped.
     *
     * @param size The vector to convert
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator= (const IVec3& size);

    /**
     * Casts from IVec2 to IVec4.
     *
     * The z and w coordinates are set to 0.
     */
    operator IVec4() const;
    
    /**
     * Creates a 2d vector from the given 4d one.
     *
     * The z and w coordinates are dropped.
     *
     * @param v The vector to convert
     */
    explicit IVec2(const IVec4& v);
    
    /**
     * Sets the coordinates of this vector to those of the given 4d vector.
     *
     * The z and w coordinates are dropped.
     *
     * @param v The vector to convert
     *
     * @return A reference to this (modified) IVec2 for chaining.
     */
    IVec2& operator= (const IVec4& v);
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
inline const IVec2 operator*(int x, const IVec2& v) {
    IVec2 result(v);
    return result.scale(x);
}

}

#endif /* __CU_IVEC2_H__ */
