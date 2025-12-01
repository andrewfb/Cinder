/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Portions Copyright (c) 2004, Laminar Research.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/Cinder.h"
#include "cinder/CinderGlm.h"

#include <cmath>
#include <climits>
#include <cfloat>
#include <functional>
#if defined( CINDER_MSW )
	#undef min
	#undef max
#endif

namespace cinder {

template<typename T>
struct CI_API math
{
	static T	acos  (T x)		{return ::acos (double(x));}
	static T	asin  (T x)		{return ::asin (double(x));}
	static T	atan  (T x)		{return ::atan (double(x));}
	static T	atan2 (T y, T x)	{return ::atan2 (double(y), double(x));}
	static T	cos   (T x)		{return ::cos (double(x));}
	static T	sin   (T x)		{return ::sin (double(x));}
	static T	tan   (T x)		{return ::tan (double(x));}
	static T	cosh  (T x)		{return ::cosh (double(x));}
	static T	sinh  (T x)		{return ::sinh (double(x));}
	static T	tanh  (T x)		{return ::tanh (double(x));}
	static T	exp   (T x)		{return ::exp (double(x));}
	static T	log   (T x)		{return ::log (double(x));}
	static T	log10 (T x)		{return ::log10 (double(x));}
	static T	modf  (T x, T *iptr)
	{
		double ival;
		T rval( ::modf (double(x),&ival));
	*iptr = ival;
	return rval;
	}
	static T	pow   (T x, T y)	{return ::pow (double(x), double(y));}
	static T	sqrt  (T x)		{return ::sqrt (double(x));}
#if defined( _MSC_VER )
	static T	cbrt( T x )		{ return ( x > 0 ) ? (::pow( x, 1.0 / 3.0 )) : (- ::pow( -x, 1.0 / 3.0 ) ); }
#else
	static T	cbrt( T x )		{ return ::cbrt( x ); }
#endif
	static T	ceil  (T x)		{return ::ceil (double(x));}
	static T	abs  (T x)		{return ::fabs (double(x));}
	static T	floor (T x)		{return ::floor (double(x));}
	static T	fmod  (T x, T y)	{return ::fmod (double(x), double(y));}
	static T	hypot (T x, T y)	{return ::hypot (double(x), double(y));}
	static T	signum (T x)		{return ( x >0.0 ) ? 1.0 : ( ( x < 0.0 ) ? -1.0 : 0.0 ); }
	static T	min(T x, T y)				{return ( x < y ) ? x : y; }
	static T	max(T x, T y)				{return ( x > y ) ? x : y; }
	static T	clamp(T x, T min=0, T max=1)	{return ( x < min ) ? min : ( ( x > max ) ? max : x );}
};


template<>
struct CI_API math<float>
{
	static float	acos  (float x)			{return ::acosf (x);}
	static float	asin  (float x)			{return ::asinf (x);}
	static float	atan  (float x)			{return ::atanf (x);}
	static float	atan2 (float y, float x)	{return ::atan2f (y, x);}
	static float	cos   (float x)			{return ::cosf (x);}
	static float	sin   (float x)			{return ::sinf (x);}
	static float	tan   (float x)			{return ::tanf (x);}
	static float	cosh  (float x)			{return ::coshf (x);}
	static float	sinh  (float x)			{return ::sinhf (x);}
	static float	tanh  (float x)			{return ::tanhf (x);}
	static float	exp   (float x)			{return ::expf (x);}
	static float	log   (float x)			{return ::logf (x);}
	static float	log10 (float x)			{return ::log10f (x);}
	static float	modf  (float x, float *y)	{return ::modff (x, y);}
	static float	pow   (float x, float y)	{return ::powf (x, y);}
	static float	sqrt  (float x)			{return ::sqrtf (x);}
#if defined( _MSC_VER )
	static float	cbrt( float x )		{ return ( x > 0 ) ? (::powf( x, 1.0f / 3.0f )) : (- ::powf( -x, 1.0f / 3.0f ) ); }
#else
	static float	cbrt  (float x)			{ return ::cbrtf( x ); }	
#endif
	static float	ceil  (float x)			{return ::ceilf (x);}
	static float	abs   (float x)			{return ::fabsf (x);}
	static float	floor (float x)			{return ::floorf (x);}
	static float	fmod  (float x, float y)	{return ::fmodf (x, y);}
	#if !defined(_MSC_VER)
	static float	hypot (float x, float y)	{return ::hypotf (x, y);}
	#else
	static float hypot (float x, float y)	{return ::sqrtf(x*x + y*y);}
	#endif
	static float signum (float x)		{return ( x > 0.0f ) ? 1.0f : ( ( x < 0.0f ) ? -1.0f : 0.0f ); }
	static float min(float x, float y)					{return ( x < y ) ? x : y; }
	static float max(float x, float y)					{return ( x > y ) ? x : y; }
	static float clamp(float x, float min=0, float max=1)	{return ( x < min ) ? min : ( ( x > max ) ? max : x );}
};

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

constexpr double EPSILON_VALUE = 4.37114e-05;
#define EPSILON EPSILON_VALUE

CI_API inline bool approxZero( float n, float epsilon = float(EPSILON_VALUE) )
{
	return std::abs( n ) < epsilon;
}

CI_API inline bool approxZero( double n, double epsilon = EPSILON_VALUE )
{
	return std::abs( n ) < epsilon;
}

CI_API inline float roundToZero( float n, float epsilon = float(EPSILON_VALUE) )
{
	return approxZero( n, epsilon ) ? 0.0f : n;
}

CI_API inline double roundToZero( double n, double epsilon = EPSILON_VALUE )
{
	return approxZero( n, epsilon ) ? 0.0 : n;
}

CI_API inline bool approxEqual( float a, float b, float epsilon = float(EPSILON_VALUE) )
{
	return std::abs( b - a ) < epsilon;
}

CI_API inline bool approxEqual( double a, double b, double epsilon = EPSILON_VALUE )
{
	return std::abs( b - a ) < epsilon;
}

CI_API inline bool approxEqualRelative( float a, float b, float maxRelDiff = float(EPSILON_VALUE) )
{
	// See: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

	// Calculate the difference.
	const float diff = std::abs( a - b );

	// Find the largest.
	a = std::abs( a );
	b = std::abs( b );
	const float largest = ( b > a ) ? b : a;

	if( diff <= largest * maxRelDiff )
		return true;

	return false;
}

CI_API inline bool approxEqualRelative( double a, double b, double maxRelDiff = EPSILON_VALUE )
{
	// See: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

	// Calculate the difference.
	const double diff = std::abs( a - b );

	// Find the largest.
	a = std::abs( a );
	b = std::abs( b );
	const double largest = ( b > a ) ? b : a;

	if( diff <= largest * maxRelDiff )
		return true;

	return false;
}

inline float toRadians( float x )
{
	return x * 0.017453292519943295769f; // ( x * PI / 180 )
}

inline double toRadians( double x )
{
	return x * 0.017453292519943295769; // ( x * PI / 180 )
}

inline float toDegrees( float x )
{
	return x * 57.295779513082321f; // ( x * 180 / PI )
}

inline double toDegrees( double x )
{
	return x * 57.295779513082321; // ( x * 180 / PI )
}

template<typename T, typename L>
T lerp( const T &a, const T &b, L factor )
{
	return a + ( b - a ) * factor;
}

template<typename T>
T lmap(T val, T inMin, T inMax, T outMin, T outMax)
{
	return outMin + ((outMax - outMin) * (val - inMin)) / (inMax - inMin);
}

template<typename T, typename L>
T bezierInterp( T a, T b, T c, T d, L t)
{
    L t1 = static_cast<L>(1.0) - t;
    return a*(t1*t1*t1) + b*(3*t*t1*t1) + c*(3*t*t*t1) + d*(t*t*t);
}

template<typename T, typename L>
T bezierInterpRef( const T &a, const T &b, const T &c, const T &d, L t)
{
    L t1 = static_cast<L>(1.0) - t;
    return a*(t1*t1*t1) + b*(3*t*t1*t1) + c*(3*t*t*t1) + d*(t*t*t);
}

template<typename T>
T constrain( T val, T minVal, T maxVal )
{
	if( val < minVal ) return minVal;
	else if( val > maxVal ) return maxVal;
	else return val;
}

//! Returns the fractional part of \a x, calculated as `x - floor( x )`
template<typename T>
T fract( T x )
{
	return x - math<T>::floor( x );
}

// Don Hatch's version of sin(x)/x, which is accurate for very small x.
// Returns 1 for x == 0.
template <class T>
T sinx_over_x( T x )
{
    if( x * x < 1.19209290E-07F )
	return T( 1 );
    else
	return math<T>::sin( x ) / x;
}

// There are faster techniques for this, but this is portable
inline uint32_t log2floor( uint32_t x )
{
    uint32_t result = 0;
    while( x >>= 1 )
        ++result;

    return result;
}

inline uint32_t log2ceil( uint32_t x )
{
	uint32_t isNotPowerOf2 = (x & (x - 1));
	return ( isNotPowerOf2 ) ? (log2floor( x ) + 1) : log2floor( x );
}

inline uint32_t nextPowerOf2( uint32_t x )
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return(x+1);
}

