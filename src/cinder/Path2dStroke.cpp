/*
 Copyright (c) 2024, The Cinder Project, All rights reserved.

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

// Path2dStroke.cpp - Consolidated path offset and stroke expansion
// Based on algorithms from Kurbo (https://github.com/linebender/kurbo)

#include "cinder/Path2dStroke.h"
#include "cinder/CinderGlm.h"
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <optional>
#include <limits>

namespace cinder {

namespace {

//=============================================================================
// Constants
//=============================================================================

constexpr double PI = 3.14159265358979323846;

// Offset algorithm constants
constexpr size_t N_LSE = 8;          // Number of points for least-squares fit
constexpr double BLEND = 1e-3;       // Transverse error blend proportion
constexpr size_t MAX_DEPTH = 8;      // Maximum recursion depth
constexpr double CUSP_EPSILON = 1e-12;
constexpr double ITP_EPS = 1e-12;
constexpr double TAN_DIST_EPSILON = 1e-12;
constexpr double UTAN_EPSILON = 1e-12;
constexpr double SUBDIVIDE_THRESH = 0.1;
constexpr double DIM_TUNE = 0.25;
constexpr double DASH_ACCURACY = 1e-6;

// Gauss-Legendre 5-point quadrature coefficients
constexpr double GAUSS_LEGENDRE_5_WEIGHTS[] = {
	0.5688888888888889,
	0.4786286704993665,
	0.4786286704993665,
	0.2369268850561891,
	0.2369268850561891
};

constexpr double GAUSS_LEGENDRE_5_NODES[] = {
	0.0,
	-0.5384693101056831,
	0.5384693101056831,
	-0.9061798459386640,
	0.9061798459386640
};

//=============================================================================
// Vector Utilities
//=============================================================================

template<typename T>
inline T cross2d( const glm::tvec2<T>& a, const glm::tvec2<T>& b )
{
	return a.x * b.y - a.y * b.x;
}

template<typename T>
inline glm::tvec2<T> perp( const glm::tvec2<T>& v )
{
	return glm::tvec2<T>( -v.y, v.x );
}

//=============================================================================
// Polynomial Solvers
//=============================================================================

template<typename T>
int solveQuadraticStable( T c0, T c1, T c2, T result[2] )
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

	T q = T( -0.5 ) * ( c1 + std::copysign( std::sqrt( disc ), c1 ) );
	result[0] = q / c2;
	result[1] = c0 / q;

	if( result[0] > result[1] ) {
		std::swap( result[0], result[1] );
	}

	return 2;
}

// Blinn's method for cubic root solving
template<typename T>
int solveCubicStable( T c0, T c1, T c2, T c3, T result[3] )
{
	constexpr T ONETHIRD = T( 1.0 / 3.0 );

	T c3_recip = T( 1.0 ) / c3;
	T scaled_c2 = c2 * ( ONETHIRD * c3_recip );
	T scaled_c1 = c1 * ( ONETHIRD * c3_recip );
	T scaled_c0 = c0 * c3_recip;

	if( !std::isfinite( scaled_c0 ) || !std::isfinite( scaled_c1 ) || !std::isfinite( scaled_c2 ) ) {
		return solveQuadraticStable( c0, c1, c2, result );
	}

	T d0 = -scaled_c2 * scaled_c2 + scaled_c1;
	T d1 = -scaled_c1 * scaled_c2 + scaled_c0;
	T d2 = scaled_c2 * scaled_c0 - scaled_c1 * scaled_c1;

	T d = T( 4.0 ) * d0 * d2 - d1 * d1;
	T de = T( -2.0 ) * scaled_c2 * d0 + d1;

	if( d < 0 ) {
		T sq = std::sqrt( T( -0.25 ) * d );
		T r = T( -0.5 ) * de;
		T t1 = std::cbrt( r + sq ) + std::cbrt( r - sq );
		result[0] = t1 - scaled_c2;
		return 1;
	}
	else if( d == 0 ) {
		T t1 = std::copysign( std::sqrt( -d0 ), de );
		result[0] = t1 - scaled_c2;
		result[1] = T( -2.0 ) * t1 - scaled_c2;
		if( result[0] > result[1] ) std::swap( result[0], result[1] );
		return 2;
	}
	else {
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

// ITP root-finding method
template<typename T, typename F>
T solveItp( F&& f, T a, T b, T epsilon, int n0, T k1, T ya, T yb )
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
// Bezier Evaluation Utilities
//=============================================================================

inline glm::dvec2 evalCubic( const glm::dvec2 p[4], double t )
{
	double t2 = t * t;
	double t3 = t2 * t;
	double mt = 1.0 - t;
	double mt2 = mt * mt;
	double mt3 = mt2 * mt;
	return mt3 * p[0] + 3.0 * mt2 * t * p[1] + 3.0 * mt * t2 * p[2] + t3 * p[3];
}

inline glm::dvec2 evalCubicDeriv( const glm::dvec2 p[4], double t )
{
	glm::dvec2 q0 = 3.0 * ( p[1] - p[0] );
	glm::dvec2 q1 = 3.0 * ( p[2] - p[1] );
	glm::dvec2 q2 = 3.0 * ( p[3] - p[2] );
	double mt = 1.0 - t;
	return mt * mt * q0 + 2.0 * mt * t * q1 + t * t * q2;
}

inline glm::dvec2 evalQuad( const glm::dvec2 p[3], double t )
{
	double mt = 1.0 - t;
	return mt * mt * p[0] + 2.0 * mt * t * p[1] + t * t * p[2];
}

inline void raiseQuadToCubic( const glm::dvec2 q[3], glm::dvec2 c[4] )
{
	c[0] = q[0];
	c[1] = q[0] + ( 2.0 / 3.0 ) * ( q[1] - q[0] );
	c[2] = q[2] + ( 2.0 / 3.0 ) * ( q[1] - q[2] );
	c[3] = q[2];
}

inline void cubicSubsegment( const glm::dvec2 p[4], double t0, double t1, glm::dvec2 result[4] )
{
	glm::dvec2 a0 = p[0] + t0 * ( p[1] - p[0] );
	glm::dvec2 a1 = p[1] + t0 * ( p[2] - p[1] );
	glm::dvec2 a2 = p[2] + t0 * ( p[3] - p[2] );
	glm::dvec2 b0 = a0 + t0 * ( a1 - a0 );
	glm::dvec2 b1 = a1 + t0 * ( a2 - a1 );
	glm::dvec2 c0 = b0 + t0 * ( b1 - b0 );

	double s = ( t1 - t0 ) / ( 1.0 - t0 );

	glm::dvec2 d0 = c0 + s * ( b1 - c0 );
	glm::dvec2 d1 = b1 + s * ( a2 - b1 );
	glm::dvec2 d2 = a2 + s * ( p[3] - a2 );
	glm::dvec2 e0 = d0 + s * ( d1 - d0 );
	glm::dvec2 e1 = d1 + s * ( d2 - d1 );
	glm::dvec2 f0 = e0 + s * ( e1 - e0 );

	result[0] = c0;
	result[1] = d0;
	result[2] = e0;
	result[3] = f0;
}

inline void cubicSubsegmentLeft( const glm::dvec2 p[4], double t, glm::dvec2 result[4] )
{
	glm::dvec2 a0 = p[0] + t * ( p[1] - p[0] );
	glm::dvec2 a1 = p[1] + t * ( p[2] - p[1] );
	glm::dvec2 a2 = p[2] + t * ( p[3] - p[2] );
	glm::dvec2 b0 = a0 + t * ( a1 - a0 );
	glm::dvec2 b1 = a1 + t * ( a2 - a1 );
	glm::dvec2 c0 = b0 + t * ( b1 - b0 );

	result[0] = p[0];
	result[1] = a0;
	result[2] = b0;
	result[3] = c0;
}

inline void cubicSubsegmentRight( const glm::dvec2 p[4], double t, glm::dvec2 result[4] )
{
	glm::dvec2 a0 = p[0] + t * ( p[1] - p[0] );
	glm::dvec2 a1 = p[1] + t * ( p[2] - p[1] );
	glm::dvec2 a2 = p[2] + t * ( p[3] - p[2] );
	glm::dvec2 b0 = a0 + t * ( a1 - a0 );
	glm::dvec2 b1 = a1 + t * ( a2 - a1 );
	glm::dvec2 c0 = b0 + t * ( b1 - b0 );

	result[0] = c0;
	result[1] = b1;
	result[2] = a2;
	result[3] = p[3];
}

inline void quadSubsegmentLeft( const glm::dvec2 q[3], double t, glm::dvec2 result[3] )
{
	glm::dvec2 a0 = q[0] + t * ( q[1] - q[0] );
	glm::dvec2 a1 = q[1] + t * ( q[2] - q[1] );
	glm::dvec2 b0 = a0 + t * ( a1 - a0 );

	result[0] = q[0];
	result[1] = a0;
	result[2] = b0;
}

inline void quadSubsegmentRight( const glm::dvec2 q[3], double t, glm::dvec2 result[3] )
{
	glm::dvec2 a0 = q[0] + t * ( q[1] - q[0] );
	glm::dvec2 a1 = q[1] + t * ( q[2] - q[1] );
	glm::dvec2 b0 = a0 + t * ( a1 - a0 );

	result[0] = b0;
	result[1] = a1;
	result[2] = q[2];
}

//=============================================================================
// Nearest Point on Quadratic (Analytical)
//=============================================================================

struct NearestResult {
	double t;
	double distanceSq;
};

inline NearestResult quadNearest( const glm::dvec2 q[3], const glm::dvec2& p )
{
	glm::dvec2 d0 = q[1] - q[0];
	glm::dvec2 d1 = q[0] + q[2] - 2.0 * q[1];
	glm::dvec2 d = q[0] - p;

	double c0 = glm::dot( d, d0 );
	double c1 = 2.0 * glm::dot( d0, d0 ) + glm::dot( d, d1 );
	double c2 = 3.0 * glm::dot( d1, d0 );
	double c3 = glm::dot( d1, d1 );

	double roots[3];
	int nRoots = solveCubicStable( c0, c1, c2, c3, roots );

	double bestT = 0.0;
	double bestDistSq = glm::length2( q[0] - p );
	bool needEnds = ( nRoots == 0 );

	for( int i = 0; i < nRoots; ++i ) {
		double t = roots[i];
		if( t >= 0.0 && t <= 1.0 ) {
			double mt = 1.0 - t;
			glm::dvec2 pt = mt * mt * q[0] + 2.0 * mt * t * q[1] + t * t * q[2];
			double distSq = glm::length2( pt - p );
			if( distSq < bestDistSq ) {
				bestDistSq = distSq;
				bestT = t;
			}
		} else {
			needEnds = true;
		}
	}

	if( needEnds ) {
		double distSq0 = glm::length2( q[0] - p );
		if( distSq0 < bestDistSq ) {
			bestDistSq = distSq0;
			bestT = 0.0;
		}

		double distSq1 = glm::length2( q[2] - p );
		if( distSq1 < bestDistSq ) {
			bestDistSq = distSq1;
			bestT = 1.0;
		}
	}

	return NearestResult{ bestT, bestDistSq };
}

//=============================================================================
// Arc Length Utilities
//=============================================================================

inline double cubicArclenGauss( const glm::dvec2 p[4] )
{
	double sum = 0.0;
	for( int i = 0; i < 5; i++ ) {
		double t = 0.5 * ( GAUSS_LEGENDRE_5_NODES[i] + 1.0 );
		sum += GAUSS_LEGENDRE_5_WEIGHTS[i] * glm::length( evalCubicDeriv( p, t ) );
	}
	return sum * 0.5;
}

inline double cubicArclenToT( const glm::dvec2 p[4], double t )
{
	if( t <= 0.0 ) return 0.0;
	if( t >= 1.0 ) return cubicArclenGauss( p );

	double sum = 0.0;
	for( int i = 0; i < 5; i++ ) {
		double u = 0.5 * t * ( GAUSS_LEGENDRE_5_NODES[i] + 1.0 );
		sum += GAUSS_LEGENDRE_5_WEIGHTS[i] * glm::length( evalCubicDeriv( p, u ) );
	}
	return sum * 0.5 * t;
}

inline double lineArclen( const glm::dvec2& p0, const glm::dvec2& p1 )
{
	return glm::length( p1 - p0 );
}

inline double quadArclenGauss( const glm::dvec2 q[3] )
{
	glm::dvec2 d0 = q[1] - q[0];
	glm::dvec2 d1 = q[2] - q[1];

	double sum = 0.0;
	for( int i = 0; i < 5; i++ ) {
		double t = 0.5 * ( GAUSS_LEGENDRE_5_NODES[i] + 1.0 );
		double mt = 1.0 - t;
		glm::dvec2 deriv = 2.0 * ( mt * d0 + t * d1 );
		sum += GAUSS_LEGENDRE_5_WEIGHTS[i] * glm::length( deriv );
	}
	return sum * 0.5;
}

inline double cubicInvArclen( const glm::dvec2 p[4], double targetLen, double accuracy )
{
	if( targetLen <= 0.0 ) return 0.0;

	double totalLen = cubicArclenGauss( p );
	if( targetLen >= totalLen ) return 1.0;

	double t = targetLen / totalLen;

	for( int i = 0; i < 20; ++i ) {
		double currentLen = cubicArclenToT( p, t );
		double error = currentLen - targetLen;
		if( std::abs( error ) < accuracy ) break;

		double speed = glm::length( evalCubicDeriv( p, t ) );
		if( speed < 1e-12 ) break;

		double dt = -error / speed;
		t = std::clamp( t + dt, 0.0, 1.0 );
	}
	return t;
}

inline double quadInvArclen( const glm::dvec2 q[3], double targetLen, double accuracy )
{
	if( targetLen <= 0.0 ) return 0.0;

	double totalLen = quadArclenGauss( q );
	if( targetLen >= totalLen ) return 1.0;

	glm::dvec2 d0 = q[1] - q[0];
	glm::dvec2 d1 = q[2] - q[1];

	double t = targetLen / totalLen;

	for( int i = 0; i < 20; ++i ) {
		double sum = 0.0;
		for( int j = 0; j < 5; j++ ) {
			double u = 0.5 * t * ( GAUSS_LEGENDRE_5_NODES[j] + 1.0 );
			double mu = 1.0 - u;
			glm::dvec2 deriv = 2.0 * ( mu * d0 + u * d1 );
			sum += GAUSS_LEGENDRE_5_WEIGHTS[j] * glm::length( deriv );
		}
		double currentLen = sum * 0.5 * t;
		double error = currentLen - targetLen;
		if( std::abs( error ) < accuracy ) break;

		double mt = 1.0 - t;
		double speed = glm::length( 2.0 * ( mt * d0 + t * d1 ) );
		if( speed < 1e-12 ) break;

		double dt = -error / speed;
		t = std::clamp( t + dt, 0.0, 1.0 );
	}
	return t;
}

//=============================================================================
// Internal Path Element Types
//=============================================================================

enum class PathElType {
	MoveTo,
	LineTo,
	QuadTo,
	CurveTo,
	Close
};

struct PathEl {
	PathElType type;
	glm::dvec2 p1;
	glm::dvec2 p2;
	glm::dvec2 p3;

	static PathEl moveTo( const glm::dvec2& p ) {
		return { PathElType::MoveTo, p, {}, {} };
	}

	static PathEl lineTo( const glm::dvec2& p ) {
		return { PathElType::LineTo, p, {}, {} };
	}

	static PathEl quadTo( const glm::dvec2& ctrl, const glm::dvec2& end ) {
		return { PathElType::QuadTo, ctrl, end, {} };
	}

	static PathEl curveTo( const glm::dvec2& c1, const glm::dvec2& c2, const glm::dvec2& end ) {
		return { PathElType::CurveTo, c1, c2, end };
	}

	static PathEl close() {
		return { PathElType::Close, {}, {}, {} };
	}
};

//=============================================================================
// Internal Path Container (double precision)
//=============================================================================

class BezPathD {
public:
	BezPathD() = default;

	void moveTo( const glm::dvec2& p ) {
		mElements.push_back( PathEl::moveTo( p ) );
	}

	void lineTo( const glm::dvec2& p ) {
		mElements.push_back( PathEl::lineTo( p ) );
	}

	void quadTo( const glm::dvec2& ctrl, const glm::dvec2& end ) {
		mElements.push_back( PathEl::quadTo( ctrl, end ) );
	}

	void curveTo( const glm::dvec2& c1, const glm::dvec2& c2, const glm::dvec2& end ) {
		mElements.push_back( PathEl::curveTo( c1, c2, end ) );
	}

	void closePath() {
		mElements.push_back( PathEl::close() );
	}

	void clear() {
		mElements.clear();
	}

	void reserve( size_t n ) {
		mElements.reserve( n );
	}

	bool empty() const { return mElements.empty(); }
	size_t size() const { return mElements.size(); }

	const std::vector<PathEl>& elements() const { return mElements; }
	std::vector<PathEl>& elements() { return mElements; }

	const PathEl& operator[]( size_t i ) const { return mElements[i]; }
	PathEl& operator[]( size_t i ) { return mElements[i]; }

	auto begin() { return mElements.begin(); }
	auto end() { return mElements.end(); }
	auto begin() const { return mElements.begin(); }
	auto end() const { return mElements.end(); }

private:
	std::vector<PathEl> mElements;
};

//=============================================================================
// Bezier Segment Types
//=============================================================================

struct CubicBezD {
	glm::dvec2 p0, p1, p2, p3;

	CubicBezD() = default;
	CubicBezD( const glm::dvec2& p0_, const glm::dvec2& p1_, const glm::dvec2& p2_, const glm::dvec2& p3_ )
		: p0( p0_ ), p1( p1_ ), p2( p2_ ), p3( p3_ ) {}

	const glm::dvec2* data() const { return &p0; }
	glm::dvec2* data() { return &p0; }

	glm::dvec2 eval( double t ) const {
		glm::dvec2 p[4] = { p0, p1, p2, p3 };
		return evalCubic( p, t );
	}

	glm::dvec2 evalDeriv( double t ) const {
		glm::dvec2 p[4] = { p0, p1, p2, p3 };
		return evalCubicDeriv( p, t );
	}

	CubicBezD subsegment( double t0, double t1 ) const {
		glm::dvec2 p[4] = { this->p0, this->p1, this->p2, this->p3 };
		glm::dvec2 result[4];
		cubicSubsegment( p, t0, t1, result );
		return CubicBezD( result[0], result[1], result[2], result[3] );
	}

	double arclen( double accuracy = 1e-6 ) const {
		glm::dvec2 p[4] = { p0, p1, p2, p3 };
		return cubicArclenGauss( p );
	}
};

struct QuadBezD {
	glm::dvec2 p0, p1, p2;

	QuadBezD() = default;
	QuadBezD( const glm::dvec2& p0_, const glm::dvec2& p1_, const glm::dvec2& p2_ )
		: p0( p0_ ), p1( p1_ ), p2( p2_ ) {}

	const glm::dvec2* data() const { return &p0; }
	glm::dvec2* data() { return &p0; }

	glm::dvec2 eval( double t ) const {
		glm::dvec2 p[3] = { p0, p1, p2 };
		return evalQuad( p, t );
	}

	glm::dvec2 evalDeriv( double t ) const {
		double mt = 1.0 - t;
		return 2.0 * ( mt * ( p1 - p0 ) + t * ( p2 - p1 ) );
	}
};

//=============================================================================
// Conversion Utilities
//=============================================================================

inline BezPathD toDoublePrecision( const Path2d& path )
{
	BezPathD result;
	result.reserve( path.getNumSegments() + 1 );

	const auto& points = path.getPoints();
	const auto& segments = path.getSegments();

	if( points.empty() ) {
		return result;
	}

	result.moveTo( glm::dvec2( points[0] ) );

	size_t ptIdx = 1;
	for( size_t i = 0; i < segments.size(); ++i ) {
		switch( segments[i] ) {
			case Path2d::MOVETO:
				result.moveTo( glm::dvec2( points[ptIdx++] ) );
				break;
			case Path2d::LINETO:
				result.lineTo( glm::dvec2( points[ptIdx++] ) );
				break;
			case Path2d::QUADTO:
				result.quadTo(
					glm::dvec2( points[ptIdx] ),
					glm::dvec2( points[ptIdx + 1] )
				);
				ptIdx += 2;
				break;
			case Path2d::CUBICTO:
				result.curveTo(
					glm::dvec2( points[ptIdx] ),
					glm::dvec2( points[ptIdx + 1] ),
					glm::dvec2( points[ptIdx + 2] )
				);
				ptIdx += 3;
				break;
			case Path2d::CLOSE:
				result.closePath();
				break;
		}
	}

	return result;
}

inline Shape2d bezPathToShape2d( const BezPathD& bezPath )
{
	Shape2d result;
	Path2d currentContour;
	bool hasMoveTo = false;

	for( const auto& el : bezPath ) {
		switch( el.type ) {
			case PathElType::MoveTo:
				if( hasMoveTo && !currentContour.empty() ) {
					result.appendContour( currentContour );
					currentContour.clear();
				}
				currentContour.moveTo( glm::vec2( el.p1 ) );
				hasMoveTo = true;
				break;

			case PathElType::LineTo:
				currentContour.lineTo( glm::vec2( el.p1 ) );
				break;

			case PathElType::QuadTo:
				currentContour.quadTo( glm::vec2( el.p1 ), glm::vec2( el.p2 ) );
				break;

			case PathElType::CurveTo:
				currentContour.curveTo( glm::vec2( el.p1 ), glm::vec2( el.p2 ), glm::vec2( el.p3 ) );
				break;

			case PathElType::Close:
				currentContour.close();
				break;
		}
	}

	if( !currentContour.empty() ) {
		result.appendContour( currentContour );
	}

	return result;
}

//=============================================================================
// Offset Algorithm Helpers
//=============================================================================

inline QuadBezD derivAsCurve( const CubicBezD& c )
{
	return QuadBezD(
		3.0 * ( c.p1 - c.p0 ),
		3.0 * ( c.p2 - c.p1 ),
		3.0 * ( c.p3 - c.p2 )
	);
}

inline glm::dvec2 startTangent( const CubicBezD& c )
{
	glm::dvec2 d = c.p1 - c.p0;
	if( glm::length2( d ) < 1e-12 ) {
		d = c.p2 - c.p0;
		if( glm::length2( d ) < 1e-12 ) {
			d = c.p3 - c.p0;
		}
	}
	double len = glm::length( d );
	return ( len > 0.0 ) ? d / len : glm::dvec2( 1.0, 0.0 );
}

inline glm::dvec2 endTangent( const CubicBezD& c )
{
	glm::dvec2 d = c.p3 - c.p2;
	if( glm::length2( d ) < 1e-12 ) {
		d = c.p3 - c.p1;
		if( glm::length2( d ) < 1e-12 ) {
			d = c.p3 - c.p0;
		}
	}
	double len = glm::length( d );
	return ( len > 0.0 ) ? d / len : glm::dvec2( 1.0, 0.0 );
}

enum class CuspType {
	Loop,
	DoubleInflection
};

std::optional<CuspType> detectCusp( const CubicBezD& c, double dimension )
{
	glm::dvec2 d01 = c.p1 - c.p0;
	glm::dvec2 d02 = c.p2 - c.p0;
	glm::dvec2 d03 = c.p3 - c.p0;
	glm::dvec2 d12 = c.p2 - c.p1;
	glm::dvec2 d23 = c.p3 - c.p2;

	double det_012 = cross2d( d01, d02 );
	double det_123 = cross2d( d12, d23 );
	double det_013 = cross2d( d01, d03 );
	double det_023 = cross2d( d02, d03 );

	if( det_012 * det_123 > 0.0 && det_012 * det_013 < 0.0 && det_012 * det_023 < 0.0 ) {
		QuadBezD q = derivAsCurve( c );

		glm::dvec2 qpts[3] = { q.p0, q.p1, q.p2 };
		auto [nearestT, nearestDistSq] = quadNearest( qpts, glm::dvec2( 0.0, 0.0 ) );

		glm::dvec2 d = q.eval( nearestT );
		glm::dvec2 d2 = q.evalDeriv( nearestT );
		double crossVal = cross2d( d, d2 );

		double lhs = nearestDistSq * nearestDistSq * nearestDistSq;
		double rhs = ( crossVal * dimension ) * ( crossVal * dimension );

		if( lhs <= rhs ) {
			double a = 3.0 * det_012 + det_023 - 2.0 * det_013;
			double b = -3.0 * det_012 + det_013;
			double c_coef = det_012;
			double disc = b * b - 4.0 * a * c_coef;

			if( disc > 0.0 ) {
				return CuspType::DoubleInflection;
			} else {
				return CuspType::Loop;
			}
		}
	}

	return std::nullopt;
}

CubicBezD regularizeCusp( const CubicBezD& c, double dimension )
{
	CubicBezD result = c;

	auto cuspType = detectCusp( c, dimension );
	if( cuspType.has_value() ) {
		glm::dvec2 d01 = result.p1 - result.p0;
		double d01h = glm::length( d01 );
		glm::dvec2 d23 = result.p3 - result.p2;
		double d23h = glm::length( d23 );

		switch( *cuspType ) {
			case CuspType::Loop:
				if( d01h > 0.0 ) {
					result.p1 = result.p1 + ( dimension / d01h ) * d01;
				}
				if( d23h > 0.0 ) {
					result.p2 = result.p2 - ( dimension / d23h ) * d23;
				}
				break;
			case CuspType::DoubleInflection:
				if( d01h > 2.0 * dimension ) {
					result.p1 = result.p1 - ( dimension / d01h ) * d01;
				}
				if( d23h > 2.0 * dimension ) {
					result.p2 = result.p2 + ( dimension / d23h ) * d23;
				}
				break;
		}
	}

	return result;
}

std::array<double, 2> inflections( const CubicBezD& c, int& numInflections )
{
	std::array<double, 2> result = { 0.0, 0.0 };
	numInflections = 0;

	glm::dvec2 a = c.p1 - c.p0;
	glm::dvec2 b = ( c.p2 - c.p1 ) - a;
	glm::dvec2 c_coef = ( c.p3 - c.p0 ) - 3.0 * ( c.p2 - c.p1 );

	double c0 = cross2d( a, b );
	double c1 = cross2d( a, c_coef );
	double c2 = cross2d( b, c_coef );

	double roots[2];
	int nRoots = solveQuadraticStable( c0, c1, c2, roots );

	for( int i = 0; i < nRoots; ++i ) {
		if( roots[i] >= 0.0 && roots[i] <= 1.0 ) {
			result[numInflections++] = roots[i];
		}
	}

	return result;
}

//=============================================================================
// Offset Algorithm Structures
//=============================================================================

struct OffsetRec {
	double t0;
	double t1;
	glm::dvec2 utan0;
	glm::dvec2 utan1;
	double cusp0;
	double cusp1;
	size_t depth;

	OffsetRec( double t0_, double t1_, const glm::dvec2& utan0_, const glm::dvec2& utan1_,
			   double cusp0_, double cusp1_, size_t depth_ )
		: t0( t0_ ), t1( t1_ ), utan0( utan0_ ), utan1( utan1_ )
		, cusp0( cusp0_ ), cusp1( cusp1_ ), depth( depth_ )
	{}
};

struct ErrEval {
	double errSquared;
	std::array<glm::dvec2, N_LSE> unorms;
	std::array<glm::dvec2, N_LSE> errVecs;
};

struct SubdivisionPoint {
	double t;
	glm::dvec2 utan;
};

//=============================================================================
// CubicOffset Class
//=============================================================================

class CubicOffset {
public:
	CubicOffset( const CubicBezD& c, double d, double tolerance );

	void offsetRec( const OffsetRec& rec, BezPathD& result );

	const CubicBezD& getCubic() const { return mC; }
	const QuadBezD& getDeriv() const { return mQ; }
	double getC0() const { return mC0; }
	double getC1() const { return mC1; }
	double getC2() const { return mC2; }

	double endpointCusp( const glm::dvec2& tan, double y ) const;

private:
	double cuspSign( double t ) const;

	void subdivide( const OffsetRec& rec, BezPathD& result,
					double t, const glm::dvec2& utanT,
					double cuspTMinus, double cuspTPlus );

	CubicBezD apply( const OffsetRec& rec, double a, double b ) const;
	std::pair<double, double> drawArc( const OffsetRec& rec ) const;
	ErrEval evalErr( const OffsetRec& rec, const CubicBezD& cApprox,
					 std::array<double, N_LSE>& ts ) const;
	std::pair<double, double> refineLeastSquares( const OffsetRec& rec,
												   double a, double b,
												   const ErrEval& err ) const;
	SubdivisionPoint findSubdivisionPoint( const OffsetRec& rec ) const;
	std::optional<SubdivisionPoint> subdivideForTangent( const glm::dvec2& utan0,
														  double t0, double t1,
														  const glm::dvec2& tan,
														  bool force ) const;

	CubicBezD mC;
	QuadBezD mQ;
	double mD;
	double mC0, mC1, mC2;
	double mTolerance;
};

CubicOffset::CubicOffset( const CubicBezD& c, double d, double tolerance )
	: mC( c ), mD( d ), mTolerance( tolerance )
{
	mQ = derivAsCurve( c );

	double d2 = 2.0 * d;
	double p1xp0 = cross2d( mQ.p1, mQ.p0 );
	double p2xp0 = cross2d( mQ.p2, mQ.p0 );
	double p2xp1 = cross2d( mQ.p2, mQ.p1 );

	mC0 = d2 * p1xp0;
	mC1 = d2 * ( p2xp0 - 2.0 * p1xp0 );
	mC2 = d2 * ( p2xp1 - p2xp0 + p1xp0 );
}

double CubicOffset::cuspSign( double t ) const
{
	glm::dvec2 ds = mQ.eval( t );
	double ds2 = glm::length2( ds );
	return ( ( mC2 * t + mC1 ) * t + mC0 ) / ( ds2 * std::sqrt( ds2 ) ) + 1.0;
}

double CubicOffset::endpointCusp( const glm::dvec2& tan, double y ) const
{
	double tanDist = std::max( glm::length( tan ), TAN_DIST_EPSILON );
	double rsqrt = 1.0 / tanDist;
	return y * ( rsqrt * rsqrt * rsqrt ) + 1.0;
}

void CubicOffset::subdivide( const OffsetRec& rec, BezPathD& result,
							 double t, const glm::dvec2& utanT,
							 double cuspTMinus, double cuspTPlus )
{
	OffsetRec rec0( rec.t0, t, rec.utan0, utanT, rec.cusp0, cuspTMinus, rec.depth + 1 );
	offsetRec( rec0, result );

	OffsetRec rec1( t, rec.t1, utanT, rec.utan1, cuspTPlus, rec.cusp1, rec.depth + 1 );
	offsetRec( rec1, result );
}

CubicBezD CubicOffset::apply( const OffsetRec& rec, double a, double b ) const
{
	double s = ( 1.0 / 3.0 ) * ( rec.t1 - rec.t0 );

	glm::dvec2 p0 = mC.eval( rec.t0 ) + mD * perp( rec.utan0 );
	double l0 = s * glm::length( mQ.eval( rec.t0 ) ) + a * mD;
	glm::dvec2 p1 = p0;
	if( l0 * rec.cusp0 > 0.0 ) {
		p1 = p1 + l0 * rec.utan0;
	}

	glm::dvec2 p3 = mC.eval( rec.t1 ) + mD * perp( rec.utan1 );
	double l1 = s * glm::length( mQ.eval( rec.t1 ) ) - b * mD;
	glm::dvec2 p2 = p3;
	if( l1 * rec.cusp1 > 0.0 ) {
		p2 = p2 - l1 * rec.utan1;
	}

	return CubicBezD( p0, p1, p2, p3 );
}

std::pair<double, double> CubicOffset::drawArc( const OffsetRec& rec ) const
{
	double th = std::atan2( cross2d( rec.utan1, rec.utan0 ), glm::dot( rec.utan1, rec.utan0 ) );
	double a = ( 2.0 / 3.0 ) / ( 1.0 + std::cos( 0.5 * th ) ) * 2.0 * std::sin( 0.5 * th );
	return { a, -a };
}

ErrEval CubicOffset::evalErr( const OffsetRec& rec, const CubicBezD& cApprox,
							  std::array<double, N_LSE>& ts ) const
{
	QuadBezD qa = derivAsCurve( cApprox );

	ErrEval result;
	result.errSquared = 0.0;

	for( size_t i = 0; i < N_LSE; ++i ) {
		double ta = static_cast<double>( i + 1 ) / static_cast<double>( N_LSE + 1 );
		double t = ts[i];

		glm::dvec2 p = mC.eval( t );

		glm::dvec2 pa = cApprox.eval( ta );
		glm::dvec2 tana = qa.eval( ta );
		double denom = glm::dot( tana, mQ.eval( t ) );
		if( std::abs( denom ) > 1e-12 ) {
			t += glm::dot( tana, pa - p ) / denom;
		}
		t = std::clamp( t, rec.t0, rec.t1 );
		ts[i] = t;

		double cusp = ( rec.cusp0 >= 0.0 ) ? 1.0 : -1.0;
		glm::dvec2 unorm = cusp * perp( glm::normalize( tana ) );
		result.unorms[i] = unorm;

		glm::dvec2 pNew = mC.eval( t ) + mD * unorm;
		glm::dvec2 errVec = pa - pNew;
		result.errVecs[i] = errVec;

		double distErrSquared = glm::length2( errVec );
		if( !std::isfinite( distErrSquared ) ) {
			distErrSquared = 1e12;
		}
		result.errSquared = std::max( distErrSquared, result.errSquared );
	}

	return result;
}

std::pair<double, double> CubicOffset::refineLeastSquares( const OffsetRec& rec,
														   double a, double b,
														   const ErrEval& err ) const
{
	double aWeights[N_LSE], bWeights[N_LSE];
	for( size_t i = 0; i < N_LSE; ++i ) {
		double t = static_cast<double>( i + 1 ) / static_cast<double>( N_LSE + 1 );
		double mt = 1.0 - t;
		aWeights[i] = 3.0 * mt * t * mt;
		bWeights[N_LSE - 1 - i] = 3.0 * mt * t * mt;
	}

	double aa = 0.0, ab = 0.0, ac = 0.0, bb = 0.0, bc = 0.0;

	for( size_t i = 0; i < N_LSE; ++i ) {
		glm::dvec2 n = err.unorms[i];
		glm::dvec2 errVec = err.errVecs[i];

		double c_n = glm::dot( errVec, n );
		double c_t = cross2d( errVec, n );
		double a_n = aWeights[i] * glm::dot( rec.utan0, n );
		double a_t = aWeights[i] * cross2d( rec.utan0, n );
		double b_n = bWeights[i] * glm::dot( rec.utan1, n );
		double b_t = bWeights[i] * cross2d( rec.utan1, n );

		aa += a_n * a_n + BLEND * ( a_t * a_t );
		ab += a_n * b_n + BLEND * a_t * b_t;
		ac += a_n * c_n + BLEND * a_t * c_t;
		bb += b_n * b_n + BLEND * ( b_t * b_t );
		bc += b_n * c_n + BLEND * b_t * c_t;
	}

	double det = aa * bb - ab * ab;
	if( std::abs( det ) < 1e-12 ) {
		return { a, b };
	}

	double idet = 1.0 / ( mD * det );
	double deltaA = idet * ( ac * bb - ab * bc );
	double deltaB = idet * ( aa * bc - ac * ab );

	return { a - deltaA, b - deltaB };
}

SubdivisionPoint CubicOffset::findSubdivisionPoint( const OffsetRec& rec ) const
{
	double t = 0.5 * ( rec.t0 + rec.t1 );
	glm::dvec2 qT = mQ.eval( t );

	double x0 = std::abs( cross2d( rec.utan0, qT ) );
	double x1 = std::abs( cross2d( rec.utan1, qT ) );

	if( x0 > SUBDIVIDE_THRESH * x1 && x1 > SUBDIVIDE_THRESH * x0 ) {
		glm::dvec2 utan = glm::normalize( qT );
		return SubdivisionPoint{ t, utan };
	}

	glm::dvec2 chord = mC.eval( rec.t1 ) - mC.eval( rec.t0 );
	if( cross2d( chord, rec.utan0 ) * cross2d( chord, rec.utan1 ) < 0.0 ) {
		glm::dvec2 tan = rec.utan0 + rec.utan1;
		auto subdivision = subdivideForTangent( rec.utan0, rec.t0, rec.t1, tan, false );
		if( subdivision.has_value() ) {
			return *subdivision;
		}
	}

	int numInflect;
	auto inflectTs = inflections( mC, numInflect );

	std::vector<glm::dvec2> tangents;
	std::vector<double> ts;

	tangents.push_back( rec.utan0 );
	ts.push_back( rec.t0 );

	for( int i = 0; i < numInflect; ++i ) {
		double inflectT = inflectTs[i];
		if( inflectT > rec.t0 && inflectT < rec.t1 ) {
			tangents.push_back( mQ.eval( inflectT ) );
			ts.push_back( inflectT );
		}
	}

	tangents.push_back( rec.utan1 );
	ts.push_back( rec.t1 );

	std::vector<double> arcAngles;
	double sum = 0.0;

	for( size_t i = 0; i < tangents.size() - 1; ++i ) {
		glm::dvec2 tan0 = tangents[i];
		glm::dvec2 tan1 = tangents[i + 1];
		double th = std::atan2( cross2d( tan0, tan1 ), glm::dot( tan0, tan1 ) );
		sum += std::abs( th );
		arcAngles.push_back( th );
	}

	double target = sum * 0.5;
	size_t idx = 0;
	while( idx < arcAngles.size() && std::abs( arcAngles[idx] ) < target ) {
		target -= std::abs( arcAngles[idx] );
		++idx;
	}

	if( idx >= arcAngles.size() ) {
		idx = arcAngles.size() - 1;
	}

	double rotAngle = std::copysign( target, arcAngles[idx] );
	glm::dvec2 base = tangents[idx];
	glm::dvec2 tan = glm::dvec2(
		base.x * std::cos( rotAngle ) - base.y * std::sin( rotAngle ),
		base.x * std::sin( rotAngle ) + base.y * std::cos( rotAngle )
	);

	glm::dvec2 utan0 = ( idx == 0 ) ? rec.utan0 : glm::normalize( base );
	auto result = subdivideForTangent( utan0, ts[idx], ts[idx + 1], tan, true );

	return result.value_or( SubdivisionPoint{ t, glm::normalize( qT ) } );
}

std::optional<SubdivisionPoint> CubicOffset::subdivideForTangent( const glm::dvec2& utan0,
																   double t0, double t1,
																   const glm::dvec2& tan,
																   bool force ) const
{
	double t = 0.0;
	int nSoln = 0;

	double z0 = cross2d( tan, mQ.p0 );
	double z1 = cross2d( tan, mQ.p1 );
	double z2 = cross2d( tan, mQ.p2 );

	double c0 = z0;
	double c1 = 2.0 * ( z1 - z0 );
	double c2 = ( z2 - z1 ) - ( z1 - z0 );

	double roots[2];
	int nRoots = solveQuadraticStable( c0, c1, c2, roots );

	for( int i = 0; i < nRoots; ++i ) {
		if( roots[i] >= t0 && roots[i] <= t1 ) {
			t = roots[i];
			++nSoln;
		}
	}

	if( nSoln != 1 ) {
		if( !force ) {
			return std::nullopt;
		}
		if( glm::length2( mQ.eval( t0 ) ) > glm::length2( mQ.eval( t1 ) ) ) {
			t = t1;
		} else {
			t = t0;
		}
	}

	glm::dvec2 q = mQ.eval( t );
	glm::dvec2 utan;

	if( nSoln == 1 && glm::length2( q ) >= UTAN_EPSILON ) {
		utan = glm::normalize( q );
	} else if( glm::length2( tan ) >= UTAN_EPSILON ) {
		utan = glm::normalize( tan );
	} else {
		utan = perp( utan0 );
	}

	return SubdivisionPoint{ t, utan };
}

void CubicOffset::offsetRec( const OffsetRec& rec, BezPathD& result )
{
	if( rec.cusp0 * rec.cusp1 < 0.0 ) {
		double a = rec.t0;
		double b = rec.t1;
		double s = ( rec.cusp1 > 0.0 ) ? 1.0 : -1.0;

		auto f = [this, s]( double t ) { return s * cuspSign( t ); };
		double k1 = 0.2 / ( b - a );
		double t = solveItp( f, a, b, ITP_EPS, 1, k1, s * rec.cusp0, s * rec.cusp1 );

		glm::dvec2 utanT = glm::normalize( mQ.eval( t ) );
		double cuspTMinus = std::copysign( CUSP_EPSILON, rec.cusp0 );
		double cuspTPlus = std::copysign( CUSP_EPSILON, rec.cusp1 );

		subdivide( rec, result, t, utanT, cuspTMinus, cuspTPlus );
		return;
	}

	auto [a, b] = drawArc( rec );
	double dt = ( rec.t1 - rec.t0 ) / static_cast<double>( N_LSE + 1 );

	std::array<double, N_LSE> ts;
	for( size_t i = 0; i < N_LSE; ++i ) {
		ts[i] = rec.t0 + static_cast<double>( i + 1 ) * dt;
	}

	CubicBezD cApprox = apply( rec, a, b );
	ErrEval err = evalErr( rec, cApprox, ts );

	constexpr int N_REFINE = 2;
	for( int i = 0; i < N_REFINE; ++i ) {
		if( err.errSquared <= mTolerance * mTolerance ) {
			break;
		}

		auto [a2, b2] = refineLeastSquares( rec, a, b, err );
		CubicBezD cApprox2 = apply( rec, a2, b2 );
		ErrEval err2 = evalErr( rec, cApprox2, ts );

		if( err2.errSquared >= err.errSquared ) {
			break;
		}

		err = err2;
		a = a2;
		b = b2;
		cApprox = cApprox2;
	}

	if( rec.depth < MAX_DEPTH && err.errSquared > mTolerance * mTolerance ) {
		SubdivisionPoint sub = findSubdivisionPoint( rec );
		double cusp = cuspSign( sub.t );
		subdivide( rec, result, sub.t, sub.utan, cusp, cusp );
	} else {
		result.curveTo( cApprox.p1, cApprox.p2, cApprox.p3 );
	}
}

void offsetCubic( const CubicBezD& c, double d, double tolerance, BezPathD& result )
{
	result.clear();

	CubicBezD cRegularized = regularizeCusp( c, tolerance * DIM_TUNE );
	CubicOffset co( cRegularized, d, tolerance );

	glm::dvec2 tan0 = startTangent( c );
	glm::dvec2 tan1 = endTangent( c );
	glm::dvec2 utan0 = glm::normalize( tan0 );
	glm::dvec2 utan1 = glm::normalize( tan1 );

	double cusp0 = co.endpointCusp( co.getDeriv().p0, co.getC0() );
	double cusp1 = co.endpointCusp( co.getDeriv().p2, co.getC0() + co.getC1() + co.getC2() );

	result.moveTo( c.p0 + d * perp( utan0 ) );

	OffsetRec rec( 0.0, 1.0, utan0, utan1, cusp0, cusp1, 0 );
	co.offsetRec( rec, result );
}

//=============================================================================
// Stroke Algorithm: Arc and Join Helpers
//=============================================================================

struct ArcD {
	glm::dvec2 center;
	glm::dvec2 radii;
	double startAngle;
	double sweepAngle;
	double rotation;

	ArcD( const glm::dvec2& c, const glm::dvec2& r, double start, double sweep, double rot = 0.0 )
		: center( c ), radii( r ), startAngle( start ), sweepAngle( sweep ), rotation( rot ) {}

	void toCubicBeziers( double tolerance, BezPathD& path ) const {
		double angle = sweepAngle;
		if( std::abs( angle ) < 1e-12 ) return;

		double effectiveRadius = std::max( std::abs( radii.x ), std::abs( radii.y ) );
		effectiveRadius = std::max( effectiveRadius, 1e-9 );

		double maxAngle = PI / 2.0;
		if( tolerance < effectiveRadius ) {
			double toleranceAngle = std::pow( 54.0 * tolerance / effectiveRadius, 0.25 );
			maxAngle = std::min( maxAngle, toleranceAngle );
		}

		int numSegments = static_cast<int>( std::ceil( std::abs( angle ) / maxAngle ) );
		numSegments = std::max( 1, numSegments );
		double segmentAngle = angle / numSegments;

		double cosRot = std::cos( rotation );
		double sinRot = std::sin( rotation );

		double currentAngle = startAngle;
		for( int i = 0; i < numSegments; ++i ) {
			double endAngle = currentAngle + segmentAngle;
			double alpha = ( 4.0 / 3.0 ) * std::tan( segmentAngle / 4.0 );

			double cos0 = std::cos( currentAngle );
			double sin0 = std::sin( currentAngle );
			double cos1 = std::cos( endAngle );
			double sin1 = std::sin( endAngle );

			glm::dvec2 p0( radii.x * cos0, radii.y * sin0 );
			glm::dvec2 p3( radii.x * cos1, radii.y * sin1 );

			glm::dvec2 p1 = p0 + alpha * glm::dvec2( -radii.x * sin0, radii.y * cos0 );
			glm::dvec2 p2 = p3 - alpha * glm::dvec2( -radii.x * sin1, radii.y * cos1 );

			auto transform = [&]( const glm::dvec2& p ) {
				return center + glm::dvec2(
					cosRot * p.x - sinRot * p.y,
					sinRot * p.x + cosRot * p.y
				);
			};

			path.curveTo( transform( p1 ), transform( p2 ), transform( p3 ) );
			currentAngle = endAngle;
		}
	}
};

void roundCap( BezPathD& out, double tolerance, const glm::dvec2& center, const glm::dvec2& norm );
void roundJoin( BezPathD& out, double tolerance, const glm::dvec2& center,
				const glm::dvec2& norm, double angle );
void roundJoinRev( BezPathD& out, double tolerance, const glm::dvec2& center,
				   const glm::dvec2& norm, double angle );

void roundJoin( BezPathD& out, double tolerance, const glm::dvec2& center,
				const glm::dvec2& norm, double angle )
{
	double radius = glm::length( norm );
	if( radius < 1e-12 ) return;

	auto transform = [&]( const glm::dvec2& p ) {
		return glm::dvec2(
			norm.x * p.x - norm.y * p.y + center.x,
			norm.y * p.x + norm.x * p.y + center.y
		);
	};

	double maxAngle = PI / 2.0;
	if( tolerance < radius ) {
		double toleranceAngle = std::pow( 54.0 * tolerance / radius, 0.25 );
		maxAngle = std::min( maxAngle, toleranceAngle );
	}

	int numSegments = static_cast<int>( std::ceil( std::abs( angle ) / maxAngle ) );
	numSegments = std::max( 1, numSegments );
	double segAngle = angle / numSegments;

	double currentAngle = PI - angle;
	for( int i = 0; i < numSegments; ++i ) {
		double endAngle = currentAngle + segAngle;
		double alpha = ( 4.0 / 3.0 ) * std::tan( segAngle / 4.0 );

		double cos0 = std::cos( currentAngle );
		double sin0 = std::sin( currentAngle );
		double cos1 = std::cos( endAngle );
		double sin1 = std::sin( endAngle );

		glm::dvec2 p0( cos0, sin0 );
		glm::dvec2 p3( cos1, sin1 );
		glm::dvec2 p1 = p0 + alpha * glm::dvec2( -sin0, cos0 );
		glm::dvec2 p2 = p3 - alpha * glm::dvec2( -sin1, cos1 );

		out.curveTo( transform( p1 ), transform( p2 ), transform( p3 ) );
		currentAngle = endAngle;
	}
}

void roundJoinRev( BezPathD& out, double tolerance, const glm::dvec2& center,
				   const glm::dvec2& norm, double angle )
{
	double radius = glm::length( norm );
	if( radius < 1e-12 ) return;

	auto transform = [&]( const glm::dvec2& p ) {
		return glm::dvec2(
			norm.x * p.x + norm.y * p.y + center.x,
			norm.y * p.x - norm.x * p.y + center.y
		);
	};

	double maxAngle = PI / 2.0;
	if( tolerance < radius ) {
		double toleranceAngle = std::pow( 54.0 * tolerance / radius, 0.25 );
		maxAngle = std::min( maxAngle, toleranceAngle );
	}

	int numSegments = static_cast<int>( std::ceil( std::abs( angle ) / maxAngle ) );
	numSegments = std::max( 1, numSegments );
	double segAngle = angle / numSegments;

	double currentAngle = PI - angle;
	for( int i = 0; i < numSegments; ++i ) {
		double endAngle = currentAngle + segAngle;
		double alpha = ( 4.0 / 3.0 ) * std::tan( segAngle / 4.0 );

		double cos0 = std::cos( currentAngle );
		double sin0 = std::sin( currentAngle );
		double cos1 = std::cos( endAngle );
		double sin1 = std::sin( endAngle );

		glm::dvec2 p0( cos0, sin0 );
		glm::dvec2 p3( cos1, sin1 );
		glm::dvec2 p1 = p0 + alpha * glm::dvec2( -sin0, cos0 );
		glm::dvec2 p2 = p3 - alpha * glm::dvec2( -sin1, cos1 );

		out.curveTo( transform( p1 ), transform( p2 ), transform( p3 ) );
		currentAngle = endAngle;
	}
}

void roundCap( BezPathD& out, double tolerance, const glm::dvec2& center, const glm::dvec2& norm )
{
	roundJoin( out, tolerance, center, norm, PI );
}

void squareCap( BezPathD& out, bool close, const glm::dvec2& center, const glm::dvec2& norm )
{
	auto transform = [&]( const glm::dvec2& p ) {
		return glm::dvec2(
			norm.x * p.x - norm.y * p.y + center.x,
			norm.y * p.x + norm.x * p.y + center.y
		);
	};

	out.lineTo( transform( glm::dvec2( 1.0, 1.0 ) ) );
	out.lineTo( transform( glm::dvec2( -1.0, 1.0 ) ) );
	if( close ) {
		out.closePath();
	} else {
		out.lineTo( transform( glm::dvec2( -1.0, 0.0 ) ) );
	}
}

void extendReversed( BezPathD& out, const BezPathD& elements )
{
	const auto& els = elements.elements();
	for( size_t i = els.size(); i > 1; --i ) {
		glm::dvec2 end;
		switch( els[i - 2].type ) {
			case PathElType::MoveTo:
			case PathElType::LineTo:
				end = els[i - 2].p1;
				break;
			case PathElType::QuadTo:
				end = els[i - 2].p2;
				break;
			case PathElType::CurveTo:
				end = els[i - 2].p3;
				break;
			default:
				continue;
		}

		switch( els[i - 1].type ) {
			case PathElType::LineTo:
				out.lineTo( end );
				break;
			case PathElType::QuadTo:
				out.quadTo( els[i - 1].p1, end );
				break;
			case PathElType::CurveTo:
				out.curveTo( els[i - 1].p2, els[i - 1].p1, end );
				break;
			default:
				break;
		}
	}
}

//=============================================================================
// Internal Stroke Style
//=============================================================================

enum class InternalJoin {
	Bevel,
	Miter,
	Round
};

enum class InternalCap {
	Butt,
	Square,
	Round
};

struct InternalStrokeStyle {
	double width = 1.0;
	InternalJoin join = InternalJoin::Round;
	double miterLimit = 4.0;
	InternalCap startCap = InternalCap::Round;
	InternalCap endCap = InternalCap::Round;
	std::vector<double> dashPattern;
	double dashOffset = 0.0;

	InternalStrokeStyle() = default;
	explicit InternalStrokeStyle( double w ) : width( w ) {}
};

InternalStrokeStyle convertStrokeStyle( const StrokeStyle& style )
{
	InternalStrokeStyle result;
	result.width = static_cast<double>( style.width );
	result.miterLimit = static_cast<double>( style.miterLimit );
	result.dashOffset = static_cast<double>( style.dashOffset );

	switch( style.join ) {
		case StrokeJoin::Bevel: result.join = InternalJoin::Bevel; break;
		case StrokeJoin::Miter: result.join = InternalJoin::Miter; break;
		case StrokeJoin::Round: result.join = InternalJoin::Round; break;
	}

	switch( style.startCap ) {
		case StrokeCap::Butt: result.startCap = InternalCap::Butt; break;
		case StrokeCap::Square: result.startCap = InternalCap::Square; break;
		case StrokeCap::Round: result.startCap = InternalCap::Round; break;
	}

	switch( style.endCap ) {
		case StrokeCap::Butt: result.endCap = InternalCap::Butt; break;
		case StrokeCap::Square: result.endCap = InternalCap::Square; break;
		case StrokeCap::Round: result.endCap = InternalCap::Round; break;
	}

	result.dashPattern.reserve( style.dashPattern.size() );
	for( float d : style.dashPattern ) {
		result.dashPattern.push_back( static_cast<double>( d ) );
	}

	return result;
}

//=============================================================================
// StrokeContext Class
//=============================================================================

class StrokeContext {
public:
	StrokeContext() = default;

	void reset();
	const BezPathD& output() const { return mOutput; }
	void processElement( const PathEl& el, const InternalStrokeStyle& style, double tolerance );
	void finish( const InternalStrokeStyle& style );
	void finishClosed( const InternalStrokeStyle& style );

	double mJoinThresh = 0.0;

private:
	void doJoin( const InternalStrokeStyle& style, const glm::dvec2& tan0 );
	void doLine( const InternalStrokeStyle& style, const glm::dvec2& tangent, const glm::dvec2& p1 );
	void doCubic( const InternalStrokeStyle& style, const CubicBezD& c, double tolerance );
	void doLinear( const InternalStrokeStyle& style, const CubicBezD& c,
				   double p[4], const glm::dvec2& refPt, const glm::dvec2& refVec );

	BezPathD mOutput;
	BezPathD mForwardPath;
	BezPathD mBackwardPath;
	BezPathD mResultPath;

	glm::dvec2 mStartPt = glm::dvec2( 0.0 );
	glm::dvec2 mStartNorm = glm::dvec2( 0.0 );
	glm::dvec2 mStartTan = glm::dvec2( 0.0 );
	glm::dvec2 mLastPt = glm::dvec2( 0.0 );
	glm::dvec2 mLastTan = glm::dvec2( 0.0 );
};

void StrokeContext::reset()
{
	mOutput.clear();
	mForwardPath.clear();
	mBackwardPath.clear();
	mStartPt = glm::dvec2( 0.0 );
	mStartNorm = glm::dvec2( 0.0 );
	mStartTan = glm::dvec2( 0.0 );
	mLastPt = glm::dvec2( 0.0 );
	mLastTan = glm::dvec2( 0.0 );
	mJoinThresh = 0.0;
}

void StrokeContext::finish( const InternalStrokeStyle& style )
{
	constexpr double tolerance = 1e-3;

	if( mForwardPath.empty() ) return;

	for( const auto& el : mForwardPath ) {
		switch( el.type ) {
			case PathElType::MoveTo: mOutput.moveTo( el.p1 ); break;
			case PathElType::LineTo: mOutput.lineTo( el.p1 ); break;
			case PathElType::QuadTo: mOutput.quadTo( el.p1, el.p2 ); break;
			case PathElType::CurveTo: mOutput.curveTo( el.p1, el.p2, el.p3 ); break;
			case PathElType::Close: mOutput.closePath(); break;
		}
	}

	const auto& backEls = mBackwardPath.elements();
	if( backEls.empty() ) return;

	glm::dvec2 returnP;
	const auto& lastEl = backEls.back();
	switch( lastEl.type ) {
		case PathElType::MoveTo:
		case PathElType::LineTo:
			returnP = lastEl.p1;
			break;
		case PathElType::QuadTo:
			returnP = lastEl.p2;
			break;
		case PathElType::CurveTo:
			returnP = lastEl.p3;
			break;
		default:
			return;
	}

	glm::dvec2 d = mLastPt - returnP;

	switch( style.endCap ) {
		case InternalCap::Butt:
			mOutput.lineTo( returnP );
			break;
		case InternalCap::Round:
			roundCap( mOutput, tolerance, mLastPt, d );
			break;
		case InternalCap::Square:
			squareCap( mOutput, false, mLastPt, d );
			break;
	}

	extendReversed( mOutput, mBackwardPath );

	switch( style.startCap ) {
		case InternalCap::Butt:
			mOutput.closePath();
			break;
		case InternalCap::Round:
			roundCap( mOutput, tolerance, mStartPt, mStartNorm );
			break;
		case InternalCap::Square:
			squareCap( mOutput, true, mStartPt, mStartNorm );
			break;
	}

	mForwardPath.clear();
	mBackwardPath.clear();
}

void StrokeContext::finishClosed( const InternalStrokeStyle& style )
{
	if( mForwardPath.empty() ) return;

	doJoin( style, mStartTan );

	for( const auto& el : mForwardPath ) {
		switch( el.type ) {
			case PathElType::MoveTo: mOutput.moveTo( el.p1 ); break;
			case PathElType::LineTo: mOutput.lineTo( el.p1 ); break;
			case PathElType::QuadTo: mOutput.quadTo( el.p1, el.p2 ); break;
			case PathElType::CurveTo: mOutput.curveTo( el.p1, el.p2, el.p3 ); break;
			case PathElType::Close: mOutput.closePath(); break;
		}
	}
	mOutput.closePath();

	const auto& backEls = mBackwardPath.elements();
	if( backEls.empty() ) {
		mForwardPath.clear();
		mBackwardPath.clear();
		return;
	}

	glm::dvec2 lastPt;
	const auto& lastEl = backEls.back();
	switch( lastEl.type ) {
		case PathElType::MoveTo:
		case PathElType::LineTo:
			lastPt = lastEl.p1;
			break;
		case PathElType::QuadTo:
			lastPt = lastEl.p2;
			break;
		case PathElType::CurveTo:
			lastPt = lastEl.p3;
			break;
		default:
			lastPt = glm::dvec2( 0.0 );
	}

	mOutput.moveTo( lastPt );
	extendReversed( mOutput, mBackwardPath );
	mOutput.closePath();

	mForwardPath.clear();
	mBackwardPath.clear();
}

void StrokeContext::doJoin( const InternalStrokeStyle& style, const glm::dvec2& tan0 )
{
	constexpr double tolerance = 1e-3;

	double scale = 0.5 * style.width / glm::length( tan0 );
	glm::dvec2 norm = scale * glm::dvec2( -tan0.y, tan0.x );
	glm::dvec2 p0 = mLastPt;

	if( mForwardPath.empty() ) {
		mForwardPath.moveTo( p0 - norm );
		mBackwardPath.moveTo( p0 + norm );
		mStartTan = tan0;
		mStartNorm = norm;
	} else {
		glm::dvec2 ab = mLastTan;
		glm::dvec2 cd = tan0;
		double crossVal = cross2d( ab, cd );
		double dotVal = glm::dot( ab, cd );
		double hypot = std::sqrt( crossVal * crossVal + dotVal * dotVal );

		if( dotVal <= 0.0 || std::abs( crossVal ) >= hypot * mJoinThresh ) {
			switch( style.join ) {
				case InternalJoin::Bevel:
					mForwardPath.lineTo( p0 - norm );
					mBackwardPath.lineTo( p0 + norm );
					break;

				case InternalJoin::Miter: {
					if( 2.0 * hypot < ( hypot + dotVal ) * style.miterLimit * style.miterLimit ) {
						double lastScale = 0.5 * style.width / glm::length( ab );
						glm::dvec2 lastNorm = lastScale * glm::dvec2( -ab.y, ab.x );

						if( crossVal > 0.0 ) {
							glm::dvec2 fpLast = p0 - lastNorm;
							glm::dvec2 fpThis = p0 - norm;
							double h = cross2d( ab, fpThis - fpLast ) / crossVal;
							glm::dvec2 miterPt = fpThis - cd * h;
							mForwardPath.lineTo( miterPt );
							mBackwardPath.lineTo( p0 );
						} else if( crossVal < 0.0 ) {
							glm::dvec2 fpLast = p0 + lastNorm;
							glm::dvec2 fpThis = p0 + norm;
							double h = cross2d( ab, fpThis - fpLast ) / crossVal;
							glm::dvec2 miterPt = fpThis - cd * h;
							mBackwardPath.lineTo( miterPt );
							mForwardPath.lineTo( p0 );
						}
					}
					mForwardPath.lineTo( p0 - norm );
					mBackwardPath.lineTo( p0 + norm );
					break;
				}

				case InternalJoin::Round: {
					double angle = std::atan2( crossVal, dotVal );
					if( angle > 0.0 ) {
						mBackwardPath.lineTo( p0 + norm );
						roundJoin( mForwardPath, tolerance, p0, norm, angle );
					} else {
						mForwardPath.lineTo( p0 - norm );
						roundJoinRev( mBackwardPath, tolerance, p0, -norm, -angle );
					}
					break;
				}
			}
		}
	}
}

void StrokeContext::doLine( const InternalStrokeStyle& style, const glm::dvec2& tangent, const glm::dvec2& p1 )
{
	double scale = 0.5 * style.width / glm::length( tangent );
	glm::dvec2 norm = scale * glm::dvec2( -tangent.y, tangent.x );
	mForwardPath.lineTo( p1 - norm );
	mBackwardPath.lineTo( p1 + norm );
	mLastPt = p1;
}

void StrokeContext::doCubic( const InternalStrokeStyle& style, const CubicBezD& c, double tolerance )
{
	glm::dvec2 chord = c.p3 - c.p0;
	glm::dvec2 chordRef = chord;
	double chordRefHypot2 = glm::length2( chordRef );

	glm::dvec2 d01 = c.p1 - c.p0;
	if( glm::length2( d01 ) > chordRefHypot2 ) {
		chordRef = d01;
		chordRefHypot2 = glm::length2( chordRef );
	}

	glm::dvec2 d23 = c.p3 - c.p2;
	if( glm::length2( d23 ) > chordRefHypot2 ) {
		chordRef = d23;
		chordRefHypot2 = glm::length2( chordRef );
	}

	double p0 = glm::dot( glm::dvec2( c.p0 ), chordRef );
	double p1 = glm::dot( glm::dvec2( c.p1 ), chordRef );
	double p2 = glm::dot( glm::dvec2( c.p2 ), chordRef );
	double p3 = glm::dot( glm::dvec2( c.p3 ), chordRef );

	constexpr double ENDPOINT_D = 0.01;
	if( p3 <= p0 || p1 > p2 || p1 < p0 + ENDPOINT_D * ( p3 - p0 )
		|| p2 > p3 - ENDPOINT_D * ( p3 - p0 ) )
	{
		double x01 = cross2d( d01, chordRef );
		double x23 = cross2d( d23, chordRef );
		double x03 = cross2d( chord, chordRef );
		double thresh = tolerance * tolerance * chordRefHypot2;

		if( x01 * x01 < thresh && x23 * x23 < thresh && x03 * x03 < thresh ) {
			glm::dvec2 midpoint = 0.5 * ( c.p0 + c.p3 );
			glm::dvec2 refVec = chordRef / chordRefHypot2;
			glm::dvec2 refPt = midpoint - 0.5 * ( p0 + p3 ) * refVec;
			double pArr[4] = { p0, p1, p2, p3 };
			doLinear( style, c, pArr, refPt, refVec );
			return;
		}
	}

	mResultPath.clear();
	offsetCubic( c, -0.5 * style.width, tolerance, mResultPath );

	bool first = true;
	for( const auto& el : mResultPath ) {
		if( first && el.type == PathElType::MoveTo ) {
			first = false;
			continue;
		}
		switch( el.type ) {
			case PathElType::LineTo: mForwardPath.lineTo( el.p1 ); break;
			case PathElType::CurveTo: mForwardPath.curveTo( el.p1, el.p2, el.p3 ); break;
			default: break;
		}
	}

	mResultPath.clear();
	offsetCubic( c, 0.5 * style.width, tolerance, mResultPath );

	first = true;
	for( const auto& el : mResultPath ) {
		if( first && el.type == PathElType::MoveTo ) {
			first = false;
			continue;
		}
		switch( el.type ) {
			case PathElType::LineTo: mBackwardPath.lineTo( el.p1 ); break;
			case PathElType::CurveTo: mBackwardPath.curveTo( el.p1, el.p2, el.p3 ); break;
			default: break;
		}
	}

	mLastPt = c.p3;
}

void StrokeContext::doLinear( const InternalStrokeStyle& style, const CubicBezD& c,
							  double p[4], const glm::dvec2& refPt, const glm::dvec2& refVec )
{
	InternalStrokeStyle roundStyle = style;
	roundStyle.join = InternalJoin::Round;

	glm::dvec2 tan0 = startTangent( c );
	glm::dvec2 tan1 = endTangent( c );
	mLastTan = tan0;

	double c0 = p[1] - p[0];
	double c1 = 2.0 * p[2] - 4.0 * p[1] + 2.0 * p[0];
	double c2 = p[3] - 3.0 * p[2] + 3.0 * p[1] - p[0];

	double roots[2];
	int nRoots = solveQuadraticStable( c0, c1, c2, roots );

	constexpr double EPSILON = 1e-6;
	for( int i = 0; i < nRoots; ++i ) {
		double t = roots[i];
		if( t > EPSILON && t < 1.0 - EPSILON ) {
			double mt = 1.0 - t;
			double z = mt * ( mt * mt * p[0] + 3.0 * t * ( mt * p[1] + t * p[2] ) )
					   + t * t * t * p[3];
			glm::dvec2 pt = refPt + z * refVec;
			glm::dvec2 tan = pt - mLastPt;
			doJoin( roundStyle, tan );
			doLine( roundStyle, tan, pt );
			mLastTan = tan;
		}
	}

	glm::dvec2 tan = c.p3 - mLastPt;
	doJoin( roundStyle, tan );
	doLine( roundStyle, tan, c.p3 );
	mLastTan = tan;
	doJoin( roundStyle, tan1 );
}

void StrokeContext::processElement( const PathEl& el, const InternalStrokeStyle& style, double tolerance )
{
	glm::dvec2 p0 = mLastPt;

	switch( el.type ) {
		case PathElType::MoveTo:
			finish( style );
			mStartPt = el.p1;
			mLastPt = el.p1;
			break;

		case PathElType::LineTo:
			if( el.p1 != p0 ) {
				glm::dvec2 tangent = el.p1 - p0;
				doJoin( style, tangent );
				mLastTan = tangent;
				doLine( style, tangent, el.p1 );
			}
			break;

		case PathElType::QuadTo:
			if( el.p1 != p0 || el.p2 != p0 ) {
				glm::dvec2 q[3] = { p0, el.p1, el.p2 };
				glm::dvec2 cubic[4];
				raiseQuadToCubic( q, cubic );
				CubicBezD c( cubic[0], cubic[1], cubic[2], cubic[3] );

				glm::dvec2 tan0 = startTangent( c );
				glm::dvec2 tan1 = endTangent( c );
				doJoin( style, tan0 );
				doCubic( style, c, tolerance );
				mLastTan = tan1;
			}
			break;

		case PathElType::CurveTo:
			if( el.p1 != p0 || el.p2 != p0 || el.p3 != p0 ) {
				CubicBezD c( p0, el.p1, el.p2, el.p3 );
				glm::dvec2 tan0 = startTangent( c );
				glm::dvec2 tan1 = endTangent( c );
				doJoin( style, tan0 );
				doCubic( style, c, tolerance );
				mLastTan = tan1;
			}
			break;

		case PathElType::Close:
			if( p0 != mStartPt ) {
				glm::dvec2 tangent = mStartPt - p0;
				doJoin( style, tangent );
				mLastTan = tangent;
				doLine( style, tangent, mStartPt );
			}
			finishClosed( style );
			break;
	}
}

//=============================================================================
// Dashing Implementation
//=============================================================================

double elementArclen( const PathEl& el, const glm::dvec2& prevPt )
{
	switch( el.type ) {
		case PathElType::MoveTo:
			return 0.0;
		case PathElType::LineTo:
			return lineArclen( prevPt, el.p1 );
		case PathElType::QuadTo: {
			glm::dvec2 q[3] = { prevPt, el.p1, el.p2 };
			return quadArclenGauss( q );
		}
		case PathElType::CurveTo: {
			glm::dvec2 p[4] = { prevPt, el.p1, el.p2, el.p3 };
			return cubicArclenGauss( p );
		}
		case PathElType::Close:
			return 0.0;
	}
	return 0.0;
}

void splitElementLeft( const PathEl& el, const glm::dvec2& prevPt, double t, BezPathD& out )
{
	if( t <= 0.0 ) return;
	if( t >= 1.0 ) {
		switch( el.type ) {
			case PathElType::LineTo: out.lineTo( el.p1 ); break;
			case PathElType::QuadTo: out.quadTo( el.p1, el.p2 ); break;
			case PathElType::CurveTo: out.curveTo( el.p1, el.p2, el.p3 ); break;
			default: break;
		}
		return;
	}

	switch( el.type ) {
		case PathElType::LineTo: {
			glm::dvec2 pt = prevPt + t * ( el.p1 - prevPt );
			out.lineTo( pt );
			break;
		}
		case PathElType::QuadTo: {
			glm::dvec2 q[3] = { prevPt, el.p1, el.p2 };
			glm::dvec2 result[3];
			quadSubsegmentLeft( q, t, result );
			out.quadTo( result[1], result[2] );
			break;
		}
		case PathElType::CurveTo: {
			glm::dvec2 p[4] = { prevPt, el.p1, el.p2, el.p3 };
			glm::dvec2 result[4];
			cubicSubsegmentLeft( p, t, result );
			out.curveTo( result[1], result[2], result[3] );
			break;
		}
		default:
			break;
	}
}

PathEl splitElementRight( const PathEl& el, const glm::dvec2& prevPt, double t )
{
	PathEl result = el;
	if( t <= 0.0 ) return result;
	if( t >= 1.0 ) {
		result.p1 = result.p2 = result.p3 = el.p3;
		return result;
	}

	switch( el.type ) {
		case PathElType::LineTo: {
			result.p1 = prevPt + t * ( el.p1 - prevPt );
			break;
		}
		case PathElType::QuadTo: {
			glm::dvec2 q[3] = { prevPt, el.p1, el.p2 };
			glm::dvec2 rightQ[3];
			quadSubsegmentRight( q, t, rightQ );
			result.p1 = rightQ[1];
			result.p2 = rightQ[2];
			break;
		}
		case PathElType::CurveTo: {
			glm::dvec2 p[4] = { prevPt, el.p1, el.p2, el.p3 };
			glm::dvec2 rightP[4];
			cubicSubsegmentRight( p, t, rightP );
			result.p1 = rightP[1];
			result.p2 = rightP[2];
			result.p3 = rightP[3];
			break;
		}
		default:
			break;
	}
	return result;
}

glm::dvec2 elementEndPoint( const PathEl& el )
{
	switch( el.type ) {
		case PathElType::MoveTo:
		case PathElType::LineTo:
			return el.p1;
		case PathElType::QuadTo:
			return el.p2;
		case PathElType::CurveTo:
			return el.p3;
		default:
			return glm::dvec2( 0.0 );
	}
}

double elementInvArclen( const PathEl& el, const glm::dvec2& prevPt, double targetLen )
{
	switch( el.type ) {
		case PathElType::LineTo: {
			double totalLen = lineArclen( prevPt, el.p1 );
			return ( totalLen > 0.0 ) ? std::clamp( targetLen / totalLen, 0.0, 1.0 ) : 0.0;
		}
		case PathElType::QuadTo: {
			glm::dvec2 q[3] = { prevPt, el.p1, el.p2 };
			return quadInvArclen( q, targetLen, DASH_ACCURACY );
		}
		case PathElType::CurveTo: {
			glm::dvec2 p[4] = { prevPt, el.p1, el.p2, el.p3 };
			return cubicInvArclen( p, targetLen, DASH_ACCURACY );
		}
		default:
			return 0.0;
	}
}

BezPathD dashInternal( const BezPathD& path, double dashOffset, const std::vector<double>& dashPattern )
{
	if( dashPattern.empty() ) {
		return path;
	}

	double patternLen = 0.0;
	for( double d : dashPattern ) {
		patternLen += std::abs( d );
	}
	if( patternLen <= 0.0 ) {
		return path;
	}

	double offset = std::fmod( dashOffset, patternLen );
	if( offset < 0.0 ) {
		offset += patternLen;
	}

	BezPathD result;
	glm::dvec2 currentPt( 0.0 );
	glm::dvec2 startPt( 0.0 );
	bool needMoveTo = true;

	size_t dashIdx = 0;
	double dashRemaining = dashPattern[0];
	bool isActive = true;

	double remainingOffset = offset;
	while( remainingOffset > 0.0 && dashRemaining <= remainingOffset ) {
		remainingOffset -= dashRemaining;
		dashIdx = ( dashIdx + 1 ) % dashPattern.size();
		dashRemaining = dashPattern[dashIdx];
		isActive = !isActive;
	}
	dashRemaining -= remainingOffset;

	for( const auto& el : path ) {
		if( el.type == PathElType::MoveTo ) {
			dashIdx = 0;
			dashRemaining = dashPattern[0];
			isActive = true;

			remainingOffset = offset;
			while( remainingOffset > 0.0 && dashRemaining <= remainingOffset ) {
				remainingOffset -= dashRemaining;
				dashIdx = ( dashIdx + 1 ) % dashPattern.size();
				dashRemaining = dashPattern[dashIdx];
				isActive = !isActive;
			}
			dashRemaining -= remainingOffset;

			currentPt = el.p1;
			startPt = el.p1;
			needMoveTo = true;
			continue;
		}

		if( el.type == PathElType::Close ) {
			if( currentPt != startPt ) {
				PathEl lineEl;
				lineEl.type = PathElType::LineTo;
				lineEl.p1 = startPt;

				glm::dvec2 prevPt = currentPt;
				double segLen = lineArclen( prevPt, lineEl.p1 );
				double segRemaining = segLen;

				while( segRemaining > 1e-12 ) {
					if( dashRemaining <= segRemaining ) {
						double t = elementInvArclen( lineEl, prevPt, dashRemaining );
						if( isActive ) {
							if( needMoveTo ) {
								result.moveTo( prevPt );
								needMoveTo = false;
							}
							splitElementLeft( lineEl, prevPt, t, result );
						}
						prevPt = prevPt + t * ( lineEl.p1 - prevPt );
						segRemaining -= dashRemaining;
						dashIdx = ( dashIdx + 1 ) % dashPattern.size();
						dashRemaining = dashPattern[dashIdx];
						isActive = !isActive;
						needMoveTo = true;
					}
					else {
						if( isActive ) {
							if( needMoveTo ) {
								result.moveTo( prevPt );
								needMoveTo = false;
							}
							result.lineTo( lineEl.p1 );
						}
						dashRemaining -= segRemaining;
						segRemaining = 0.0;
					}
				}
			}
			currentPt = startPt;
			needMoveTo = true;
			continue;
		}

		glm::dvec2 prevPt = currentPt;
		PathEl remainingEl = el;
		double segLen = elementArclen( el, prevPt );
		double segRemaining = segLen;

		while( segRemaining > 1e-12 ) {
			if( dashRemaining <= segRemaining ) {
				double t = elementInvArclen( remainingEl, prevPt, dashRemaining );
				if( isActive ) {
					if( needMoveTo ) {
						result.moveTo( prevPt );
						needMoveTo = false;
					}
					splitElementLeft( remainingEl, prevPt, t, result );
				}

				glm::dvec2 splitPt;
				switch( remainingEl.type ) {
					case PathElType::LineTo:
						splitPt = prevPt + t * ( remainingEl.p1 - prevPt );
						break;
					case PathElType::QuadTo: {
						glm::dvec2 q[3] = { prevPt, remainingEl.p1, remainingEl.p2 };
						splitPt = evalQuad( q, t );
						break;
					}
					case PathElType::CurveTo: {
						glm::dvec2 p[4] = { prevPt, remainingEl.p1, remainingEl.p2, remainingEl.p3 };
						splitPt = evalCubic( p, t );
						break;
					}
					default:
						splitPt = prevPt;
				}

				remainingEl = splitElementRight( remainingEl, prevPt, t );
				prevPt = splitPt;

				segRemaining -= dashRemaining;
				dashIdx = ( dashIdx + 1 ) % dashPattern.size();
				dashRemaining = dashPattern[dashIdx];
				isActive = !isActive;
				needMoveTo = true;
			}
			else {
				if( isActive ) {
					if( needMoveTo ) {
						result.moveTo( prevPt );
						needMoveTo = false;
					}
					switch( remainingEl.type ) {
						case PathElType::LineTo:
							result.lineTo( remainingEl.p1 );
							break;
						case PathElType::QuadTo:
							result.quadTo( remainingEl.p1, remainingEl.p2 );
							break;
						case PathElType::CurveTo:
							result.curveTo( remainingEl.p1, remainingEl.p2, remainingEl.p3 );
							break;
						default:
							break;
					}
				}
				dashRemaining -= segRemaining;
				segRemaining = 0.0;
			}
		}

		currentPt = elementEndPoint( el );
	}

	return result;
}

//=============================================================================
// Internal Stroke Functions
//=============================================================================

BezPathD strokeInternal( const BezPathD& path, const InternalStrokeStyle& style, double tolerance )
{
	StrokeContext ctx;
	ctx.mJoinThresh = 2.0 * tolerance / style.width;

	if( style.dashPattern.empty() ) {
		for( const auto& el : path ) {
			ctx.processElement( el, style, tolerance );
		}
	} else {
		BezPathD dashedPath = dashInternal( path, style.dashOffset, style.dashPattern );
		for( const auto& el : dashedPath ) {
			ctx.processElement( el, style, tolerance );
		}
	}
	ctx.finish( style );
	return ctx.output();
}

} // anonymous namespace

//=============================================================================
// Public API Implementation
//=============================================================================

Shape2d offset( const Path2d& path, float distance, float tolerance )
{
	BezPathD bezPath = toDoublePrecision( path );

	BezPathD result;
	glm::dvec2 lastPt( 0.0 );
	glm::dvec2 startPt( 0.0 );

	for( const auto& el : bezPath ) {
		switch( el.type ) {
			case PathElType::MoveTo:
				lastPt = el.p1;
				startPt = el.p1;
				result.moveTo( el.p1 );
				break;

			case PathElType::LineTo: {
				glm::dvec2 tangent = el.p1 - lastPt;
				double len = glm::length( tangent );
				if( len > 1e-9 ) {
					glm::dvec2 norm( -tangent.y / len, tangent.x / len );
					double d = static_cast<double>( distance );
					if( result.empty() || result.elements().back().type == PathElType::MoveTo ) {
						result.moveTo( lastPt + d * norm );
					}
					result.lineTo( el.p1 + d * norm );
				}
				lastPt = el.p1;
				break;
			}

			case PathElType::QuadTo: {
				glm::dvec2 q[3] = { lastPt, el.p1, el.p2 };
				glm::dvec2 c[4];
				raiseQuadToCubic( q, c );
				CubicBezD cubic( c[0], c[1], c[2], c[3] );

				BezPathD offsetResult;
				offsetCubic( cubic, static_cast<double>( distance ), static_cast<double>( tolerance ), offsetResult );

				bool first = true;
				for( const auto& oel : offsetResult ) {
					if( first && oel.type == PathElType::MoveTo ) {
						if( result.empty() || result.elements().back().type == PathElType::MoveTo ) {
							result.moveTo( oel.p1 );
						}
						first = false;
						continue;
					}
					switch( oel.type ) {
						case PathElType::LineTo: result.lineTo( oel.p1 ); break;
						case PathElType::CurveTo: result.curveTo( oel.p1, oel.p2, oel.p3 ); break;
						default: break;
					}
				}
				lastPt = el.p2;
				break;
			}

			case PathElType::CurveTo: {
				CubicBezD cubic( lastPt, el.p1, el.p2, el.p3 );

				BezPathD offsetResult;
				offsetCubic( cubic, static_cast<double>( distance ), static_cast<double>( tolerance ), offsetResult );

				bool first = true;
				for( const auto& oel : offsetResult ) {
					if( first && oel.type == PathElType::MoveTo ) {
						if( result.empty() || result.elements().back().type == PathElType::MoveTo ) {
							result.moveTo( oel.p1 );
						}
						first = false;
						continue;
					}
					switch( oel.type ) {
						case PathElType::LineTo: result.lineTo( oel.p1 ); break;
						case PathElType::CurveTo: result.curveTo( oel.p1, oel.p2, oel.p3 ); break;
						default: break;
					}
				}
				lastPt = el.p3;
				break;
			}

			case PathElType::Close:
				result.closePath();
				lastPt = startPt;
				break;
		}
	}

	return bezPathToShape2d( result );
}

Shape2d offset( const Shape2d& shape, float distance, float tolerance )
{
	Shape2d result;
	for( const auto& contour : shape.getContours() ) {
		Shape2d offsetContour = offset( contour, distance, tolerance );
		result.append( offsetContour );
	}
	return result;
}

Shape2d stroke( const Path2d& path, const StrokeStyle& style, float tolerance )
{
	BezPathD bezPath = toDoublePrecision( path );
	InternalStrokeStyle internalStyle = convertStrokeStyle( style );

	BezPathD result = strokeInternal( bezPath, internalStyle, static_cast<double>( tolerance ) );

	return bezPathToShape2d( result );
}

Shape2d stroke( const Path2d& path, float width, float tolerance )
{
	return stroke( path, StrokeStyle( width ), tolerance );
}

Shape2d stroke( const Shape2d& shape, const StrokeStyle& style, float tolerance )
{
	Shape2d result;
	for( const auto& contour : shape.getContours() ) {
		Shape2d strokeContour = stroke( contour, style, tolerance );
		result.append( strokeContour );
	}
	return result;
}

Shape2d stroke( const Shape2d& shape, float width, float tolerance )
{
	return stroke( shape, StrokeStyle( width ), tolerance );
}

Shape2d dash( const Path2d& path, float offset, const std::vector<float>& pattern )
{
	BezPathD bezPath = toDoublePrecision( path );

	std::vector<double> doublePattern;
	doublePattern.reserve( pattern.size() );
	for( float d : pattern ) {
		doublePattern.push_back( static_cast<double>( d ) );
	}

	BezPathD result = dashInternal( bezPath, static_cast<double>( offset ), doublePattern );

	return bezPathToShape2d( result );
}

Shape2d dash( const Shape2d& shape, float offset, const std::vector<float>& pattern )
{
	Shape2d result;
	for( const auto& contour : shape.getContours() ) {
		Shape2d dashContour = dash( contour, offset, pattern );
		result.append( dashContour );
	}
	return result;
}

} // namespace cinder
