//
//  CUSpinor.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a spinor. This is a complex number that
//  has special rotational properties. For information on how spinors work, see
//
//      https://en.wikipedia.org/wiki/Spinor
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
//  Version: 11/23/25 (SDL3 Integration)
//
#ifndef __CU_SPINOR_H__
#define __CU_SPINOR_H__
#include <cugl/core/math/CUVec2.h>

namespace cugl {

/**
 * This class represents a spinor, which is often useful for lighting.
 *
 * Spinors represent an angle about the origin, but behave like a mobius strip
 * when rotated. For information on how spinors work, see
 *
 *      https://en.wikipedia.org/wiki/Spinor
 *
 * For the most part, this class works like a complex number with additional
 * methods representing the spinor characteristics.
 */
class Spinor {
#pragma mark Values
public:
    /** The real component */
    float real;
    /** The imaginary component */
    float complex;

#pragma mark -
#pragma mark Constructors
    /** Creates the 0 spinor */
    Spinor(): real(0), complex(0) {
    }

    /**
     * Creates a spinor for the given angle (in radians)
     *
     * @param angle    The spinor angle
     */
    Spinor(float angle) {
        set(angle);
    }

    /**
     * Creates a spinor with the given real and imaginary parts
     *
     * @param real      The real component
     * @param complex   The imaginary component
     */
    Spinor(float real, float complex) {
        set(real, complex);
    }

#pragma mark -
#pragma mark Setters
    /**
     * Sets the spinor to have the given angle (in radians)
     *
     * @param angle    The spinor angle
     *
     * @return a reference to this spinor for chaining
     */
    Spinor& operator=(float angle) {
        return set(angle);
    }
    
    /**
     * Sets the spinor to have the given angle (in radians)
     *
     * @param angle    The spinor angle
     *
     * @return A reference to this (modified) Spinor for chaining
     */
    Spinor& set(float angle);

    /**
     * Sets the elements of this vector to the specified values.
     *
     * @param real      The real component
     * @param complex   The imaginary component
     *
     * @return A reference to this (modified) Spinor for chaining
     */
    Spinor& set(float real, float complex) {
        this->real = real; this->complex = complex;
        return *this;
    }
    
    /**
     * Sets the spinor to be a copy of the given one
     *
     * @param s The spinor to copy.
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& set(const Spinor s) {
        real = s.real; complex = s.complex;
        return *this;
    }

#pragma mark -
#pragma mark Arithmetic
    /**
     * Scales this spinor by the given factor
     *
     * @param t    The scaling factor
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& scale(float t) {
        real *= t; complex *= t;
        return *this;
    }
    
    /**
     * Adds the given spinor to this one.
     *
     * @param other    The spinor to add
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& add(const Spinor other) {
        real += other.real;
        complex += other.complex;
        return *this;
    }
    
    /**
     * Adds the given angle to this spinor.
     *
     * This is the same as adding a spinor of the given angle.
     *
     * @param angle    The angle to add
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& add(float angle);

    /**
     * Subtracts the given spinor from this one.
     *
     * @param other    The spinor to subtract
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& subtract(const Spinor other) {
        real -= other.real;
        complex -= other.complex;
        return *this;
    }
  
    /**
     * Subtracts the given angle from this spinor.
     *
     * This is the same as subtracting a spinor of the given angle.
     *
     * @param angle    The angle to subtract
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& subtract(float angle);

    /**
     * Multiplies this spinor by the given one.
     *
     * @param other    The spinor to multiply
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& multiply(const Spinor other);

#pragma mark -
#pragma mark Operators
    /**
     * Scales this spinor by the given factor
     *
     * @param t    The scaling factor
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator*=(float t) {
        real *= t; complex *= t;
        return *this;
    }
    
    /**
     * Multiplies this spinor by the given one.
     *
     * @param other    The spinor to multiply
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator*=(const Spinor other) {
        return multiply(other);
    }

    /**
     * Adds the given spinor to this one.
     *
     * @param other    The spinor to add
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator+=(const Spinor other) {
        real += other.real;
        complex += other.complex;
        return *this;
    }
    
    /**
     * Adds the given angle to this spinor.
     *
     * This is the same as adding a spinor of the given angle.
     *
     * @param angle    The angle to add
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator+=(float angle) {
        return add(angle);
    }
    
    /**
     * Subtracts the given spinor from this one.
     *
     * @param other    The spinor to subtract
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator-=(const Spinor other) {
        real -= other.real;
        complex -= other.complex;
        return *this;
    }
    
    /**
     * Subtracts the given angle from this spinor.
     * <p>
     * This is the same as subtracting a spinor of the given angle.
     *
     * @param angle    The angle to subtract
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator-=(float angle) {
        return subtract(angle);
    }
    
    /**
     * Returns the scale this spinor by the given factor
     *
     * Note: this does not modify this spinor.
     *
     * @param t    The scaling factor
     *
     * @return The scale this spinor by the given factor
     */
    Spinor operator*(float t) {
        Spinor result(*this);
        return result.scale(t);
    }
    