//! Returns true if \a x is a power of two, false otherwise.
inline bool isPowerOf2( size_t x )
{
	return ( x & ( x - 1 ) ) == 0;
}

template<typename T>
inline int solveLinear( T a, T b, T result[1] )
{
	if( a == 0 ) return (b == 0 ? -1 : 0 );
	result[0] = -b / a;
	return 1;
}

template<typename T>
inline int solveQuadratic( T a, T b, T c, T result[2] )
{
	if( a == 0 ) return solveLinear( b, c, result );

	T radical = b * b - 4 * a * c;
	if( radical < 0 ) return 0;

	if( radical == 0 ) {
		result[0] = -b / (2 * a);
		return 1;
	}

	T srad = math<T>::sqrt( radical );
	result[0] = ( -b - srad ) / (2 * a);
	result[1] = ( -b + srad ) / (2 * a);
	if( a < 0 ) std::swap( result[0], result[1] );
	return 2;
}

template<typename T,int ORDER>
T rombergIntegral( T a, T b, const std::function<T(T)> &SPEEDFN )
{
	static_assert(ORDER > 2, "ORDER must be greater than 2" );
	T rom[2][ORDER];
	T half = b - a;

	rom[0][0] = ((T)0.5) * half * ( SPEEDFN(a)+SPEEDFN(b) );
	for( int i0=2, iP0=1; i0 <= ORDER; i0++, iP0 *= 2, half *= (T)0.5) {
		// approximations via the trapezoid rule
		T sum = 0;
		for( int i1 = 1; i1 <= iP0; i1++ )
			sum += SPEEDFN(a + half*(i1-((T)0.5)));

		// Richardson extrapolation
		rom[1][0] = ((T)0.5)*(rom[0][0] + half*sum);
		for( int i2 = 1, iP2 = 4; i2 < i0; i2++, iP2 *= 4 )
			rom[1][i2] = (iP2*rom[1][i2-1] - rom[0][i2-1])/(iP2-1);
		for( int i1 = 0; i1 < i0; i1++ )
			rom[0][i1] = rom[1][i1];
	}

	return rom[0][ORDER-1];
}

