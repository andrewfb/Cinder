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

//! Numerically stable quadratic solver using Kahan's method to avoid cancellation when b² >> 4ac. Recommended over solveQuadratic() for numerical robustness. Returns roots in ascending order: result[0] <= result[1].
template<typename T>
inline int solveQuadraticStable( T a, T b, T c, T result[2] )
{
	if( a == 0 ) return solveLinear( b, c, result );

	T discriminant = b * b - 4 * a * c;
	if( discriminant < 0 ) return 0;

	if( discriminant == 0 ) {
		result[0] = -b / (2 * a);
		return 1;
	}

	// Use Kahan's method to avoid catastrophic cancellation
	// When b and sqrt(discriminant) are close, subtracting them loses precision
	// Instead, compute q = -0.5 * (b + sign(b) * sqrt(discriminant))
	// Then x1 = q/a and x2 = c/q (using Vieta's formula)
	T sqrtDisc = math<T>::sqrt( discriminant );
	T q = (b >= 0) ? -T(0.5) * (b + sqrtDisc) : -T(0.5) * (b - sqrtDisc);

	result[0] = q / a;
	result[1] = c / q;

	// Ensure ascending order to match solveQuadratic() API contract
	if( result[0] > result[1] )
		std::swap( result[0], result[1] );

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

//! Gauss-Legendre quadrature for integrating functions over [a,b]. Typically faster than Romberg for smooth functions (like Bezier arc length).
//! Uses 7-point Gauss-Legendre (degree 13 polynomial exactness), which provides similar accuracy to rombergIntegral<T,7> with fewer function evaluations.
template<typename T>
T gaussLegendreIntegral7( T a, T b, const std::function<T(T)> &func )
{
	// 7-point Gauss-Legendre abscissas and weights for [-1, 1]
	static const T abscissas[7] = {
		T(-0.9491079123427585),
		T(-0.7415311855993945),
		T(-0.4058451513773972),
		T(0.0),
		T(0.4058451513773972),
		T(0.7415311855993945),
		T(0.9491079123427585)
	};
	static const T weights[7] = {
		T(0.1294849661688697),
		T(0.2797053914892766),
		T(0.3818300505051189),
		T(0.4179591836734694),
		T(0.3818300505051189),
		T(0.2797053914892766),
		T(0.1294849661688697)
	};

	// Transform from [-1, 1] to [a, b]
	T halfRange = (b - a) * T(0.5);
	T midPoint = (a + b) * T(0.5);

	T sum = 0;
	for( int i = 0; i < 7; ++i ) {
		T x = midPoint + halfRange * abscissas[i];
		sum += weights[i] * func( x );
	}

	return halfRange * sum;
}

//! 16-point Gauss-Legendre quadrature for higher accuracy integration. Useful for offset curve calculations and precise arc length computation.
template<typename T>
T gaussLegendreIntegral16( T a, T b, const std::function<T(T)> &func )
{
	// 16-point Gauss-Legendre abscissas and weights for [-1, 1]
	static const T abscissas[16] = {
		T(-0.9894009349916499),
		T(-0.9445750230732326),
		T(-0.8656312023878318),
		T(-0.7554044083550030),
		T(-0.6178762444026438),
		T(-0.4580167776572274),
		T(-0.2816035507792589),
		T(-0.0950125098376374),
		T(0.0950125098376374),
		T(0.2816035507792589),
		T(0.4580167776572274),
		T(0.6178762444026438),
		T(0.7554044083550030),
		T(0.8656312023878318),
		T(0.9445750230732326),
		T(0.9894009349916499)
	};
	static const T weights[16] = {
		T(0.0271524594117541),
		T(0.0622535239386479),
		T(0.0951585116824928),
		T(0.1246289712555339),
		T(0.1495959888165767),
		T(0.1691565193950025),
		T(0.1826034150449236),
		T(0.1894506104550685),
		T(0.1894506104550685),
		T(0.1826034150449236),
		T(0.1691565193950025),
		T(0.1495959888165767),
		T(0.1246289712555339),
		T(0.0951585116824928),
		T(0.0622535239386479),
		T(0.0271524594117541)
	};

	// Transform from [-1, 1] to [a, b]
	T halfRange = (b - a) * T(0.5);
	T midPoint = (a + b) * T(0.5);

	T sum = 0;
	for( int i = 0; i < 16; ++i ) {
		T x = midPoint + halfRange * abscissas[i];
		sum += weights[i] * func( x );
	}

	return halfRange * sum;
}

template<typename T>
CI_API int solveCubic( T a, T b, T c, T d, T result[3] );

//! Yuksel's robust Newton-Raphson root finding. Given a function, its derivative, and a bracketing interval [lower, upper] where the function has opposite signs, finds the root to within x_error tolerance.
template<typename T, typename F, typename DF>
T findRootYuksel( F func, DF deriv, T lower, T upper, T val_lower, T val_upper, T x_error )
{
	auto different_signs = [](T x, T y) { return (x < 0) != (y < 0); };

	if( !std::isfinite(val_lower) || !std::isfinite(val_upper) )
		return std::numeric_limits<T>::quiet_NaN();

	T x = lower + (upper - lower) / T(2);
	T step = (upper - lower) / T(2);

	if( math<T>::abs(step) <= x_error )
		return x;

	while( math<T>::abs(step) > x_error && std::isfinite(x) ) {
		T deriv_x = deriv(x);
		T val_x = func(x);

		if( val_x == T(0) )
			return x;

		bool root_in_first_half = different_signs(val_lower, val_x);
		if( root_in_first_half )
			upper = x;
		else
			lower = x;

		step = -val_x / deriv_x;
		T new_x = x + step;

		// Fall back to bisection if Newton step goes out of bounds
		if( new_x <= lower || new_x >= upper ) {
			new_x = lower + (upper - lower) / T(2);
			if( new_x == upper || new_x == lower )
				return new_x;
		}
		step = new_x - x;
		x = new_x;
	}
	return x;
}

//! Solves a cubic equation ax³ + bx² + cx + d = 0 using Yuksel's robust method with critical point bracketing. Returns the number of real roots found (0-3). Roots are returned in ascending order.
template<typename T>
int solveCubicYuksel( T a, T b, T c, T d, T result[3], T x_error = T(1e-6) )
{
	if( a == T(0) )
		return solveQuadratic( b, c, d, result );

	// Normalize to monic form: x³ + px² + qx + r = 0
	T p = b / a;
	T q = c / a;
	T r = d / a;

	// Find critical points by solving derivative: 3x² + 2px + q = 0
	T crit[2];
	int numCrit = solveQuadratic( T(3), T(2) * p, q, crit );

	auto eval = [=](T x) { return ((a * x + b) * x + c) * x + d; };
	auto deriv_eval = [=](T x) { return (T(3) * a * x + T(2) * b) * x + c; };

	int numRoots = 0;

	if( numCrit == 2 ) {
		// Check three intervals: (-∞, crit[0]), (crit[0], crit[1]), (crit[1], +∞)
		T x0 = crit[0];
		T x1 = crit[1];

		// Use a large but finite bound
		T bound = std::max({math<T>::abs(p), math<T>::abs(q), math<T>::abs(r)}) * T(10) + T(100);

		T intervals[][2] = { {-bound, x0}, {x0, x1}, {x1, bound} };

		for( int i = 0; i < 3; ++i ) {
			T lo = intervals[i][0];
			T hi = intervals[i][1];
			T val_lo = eval(lo);
			T val_hi = eval(hi);

			if( (val_lo < T(0)) != (val_hi < T(0)) ) {
				result[numRoots++] = findRootYuksel(eval, deriv_eval, lo, hi, val_lo, val_hi, x_error);
			}
		}
	} else {
		// Monotonic cubic - at most one root
		T bound = std::max({math<T>::abs(p), math<T>::abs(q), math<T>::abs(r)}) * T(10) + T(100);
		T val_lo = eval(-bound);
		T val_hi = eval(bound);

		if( (val_lo < T(0)) != (val_hi < T(0)) ) {
			result[numRoots++] = findRootYuksel(eval, deriv_eval, -bound, bound, val_lo, val_hi, x_error);
		}
	}

	return numRoots;
}

//! Helper for solveQuartic: factor a monic quartic x⁴ + ax³ + bx² + cx + d = 0 into two quadratics. Returns false if no real factorization exists or overflow occurred.
template<typename T>
bool factorQuarticInner( T a, T b, T c, T d, bool rescale, T &alpha_1, T &beta_1, T &alpha_2, T &beta_2 )
{
	// Helper: relative error metric from paper
	auto eps_rel = [](T raw, T ref) {
		return (ref == T(0)) ? math<T>::abs(raw) : math<T>::abs((raw - ref) / ref);
	};

	auto calc_eps_q = [&](T a1, T b1, T a2, T b2) {
		T eps_a = eps_rel(a1 + a2, a);
		T eps_b = eps_rel(b1 + a1 * a2 + b2, b);
		T eps_c = eps_rel(b1 * a2 + a1 * b2, c);
		return eps_a + eps_b + eps_c;
	};

	auto calc_eps_t = [&](T a1, T b1, T a2, T b2) {
		return calc_eps_q(a1, b1, a2, b2) + eps_rel(b1 * b2, d);
	};

	// Calculate discriminant to find parameter s
	T disc = T(9) * a * a - T(24) * b;
	T s = (disc >= T(0)) ? (T(-2) * b / (T(3) * a + std::copysign(math<T>::sqrt(disc), a))) : (T(-0.25) * a);

	// Compute shifted coefficients
	T a_prime = a + T(4) * s;
	T b_prime = b + T(3) * s * (a + T(2) * s);
	T c_prime = c + s * (T(2) * b + s * (T(3) * a + T(4) * s));
	T d_prime = d + s * (c + s * (b + s * (a + s)));

	// Solve depressed cubic for phi with optional rescaling
	T g_prime, h_prime;
	const T K_C = T(3.49e102);
	if( rescale ) {
		T a_prime_s = a_prime / K_C;
		T b_prime_s = b_prime / K_C;
		T c_prime_s = c_prime / K_C;
		T d_prime_s = d_prime / K_C;
		g_prime = a_prime_s * c_prime_s - (T(4) / K_C) * d_prime_s - (T(1) / T(3)) * b_prime_s * b_prime_s;
		h_prime = (a_prime_s * c_prime_s + (T(8) / K_C) * d_prime_s - (T(2) / T(9)) * b_prime_s * b_prime_s) * (T(1) / T(3)) * b_prime_s
		        - c_prime_s * (c_prime_s / K_C) - a_prime_s * a_prime_s * d_prime_s;
	} else {
		g_prime = a_prime * c_prime - T(4) * d_prime - (T(1) / T(3)) * b_prime * b_prime;
		h_prime = (a_prime * c_prime + T(8) * d_prime - (T(2) / T(9)) * b_prime * b_prime) * (T(1) / T(3)) * b_prime
		        - c_prime * c_prime - a_prime * a_prime * d_prime;
	}

	if( !std::isfinite(g_prime) || !std::isfinite(h_prime) )
		return false;

	// Solve depressed cubic x³ + gx + h = 0 for dominant root using overflow-aware algorithm
	T q = (-T(1) / T(3)) * g_prime;
	T r = T(0.5) * h_prime;
	T phi_0;

	// k heuristic for extreme q/r to prevent overflow
	bool use_k = false;
	T k_val = T(0);
	if( math<T>::abs(q) >= T(1e102) || math<T>::abs(r) >= T(1e154) ) {
		use_k = true;
		if( math<T>::abs(q) < math<T>::abs(r) ) {
			k_val = T(1) - q * (q / r) * (q / r);
		} else {
			k_val = std::copysign(T(1), q) * ((r / q) * (r / q) / q - T(1));
		}
	}

	// Special case: r == 0 with scaling active
	if( use_k && r == T(0) ) {
		if( g_prime > T(0) ) {
			phi_0 = T(0);
		} else {
			phi_0 = math<T>::sqrt(-g_prime);
		}
	}
	// Three real roots case
	else if( (use_k && k_val < T(0)) || (!use_k && r * r < q * q * q) ) {
		T t;
		T sqrt_q = math<T>::sqrt(math<T>::abs(q));
		if( use_k ) {
			t = r / q / sqrt_q;
		} else {
			t = r / (sqrt_q * sqrt_q * sqrt_q);
		}
		// Clamp t to [-1, 1] to handle floating point errors
		t = math<T>::clamp(t, T(-1), T(1));
		T cos_val = math<T>::cos(math<T>::acos(math<T>::abs(t)) / T(3));
		phi_0 = std::copysign(T(-2) * sqrt_q * cos_val, t);
	}
	// Cardano formula with overflow-aware computation
	else {
		T a_cbrt;
		if( use_k ) {
			if( math<T>::abs(q) < math<T>::abs(r) ) {
				a_cbrt = -r * (T(1) + math<T>::sqrt(k_val));
			} else {
				a_cbrt = -r - std::copysign(math<T>::sqrt(math<T>::abs(q)) * q * math<T>::sqrt(k_val), r);
			}
		} else {
			a_cbrt = -r - std::copysign(math<T>::sqrt(r * r - q * q * q), r);
		}
		T A = std::cbrt(a_cbrt);
		T B = (A == T(0)) ? T(0) : q / A;
		phi_0 = A + B;
	}

	// Refine phi with 8-step Newton-Raphson
	T phi = phi_0;
	T f = (phi * phi + g_prime) * phi + h_prime;
	const T EPS_M = std::numeric_limits<T>::epsilon();
	T tolerance = EPS_M * math<T>::max(math<T>::max(phi * phi * phi, g_prime * phi), h_prime);

	if( math<T>::abs(f) >= tolerance ) {
		for( int iter = 0; iter < 8; ++iter ) {
			T delt_f = T(3) * phi * phi + g_prime;
			if( delt_f == T(0) ) break;

			T new_phi = phi - f / delt_f;
			T new_f = (new_phi * new_phi + g_prime) * new_phi + h_prime;

			if( new_f == T(0) || math<T>::abs(new_f) >= math<T>::abs(f) )
				break;

			phi = new_phi;
			f = new_f;
		}
	}

	if( rescale ) phi *= K_C;

	// Calculate factorization parameters
	T l_1 = a * T(0.5);
	T l_3 = (T(1) / T(6)) * b + T(0.5) * phi;
	T delt_2 = c - a * l_3;

	// Try 3 candidate pairs for (d_2, l_2)
	T d_2_cand_1 = (T(2) / T(3)) * b - phi - l_1 * l_1;
	T l_2_cand_1 = T(0.5) * delt_2 / d_2_cand_1;
	T l_2_cand_2 = T(2) * (d - l_3 * l_3) / delt_2;
	T d_2_cand_2 = T(0.5) * delt_2 / l_2_cand_2;
	T d_2_cand_3 = d_2_cand_1;
	T l_2_cand_3 = l_2_cand_2;

	T d_2_best = T(0), l_2_best = T(0), eps_l_best = T(0);
	for( int i = 0; i < 3; ++i ) {
		T d_2 = (i == 0) ? d_2_cand_1 : (i == 1) ? d_2_cand_2 : d_2_cand_3;
		T l_2 = (i == 0) ? l_2_cand_1 : (i == 1) ? l_2_cand_2 : l_2_cand_3;
		T eps_0 = eps_rel(d_2 + l_1 * l_1 + T(2) * l_3, b);
		T eps_1 = eps_rel(T(2) * (d_2 * l_2 + l_1 * l_3), c);
		T eps_2 = eps_rel(d_2 * l_2 * l_2 + l_3 * l_3, d);
		T eps_l = eps_0 + eps_1 + eps_2;
		if( i == 0 || eps_l < eps_l_best ) {
			d_2_best = d_2;
			l_2_best = l_2;
			eps_l_best = eps_l;
		}
	}

	T d_2 = d_2_best;
	T l_2 = l_2_best;

	// Factor based on d_2 value
	// Type-aware tolerance scaled by coefficient magnitudes (avoids being too loose for double)
	T scale = math<T>::max(math<T>::max(T(1), math<T>::abs(b)), math<T>::max(math<T>::abs(c), math<T>::abs(d)));
	const T d_2_tolerance = std::numeric_limits<T>::epsilon() * scale * T(10);

	if( d_2 < T(0) ) {
		T sq = math<T>::sqrt(-d_2);
		alpha_1 = l_1 + sq;
		beta_1 = l_3 + sq * l_2;
		alpha_2 = l_1 - sq;
		beta_2 = l_3 - sq * l_2;

		// Stabilize betas
		if( math<T>::abs(beta_2) < math<T>::abs(beta_1) )
			beta_2 = d / beta_1;
		else if( math<T>::abs(beta_2) > math<T>::abs(beta_1) )
			beta_1 = d / beta_2;

		// Try alternative alpha candidates if they differ
		if( math<T>::abs(alpha_1) != math<T>::abs(alpha_2) ) {
			T cands[3][2];
			if( math<T>::abs(alpha_1) < math<T>::abs(alpha_2) ) {
				cands[0][0] = a - alpha_2;          cands[0][1] = alpha_2;
				cands[1][0] = (c - beta_1 * alpha_2) / beta_2; cands[1][1] = alpha_2;
				cands[2][0] = (b - beta_2 - beta_1) / alpha_2; cands[2][1] = alpha_2;
			} else {
				cands[0][0] = alpha_1; cands[0][1] = a - alpha_1;
				cands[1][0] = alpha_1; cands[1][1] = (c - alpha_1 * beta_2) / beta_1;
				cands[2][0] = alpha_1; cands[2][1] = (b - beta_2 - beta_1) / alpha_1;
			}

			T eps_q_best = T(0);
			for( int i = 0; i < 3; ++i ) {
				T a1 = cands[i][0];
				T a2 = cands[i][1];
				if( std::isfinite(a1) && std::isfinite(a2) ) {
					T eps_q = calc_eps_q(a1, beta_1, a2, beta_2);
					if( i == 0 || eps_q < eps_q_best ) {
						alpha_1 = a1;
						alpha_2 = a2;
						eps_q_best = eps_q;
					}
				}
			}
		}
	} else if( math<T>::abs(d_2) <= d_2_tolerance ) {
		T d_3 = d - l_3 * l_3;

		// Similar tolerance for d_3 to handle rounding errors
		const T d_3_tolerance = std::numeric_limits<T>::epsilon() * math<T>::abs(d) * T(10);
		if( d_3 > d_3_tolerance ) {
			// No real factorization (d_3 should be <= 0 for real roots)
			return false;
		}

		alpha_1 = l_1;
		beta_1 = l_3 + math<T>::sqrt(math<T>::max(T(0), -d_3));  // Clamp to avoid NaN from slight positive d_3
		alpha_2 = l_1;
		beta_2 = l_3 - math<T>::sqrt(math<T>::max(T(0), -d_3));

		if( math<T>::abs(beta_1) > math<T>::abs(beta_2) )
			beta_2 = d / beta_1;
		else if( math<T>::abs(beta_2) > math<T>::abs(beta_1) )
			beta_1 = d / beta_2;
	} else {
		// d_2 > 0: no real factorization
		return false;
	}

	// Newton-Raphson refinement (8 iterations)
	T eps_t = calc_eps_t(alpha_1, beta_1, alpha_2, beta_2);
	for( int iter = 0; iter < 8; ++iter ) {
		if( eps_t == T(0) )
			break;

		T f_0 = beta_1 * beta_2 - d;
		T f_1 = beta_1 * alpha_2 + alpha_1 * beta_2 - c;
		T f_2 = beta_1 + alpha_1 * alpha_2 + beta_2 - b;
		T f_3 = alpha_1 + alpha_2 - a;
		T c_1 = alpha_1 - alpha_2;
		T det_j = beta_1 * beta_1 - beta_1 * (alpha_2 * c_1 + T(2) * beta_2) + beta_2 * (alpha_1 * c_1 + beta_2);

		if( det_j == T(0) )
			break;

		T inv = T(1) / det_j;
		T c_2 = beta_2 - beta_1;
		T c_3 = beta_1 * alpha_2 - alpha_1 * beta_2;
		T dz_0 = c_1 * f_0 + c_2 * f_1 + c_3 * f_2 - (beta_1 * c_2 + alpha_1 * c_3) * f_3;
		T dz_1 = (alpha_1 * c_1 + c_2) * f_0 - beta_1 * c_1 * f_1 - beta_1 * c_2 * f_2 - beta_1 * c_3 * f_3;
		T dz_2 = -c_1 * f_0 - c_2 * f_1 - c_3 * f_2 + (alpha_2 * c_3 + beta_2 * c_2) * f_3;
		T dz_3 = -(alpha_2 * c_1 + c_2) * f_0 + beta_2 * c_1 * f_1 + beta_2 * c_2 * f_2 + beta_2 * c_3 * f_3;

		T a1 = alpha_1 - inv * dz_0;
		T b1 = beta_1 - inv * dz_1;
		T a2 = alpha_2 - inv * dz_2;
		T b2 = beta_2 - inv * dz_3;
		T new_eps_t = calc_eps_t(a1, b1, a2, b2);

		if( new_eps_t < eps_t ) {
			alpha_1 = a1;
			beta_1 = b1;
			alpha_2 = a2;
			beta_2 = b2;
			eps_t = new_eps_t;
		} else {
			break;
		}
	}

	// Reject only if NaN (reference implementation doesn't use an eps_t threshold)
	if( std::isnan(eps_t) )
		return false;

	return true;
}

//! Solves a quartic equation c0 + c1*x + c2*x² + c3*x³ + c4*x⁴ = 0 using the Orellana & De Michele Algorithm 1010. Returns the number of real roots found (0-4). This is a robust, accurate method that factors the quartic into two quadratics.
template<typename T>
int solveQuartic( T c0, T c1, T c2, T c3, T c4, T result[4] )
{
	// Degenerate cases
	if( c4 == T(0) )
		return solveCubic( c3, c2, c1, c0, result );
	if( c0 == T(0) ) {
		int n = solveCubic( c4, c3, c2, c1, result );
		result[n++] = T(0);
		return n;
	}

	// Normalize to monic form: x⁴ + ax³ + bx² + cx + d = 0
	T a = c3 / c4;
	T b = c2 / c4;
	T c = c1 / c4;
	T d = c0 / c4;

	// Try factorization without rescaling first
	T alpha_1, beta_1, alpha_2, beta_2;
	if( factorQuarticInner(a, b, c, d, false, alpha_1, beta_1, alpha_2, beta_2) ) {
		// Solve the two quadratics
		int numRoots = 0;
		T roots1[2], roots2[2];
		int n1 = solveQuadraticStable( T(1), alpha_1, beta_1, roots1 );
		int n2 = solveQuadraticStable( T(1), alpha_2, beta_2, roots2 );

		for( int i = 0; i < n1; ++i )
			result[numRoots++] = roots1[i];
		for( int i = 0; i < n2; ++i )
			result[numRoots++] = roots2[i];

		if( numRoots > 1 )
			std::sort(result, result + numRoots);

		return numRoots;
	}

	// Try with polynomial rescaling
	const T K_Q = T(7.16e76);
	for( int rescale_outer = 0; rescale_outer < 2; ++rescale_outer ) {
		T a_scaled = a / K_Q;
		T b_scaled = b / (K_Q * K_Q);
		T c_scaled = c / (K_Q * K_Q * K_Q);
		T d_scaled = d / (K_Q * K_Q * K_Q * K_Q);

		if( factorQuarticInner(a_scaled, b_scaled, c_scaled, d_scaled, rescale_outer == 1, alpha_1, beta_1, alpha_2, beta_2) ) {
			// Solve the two quadratics and scale roots back
			int numRoots = 0;
			T roots1[2], roots2[2];
			int n1 = solveQuadraticStable( T(1), alpha_1, beta_1, roots1 );
			int n2 = solveQuadraticStable( T(1), alpha_2, beta_2, roots2 );

			for( int i = 0; i < n1; ++i )
				result[numRoots++] = roots1[i] * K_Q;
			for( int i = 0; i < n2; ++i )
				result[numRoots++] = roots2[i] * K_Q;

			if( numRoots > 1 )
				std::sort(result, result + numRoots);

			return numRoots;
		}
	}

	// Overflow or no real roots
	return 0;
}

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

// ============================================================================
// BEZIER CURVE UTILITIES (2D only)
// These functions operate on 2D Bezier curves represented as glm::tvec2.
// For subdivision functions, dst buffer must be sized appropriately:
//   - subdivideQuadraticBezier: dst[5]
//   - subdivideCubicBezier: dst[7]
// ============================================================================

//! Evaluates a 2D quadratic Bezier curve at parameter t ∈ [0,1] using the formula: (1-t)²P₀ + 2t(1-t)P₁ + t²P₂
template<typename T>
glm::tvec2<T, glm::defaultp> evaluateQuadraticBezier( const glm::tvec2<T, glm::defaultp> controlPoints[3], T t )
{
	T nt = T(1) - t;
	return controlPoints[0] * (nt * nt) + controlPoints[1] * (T(2) * t * nt) + controlPoints[2] * (t * t);
}

//! Evaluates a 2D cubic Bezier curve at parameter t ∈ [0,1] using the formula: (1-t)³P₀ + 3t(1-t)²P₁ + 3t²(1-t)P₂ + t³P₃
template<typename T>
glm::tvec2<T, glm::defaultp> evaluateCubicBezier( const glm::tvec2<T, glm::defaultp> controlPoints[4], T t )
{
	T nt = T(1) - t;
	T w0 = nt * nt * nt;
	T w1 = T(3) * nt * nt * t;
	T w2 = T(3) * nt * t * t;
	T w3 = t * t * t;
	return controlPoints[0] * w0 + controlPoints[1] * w1 + controlPoints[2] * w2 + controlPoints[3] * w3;
}

//! Returns the derivative (tangent vector) of a quadratic Bezier curve at parameter t
template<typename T>
glm::tvec2<T, glm::defaultp> derivativeQuadraticBezier( const glm::tvec2<T, glm::defaultp> controlPoints[3], T t )
{
	T nt = T(1) - t;
	return T(-2) * (nt * controlPoints[0] - (T(1) - T(2) * t) * controlPoints[1] - t * controlPoints[2]);
}

//! Returns the derivative (tangent vector) of a cubic Bezier curve at parameter t
template<typename T>
glm::tvec2<T, glm::defaultp> derivativeCubicBezier( const glm::tvec2<T, glm::defaultp> controlPoints[4], T t )
{
	T nt = T(1) - t;
	T w0 = T(-3) * nt * nt;
	T w1 = T(3) * nt * nt - T(6) * t * nt;
	T w2 = T(-3) * t * t + T(6) * t * nt;
	T w3 = T(3) * t * t;
	return controlPoints[0] * w0 + controlPoints[1] * w1 + controlPoints[2] * w2 + controlPoints[3] * w3;
}

//! Returns the second derivative (acceleration vector) of a quadratic Bezier curve. Note: constant for quadratics (independent of t).
template<typename T>
glm::tvec2<T, glm::defaultp> secondDerivativeQuadraticBezier( const glm::tvec2<T, glm::defaultp> controlPoints[3] )
{
	return T(2) * (controlPoints[2] - T(2) * controlPoints[1] + controlPoints[0]);
}

//! Returns the second derivative (acceleration vector) of a cubic Bezier curve at parameter t
template<typename T>
glm::tvec2<T, glm::defaultp> secondDerivativeCubicBezier( const glm::tvec2<T, glm::defaultp> controlPoints[4], T t )
{
	T nt = T(1) - t;
	glm::tvec2<T, glm::defaultp> d0 = controlPoints[2] - T(2) * controlPoints[1] + controlPoints[0];
	glm::tvec2<T, glm::defaultp> d1 = controlPoints[3] - T(2) * controlPoints[2] + controlPoints[1];
	return T(6) * (nt * d0 + t * d1);
}

//! Returns the curvature κ of a 2D curve given first and second derivatives. κ = (x' × x'') / |x'|³. Returns 0 if velocity is near zero.
template<typename T>
T curvature( const glm::tvec2<T, glm::defaultp> &firstDeriv, const glm::tvec2<T, glm::defaultp> &secondDeriv )
{
	// Cross product in 2D: x' × x'' = x'[0]*x''[1] - x'[1]*x''[0]
	T cross = firstDeriv.x * secondDeriv.y - firstDeriv.y * secondDeriv.x;
	T speedSq = firstDeriv.x * firstDeriv.x + firstDeriv.y * firstDeriv.y;

	// Handle near-zero velocity (cusps/stationary points)
	if( speedSq < std::numeric_limits<T>::epsilon() )
		return T(0);

	T speed = math<T>::sqrt( speedSq );
	return cross / (speed * speedSq);
}

//! Returns the curvature of a quadratic Bezier curve at parameter t
template<typename T>
T curvatureQuadraticBezier( const glm::tvec2<T, glm::defaultp> controlPoints[3], T t )
{
	glm::tvec2<T, glm::defaultp> d1 = derivativeQuadraticBezier( controlPoints, t );
	glm::tvec2<T, glm::defaultp> d2 = secondDerivativeQuadraticBezier( controlPoints );
	return curvature( d1, d2 );
}

//! Returns the curvature of a cubic Bezier curve at parameter t
template<typename T>
T curvatureCubicBezier( const glm::tvec2<T, glm::defaultp> controlPoints[4], T t )
{
	glm::tvec2<T, glm::defaultp> d1 = derivativeCubicBezier( controlPoints, t );
	glm::tvec2<T, glm::defaultp> d2 = secondDerivativeCubicBezier( controlPoints, t );
	return curvature( d1, d2 );
}

//! Subdivides a 2D quadratic Bezier at parameter t into two curves. First curve: dst[0..2], second curve: dst[2..4]. Buffer dst must have space for 5 points.
template<typename T>
void subdivideQuadraticBezier( const glm::tvec2<T, glm::defaultp> src[3], glm::tvec2<T, glm::defaultp> dst[5], T t )
{
	glm::tvec2<T, glm::defaultp> p0 = src[0];
	glm::tvec2<T, glm::defaultp> p1 = src[1];
	glm::tvec2<T, glm::defaultp> p2 = src[2];
	glm::tvec2<T, glm::defaultp> tt(t);

	glm::tvec2<T, glm::defaultp> p01 = lerp( p0, p1, tt );
	glm::tvec2<T, glm::defaultp> p12 = lerp( p1, p2, tt );

	dst[0] = p0;
	dst[1] = p01;
	dst[2] = lerp( p01, p12, tt );
	dst[3] = p12;
	dst[4] = p2;
}

//! Subdivides a 2D cubic Bezier at parameter t into two curves. First curve: dst[0..3], second curve: dst[3..6]. Buffer dst must have space for 7 points.
template<typename T>
void subdivideCubicBezier( const glm::tvec2<T, glm::defaultp> src[4], glm::tvec2<T, glm::defaultp> dst[7], T t )
{
	glm::tvec2<T, glm::defaultp> p0 = src[0];
	glm::tvec2<T, glm::defaultp> p1 = src[1];
	glm::tvec2<T, glm::defaultp> p2 = src[2];
	glm::tvec2<T, glm::defaultp> p3 = src[3];
	glm::tvec2<T, glm::defaultp> tt(t);

	glm::tvec2<T, glm::defaultp> p01 = lerp( p0, p1, tt );
	glm::tvec2<T, glm::defaultp> p12 = lerp( p1, p2, tt );
	glm::tvec2<T, glm::defaultp> p23 = lerp( p2, p3, tt );
	glm::tvec2<T, glm::defaultp> p012 = lerp( p01, p12, tt );
	glm::tvec2<T, glm::defaultp> p123 = lerp( p12, p23, tt );

	dst[0] = p0;
	dst[1] = p01;
	dst[2] = p012;
	dst[3] = lerp( p012, p123, tt );
	dst[4] = p123;
	dst[5] = p23;
	dst[6] = p3;
}

//! Finds the t-values of extrema (for bounding box calculation) of a quadratic Bezier. Returns the number of extrema found (0-2).
template<typename T>
int findQuadraticBezierExtrema( const glm::tvec2<T, glm::defaultp> controlPoints[3], T resultT[2] )
{
	int resultIdx = 0;
	T dx = controlPoints[0].x - T(2) * controlPoints[1].x + controlPoints[2].x;
	if( dx != 0 ) {
		T t = (controlPoints[0].x - controlPoints[1].x) / dx;
		if( t > 0 && t < 1 )
			resultT[resultIdx++] = t;
	}
	T dy = controlPoints[0].y - T(2) * controlPoints[1].y + controlPoints[2].y;
	if( dy != 0 ) {
		T t = (controlPoints[0].y - controlPoints[1].y) / dy;
		if( t > 0 && t < 1 )
			resultT[resultIdx++] = t;
	}
	return resultIdx;
}

//! Finds the t-values of extrema (for bounding box calculation) of a cubic Bezier. Returns the number of extrema found (0-4).
template<typename T>
int findCubicBezierExtrema( const glm::tvec2<T, glm::defaultp> controlPoints[4], T resultT[4] )
{
	// Derivative of cubic Bezier is: 3(1-t)²(P₁-P₀) + 6(1-t)t(P₂-P₁) + 3t²(P₃-P₂)
	// Setting derivative = 0 and simplifying gives a quadratic in each dimension
	T Ax = -controlPoints[0].x + T(3) * controlPoints[1].x - T(3) * controlPoints[2].x + controlPoints[3].x;
	T Bx = T(3) * controlPoints[0].x - T(6) * controlPoints[1].x + T(3) * controlPoints[2].x;
	T Cx = -T(3) * controlPoints[0].x + T(3) * controlPoints[1].x;
	T ax = T(3) * Ax;
	T bx = T(2) * Bx;
	T cx = Cx;

	T Ay = -controlPoints[0].y + T(3) * controlPoints[1].y - T(3) * controlPoints[2].y + controlPoints[3].y;
	T By = T(3) * controlPoints[0].y - T(6) * controlPoints[1].y + T(3) * controlPoints[2].y;
	T Cy = -T(3) * controlPoints[0].y + T(3) * controlPoints[1].y;
	T ay = T(3) * Ay;
	T by = T(2) * By;
	T cy = Cy;

	int resultIdx = 0;
	T r1[2], r2[2];
	int o1 = solveQuadratic( ax, bx, cx, r1 );
	int o2 = solveQuadratic( ay, by, cy, r2 );

	if( o1 > 0 && r1[0] > 0 && r1[0] < 1 ) resultT[resultIdx++] = r1[0];
	if( o1 > 1 && r1[1] > 0 && r1[1] < 1 ) resultT[resultIdx++] = r1[1];
	if( o2 > 0 && r2[0] > 0 && r2[0] < 1 ) resultT[resultIdx++] = r2[0];
	if( o2 > 1 && r2[1] > 0 && r2[1] < 1 ) resultT[resultIdx++] = r2[1];

	return resultIdx;
}

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
