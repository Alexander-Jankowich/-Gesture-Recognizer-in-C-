//
//  CUSpinor.cpp
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
#include <cugl/core/math/CUSpinor.h>
#include <cugl/core/util/CUStringTools.h>
#include <sstream>

using namespace cugl;


#pragma mark Angle Operations

/** The cosine threshold for interpolation */
#define COSINE_THRESHOLD 0.001f

/**
 * Sets the spinor to have the given angle (in radians)
 *
 * @param angle    The spinor angle
 *
 * @return A reference to this (modified) Spinor for chaining
 */
Spinor& Spinor::set(float angle) {
    angle /= 2;
    real = cos(angle);
    complex = sin(angle);
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
Spinor& Spinor::add(float angle) {
    angle /= 2;
    real += cos(angle);
    complex += sin(angle);
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
Spinor& Spinor::subtract(float angle) {
    angle /= 2;
    real -= cos(angle);
    complex -= sin(angle);
    return *this;
}

/**
 * Returns the angle represented by this spinor
 *
 * @return the angle represented by this spinor
 */
float Spinor::getAngle() const {
    return atan2(complex, real) * 2;
}
    

#pragma mark -
#pragma mark Linear Algebra
/**
 * Multiplies this spinor by the given one.
 *
 * @param other    The spinor to multiply
 *
 * @return A reference to this (modified) Spinor for chaining.
 */
Spinor& Spinor::multiply(const Spinor other) {
    float r = real * other.real - complex * other.complex;
    float c = real * other.complex + complex * other.real;
    real = r; complex = c;
    return *this;
}

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
Spinor& Spinor::normalize() {
    float n = real * real + complex * complex;
    // Already normalized.
    if (n == 1.0f) {
        return *this;
    }
    
    n = sqrt(n);

    // Too close to zero.
    if (n < CU_MATH_FLOAT_SMALL) {
        return *this;
    }
    
    n = 1.0f / n;
    real *= n;
    complex *= n;

    return *this;
}
    
/**
 * Sets this spinor to its inverse.
 *
 * @return A reference to this (modified) Spinor for chaining.
 */
Spinor& Spinor::invert() {
    complex = -complex;
    scale(lengthSquared());
    return *this;
}

/**
 * Linearly interpolates this spinor with the given end result
 *
 * @param dst   The target spinor
 * @param t     The interpolation factor
 *
 * @return A reference to this (modified) Spinor for chaining
 */
Spinor& Spinor::lerp(Spinor dst, float t) {
    scale(1 - t);
    add(dst*t);
    normalize();
    return *this;
}

/**
 * Spherically interpolates this spinor with the given end result
 *
 * @param dst   The target spinor
 * @param t     The interpolation factor
 *
 * @return A reference to this (modified) Spinor for chaining
 */
Spinor& Spinor::slerp(Spinor dst, float t) {
    float tr, tc, omega, cosom, sinom;
    float scale0, scale1;

    // cosine
    cosom = real * dst.real + complex * dst.complex;

    // adjust signs
    if (cosom < 0) {
        cosom = -cosom;
        tc = -dst.complex;
        tr = -dst.real;
    } else {
        tc = dst.complex;
        tr = dst.real;
    }

    // coefficients
    if (1.0f - cosom > COSINE_THRESHOLD) {
        omega = (float)acos(cosom);
        sinom = (float)sin(omega);
        scale0 = (float)sin((1.0f - t) * omega) / sinom;
        scale1 = (float)sin(t * omega) / sinom;
    } else {
        scale0 = 1.0f - t;
        scale1 = t;
    }

    // final calculation
    complex = scale0 * complex + scale1 * tc;
    real = scale0 * real + scale1 * tr;

    return *this;
}
  
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
std::string Spinor::toString(bool verbose) const {
    std::stringstream ss;
    if (verbose) {
        ss << "cugl::Spinor[";
    }
    ss << real;
    ss << "i+";
    ss << complex;
    ss << "j";
    if (verbose) {
        ss << "]";
    }
    return ss.str();
}

/**
 * Creates a spinor from the given vector.
 *
 * The x is converted to the real component and y to complex.
 *
 * @param v The vector to convert
 */
Spinor::Spinor(const Vec2& v) {
    real = v.x;
    complex = v.y;
}

/**
 * Sets the values of this spinor to those of the given vector.
 *
 * The x is converted to the real component and y to complex.
 *
 * @param v The vector to convert
 *
 * @return A reference to this (modified) Spinor for chaining.
 */
Spinor& Spinor::operator= (const Vec2& v) {
    real = v.x;
    complex = v.y;
    return *this;
}

/**
 * Adds the given vector from this spinor in place.
 *
 * @param right The vector to add
 *
 * @return A reference to this (modified) Spinor for chaining.
 */
Spinor& Spinor::operator+=(const Vec2&right) {
    real += right.x;
    complex += right.y;
    return *this;
}

/**
 * Subtracts the given vector from this spinor in place.
 *
 * @param right The vector to subtract
 *
 * @return A reference to this (modified) Spinor for chaining.
 */
Spinor& Spinor::operator-=(const Vec2&right) {
    real -= right.x;
    complex -= right.y;
    return *this;
}

/**
 * Returns the sum of this spinor with the given vector.
 *
 * Note: this does not modify this spinor.
 *
 * @param right The vector to add.
 *
 * @return The sum of this spinor with the given vector.
 */
const Spinor Spinor::operator+(const Vec2&right) const {
    Spinor result(*this);
    result += right;
    return result;
}

/**
 * Returns the difference of this spinor with the given vector.
 *
 * Note: this does not modify this spinor.
 *
 * @param right The vector to subtract.
 *
 * @return The difference of this spinor with the given vector.
 */
const Spinor Spinor::operator-(const Vec2&right) const {
    Spinor result(*this);
    result -= right;
    return result;
}

/**
 * Casts from Spinor to Vec2.
 */
Spinor::operator Vec2() const {
    return Vec2(real,complex);
}