template<typename T>
CI_API int solveCubic( T a, T b, T c, T d, T result[3] );

//! Returns the closest point to \a testPoint on the boundary of the ellipse defined by \a center, \a axisA and \a axisB. Algorithm due to David Eberly, http://www.geometrictools.com/Documentation/DistancePointEllipseEllipsoid.pdf
CI_API glm::vec2 getClosestPointEllipse( const glm::vec2& center, const glm::vec2& axisA, const glm::vec2& axisB, const glm::vec2& testPoint );

//! Returns the closest point to \a testPoint on the line defined by the 2 \a controlPoints.
template<typename T>
CI_API glm::tvec2<T, glm::defaultp> getClosestPointLinear( const glm::tvec2<T, glm::defaultp> *controlPoints, const glm::tvec2<T, glm::defaultp> &testPoint );
//! Returns the closest point to \a testPoint on the line defined by the control points \a p0 and \a p1.
template<typename T>
glm::tvec2<T, glm::defaultp>		getClosestPointLinear( const glm::tvec2<T, glm::defaultp> &p0, const glm::tvec2<T, glm::defaultp> &p1, const glm::tvec2<T, glm::defaultp> &testPoint )
{
	glm::tvec2<T, glm::defaultp> controlPoints[] = { p0, p1 };
	return getClosestPointLinear<T>( controlPoints, testPoint );
}