    /**
     * Returns the product this spinor by the given one.
     *
     * Note: this does not modify this spinor.
     *
     * @param other    The spinor to multiply
     *
     * @return The product this spinor by the given one.
     */
    Spinor operator*(const Spinor other) {
        Spinor result(*this);
        return result.multiply(other);
    }

    /**
     * Returns the sum of this spinor with the given spinor.
     *
     * Note: this does not modify this spinor.
     *
     * @param other The spinor to add.
     *
     * @return The sum of this spinor with the given spinor.
     */
    const Spinor operator+(const Spinor other) const {
        Spinor result(*this);
        return result.add(other);
    }
    
    /**
     * Returns the sum of this spinor with the given angle.
     *
     * This is the same as adding a spinor of the given angle.
     * Note: this does not modify this spinor.
     *
     * @param angle    The angle to add
     *
     * @return The sum of this spinor with the given angle.
     */
    Spinor operator+(float angle) {
        Spinor result(*this);
        return result.add(angle);
    }
    
    /**
     * Returns the result of subtracting the given spinor from this one.
     *
     * Note: this does not modify this spinor.
     *
     * @param other    The spinor to subtract
     *
     * @return The result of subtracting the given spinor from this one.
     */
    Spinor& operator-(const Spinor other) {
        Spinor result(*this);
        return result.subtract(other);
    }
    
    /**
     * Returns the result of subtracting the given angle from this spinor.
     *
     * This is the same as subtracting a spinor of the given angle.
     *
     * @param angle    The angle to subtract
     *
     * @return The result of subtracting the given angle from this spinor.
     */
    Spinor operator-(float angle) {
        Spinor result(*this);
        return result.subtract(angle);
    }
    
#pragma mark -
#pragma mark Linear Attributes
    /**
     * Returns the angle represented by this spinor
     *
     * @return the angle represented by this spinor
     */
    float getAngle() const;

    /**
     * Returns true this spinor contains all zeros.
     *
     * @return true if this spinor contains all zeros, false otherwise.
     */
    bool isZero() const {
        return real == 0.0f && complex == 0.0f;
    }

    /**
     * Returns true if this spinor is with tolerance of the origin.
     *
     * @param epsilon   The comparison tolerance
     *
     * @return true if this spinor is with tolerance of the origin.
     */
    bool isNearZero(float epsilon=CU_MATH_EPSILON) const {
        return lengthSquared() < epsilon*epsilon;
    }

    /**
     * Returns the length of this spinor.
     *
     * @return The length of the spinor.
     *
     * {@see lengthSquared}
     */
    float length() const {
        return sqrt(lengthSquared());
    }
    