//! Returns the closest point to \a testPoint on the quadratic curve defined by the 3 \a controlPoints. Algorithm due to Olivier Besson, http://blog.gludion.com/2009/08/distance-to-quadratic-bezier-curve.html
template<typename T>
CI_API glm::tvec2<T, glm::defaultp> getClosestPointQuadratic( const glm::tvec2<T, glm::defaultp> *controlPoints, const glm::tvec2<T, glm::defaultp> &testPoint );
//! Returns the closest point to \a testPoint on the quadratic curve defined by the control points \a p0, \a p1 and \a p2. Algorithm due to Olivier Besson, http://blog.gludion.com/2009/08/distance-to-quadratic-bezier-curve.html
template<typename T>
glm::tvec2<T, glm::defaultp>		getClosestPointQuadratic( const glm::tvec2<T, glm::defaultp> &p0, const glm::tvec2<T, glm::defaultp> &p1, const glm::tvec2<T, glm::defaultp> &p2, const glm::tvec2<T, glm::defaultp> &testPoint )
{
	glm::tvec2<T, glm::defaultp> controlPoints[] = { p0, p1, p2 };
	return getClosestPointQuadratic<T>( controlPoints, testPoint );
}

//! Returns the closest point to \a testPoint on the cubic curve defined by the 4 \a controlPoints. Algorithm due to Philip J. Schneider, https://github.com/erich666/GraphicsGems/blob/master/gems/NearestPoint.c
template<typename T>
CI_API glm::tvec2<T, glm::defaultp> getClosestPointCubic( const glm::tvec2<T, glm::defaultp> *controlPoints, const glm::tvec2<T, glm::defaultp> &testPoint );
//! Returns the closest point to \a testPoint on the cubic curve defined by the control points \a p0, \a p1, \a p2 and \a p3. Algorithm due to Philip J. Schneider, https://github.com/erich666/GraphicsGems/blob/master/gems/NearestPoint.c
template<typename T>
glm::tvec2<T, glm::defaultp>		getClosestPointCubic( const glm::tvec2<T, glm::defaultp> &p0, const glm::tvec2<T, glm::defaultp> &p1, const glm::tvec2<T, glm::defaultp> &p2, const glm::tvec2<T, glm::defaultp> &p3, const glm::tvec2<T, glm::defaultp> &testPoint )
{
	glm::tvec2<T, glm::defaultp> controlPoints[] = { p0, p1, p2, p3 };
	return getClosestPointCubic<T>( controlPoints, testPoint );
}

//=============================================================================
// 2D Vector Utilities
//=============================================================================

//! 2D cross product (returns scalar z-component of 3D cross product)
template<typename T>
inline T cross2d( const glm::tvec2<T>& a, const glm::tvec2<T>& b )
{
	return a.x * b.y - a.y * b.x;
}

//! Perpendicular vector (90 degree counter-clockwise rotation)
template<typename T>
inline glm::tvec2<T> perp( const glm::tvec2<T>& v )
{
	return glm::tvec2<T>( -v.y, v.x );
}

//=============================================================================
// Numerically Stable Polynomial Solvers
//=============================================================================
//
// These "Stable" variants complement the existing solveQuadratic() and solveCubic()
// functions with improved numerical stability for edge cases:
//
// solveQuadratic(a,b,c):  Solves ax² + bx + c = 0 using the classic quadratic formula.
//                         Simple and fast, but can suffer from catastrophic cancellation
//                         when -b and √(b²-4ac) are nearly equal.
//
// solveQuadraticStable(): Uses the "citardauq" formula which avoids cancellation by
//                         computing one root via the standard formula and the other via
//                         Vieta's formula (c/q). Uses copysign() to always add terms of
//                         the same sign in the discriminant.
//                         Reference: Press et al., "Numerical Recipes" §5.6
//                         https://numerical.recipes/book.html
//
// solveCubicStable():     Uses Jim Blinn's method which is more numerically robust than
//                         Cardano's formula, especially for the "three real roots" case
//                         where it uses trigonometric identities instead of complex cube roots.
//                         Reference: Blinn, "How to Solve a Cubic Equation" (IEEE CG&A 2006-2007)
//                         https://courses.cs.washington.edu/courses/cse590b/13au/lecture_notes/solvecubic_p5.pdf
//
// Note: Coefficient order differs - stable versions use c0 + c1*x + c2*x² form
//       (constant term first), while the original uses ax² + bx + c form.
//=============================================================================

//! Solve quadratic c0 + c1*x + c2*x² = 0 using numerically stable "citardauq" formula.
//! Avoids catastrophic cancellation that can occur with the classic quadratic formula.
//! Returns number of real roots (0, 1, or 2). Results sorted in ascending order.
//! @see solveQuadratic() for the classic formula (faster but less stable)
template<typename T>
inline int solveQuadraticStable( T c0, T c1, T c2, T result[2] )
{
	constexpr T epsilon = T( 1e-12 );

	if( std::abs( c2 ) < epsilon ) {
		if( std::abs( c1 ) < epsilon ) {
			return 0;
		}
		result[0] = -c0 / c1;
		return 1;
	}

	T disc = c1 * c1 - T( 4 ) * c2 * c0;
	if( disc < 0 ) {
		return 0;
	}

	if( disc == 0 ) {
		result[0] = -c1 / ( T( 2 ) * c2 );
		return 1;
	}

	// Citardauq formula: q = -0.5 * (c1 + sign(c1)*sqrt(disc))
	// Then roots are q/c2 and c0/q (Vieta's formula)
	T q = T( -0.5 ) * ( c1 + std::copysign( std::sqrt( disc ), c1 ) );
	result[0] = q / c2;
	result[1] = c0 / q;

	if( result[0] > result[1] ) {
		std::swap( result[0], result[1] );
	}

	return 2;
}

//! Solve cubic c0 + c1*x + c2*x² + c3*x³ = 0 using Blinn's method.
//! More numerically stable than Cardano's formula, especially for three real roots.
//! Returns number of real roots (1, 2, or 3). Results sorted in ascending order.
//! Based on Jim Blinn's "How to Solve a Cubic Equation" (IEEE CG&A 2006-2007).
//! @see solveCubic() for the classic Cardano formula
template<typename T>
inline int solveCubicStable( T c0, T c1, T c2, T c3, T result[3] )
{
	constexpr T ONETHIRD = T( 1.0 / 3.0 );

	T c3_recip = T( 1.0 ) / c3;
	T scaled_c2 = c2 * ( ONETHIRD * c3_recip );
	T scaled_c1 = c1 * ( ONETHIRD * c3_recip );
	T scaled_c0 = c0 * c3_recip;

	if( !std::isfinite( scaled_c0 ) || !std::isfinite( scaled_c1 ) || !std::isfinite( scaled_c2 ) ) {
		return solveQuadraticStable( c0, c1, c2, result );
	}

	// Blinn's d-form coefficients
	T d0 = -scaled_c2 * scaled_c2 + scaled_c1;
	T d1 = -scaled_c1 * scaled_c2 + scaled_c0;
	T d2 = scaled_c2 * scaled_c0 - scaled_c1 * scaled_c1;

	T d = T( 4.0 ) * d0 * d2 - d1 * d1;  // Discriminant
	T de = T( -2.0 ) * scaled_c2 * d0 + d1;

	if( d < 0 ) {
		// One real root - use Cardano-like formula
		T sq = std::sqrt( T( -0.25 ) * d );
		T r = T( -0.5 ) * de;
		T t1 = std::cbrt( r + sq ) + std::cbrt( r - sq );
		result[0] = t1 - scaled_c2;
		return 1;
	}
	else if( d == 0 ) {
		// Two real roots (one repeated)
		T t1 = std::copysign( std::sqrt( -d0 ), de );
		result[0] = t1 - scaled_c2;
		result[1] = T( -2.0 ) * t1 - scaled_c2;
		if( result[0] > result[1] ) std::swap( result[0], result[1] );
		return 2;
	}
	else {
		// Three real roots - use trigonometric method to avoid complex arithmetic
		T th = std::atan2( std::sqrt( d ), -de ) * ONETHIRD;
		T th_cos = std::cos( th );
		T th_sin = std::sin( th );
		T ss3 = th_sin * std::sqrt( T( 3.0 ) );
		T t = T( 2.0 ) * std::sqrt( -d0 );

		result[0] = t * th_cos - scaled_c2;
		result[1] = t * T( 0.5 ) * ( -th_cos + ss3 ) - scaled_c2;
		result[2] = t * T( 0.5 ) * ( -th_cos - ss3 ) - scaled_c2;

		if( result[0] > result[1] ) std::swap( result[0], result[1] );
		if( result[1] > result[2] ) std::swap( result[1], result[2] );
		if( result[0] > result[1] ) std::swap( result[0], result[1] );
		return 3;
	}
}