    /**
     * Returns the squared spinor of this vector.
     *
     * This method is faster than spinor because it does not need to compute
     * a square root. Hence it is best to us this method when it is not
     * necessary to get the exact length of a spinor (e.g. when simply
     * comparing the length to a threshold value).
     *
     * @return The squared length of the spinor.
     *
     * {@see length}
     */
    float lengthSquared() const {
        return real*real+complex*complex;
    }

#pragma mark -
#pragma mark Linear Algebra
    /**
     * Normalizes this spinor.
     *
     * This method normalizes the spinor so that it is of unit length (i.e.
     * the length of the spinor after calling this method will be 1.0f). If the
     * spinor already has unit length or if the length of the spinor is zero,
     * this method does nothing.
     *
     * @return This spinor, after the normalization occurs.
     */
    Spinor& normalize();
    
    /**
     * Returns a normalized copy of this spinor.
     *
     * This method creates a copy of this spinor that is of unit length (i.e.
     * the length of the spinor after calling this method will be 1.0f). If the
     * spinor already has unit length or if the length of the spinor is zero,
     * this method does nothing.
     *
     * Note: this does not modify this spinor.
     *
     * @return A normalized copy of this spinor.
     */
    Spinor getNormalization() const {
        Spinor result(*this);
        return result.normalize();
    }
    
    /**
     * Sets this spinor to its inverse.
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& invert();

    /**
     * Returns the inverse of this spinor.
     *
     * @return the inverse of this spinor.
     */
    Spinor getInverse() {
        Spinor result(real,complex);
        result.invert();
        return result;
    }

    /**
     * Linearly interpolates this spinor with the given end result
     *
     * @param dst   The target spinor
     * @param t     The interpolation factor
     *
     * @return A reference to this (modified) Spinor for chaining
     */
    Spinor& lerp(Spinor dst, float t);

    /**
     * Spherically interpolates this spinor with the given end result
     *
     * @param dst   The target spinor
     * @param t     The interpolation factor
     *
     * @return A reference to this (modified) Spinor for chaining
     */
    Spinor& slerp(Spinor dst, float t);
  
#pragma mark -
#pragma mark Conversion Methods
    /**
     * Returns a string representation of this spinor for debugging purposes.
     *
     * If verbose is true, the string will include class information. This
     * allows us to unambiguously identify the class.
     *
     * @param verbose Whether to include class information
     *
     * @return a string representation of this vector for debugging purposes.
     */
    std::string toString(bool verbose = false) const;
    
    /** Cast from Spinor to a string. */
    operator std::string() const { return toString(); }
    
    /**
     * Creates a spinor from the given vector.
     *
     * The x is converted to the real component and y to complex.
     *
     * @param v The vector to convert
     */
    explicit Spinor(const Vec2& v);
    
    /**
     * Sets the values of this spinor to those of the given vector.
     *
     * The x is converted to the real component and y to complex.
     *
     * @param v The vector to convert
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator= (const Vec2& v);

    /**
     * Adds the given vector from this spinor in place.
     *
     * @param right The vector to add
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator+=(const Vec2& right);
    
    /**
     * Subtracts the given vector from this spinor in place.
     *
     * @param right The vector to subtract
     *
     * @return A reference to this (modified) Spinor for chaining.
     */
    Spinor& operator-=(const Vec2& right);

    /**
     * Returns the sum of this spinor with the given vector.
     *
     * Note: this does not modify this spinor.
     *
     * @param right The vector to add.
     *
     * @return The sum of this spinor with the given vector.
     */
    const Spinor operator+(const Vec2& right) const;
    
    /**
     * Returns the difference of this spinor with the given vector.
     *
     * Note: this does not modify this spinor.
     *
     * @param right The vector to subtract.
     *
     * @return The difference of this spinor with the given vector.
     */
    const Spinor operator-(const Vec2& right) const;
    
    /**
     * Casts from Spinor to Vec2.
     */
    operator Vec2() const;
    
};

}
#endif /* __CU_SPINOR_H__ */