//=============================================================================
// Root Finding Algorithms
//=============================================================================

//! ITP (Interpolate-Truncate-Project) root finding method.
//! A hybrid algorithm combining bisection with the regula falsi method,
//! guaranteeing worst-case performance of bisection while achieving
//! superlinear convergence for smooth functions.
//! https://en.wikipedia.org/wiki/ITP_method
//!
//! Finds x in [a,b] where f(x) = 0. Requires ya = f(a) < 0 and yb = f(b) > 0.
//! @param f Function to find root of (callable taking T, returning T)
//! @param a Lower bound of bracket
//! @param b Upper bound of bracket
//! @param epsilon Tolerance for result (stops when interval < 2*epsilon)
//! @param n0 Slack parameter for iteration count (typically 0 or 1)
//! @param k1 Tuning parameter controlling truncation (typically 0.2 / (b - a))
//! @param ya Value of f(a), must be negative
//! @param yb Value of f(b), must be positive
template<typename T, typename F>
inline T solveItp( F&& f, T a, T b, T epsilon, int n0, T k1, T ya, T yb )
{
	T n1_2 = std::max( T( 0 ), std::ceil( std::log2( ( b - a ) / epsilon ) ) - T( 1 ) );
	int nmax = n0 + static_cast<int>( n1_2 );
	T scaledEpsilon = epsilon * static_cast<T>( 1ull << nmax );

	while( b - a > T( 2 ) * epsilon ) {
		T x1_2 = T( 0.5 ) * ( a + b );
		T r = scaledEpsilon - T( 0.5 ) * ( b - a );
		T xf = ( yb * a - ya * b ) / ( yb - ya );
		T sigma = x1_2 - xf;
		T delta = k1 * ( b - a ) * ( b - a );
		T xt = ( delta <= std::abs( x1_2 - xf ) )
			? xf + std::copysign( delta, sigma )
			: x1_2;
		T xitp = ( std::abs( xt - x1_2 ) <= r )
			? xt
			: x1_2 - std::copysign( r, sigma );
		T yitp = f( xitp );
		if( yitp > 0 ) {
			b = xitp;
			yb = yitp;
		}
		else if( yitp < 0 ) {
			a = xitp;
			ya = yitp;
		}
		else {
			return xitp;
		}
		scaledEpsilon *= T( 0.5 );
	}
	return T( 0.5 ) * ( a + b );
}

//=============================================================================
// Bezier Curve Evaluation (GLM vec2/dvec2)
//=============================================================================

//! Evaluate quadratic Bezier at parameter t using de Casteljau's algorithm
//! @tparam T Scalar type (float or double), determines vec2 vs dvec2
template<typename T>
inline glm::tvec2<T> evalQuadraticBezier( const glm::tvec2<T> p[3], T t )
{
	T mt = T( 1 ) - t;
	return mt * mt * p[0] + T( 2 ) * mt * t * p[1] + t * t * p[2];
}

//! Evaluate quadratic Bezier derivative at parameter t
//! Returns the tangent vector (not normalized)
template<typename T>
inline glm::tvec2<T> evalQuadraticBezierDeriv( const glm::tvec2<T> p[3], T t )
{
	T mt = T( 1 ) - t;
	return T( 2 ) * ( mt * ( p[1] - p[0] ) + t * ( p[2] - p[1] ) );
}

//! Evaluate cubic Bezier at parameter t using Bernstein basis
template<typename T>
inline glm::tvec2<T> evalCubicBezier( const glm::tvec2<T> p[4], T t )
{
	T t2 = t * t;
	T t3 = t2 * t;
	T mt = T( 1 ) - t;
	T mt2 = mt * mt;
	T mt3 = mt2 * mt;
	return mt3 * p[0] + T( 3 ) * mt2 * t * p[1] + T( 3 ) * mt * t2 * p[2] + t3 * p[3];
}

//! Evaluate cubic Bezier derivative at parameter t
//! Returns the tangent vector (not normalized). The derivative of a cubic is a quadratic.
template<typename T>
inline glm::tvec2<T> evalCubicBezierDeriv( const glm::tvec2<T> p[4], T t )
{
	T mt = T( 1 ) - t;
	glm::tvec2<T> q0 = T( 3 ) * ( p[1] - p[0] );
	glm::tvec2<T> q1 = T( 3 ) * ( p[2] - p[1] );
	glm::tvec2<T> q2 = T( 3 ) * ( p[3] - p[2] );
	return mt * mt * q0 + T( 2 ) * mt * t * q1 + t * t * q2;
}

//! Degree elevation: convert quadratic Bezier to equivalent cubic Bezier
//! The cubic curve passes through identical points as the quadratic.
template<typename T>
inline void raiseQuadraticToCubic( const glm::tvec2<T> q[3], glm::tvec2<T> c[4] )
{
	c[0] = q[0];
	c[1] = q[0] + T( 2.0 / 3.0 ) * ( q[1] - q[0] );
	c[2] = q[2] + T( 2.0 / 3.0 ) * ( q[1] - q[2] );
	c[3] = q[2];
}

//=============================================================================
// Bezier Curve Subdivision (GLM vec2/dvec2)
//=============================================================================
// These functions use de Casteljau's algorithm to split Bezier curves at a
// parameter value t. The algorithm computes intermediate control points by
// linear interpolation, which is numerically stable.
// https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm

//! Subdivide quadratic Bezier at parameter t, return left half [0, t]
template<typename T>
inline void subdivideQuadraticLeft( const glm::tvec2<T> q[3], T t, glm::tvec2<T> result[3] )
{
	glm::tvec2<T> a0 = q[0] + t * ( q[1] - q[0] );
	glm::tvec2<T> a1 = q[1] + t * ( q[2] - q[1] );
	glm::tvec2<T> b0 = a0 + t * ( a1 - a0 );

	result[0] = q[0];
	result[1] = a0;
	result[2] = b0;
}

//! Subdivide quadratic Bezier at parameter t, return right half [t, 1]
template<typename T>
inline void subdivideQuadraticRight( const glm::tvec2<T> q[3], T t, glm::tvec2<T> result[3] )
{
	glm::tvec2<T> a0 = q[0] + t * ( q[1] - q[0] );
	glm::tvec2<T> a1 = q[1] + t * ( q[2] - q[1] );
	glm::tvec2<T> b0 = a0 + t * ( a1 - a0 );

	result[0] = b0;
	result[1] = a1;
	result[2] = q[2];
}

//! Subdivide cubic Bezier at parameter t, return left half [0, t]
template<typename T>
inline void subdivideCubicLeft( const glm::tvec2<T> p[4], T t, glm::tvec2<T> result[4] )
{
	glm::tvec2<T> a0 = p[0] + t * ( p[1] - p[0] );
	glm::tvec2<T> a1 = p[1] + t * ( p[2] - p[1] );
	glm::tvec2<T> a2 = p[2] + t * ( p[3] - p[2] );
	glm::tvec2<T> b0 = a0 + t * ( a1 - a0 );
	glm::tvec2<T> b1 = a1 + t * ( a2 - a1 );
	glm::tvec2<T> c0 = b0 + t * ( b1 - b0 );

	result[0] = p[0];
	result[1] = a0;
	result[2] = b0;
	result[3] = c0;
}

//! Subdivide cubic Bezier at parameter t, return right half [t, 1]
template<typename T>
inline void subdivideCubicRight( const glm::tvec2<T> p[4], T t, glm::tvec2<T> result[4] )
{
	glm::tvec2<T> a0 = p[0] + t * ( p[1] - p[0] );
	glm::tvec2<T> a1 = p[1] + t * ( p[2] - p[1] );
	glm::tvec2<T> a2 = p[2] + t * ( p[3] - p[2] );
	glm::tvec2<T> b0 = a0 + t * ( a1 - a0 );
	glm::tvec2<T> b1 = a1 + t * ( a2 - a1 );
	glm::tvec2<T> c0 = b0 + t * ( b1 - b0 );

	result[0] = c0;
	result[1] = b1;
	result[2] = a2;
	result[3] = p[3];
}

//! Extract subsegment [t0, t1] from cubic Bezier
//! Useful for trimming curves or extracting dash segments
template<typename T>
inline void subdivideCubicRange( const glm::tvec2<T> p[4], T t0, T t1, glm::tvec2<T> result[4] )
{
	// First split at t0 to get right half
	glm::tvec2<T> a0 = p[0] + t0 * ( p[1] - p[0] );
	glm::tvec2<T> a1 = p[1] + t0 * ( p[2] - p[1] );
	glm::tvec2<T> a2 = p[2] + t0 * ( p[3] - p[2] );
	glm::tvec2<T> b0 = a0 + t0 * ( a1 - a0 );
	glm::tvec2<T> b1 = a1 + t0 * ( a2 - a1 );
	glm::tvec2<T> c0 = b0 + t0 * ( b1 - b0 );

	// Then split the right half at adjusted parameter
	T s = ( t1 - t0 ) / ( T( 1 ) - t0 );

	glm::tvec2<T> d0 = c0 + s * ( b1 - c0 );
	glm::tvec2<T> d1 = b1 + s * ( a2 - b1 );
	glm::tvec2<T> d2 = a2 + s * ( p[3] - a2 );
	glm::tvec2<T> e0 = d0 + s * ( d1 - d0 );
	glm::tvec2<T> e1 = d1 + s * ( d2 - d1 );
	glm::tvec2<T> f0 = e0 + s * ( e1 - e0 );

	result[0] = c0;
	result[1] = d0;
	result[2] = e0;
	result[3] = f0;
}

//=============================================================================
// Gauss-Legendre Quadrature Coefficients
//=============================================================================
// Gauss-Legendre quadrature approximates definite integrals using weighted
// sums of function values at specific nodes. For integrating f(x) over [-1,1]:
//   ∫f(x)dx ≈ Σ w[i] * f(x[i])
//
// For arbitrary intervals [a,b], transform: t = (b-a)/2 * x + (a+b)/2
//   ∫f(t)dt ≈ (b-a)/2 * Σ w[i] * f((b-a)/2 * x[i] + (a+b)/2)
//
// 5-point quadrature is exact for polynomials up to degree 9 and provides
// good accuracy for smooth functions like Bezier curve arc lengths.
// https://en.wikipedia.org/wiki/Gauss%E2%80%93Legendre_quadrature
// https://pomax.github.io/bezierinfo/legendre-gauss.html (coefficient tables)

//! 5-point Gauss-Legendre quadrature weights
constexpr double GAUSS_LEGENDRE_5_WEIGHTS[] = {
	0.5688888888888889,
	0.4786286704993665,
	0.4786286704993665,
	0.2369268850561891,
	0.2369268850561891
};

//! 5-point Gauss-Legendre quadrature nodes (in [-1, 1])
constexpr double GAUSS_LEGENDRE_5_NODES[] = {
	0.0,
	-0.5384693101056831,
	0.5384693101056831,
	-0.9061798459386640,
	0.9061798459386640
};

union half_float
{
	uint16_t u;
	struct {
		uint16_t Mantissa : 10;
		uint16_t Exponent : 5;
		uint16_t Sign : 1;
	};
};

CI_API half_float floatToHalf( float f );
CI_API float halfToFloat( half_float h );

} // namespace cinder

#if defined( _MSC_VER ) && ( _MSC_VER < 1800 )
// define math.h functions that aren't defined until vc120
namespace std {

inline bool isfinite( float arg )	{ return _finite( arg ) != 0; }
inline bool isfinite( double arg )	{ return _finite( arg ) != 0; }
inline bool isnan( float arg )		{ return _isnan( arg ) != 0; }
inline bool isnan( double arg )		{ return _isnan( arg ) != 0; }

// note that while these round* variants follow the basic premise of c99 implementations (numbers with fractional parts of 0.5 should be
// rounded away from zero), they are not 100% compliable implementations since they do not cover all edge cases like NaN's, inifinite numbers, etc.
inline double	round( double x )	{ return floor( x < 0 ? x - 0.5 : x + 0.5 ); }
inline float	roundf( float x )	{ return floorf( x < 0 ? x - 0.5f : x + 0.5f );	}
inline long int lround( double x )	{ return (long int)( x < 0 ? x - 0.5 : x + 0.5 ); }
inline long int lroundf( float x )	{ return (long int)( x < 0 ? x - 0.5f : x + 0.5f );	}

} // namespace std
#endif // defined( _MSC_VER ) && ( _MSC_VER < 1800 )
