/*
 Copyright (c) 2010, The Cinder Project, All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Portions Copyright (c) 2004, Laminar Research.

 Portions Copyright (c) 2011 Google Inc. All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
	* Neither the name of Google Inc. nor the names of its
	contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/


#include "cinder/CinderMath.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/Log.h"

#include <algorithm>
#include <iterator>

using std::vector;

namespace cinder {

namespace { // helpers defined below
	void chopQuadAt( const vec2 src[3], vec2 dst[5], float t );
	void trimQuadAt( const vec2 src[3], vec2 dst[3], float t0, float t1 );
	void chopCubicAt( const vec2 src[4], vec2 dst[7], float t );
	void trimCubicAt( const vec2 src[4], vec2 dst[4], float t0, float t1 );
}

const int Path2d::sSegmentTypePointCounts[] = { 0, 1, 2, 3, 0 }; // MOVETO, LINETO, QUADTO, CUBICTO, CLOSE

Path2d::Path2d( const BSpline2f &spline, float subdivisionStep )
{
	int numPoints = spline.getNumControlPoints();
	if( numPoints <= spline.getDegree() )
		return;

	if( spline.getDegree() == 1 ) { // linear
		moveTo( spline.getControlPoint( 0 ) );
		for( int p = 1; p < numPoints; ++p )
			lineTo( spline.getControlPoint( p ) );
		if( spline.isLoop() )
			lineTo( spline.getControlPoint( 0 ) );
	}
	else if( ( spline.getDegree() == 2 ) && ( ! spline.isOpen() ) ) { // quadratic, closed
		moveTo( ( spline.getControlPoint( 0 ) + spline.getControlPoint( 1 ) ) * 0.5f );
		int lastPt = ( spline.isLoop() ) ? numPoints + 1 : numPoints - 1;
		for( int i = 1; i < lastPt; i++ )
			quadTo( spline.getControlPoint( i % numPoints ), ( spline.getControlPoint( i % numPoints ) + spline.getControlPoint( ( i + 1 ) % numPoints ) ) * 0.5f );
	}
	else if( ( spline.getDegree() == 2 ) && spline.isOpen() ) { // quadratic, open
		vec2 spl1, spl2;
		moveTo( spline.getControlPoint( 0 ) );
		int lastPt = ( spline.isLoop() ) ? numPoints : numPoints - 1;
		for( int i = 1; i < lastPt; i++ ) {
			spl1 = spline.getControlPoint( i % numPoints );

			if( i + 1 == lastPt )
				spl2 = spline.getControlPoint( lastPt % numPoints );
			else
				spl2 = ( spline.getControlPoint( i % numPoints ) + spline.getControlPoint( ( i + 1 ) % numPoints ) ) * 0.5f;

			quadTo( spl1, spl2 );
		}
	}
	else if( ( spline.getDegree() == 3 ) && ( ! spline.isOpen() )  ) { // cubic, closed
		vec2 q0, q1, q2, q3;
		q0 = ( spline.getControlPoint( 0 ) + spline.getControlPoint( 1 ) * 4.0f + spline.getControlPoint( 2 ) ) / 6.0f;
		moveTo( q0 );
		int lastPt = ( spline.isLoop() ) ? numPoints : numPoints - 3;
		for( int i = 0; i < lastPt; ++i ) {
			vec2 p1 = spline.getControlPoint( ( i + 1 ) % numPoints ), p2 = spline.getControlPoint( ( i + 2 ) % numPoints ), p3 = spline.getControlPoint( ( i + 3 ) % numPoints );

			q1 = p1 * ( 4.0f / 6.0f ) + p2 * ( 2.0f / 6.0f );
			q2 = p1 * ( 2.0f / 6.0f ) + p2 * ( 4.0f / 6.0f );
			q3 = p1 * ( 1.0f / 6.0f ) + p2 * ( 4.0f / 6.0f ) + p3 * ( 1.0f / 6.0f );
			curveTo( q1, q2, q3 );
		}
	}
	else if( ( spline.getDegree() == 3 ) && ( spline.isOpen() )  ) { // cubic, open
		vec2 p1, p2, p3, p4;
		vec2 q1, q2, q3, q4;
		int lastPt = ( spline.isLoop() ) ? numPoints + 1 : numPoints;
		if( lastPt == 4 ) {
			moveTo( spline.getControlPoint( 0 ) );
			curveTo( spline.getControlPoint( 1 ), spline.getControlPoint( 2 ), spline.getControlPoint( 3 ) );
		}
		else if( lastPt == 5 ) {
			moveTo( spline.getControlPoint( 0 ) );
			p1 = spline.getControlPoint( 1 );
			p2 = spline.getControlPoint( 2 );
			p3 = spline.getControlPoint( 3 );
			q2 = ( p1 + p2 ) / 2.0f;
			curveTo( p1, q2, p1 * 0.25f + p2 * 0.5f + p3 * 0.25f );
			curveTo( ( p3 + p2 ) / 2.0f, p3, spline.getControlPoint( ( lastPt - 1 ) % numPoints ) );
		}
		else { // this functions properly when n >= 6
			moveTo( spline.getControlPoint( 0 ) );
			p1 = spline.getControlPoint( 1 );
			p2 = spline.getControlPoint( 2 );
			p3 = spline.getControlPoint( 3 );
			q2 = ( p1 + p2 ) / 2.0f;
			q4 = ( p2 * 2.0f + p3 ) / 3.0f;
			q3 = ( q2 + q4 ) / 2.0f;
			curveTo( p1, q2, q3 );
			for( int i = 2; i < lastPt - 4; i++ ) {
				p1 = p2;
				p2 = p3;
				p3 = spline.getControlPoint( ( i + 2 ) % numPoints );
				q1 = q4;
				q2 = ( p1 + p2 * 2.0f ) / 3.0f;
				q4 = ( p2 * 2.0f + p3 ) / 3.0f;
				q3 = ( q2 + q4 ) / 2.0f;
				curveTo( q1, q2, q3 );
			}
			p1 = p2;
			p2 = p3;
			p3 = spline.getControlPoint( ( lastPt - 2 ) % numPoints );
			q1 = q4;
			q2 = ( p1 + p2 * 2.0f) / 3.0f;
			q4 = ( p2 + p3 ) / 2.0f;
			q3 = ( q2 + q4 ) / 2.0f;
			curveTo( q1, q2, q3 );
			p2 = p3;
			p3 = spline.getControlPoint( ( lastPt - 1 ) % numPoints );
			curveTo( q4, p2, p3 );
		}
	}
	else { // this is not a case we handle directly, so we'll have to do a linear approximation
		moveTo( spline.getPosition( 0 ) );
		for( float t = subdivisionStep; t <= 1.0f; t += subdivisionStep )
			lineTo( spline.getPosition( t ) );
	}
}

void Path2d::moveTo( const vec2 &p )
{
	if( ! mPoints.empty() )
		throw Path2dExc(); // can only moveTo as the first point

	mPoints.push_back( p );
}

void Path2d::lineTo( const vec2 &p )
{
	if( mPoints.empty() )
		throw Path2dExc(); // can only lineTo as non-first point

	mPoints.push_back( p );
	mSegments.push_back( LINETO );
}

void Path2d::horizontalLineTo( float x )
{
	const vec2 &pt = getCurrentPoint();
	lineTo( x, pt.y );
}

void Path2d::verticalLineTo( float y )
{
	const vec2 &pt = getCurrentPoint();
	lineTo( pt.x, y );
}

void Path2d::quadTo( const vec2 &p1, const vec2 &p2 )
{
	if( mPoints.empty() )
		throw Path2dExc(); // can only quadTo as non-first point

	mPoints.push_back( p1 );
	mPoints.push_back( p2 );
	mSegments.push_back( QUADTO );
}

void Path2d::smoothQuadTo( const vec2 &p2 )
{
	if( mPoints.empty() )
		throw Path2dExc(); // can only smoothQuadTo as non-first point

	vec2 p1 = getCurrentPoint();

	if( ! mSegments.empty() && mSegments.back() == QUADTO ) {
		const vec2 &c = getPointBefore( mPoints.size() - 1 );
		p1.x = 2 * p1.x - c.x;
		p1.y = 2 * p1.y - c.y;
	}

	mSegments.emplace_back( QUADTO );
	mPoints.emplace_back( p1.x, p1.y );
	mPoints.emplace_back( p2.x, p2.y );
}

void Path2d::curveTo( const vec2 &p1, const vec2 &p2, const vec2 &p3 )
{
	if( mPoints.empty() )
		throw Path2dExc(); // can only curveTo as non-first point

	mPoints.push_back( p1 );
	mPoints.push_back( p2 );
	mPoints.push_back( p3 );
	mSegments.push_back( CUBICTO );
}

void Path2d::smoothCurveTo( const vec2 &p2, const vec2 &p3 )
{
	if( mPoints.empty() )
		throw Path2dExc(); // can only smoothCurveTo as non-first point

	vec2 p1 = getCurrentPoint();

	if( ! mSegments.empty() && mSegments.back() == CUBICTO ) {
		const vec2 &c = getPointBefore( mPoints.size() - 1 );
		p1.x = 2 * p1.x - c.x;
		p1.y = 2 * p1.y - c.y;
	}

	mSegments.emplace_back( CUBICTO );
	mPoints.emplace_back( p1.x, p1.y );
	mPoints.emplace_back( p2.x, p2.y );
	mPoints.emplace_back( p3.x, p3.y );
}

void Path2d::arc( const vec2 &center, float radius, float startRadians, float endRadians, bool forward )
{
	if( forward ) {
		while( endRadians < startRadians )
			endRadians += 2 * static_cast<float>( M_PI );
	}
	else {
		while( endRadians > startRadians )
			endRadians -= 2 * static_cast<float>( M_PI );
	}

	if( mPoints.empty() )
		moveTo( center + radius * vec2( math<float>::cos( startRadians ), math<float>::sin( startRadians ) ) );
	else {
		lineTo( center + radius * vec2( math<float>::cos( startRadians ), math<float>::sin( startRadians ) ) );
	}

	if( forward )
		arcHelper( center, radius, startRadians, endRadians, forward );
	else
		arcHelper( center, radius, endRadians, startRadians, forward );
}

void Path2d::arcHelper( const vec2 &center, float radius, float startRadians, float endRadians, bool forward )
{
	// wrap the angle difference around to be in the range [0, 4*pi]
    while( endRadians - startRadians > 4 * M_PI )
		endRadians -= 2 * static_cast<float>( M_PI );

    // Recurse if angle delta is larger than PI
    if( endRadians - startRadians > M_PI ) {
		float midRadians = startRadians + (endRadians - startRadians) * 0.5f;
		if( forward ) {
			arcHelper( center, radius, startRadians, midRadians, forward );
			arcHelper( center, radius, midRadians, endRadians, forward );
		}
		else {
			arcHelper( center, radius, midRadians, endRadians, forward );
			arcHelper( center, radius, startRadians, midRadians, forward );
		}
    }
	else if( math<float>::abs( endRadians - startRadians ) > 0.000001f ) {
		int segments = static_cast<int>( math<float>::ceil( math<float>::abs( endRadians - startRadians ) / (static_cast<float>( M_PI ) / 2.0f ) ) );
		float angle;
		float angleDelta = ( endRadians - startRadians ) / (float)segments;
		if( forward )
			angle = startRadians;
		else {
			angle = endRadians;
			angleDelta = -angleDelta;
		}

		for( int seg = 0; seg < segments; seg++, angle += angleDelta ) {
			arcSegmentAsCubicBezier( center, radius, angle, angle + angleDelta );
		}
    }
}

void Path2d::arcSegmentAsCubicBezier( const vec2 &center, float radius, float startRadians, float endRadians )
{
	float r_sin_A, r_cos_A;
	float r_sin_B, r_cos_B;
	float h;

	r_sin_A = radius * math<float>::sin( startRadians );
	r_cos_A = radius * math<float>::cos( startRadians );
	r_sin_B = radius * math<float>::sin( endRadians );
	r_cos_B = radius * math<float>::cos( endRadians );

	h = 4.0f/3.0f * math<float>::tan( (endRadians - startRadians) / 4 );

	curveTo( center.x + r_cos_A - h * r_sin_A, center.y + r_sin_A + h * r_cos_A, center.x + r_cos_B + h * r_sin_B,
				center.y + r_sin_B - h * r_cos_B, center.x + r_cos_B, center.y + r_sin_B );
}

// Implementation courtesy of Lennart Kudling
void Path2d::arcTo( const vec2 &p1, const vec2 &t, float radius )
{
	if( isClosed() || empty() )
		throw Path2dExc(); // can only arcTo as non-first point

	const float epsilon = 1e-8f;

	// Get current point.
	const vec2& p0 = getCurrentPoint();

	// Calculate the tangent vectors tangent1 and tangent2.
	const vec2 p0t = p0 - t;
	const vec2 p1t = p1 - t;

	// Calculate tangent distance squares.
	const float p0tSquare = length2( p0t );
	const float p1tSquare = length2( p1t );

	// Calculate tan(a/2) where a is the angle between vectors tangent1 and tangent2.
	//
	// Use the following facts:
	//
	//  p0t * p1t  = |p0t| * |p1t| * cos(a) <=> cos(a) =  p0t * p1t  / (|p0t| * |p1t|)
	// |p0t x p1t| = |p0t| * |p1t| * sin(a) <=> sin(a) = |p0t x p1t| / (|p0t| * |p1t|)
	//
	// and
	//
	// tan(a/2) = sin(a) / ( 1 - cos(a) )

	const float numerator = p0t.y * p1t.x - p1t.y * p0t.x;
	const float denominator = math<float>::sqrt( p0tSquare * p1tSquare ) - ( p0t.x * p1t.x + p0t.y * p1t.y );

	// The denominator is zero <=> p0 and p1 are colinear.
	if( math<float>::abs( denominator ) < epsilon ) {
		lineTo( t );
	}
	else {
		// |b0 - t| = |b3 - t| = radius * tan(a/2).
		const float distanceFromT = math<float>::abs( radius * numerator / denominator );

		// b0 = t + |b0 - t| * (p0 - t)/|p0 - t|.
		const vec2 b0 = t + distanceFromT * normalize( p0t );

		// If b0 deviates from p0, add a line to it.
		if( math<float>::abs(b0.x - p0.x) > epsilon || math<float>::abs(b0.y - p0.y) > epsilon ) {
			lineTo( b0 );
		}

		// b3 = t + |b3 - t| * (p1 - t)/|p1 - t|.
		const vec2 b3 = t + distanceFromT * normalize( p1t );

		// The two bezier-control points are located on the tangents at a fraction
		// of the distance[ tangent points <-> tangent intersection ].
		// See "Approxmiation of a Cubic Bezier Curve by Circular Arcs and Vice Versa" by Aleksas Riskus
		// http://itc.ktu.lt/itc354/Riskus354.pdf

		float b0tSquare = (t.x - b0.x) *  (t.x - b0.x) + (t.y - b0.y) *  (t.y - b0.y);
		float radiusSquare = radius * radius;
		float fraction;

		// Assume dist = radius = 0 if the radius is very small.
		if( math<float>::abs( radiusSquare / b0tSquare ) < epsilon )
			fraction = 0.0;
		else
			fraction = ( 4.0f / 3.0f ) / ( 1.0f + math<float>::sqrt( 1.0f + b0tSquare / radiusSquare ) );

		const vec2 b1 = b0 + fraction * (t - b0);
		const vec2 b2 = b3 + fraction * (t - b3);

		curveTo( b1, b2, b3 );
	}
}

namespace {

float angleHelper( const vec2 &u, const vec2 &v )
{
	// See: equation 5.4 of https://www.w3.org/TR/SVG/implnote.html
	const float c = u.x * v.y - u.y * v.x;
	const float d = glm::dot( glm::normalize( u ), glm::normalize( v ) );
	return c < 0 ? -math<float>::acos( d ) : math<float>::acos( d );
}

} // namespace

void Path2d::arcTo( float rx, float ry, float phi, bool largeArcFlag, bool sweepFlag, const vec2 &p2 )
{
	// See: https://www.w3.org/TR/SVG/implnote.html

	if( approxZero( rx ) || approxZero( ry ) ) {
		return lineTo( p2 );
	}

	const vec2 p1 = mPoints.back();

	const float sinPhi = math<float>::sin( phi );
	const float cosPhi = math<float>::cos( phi );

	// Step 1: move ellipse so origin will be the midpoint between p1 and p2.
	vec2 mid = ( p1 - p2 ) * 0.5f; // midpoint

	const float x1p = mid.x * cosPhi + mid.y * sinPhi; // equation 5.1
	const float y1p = mid.y * cosPhi - mid.x * sinPhi;
	if( approxZero( x1p ) && approxZero( y1p ) )
		return lineTo( p2 );

	const float x1pSquared = x1p * x1p;
	const float y1pSquared = y1p * y1p;

	float rxSquared = rx * rx;
	float rySquared = ry * ry;

	float lambda = x1pSquared / rxSquared + y1pSquared / rySquared; // equation 6.2
	if( lambda > 1.0f ) {                                           // equation 6.3
		lambda = math<float>::sqrt( lambda );
		rx *= lambda;
		ry *= lambda;
		rxSquared = rx * rx;
		rySquared = ry * ry;
	}

	// Step 2: compute coordinates of the center of the ellipse.
	const float x = rySquared * x1pSquared;
	const float y = rxSquared * y1pSquared;

	float r = roundToZero( ( rxSquared * rySquared - y - x ) / ( y + x ) );
	r = largeArcFlag == sweepFlag ? -sqrtf( r ) : sqrtf( r );

	float cxp = r * ( rx * y1p ) / ry; // equation 5.2
	float cyp = r * -( ry * x1p ) / rx;

	// Step 3: transform back to original coordinate system.
	mid = ( p1 + p2 ) * 0.5f;
	vec2 c{ cxp * cosPhi - cyp * sinPhi + mid.x, cyp * cosPhi + cxp * sinPhi + mid.y }; // equation 5.3

	// Step 4: compute angles and number of segments.
	vec2 v1{ ( x1p - cxp ) / rx, ( y1p - cyp ) / ry };
	vec2 v2{ ( -x1p - cxp ) / rx, ( -y1p - cyp ) / ry };

	float theta = angleHelper( { 1, 0 }, v1 );
	float deltaTheta = angleHelper( v1, v2 );

	if( !sweepFlag && deltaTheta > 0 )
		deltaTheta -= float( 2 * M_PI );
	else if( sweepFlag && deltaTheta < 0 )
		deltaTheta += float( 2 * M_PI );

	float segments = glm::max( 1.0f, math<float>::ceil( math<float>::abs( deltaTheta ) / float( M_PI / 2 ) ) );
	deltaTheta /= segments;

	float h = 4.0f / 3.0f * math<float>::tan( deltaTheta / 4 );

	// Step 5: generate cubic bezier curve segments.
	for( int i = 0; i < int( segments ); ++i ) {
		float x1 = roundToZero( math<float>::cos( theta ) );
		float y1 = roundToZero( math<float>::sin( theta ) );

		theta += deltaTheta;

		float x2 = roundToZero( math<float>::cos( theta ) );
		float y2 = roundToZero( math<float>::sin( theta ) );

		vec2 c1{ rx * ( x1 - y1 * h ), ry * ( y1 + x1 * h ) };
		vec2 c2{ rx * ( x2 + y2 * h ), ry * ( y2 - x2 * h ) };
		vec2 c3{ rx * x2, ry * y2 };

		c1 = glm::rotate( c1, phi ) + c;
		c2 = glm::rotate( c2, phi ) + c;
		c3 = glm::rotate( c3, phi ) + c;

		curveTo( c1, c2, c3 );
	}
}

void Path2d::relativeMoveTo( const vec2 &delta )
{
	const auto &pt = getCurrentPoint();
	moveTo( pt + delta );
}

void Path2d::relativeLineTo( const vec2 &delta )
{
	const auto &pt = getCurrentPoint();
	lineTo( pt + delta );
}

void Path2d::relativeHorizontalLineTo( float dx )
{
	const auto &pt = getCurrentPoint();
	horizontalLineTo( pt.x + dx );
}

void Path2d::relativeVerticalLineTo( float dy )
{
	const auto &pt = getCurrentPoint();
	verticalLineTo( pt.y + dy );
}

void Path2d::relativeQuadTo( const vec2 &delta1, const vec2 &delta2 )
{
	const auto &pt = getCurrentPoint();
	quadTo( pt + delta1, pt + delta2 );
}

void Path2d::relativeSmoothQuadTo( const vec2 &delta )
{
	const auto &pt = getCurrentPoint();
	smoothQuadTo( pt + delta );
}

void Path2d::relativeCurveTo( const vec2 &delta1, const vec2 &delta2, const vec2 &delta3 )
{
	const auto &pt = getCurrentPoint();
	curveTo( pt + delta1, pt + delta2, pt + delta3 );
}

void Path2d::relativeSmoothCurveTo( const vec2 &delta2, const vec2 &delta3 )
{
	const auto &pt = getCurrentPoint();
	smoothCurveTo( pt + delta2, pt + delta3 );
}

void Path2d::relativeArcTo( float rx, float ry, float phi, bool largeArcFlag, bool sweepFlag, const vec2 &delta )
{
	const auto &pt = getCurrentPoint();
	arcTo( rx, ry, phi, largeArcFlag, sweepFlag, pt + delta );
}

Path2d Path2d::circle( const vec2 &center, float radius )
{
	Path2d shape;
	shape.moveTo( center.x + radius, center.y );
	shape.relativeArcTo( radius, radius, 0, false, true, vec2( -( radius + radius ), 0 ) );
	shape.relativeArcTo( radius, radius, 0, false, true, vec2( +( radius + radius ), 0 ) );
	return shape;
}

Path2d Path2d::ellipse( const vec2 &center, float radiusX, float radiusY )
{
	Path2d shape;
	shape.moveTo( center.x + radiusX, center.y );
	shape.relativeArcTo( radiusX, radiusY, 0, false, true, vec2( -( radiusX + radiusX ), 0 ) );
	shape.relativeArcTo( radiusX, radiusY, 0, false, true, vec2( +( radiusX + radiusX ), 0 ) );
	return shape;
}

Path2d Path2d::line( const vec2 &p0, const vec2 &p1 )
{
	Path2d shape;
	shape.moveTo( p0 );
	shape.lineTo( p1 );
	return shape;
}

Path2d Path2d::polygon( const std::vector<vec2> &points, bool closed )
{
	if( points.size() < 2 )
		throw Path2dExc(); //

	Path2d shape;

	auto itr = points.begin();
	shape.moveTo( *itr++ );
	while( itr != points.end() )
		shape.lineTo( *itr++ );

	if( closed )
		shape.close();

	return shape;
}

Path2d Path2d::rectangle( float x, float y, float width, float height )
{
	Path2d shape;
	shape.moveTo( x, y );
	shape.lineTo( x + width, y );
	shape.lineTo( x + width, y + height );
	shape.lineTo( x, y + height );
	shape.close();
	return shape;
}

Path2d Path2d::roundedRectangle( float x, float y, float width, float height, float rx, float ry )
{
	if( approxZero( rx ) || approxZero( ry ) )
		return rectangle( x, y, width, height );
	
	Path2d shape;
	shape.moveTo( x + rx, y );
	shape.lineTo( x + width - rx, y );
	shape.arcTo( rx, ry, 0, false, true, vec2( x + width, y + ry ) );
	shape.lineTo( x + width, y + height - ry );
	shape.arcTo( rx, ry, 0, false, true, vec2( x + width - rx, y + height ) );
	shape.lineTo( x + rx, y + height );
	shape.arcTo( rx, ry, 0, false, true, vec2( x, y + height - ry ) );
	shape.lineTo( x, y + ry );
	shape.arcTo( rx, ry, 0, false, true, vec2( x + rx, y ) );
	shape.close();
	return shape;
}

Path2d Path2d::star( const vec2 &center, int points, float largeRadius, float smallRadius, float rotation )
{
	const float step = glm::radians( 180.0f / float( points ) );

	Path2d shape;
	for( int i = 0; i < 2 * points; i += 2 ) {
		float x = center.x + largeRadius * glm::sin( rotation + float( i + 0 ) * step );
		float y = center.y - largeRadius * glm::cos( rotation + float( i + 0 ) * step );
		if( i == 0 )
			shape.moveTo( x, y );
		else
			shape.lineTo( x, y );
		x = center.x + smallRadius * glm::sin( rotation + float( i + 1 ) * step );
		y = center.y - smallRadius * glm::cos( rotation + float( i + 1 ) * step );
		shape.lineTo( x, y );
	}
	shape.close();
	return shape;
}

Path2d Path2d::arrow( const vec2 &p0, const vec2 &p1, float thickness, float width, float length, float concavity )
{
	const float distance = glm::distance( p1, p0 );
	const vec2  direction = ( p1 - p0 ) / distance;
	const vec2  normal{ 0.5f * thickness * direction.y, -0.5f * thickness * direction.x };

	vec2 base = p0 + direction * glm::max( 0.0f, distance - thickness * length );

	Path2d shape;
	shape.moveTo( p0 - normal );
	shape.lineTo( base - normal + direction * thickness * length * concavity );
	shape.lineTo( base - normal * width );
	shape.lineTo( p1 );
	shape.lineTo( base + normal * width );
	shape.lineTo( base + normal + direction * thickness * length * concavity );
	shape.lineTo( p0 + normal );
	shape.close();
	return shape;
}

Path2d Path2d::spiral( const vec2 &center, float innerRadius, float outerRadius, float spacing, float offset )
{
	// Helper struct
	struct Point {
		float x;
		float y;
		float theta;
		float tangent;

		explicit Point( float theta, float offset = 0 )
			: theta( theta )
		{
			float c = math<float>::cos( theta + offset );
			float s = math<float>::sin( theta + offset );
			x = theta * c;
			y = theta * s;
			tangent = math<float>::atan2( s + x, c - y );
		}

		std::pair<vec2, vec2> generate( const Point &previous ) const
		{
			const auto offset = 4 * math<float>::tan( ( theta - previous.theta ) / 4 ) / 3;
			const auto p1 = vec2( math<float>::cos( previous.tangent ) * offset * previous.theta + previous.x, math<float>::sin( previous.tangent ) * offset * previous.theta + previous.y );
			const auto p2 = vec2( math<float>::cos( tangent - float( M_PI ) ) * offset * theta + x, math<float>::sin( tangent - float( M_PI ) ) * offset * theta + y );
			return std::make_pair( p1, p2 );
		}
	};

	const auto step = spacing / ( 2.0f * float( M_PI ) );
	const auto radiansStart = glm::radians( 360 * innerRadius / spacing );
	const auto radiansEnd = glm::radians( 360 * outerRadius / spacing );

	Point p0( radiansStart, offset - radiansStart );

	Path2d shape;
	shape.moveTo( center.x + p0.x * step, center.y + p0.y * step );

	float radians = radiansStart + glm::radians( clamp( radiansStart * step, 3.0f, 60.0f ) ); // Adaptive step size.
	while( radians < radiansEnd ) {
		const auto p3 = Point( radians, offset - radiansStart );
		const auto controls = p3.generate( p0 );
		shape.curveTo( center.x + controls.first.x * step, center.y + controls.first.y * step, center.x + controls.second.x * step, center.y + controls.second.y * step, center.x + p3.x * step, center.y + p3.y * step );

		p0 = p3;

		radians += glm::radians( glm::clamp( radians * step, 3.0f, 60.0f ) ); // Adaptive step size.
	}

	const auto p3 = Point( radiansEnd, offset - radiansStart );
	const auto controls = p3.generate( p0 );

	shape.curveTo( center.x + controls.first.x * step, center.y + controls.first.y * step, center.x + controls.second.x * step, center.y + controls.second.y * step, center.x + p3.x * step, center.y + p3.y * step );

	return shape;
}

void Path2d::reverse()
{
	// The path is empty: nothing to do.
	if( empty() )
		return;

	// Reverse all points.
	std::reverse( mPoints.begin(), mPoints.end() );
	
	if( isClosed() && mSegments.size() > 2 ) {
		std::reverse( mSegments.begin(), mSegments.end() - 1 );
	}
	else if( ! isClosed() && mSegments.size() > 1 ) {
		std::reverse( mSegments.begin(), mSegments.end() );
	}
}

void Path2d::appendSegment( SegmentType segmentType, const vec2 *points )
{
	mSegments.push_back( segmentType );
	// we only copy all of the segments points when we are empty. ie lineto -> line when we are empty
	if( mPoints.empty() )
		std::copy( &points[0], &points[sSegmentTypePointCounts[segmentType]+1], std::back_inserter( mPoints ) );
	else
		std::copy( &points[1], &points[sSegmentTypePointCounts[segmentType]+1], std::back_inserter( mPoints ) );
}

void Path2d::removeSegment( size_t segment )
{
	int firstPoint = 1; // we always skip the first point, since it's a moveTo
	for( size_t s = 0; s < segment; ++s )
		firstPoint += sSegmentTypePointCounts[mSegments[s]];

	int pointCount = sSegmentTypePointCounts[mSegments[segment]];
	mPoints.erase( mPoints.begin() + firstPoint, mPoints.begin() + firstPoint + pointCount );

	mSegments.erase( mSegments.begin() + segment );
}

void Path2d::getSegmentRelativeT( float t, size_t *segment, float *relativeT ) const
{
	if( mSegments.empty() ) {
		*segment = 0;
		if( relativeT )
			*relativeT = 0;
		return;
	}

	if( t <= 0 ) {
		*segment = 0;
		if( relativeT )
			*relativeT = 0;
		return;
	}
	else if( t >= 1 ) {
		*segment = mSegments.size() - 1;
		if( relativeT )
			*relativeT = 1;
		return;
	}

	size_t totalSegments = mSegments.size();
	float segParamLength = 1.0f / totalSegments;
	*segment = static_cast<size_t>( t * totalSegments );
	if( relativeT )
		*relativeT = ( t - *segment * segParamLength ) / segParamLength;
}

vec2 Path2d::getPosition( float t ) const
{
	size_t seg;
	float subSeg;
	getSegmentRelativeT( t, &seg, &subSeg );
	return getSegmentPosition( seg, subSeg );
}

vec2 Path2d::getTangent( float t ) const
{
	size_t seg;
	float subSeg;
	getSegmentRelativeT( t, &seg, &subSeg );
	return getSegmentTangent( seg, subSeg );
}

vec2 Path2d::getSegmentPosition( size_t segment, float t ) const
{
	if( mSegments.empty() )
		return vec2();

	size_t firstPoint = 0;
	for( size_t s = 0; s < segment; ++s )
		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	switch( mSegments[segment] ) {
		case CUBICTO: {
			float t1 = 1 - t;
			return mPoints[firstPoint]*(t1*t1*t1) + mPoints[firstPoint+1]*(3*t*t1*t1) + mPoints[firstPoint+2]*(3*t*t*t1) + mPoints[firstPoint+3]*(t*t*t);
		}
		break;
		case QUADTO: {
			float t1 = 1 - t;
			return mPoints[firstPoint]*(t1*t1) + mPoints[firstPoint+1]*(2*t*t1) + mPoints[firstPoint+2]*(t*t);
		}
		break;
		case LINETO: {
			float t1 = 1 - t;
			return mPoints[firstPoint]*t1 + mPoints[firstPoint+1]*t;
		}
		break;
		case CLOSE: {
			float t1 = 1 - t;
			return mPoints[firstPoint]*t1 + mPoints[0]*t;
		}
		break;
		default:
			throw Path2dExc();
	}
}

vec2 Path2d::getSegmentTangent( size_t segment, float t ) const
{
	if( mSegments.empty() )
		return vec2();

	size_t firstPoint = 0;
	for( size_t s = 0; s < segment; ++s )
		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	switch( mSegments[segment] ) {
		case CUBICTO:
			return calcCubicBezierDerivative( &mPoints[firstPoint], t );
		break;
		case QUADTO:
			return calcQuadraticBezierDerivative( &mPoints[firstPoint], t );
		break;
		case LINETO: {
			return mPoints[firstPoint+1] - mPoints[firstPoint];
		}
		break;
		case CLOSE: {
			return mPoints[0] - mPoints[firstPoint];
		}
		break;
		default:
			throw Path2dExc();
	}
}

namespace {
// This technique is due to Maxim Shemanarev but removes his tangent error estimates
void subdivideQuadratic( float distanceToleranceSqr, const vec2 &p1, const vec2 &p2, const vec2 &p3, int level, vector<vec2> *resultPositions, vector<vec2> *resultTangents )
{
	const int recursionLimit = 17;
	const float collinearEpsilon = 0.0000001f;

	if( level > recursionLimit )
		return;

	vec2 p12 = ( p1 + p2 ) * 0.5f;
	vec2 p23 = ( p2 + p3 ) * 0.5f;
	vec2 p123 = ( p12 + p23 ) * 0.5f;

	float dx = p3.x - p1.x;
	float dy = p3.y - p1.y;
	float d = math<float>::abs(((p2.x - p3.x) * dy - (p2.y - p3.y) * dx));

	if( d > collinearEpsilon ) {
		if( d * d <= distanceToleranceSqr * (dx*dx + dy*dy) ) {
			resultPositions->emplace_back( p123 );
			if( resultTangents )
				resultTangents->emplace_back( p3 - p1 );
			return;
		}
	}
	else { // Collinear case
		float da = dx * dx + dy * dy;
		if( da == 0 ) {
			d = distance2( p1, p2 );
		}
		else {
			d = ((p2.x - p1.x)*dx + (p2.y - p1.y)*dy) / da;
			if( d > 0 && d < 1 ) {
				// Simple collinear case, 1---2---3 - We can leave just two endpoints
				return;
			}

			if(d <= 0)
				d = distance2( p2, p1 );
			else if(d >= 1)
				d = distance2( p2, p3 );
			else
				d = distance2( p2, vec2( p1.x + d * dx, p1.y + d * dy ) );
		}
		if( d < distanceToleranceSqr ) {
			resultPositions->emplace_back( p2 );
			if( resultTangents )
				resultTangents->emplace_back( p3 - p1 );
			return;
		}
	}

	// Continue subdivision
	subdivideQuadratic( distanceToleranceSqr, p1, p12, p123, level + 1, resultPositions, resultTangents );
	subdivideQuadratic( distanceToleranceSqr, p123, p23, p3, level + 1, resultPositions, resultTangents );
}

// This technique is due to Maxim Shemanarev but removes his tangent error estimates
void subdivideCubic( float distanceToleranceSqr, const vec2 &p1, const vec2 &p2, const vec2 &p3, const vec2 &p4, int level, vector<vec2> *resultPositions, vector<vec2> *resultTangents )
{
	const int recursionLimit = 17;
	const float collinearEpsilon = 0.0000001f;

	if( level > recursionLimit )
		return;

	// Calculate all the mid-points of the line segments
	//----------------------

	vec2 p12 = ( p1 + p2 ) * 0.5f;
	vec2 p23 = ( p2 + p3 ) * 0.5f;
	vec2 p34 = ( p3 + p4 ) * 0.5f;
	vec2 p123 = ( p12 + p23 ) * 0.5f;
	vec2 p234 = ( p23 + p34 ) * 0.5f;
	vec2 p1234 = ( p123 + p234 ) * 0.5f;


	// Try to approximate the full cubic curve by a single straight line
	//------------------
	float dx = p4.x - p1.x;
	float dy = p4.y - p1.y;

	float d2 = math<float>::abs(((p2.x - p4.x) * dy - (p2.y - p4.y) * dx));
	float d3 = math<float>::abs(((p3.x - p4.x) * dy - (p3.y - p4.y) * dx));
	float k, da1, da2;

	switch( (int(d2 > collinearEpsilon) << 1) + int(d3 > collinearEpsilon) ) {
		case 0:
			// All collinear OR p1==p4
			k = dx*dx + dy*dy;
			if( k == 0 ) {
				d2 = distance2( p1, p2 );
				d3 = distance2( p4, p3 );
			}
			else {
				k   = 1 / k;
				da1 = p2.x - p1.x;
				da2 = p2.y - p1.y;
				d2  = k * ( da1 * dx + da2 * dy );
				da1 = p3.x - p1.x;
				da2 = p3.y - p1.y;
				d3  = k * ( da1 * dx + da2 * dy );
				if( d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1 ) {
					// Simple collinear case, 1---2---3---4
					// We can leave just two endpoints
					return;
				}
					 if(d2 <= 0) d2 = distance2( p2, p1 );
				else if(d2 >= 1) d2 = distance2( p2, p4 );
				else             d2 = distance2( p2, vec2( p1.x + d2*dx, p1.y + d2*dy ) );

					 if(d3 <= 0) d3 = distance2( p3, p1 );
				else if(d3 >= 1) d3 = distance2( p3, p4 );
				else             d3 = distance2( p3, vec2( p1.x + d3*dx, p1.y + d3*dy ) );
			}
			if(d2 > d3) {
				if( d2 < distanceToleranceSqr ) {
					resultPositions->emplace_back( p2 );
					if( resultTangents )
						resultTangents->emplace_back( p3 - p1 );
					return;
				}
			}
			else {
				if( d3 < distanceToleranceSqr ) {
					resultPositions->emplace_back( p3 );
					if( resultTangents )
						resultTangents->emplace_back( p4 - p2 );
					return;
				}
			}
		break;
		case 1:
			// p1,p2,p4 are collinear, p3 is significant
			if( d3 * d3 <= distanceToleranceSqr * ( dx*dx + dy*dy ) ) {
				resultPositions->emplace_back( p23 );
				if( resultTangents )
					resultTangents->emplace_back( p3 - p2 );
				return;
			}
		break;
		case 2:
			// p1,p3,p4 are collinear, p2 is significant
			if( d2 * d2 <= distanceToleranceSqr * ( dx*dx + dy*dy ) ) {
				resultPositions->emplace_back( p23 );
				if( resultTangents )
					resultTangents->emplace_back( p3 - p2 );
				return;
			}
		break;
		case 3:
			// Regular case
			if( (d2 + d3)*(d2 + d3) <= distanceToleranceSqr * ( dx*dx + dy*dy ) ) {
				resultPositions->emplace_back( p23 );
				if( resultTangents )
					resultTangents->emplace_back( p3 - p2 );
				return;
			}
		break;
	}

	// Continue subdivision
	subdivideCubic( distanceToleranceSqr, p1, p12, p123, p1234, level + 1, resultPositions, resultTangents );
	subdivideCubic( distanceToleranceSqr, p1234, p234, p34, p4, level + 1, resultPositions, resultTangents );
}
} // anonymous namespace

std::vector<vec2> Path2d::subdivide( float approximationScale ) const
{
	std::vector<vec2> result;
	subdivide( &result, nullptr, approximationScale );

	return result;
}

void Path2d::subdivide( std::vector<vec2> *resultPositions, std::vector<vec2> *resultTangents, float approximationScale ) const
{
	if( mSegments.empty() )
		return;

	float distanceToleranceSqr = 0.5f / approximationScale;
	distanceToleranceSqr *= distanceToleranceSqr;

	size_t firstPoint = 0;
	resultPositions->emplace_back( mPoints[0] );
	if( resultTangents )
		resultTangents->emplace_back( mPoints[1] - mPoints[0] );
	for( size_t s = 0; s < mSegments.size(); ++s ) {
		switch( mSegments[s] ) {
			case CUBICTO:
				resultPositions->emplace_back( mPoints[firstPoint] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+1] - mPoints[firstPoint] );
				subdivideCubic( distanceToleranceSqr, mPoints[firstPoint], mPoints[firstPoint+1], mPoints[firstPoint+2], mPoints[firstPoint+3], 0, resultPositions, resultTangents );
				resultPositions->emplace_back( mPoints[firstPoint+3] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+3] - mPoints[firstPoint+2] );
			break;
			case QUADTO:
				resultPositions->emplace_back( mPoints[firstPoint] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+1] - mPoints[firstPoint] );
				subdivideQuadratic( distanceToleranceSqr, mPoints[firstPoint], mPoints[firstPoint+1], mPoints[firstPoint+2], 0, resultPositions, resultTangents );
				resultPositions->emplace_back( mPoints[firstPoint+2] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+2] - mPoints[firstPoint+1] );
			break;
			case LINETO:
				resultPositions->emplace_back( mPoints[firstPoint] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+1] - mPoints[firstPoint] );
				resultPositions->emplace_back( mPoints[firstPoint+1] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[firstPoint+1] - mPoints[firstPoint] );
			break;
			case CLOSE:
				resultPositions->emplace_back( mPoints[firstPoint] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[0] - mPoints[firstPoint] );
				resultPositions->emplace_back( mPoints[0] );
				if( resultTangents )
					resultTangents->emplace_back( mPoints[0] - mPoints[firstPoint] );
			break;
			default:
				throw Path2dExc();
		}

		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	}
}

void Path2d::translate( const vec2 &offset )
{
	for( vector<vec2>::iterator ptIt = mPoints.begin(); ptIt != mPoints.end(); ++ptIt )
		*ptIt += offset;
}

void Path2d::scale( const vec2 &amount, vec2 scaleCenter )
{
	for( vector<vec2>::iterator ptIt = mPoints.begin(); ptIt != mPoints.end(); ++ptIt )
		*ptIt = scaleCenter + vec2( ( ptIt->x - scaleCenter.x ) * amount.x, ( ptIt->y - scaleCenter.y ) * amount.y );
}

void Path2d::transform( const mat3 &matrix )
{
	for( vector<vec2>::iterator ptIt = mPoints.begin(); ptIt != mPoints.end(); ++ptIt )
		*ptIt = vec2( matrix * vec3( *ptIt, 1 ) );
}

Path2d Path2d::transformed( const mat3 &matrix ) const
{
	Path2d result = *this;
	for( vector<vec2>::iterator ptIt = result.mPoints.begin(); ptIt != result.mPoints.end(); ++ptIt )
		*ptIt = vec2( matrix * vec3( *ptIt, 1 ) );
	return result;
}

void Path2d::convertQuadraticsToCubics()
{
	// Convert all QUADTO segments to CUBICTO using degree elevation
	// Degree elevation formula for quadratic to cubic:
	//   Given quadratic P0, P1, P2
	//   Cubic: Q0 = P0
	//          Q1 = P0 + 2/3*(P1 - P0) = P0/3 + 2*P1/3
	//          Q2 = P2 + 2/3*(P1 - P2) = 2*P1/3 + P2/3
	//          Q3 = P2

	// IMPORTANT: Path2d representation:
	// - mPoints[0] is the starting point from moveTo() (implicit, no segment)
	// - mSegments contains only drawing commands (LINETO, QUADTO, CUBICTO, CLOSE)
	// - Each segment refers to subsequent points in mPoints

	if( mPoints.empty() ) {
		return; // Empty path, nothing to convert
	}

	std::vector<SegmentType> newSegments;
	std::vector<vec2> newPoints;

	newSegments.reserve( mSegments.size() );
	newPoints.reserve( mPoints.size() + mSegments.size() ); // May grow if converting quadratics

	// First point is always the starting point (from moveTo)
	newPoints.push_back( mPoints[0] );
	vec2 currentPoint = mPoints[0];
	vec2 subpathStart = mPoints[0];

	size_t pointIndex = 1;  // Start at index 1 (index 0 is the initial moveTo point)

	for( size_t s = 0; s < mSegments.size(); ++s ) {
		SegmentType segType = mSegments[s];

		switch( segType ) {
			case LINETO:
				newSegments.push_back( LINETO );
				newPoints.push_back( mPoints[pointIndex] );
				currentPoint = mPoints[pointIndex];
				pointIndex++;
				break;

			case QUADTO: {
				// Convert quadratic to cubic
				vec2 P0 = currentPoint;
				vec2 P1 = mPoints[pointIndex];
				vec2 P2 = mPoints[pointIndex + 1];

				// Degree elevation
				vec2 Q1 = P0 / 3.0f + P1 * (2.0f / 3.0f);
				vec2 Q2 = P1 * (2.0f / 3.0f) + P2 / 3.0f;

				newSegments.push_back( CUBICTO );
				newPoints.push_back( Q1 );
				newPoints.push_back( Q2 );
				newPoints.push_back( P2 );

				currentPoint = P2;
				pointIndex += 2;
				break;
			}

			case CUBICTO:
				newSegments.push_back( CUBICTO );
				newPoints.push_back( mPoints[pointIndex] );
				newPoints.push_back( mPoints[pointIndex + 1] );
				newPoints.push_back( mPoints[pointIndex + 2] );
				currentPoint = mPoints[pointIndex + 2];
				pointIndex += 3;
				break;

			case CLOSE:
				newSegments.push_back( CLOSE );
				currentPoint = subpathStart;  // Reset current point to start of subpath
				break;

			case MOVETO:
				// MOVETO should not appear in mSegments (it's implicit in mPoints[0])
				// But handle it just in case
				newPoints.push_back( mPoints[pointIndex] );
				currentPoint = mPoints[pointIndex];
				subpathStart = currentPoint;
				pointIndex++;
				break;
		}
	}

	// Replace with converted data
	mSegments = std::move( newSegments );
	mPoints = std::move( newPoints );
}

Path2d Path2d::convertedToCubics() const
{
	Path2d result = *this;
	result.convertQuadraticsToCubics();
	return result;
}

namespace { // getSubPath helpers
void appendChopped( const Path2d &source, size_t segment, float segRelT, bool secondHalf, Path2d *result )
{
	auto sourceSegments = source.getSegments();
	auto sourcePoints = source.getPoints();
	// iterate to first point of segment
	size_t firstPoint = 0;
	for( size_t s = 0; s < segment; ++s )
		firstPoint += Path2d::sSegmentTypePointCounts[sourceSegments[s]];

	vec2 temp[7];
	switch( sourceSegments[segment] ) {
		case Path2d::LINETO:
			if( ! secondHalf ) {
				temp[0] = sourcePoints[firstPoint];
				temp[1] = sourcePoints[firstPoint] + segRelT * ( sourcePoints[firstPoint+1] - sourcePoints[firstPoint] );
			}
			else {
				temp[0] = sourcePoints[firstPoint] + segRelT * ( sourcePoints[firstPoint+1] - sourcePoints[firstPoint] );
				temp[1] = sourcePoints[firstPoint+1];
			}
			result->appendSegment( sourceSegments[segment], &temp[0] );
		break;
		case Path2d::QUADTO:
			chopQuadAt( &sourcePoints[firstPoint], temp, segRelT );
			result->appendSegment( sourceSegments[segment], ( secondHalf ) ? &temp[2] : &temp[0] );
		break;
		case Path2d::CUBICTO:
			chopCubicAt( &sourcePoints[firstPoint], temp, segRelT );
			result->appendSegment( sourceSegments[segment], ( secondHalf ) ? &temp[3] : &temp[0] );
		break;
		case Path2d::CLOSE:
			if( ! secondHalf ) {
				temp[0] = sourcePoints[firstPoint];
				temp[1] = sourcePoints[firstPoint] + segRelT * ( sourcePoints[0] - sourcePoints[firstPoint] );
			}
			else {
				temp[0] = sourcePoints[firstPoint] + segRelT * ( sourcePoints[0] - sourcePoints[firstPoint] );
				temp[1] = sourcePoints[0];
			}
			result->appendSegment( Path2d::LINETO, &temp[0] );
		break;
		default:
			throw Path2dExc();
	}
}

void append( const Path2d &source, size_t segment, Path2d *result )
{
	auto sourceSegments = source.getSegments();
	auto sourcePoints = source.getPoints();
	size_t firstPoint = 0;
	for( size_t s = 0; s < segment; ++s )
		firstPoint += Path2d::sSegmentTypePointCounts[sourceSegments[s]];

	result->appendSegment( sourceSegments[segment], &sourcePoints[firstPoint] );
}
} // getSubPath helpers

Path2d Path2d::getSubPath( float startT, float endT ) const
{
	if( mSegments.empty() )
		return Path2d();

	float startRelT, endRelT;
	size_t startSegment, endSegment;
	getSegmentRelativeT( startT, &startSegment, &startRelT );
	getSegmentRelativeT( endT, &endSegment, &endRelT );

	Path2d result;
	// startT and endT are the same segment
	if( startSegment == endSegment ) {
		// iterate to first point of the segment
		size_t firstPoint = 0;
		for( size_t s = 0; s < startSegment; ++s )
			firstPoint += sSegmentTypePointCounts[mSegments[s]];

		vec2 temp[4];  // Max 4 points needed for cubic
		switch( mSegments[startSegment] ) {
			case LINETO: // trim line
				temp[0] = mPoints[firstPoint] + startRelT * ( mPoints[firstPoint+1] - mPoints[firstPoint] );
				temp[1] = mPoints[firstPoint] + endRelT * ( mPoints[firstPoint+1] - mPoints[firstPoint] );
				result.appendSegment( LINETO, temp );
			break;
			case QUADTO:
				trimQuadAt( &mPoints[firstPoint], temp, startRelT, endRelT );
				result.appendSegment( QUADTO, temp );
			break;
			case CUBICTO:
				trimCubicAt( &mPoints[firstPoint], temp, startRelT, endRelT );
				result.appendSegment( CUBICTO, temp );
			break;
			case CLOSE:
				temp[0] = mPoints[firstPoint] + startRelT * ( mPoints[0] - mPoints[firstPoint] );
				temp[1] = mPoints[firstPoint] + endRelT * ( mPoints[0] - mPoints[firstPoint] );
				result.appendSegment( LINETO, temp );
			break;
			default:
				throw Path2dExc();
		}
	}
	else {
		// append first segment chopped at startRelT
		appendChopped( *this, startSegment, startRelT, true, &result );
		// append all intermediate segments
		for( size_t s = startSegment + 1; s < endSegment; ++s )
			append( *this, s, &result );
		// append last segment chopped at endRelT
		appendChopped( *this, endSegment, endRelT, false, &result );
	}

	return result;
}

Rectf Path2d::calcBoundingBox() const
{
	auto result = Rectf( vec2(), vec2() );
	if( ! mPoints.empty() )	{
		result = Rectf( mPoints[0], mPoints[0] );
		result.include( mPoints );
	}

	return result;
}

// calcPreciseBoundingBox helper routines - now use CinderMath utilities
int	Path2d::calcQuadraticBezierMonotoneRegions( const vec2 p[3], float resultT[2] )
{
	return findQuadraticBezierExtrema( p, resultT );
}

vec2 Path2d::calcQuadraticBezierPos( const vec2 p[3], float t )
{
	return evaluateQuadraticBezier( p, t );
}

vec2 Path2d::calcQuadraticBezierDerivative( const vec2 p[3], float t )
{
	return derivativeQuadraticBezier( p, t );
}

int	Path2d::calcCubicBezierMonotoneRegions( const vec2 p[4], float resultT[4] )
{
	return findCubicBezierExtrema( p, resultT );
}

vec2 Path2d::calcCubicBezierPos( const vec2 p[4], float t )
{
	return evaluateCubicBezier( p, t );
}

vec2 Path2d::calcCubicBezierDerivative( const vec2 p[4], float t )
{
	return derivativeCubicBezier( p, t );
}

Rectf Path2d::calcPreciseBoundingBox() const
{
	if( mPoints.empty() )
		return Rectf();
	else if( mPoints.size() == 1 )
		return Rectf( mPoints[0], mPoints[0] );
	else if( mPoints.size() == 2 )
		return Rectf( mPoints[0], mPoints[1] );

	Rectf result( mPoints[0], mPoints[0] );
	size_t firstPoint = 0;
	for( size_t s = 0; s < mSegments.size(); ++s ) {
		switch( mSegments[s] ) {
			case CUBICTO: {
				float monotoneT[4];
				int monotoneCnt = calcCubicBezierMonotoneRegions( &(mPoints[firstPoint]), monotoneT );
				for( int monotoneIdx = 0; monotoneIdx < monotoneCnt; ++monotoneIdx )
					result.include( calcCubicBezierPos( &(mPoints[firstPoint]), monotoneT[monotoneIdx] ) );
				result.include( mPoints[firstPoint+0] );
				result.include( mPoints[firstPoint+3] );
			}
			break;
			case QUADTO: {
				float monotoneT[2];
				int monotoneCnt = calcQuadraticBezierMonotoneRegions( &(mPoints[firstPoint]), monotoneT );
				for( int monotoneIdx = 0; monotoneIdx < monotoneCnt; ++monotoneIdx )
					result.include( calcQuadraticBezierPos( &(mPoints[firstPoint]), monotoneT[monotoneIdx] ) );
				result.include( mPoints[firstPoint+0] );
				result.include( mPoints[firstPoint+2] );
			}
			break;
			case LINETO:
				result.include( mPoints[firstPoint] );
				result.include( mPoints[firstPoint+1] );
			break;
			case CLOSE:
			break;
			default:
				throw Path2dExc();
		}

		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	}

	return result;
}

bool Path2d::calcClockwise() const
{
	// See: https://en.wikipedia.org/wiki/Curve_orientation
	size_t index = 0;
	for( size_t i = 1; i < mPoints.size(); ++i ) {
		if( mPoints.at( i ).x < mPoints.at( index ).x || ( approxEqual( mPoints.at( i ).x, mPoints.at( index ).x ) && mPoints.at( i ).y < mPoints.at( index ).y ) )
			index = i;
	}

	const auto &a = getPoint(index);
	const auto &b = getPointBefore(index);
	const auto &c = getPointAfter(index);
	const auto sign = glm::sign( (b.x-a.x)*(c.y-a.y)-(c.x-a.x)*(b.y-a.y) );

	return sign < 0;
}

namespace {
float calcCubicBezierSpeed( const vec2 p[3], float t )
{
	return length( Path2d::calcCubicBezierDerivative( p, t ) );
}

float calcQuadraticBezierSpeed( const vec2 p[3], float t )
{
	return length( Path2d::calcQuadraticBezierDerivative( p, t ) );
}
} // anonymous namespace

namespace { // Path2d::contains() helpers
int signAsInt( float x ) { return x < 0 ? -1 : (x > 0); }
bool between( float a, float b, float c ) { return (a - b) * (c - b) <= 0; }
bool isMonoQuad( float y0, float y1, float y2 )
{
	if( y0 == y1 )
		return true;
	if( y0 < y1 )
		return y1 <= y2;
	else
		return y1 >= y2;
}

int isNotMonotonic( float a, float b, float c )
{
    float ab = a - b;
    float bc = b - c;
    if( ab < 0 )
        bc = -bc;

    return ab == 0 || bc < 0;
}

int validUnitDivide( float numer, float denom, float* ratio )
{
    if( numer < 0 ) {
        numer = -numer;
        denom = -denom;
    }

    if( denom == 0 || numer == 0 || numer >= denom )
        return 0;

    float r = numer / denom;
    if( std::isnan( r ) )
        return 0;
    if( r == 0 ) // catch underflow if numer <<<< denom
        return 0;

	*ratio = r;
    return 1;
}

// Subdivides quadratic curve at 't'. First segment is ( dst[0], dst[1], dst[2] ), second is ( dst[2], dst[3], dst[4] )
void chopQuadAt( const vec2 src[3], vec2 dst[5], float t )
{
	subdivideQuadraticBezier( src, dst, t );
}

// Trims quadratic curve starting at 't0' and ending at 't1'.
void trimQuadAt( const vec2 src[3], vec2 dst[3], float t0, float t1 )
{
	float it0 = 1 - t0;
	float it1 = 1 - t1;
	dst[0] = src[0]*(it0*it0) + src[1]*(2*t0*it0) + src[2]*(t0*t0);
	vec2 m = it0 * src[1] + t0 * src[2];
	float u1 = ( t1 - t0 ) / ( 1 - t0 );
	dst[1] = (1 - u1) * dst[0] + u1 * m;
	dst[2] = src[0]*(it1*it1) + src[1]*(2*t1*it1) + src[2]*(t1*t1);
}

//Q = -1/2 (B + sign(B) sqrt[B*B - 4*A*C])
//x1 = Q / A
//x2 = C / Q
int findUnitQuadRoots( float A, float B, float C, float roots[2] )
{
    if( A == 0 ) {
        return validUnitDivide( -C, B, roots );
    }

    float* r = roots;

    float R = B*B - 4*A*C;
    if( R < 0 || ! std::isfinite( R ) ) {  // complex roots
        // if R is infinite, it's possible that it may still produce
        // useful results if the operation was repeated in doubles
        // the flipside is determining if the more precise answer
        // isn't useful because surrounding machinery (e.g., subtracting
        // the axis offset from C) already discards the extra precision
        // more investigation and unit tests required...
        return 0;
    }
    R = sqrtf( R );

    float Q = (B < 0) ? -(B-R)/2 : -(B+R)/2;
    r += validUnitDivide(Q, A, r);
    r += validUnitDivide(C, Q, r);
    if( r - roots == 2 ) {
        if( roots[0] > roots[1] )
            std::swap( roots[0], roots[1] );
        else if( roots[0] == roots[1] )  // nearly-equal?
            r -= 1; // skip the double root
    }
    return (int)(r - roots);
}

void flattenDoubleQuadExtrema( float coords[14] )
{
    coords[2] = coords[6] = coords[4];
}

void flattenDoubleCubicExtrema( float coords[14] )
{
    coords[4] = coords[8] = coords[6];
}

template<size_t N>
void findMinMaxX( const vec2 pts[], float* minPtr, float* maxPtr) {
    float minX, maxX;
    minX = maxX = pts[0].x;
    for( size_t i = 1; i < N; ++i ) {
        minX = std::min( minX, pts[i].x );
        maxX = std::max( maxX, pts[i].x );
    }
    *minPtr = minX;
    *maxPtr = maxX;
}

// Subdivides cubic curve at 't'. First segment is ( dst[0], dst[1], dst[2], dst[3] ), second is ( dst[3], dst[4], dst[5], dst[6] )
void chopCubicAt( const vec2 src[4], vec2 dst[7], float t )
{
	subdivideCubicBezier( src, dst, t );
}

// Trims cubic curve starting at 't0' and ending at 't1'.
void trimCubicAt( const vec2 src[4], vec2 dst[4], float t0, float t1 )
{
	float u0 = 1.0f - t0;
	float u1 = 1.0f - t1;

	vec2 qa =  src[0]*u0*u0 + src[1]*2.0f*t0*u0 + src[2]*t0*t0;
	vec2 qb =  src[0]*u1*u1 + src[1]*2.0f*t1*u1 + src[2]*t1*t1;
	vec2 qc = src[1]*u0*u0 + src[2]*2.0f*t0*u0 + src[3]*t0*t0;
	vec2 qd = src[1]*u1*u1 + src[2]*2.0f*t1*u1 + src[3]*t1*t1;

	dst[0] = qa*u0 + qc*t0;
	dst[1] = qa*u1 + qc*t1;
	dst[2] = qb*u0 + qd*t0;
	dst[3] = qb*u1 + qd*t1;
}

void chopCubicAt( const vec2 src[4], vec2 dst[], const float tValues[], int roots )
{
	if( roots == 0 ) { // nothing to chop
		memcpy( dst, src, 4 * sizeof(vec2) );
	}
	else {
		float t = tValues[0];
		vec2 tmp[4];

		for( int i = 0; i < roots; i++ ) {
			chopCubicAt( src, dst, t );
			if( i == roots - 1 ) {
				break;
			}

			dst += 3;
			// have src point to the remaining cubic (after the chop)
			memcpy( tmp, dst, 4 * sizeof(vec2) );
			src = tmp;

			// watch out in case the renormalized t isn't in range
			if( ! validUnitDivide( tValues[i+1] - tValues[i], 1.0f - tValues[i], &t ) ) {
				// if we can't, just create a degenerate cubic
				dst[4] = dst[5] = dst[6] = src[3];
				break;
			}
		}
	}
}

bool chopMonoAtY( const vec2 pts[4], float y, float* t )
{
	float ycrv[4];
	ycrv[0] = pts[0].y - y;
	ycrv[1] = pts[1].y - y;
	ycrv[2] = pts[2].y - y;
	ycrv[3] = pts[3].y - y;

    // Check that the endpoints straddle zero.
	float tNeg, tPos;	// Negative and positive function parameters.
	if( ycrv[0] < 0 ) {
		if( ycrv[3] < 0 )
			return false;
		tNeg = 0;
		tPos = 1.0f;
	}
	else if( ycrv[0] > 0 ) {
		if( ycrv[3] > 0 )
			return false;
		tNeg = 1.0f;
		tPos = 0;
	}
	else {
		*t = 0;
		return true;
	}

	const float tol = 1.0f / 65536;  // 1 for fixed, 1e-5 for float.
	int iters = 0;
	do {
		float tMid = (tPos + tNeg) / 2;
		float y01   = lerp( ycrv[0], ycrv[1], tMid );
		float y12   = lerp( ycrv[1], ycrv[2], tMid );
		float y23   = lerp( ycrv[2], ycrv[3], tMid );
		float y012  = lerp( y01, y12, tMid );
		float y123  = lerp( y12, y23, tMid );
		float y0123 = lerp( y012, y123, tMid );
		if( y0123 == 0 ) {
			*t = tMid;
			return true;
		}
		if (y0123 < 0)	tNeg = tMid;
		else			tPos = tMid;
		++iters;
	} while( ! (fabsf(tPos - tNeg) <= tol) );

	*t = (tNeg + tPos) / 2;
	return true;
}

// Cubic'(t) = At^2 + Bt + C, where
// A = 3(-a + 3(b - c) + d)
// B = 6(a - 2b + c)
// C = 3(b - a)
// Solve for t, keeping only those that fit betwee 0 < t < 1
int findCubicExtrema( float a, float b, float c, float d, float tValues[2] )
{
    // we divide A,B,C by 3 to simplify
	float A = d - a + 3*(b - c);
	float B = 2*(a - b - b + c);
	float C = b - a;

    return findUnitQuadRoots( A, B, C, tValues );
}

// Given 4 points on a cubic bezier, chop it into 1, 2, 3 beziers such that
// the resulting beziers are monotonic in Y. This is called by the scan
// converter.  Depending on what is returned, dst[] is treated as follows:
// 0   dst[0..3] is the original cubic
// 1   dst[0..3] and dst[3..6] are the two new cubics
// 2   dst[0..3], dst[3..6], dst[6..9] are the three new cubics
// If dst == null, it is ignored and only the count is returned.
int chopCubicAtYExtrema( const vec2 src[4], vec2 dst[10] )
{
	float tValues[2];
	int roots = findCubicExtrema( src[0].y, src[1].y, src[2].y, src[3].y, tValues );

	chopCubicAt( src, dst, tValues, roots );
	if( dst && roots > 0 ) {
		// we do some cleanup to ensure our Y extrema are flat
		flattenDoubleCubicExtrema( &dst[0].y );
		if( roots == 2 ) {
			flattenDoubleCubicExtrema( &dst[3].y );
		}
	}
	return roots;
}

// Returns 0 for 1 quad, and 1 for two quads, either way the answer is stored in dst[].
// Guarantees that the 1/2 quads will be monotonic.
int chopQuadAtYExtrema( const vec2 src[3], vec2 dst[5] )
{
	float a = src[0].y;
	float b = src[1].y;
	float c = src[2].y;

	if( isNotMonotonic( a, b, c ) ) {
		float tValue;
		if( validUnitDivide( a - b, a - b - b + c, &tValue ) ) {
			chopQuadAt( src, dst, tValue );
			flattenDoubleQuadExtrema( &dst[0].y );
			return 1;
		}
		// if we get here, we need to force dst to be monotonic, even though
		// we couldn't compute a unit_divide value (probably underflow).
		b = fabsf(a - b) < fabsf(b - c) ? a : c;
	}

	dst[0] = { src[0].x, a };
	dst[1] = { src[1].x, b };
	dst[2] = { src[2].x, c };
	return 0;
}

float evalCubicPts( float c0, float c1, float c2, float c3, float t )
{
	float A = c3 + 3*(c1 - c2) - c0;
	float B = 3*(c2 - c1 - c1 + c0);
	float C = 3*(c1 - c0);
	float D = c0;
	return ((A * t + B) * t + C) * t + D;//evalCubicCoeff( A, B, C, D, t );
}

bool checkOnCurve( const vec2 &test, const vec2& start, const vec2& end )
{
    if( start.y == end.y )
        return between( start.x, test.x, end.x ) && test.x != end.x;
	else
        return test.x == start.x && test.y == start.y;
}

int windingLine( const vec2 points[2], const vec2 &test, int *onCurveCount )
{
	float x0 = points[0].x;
	float y0 = points[0].y;
	float x1 = points[1].x;
	float y1 = points[1].y;

	float dy = y1 - y0;

	int dir = 1;
	if( y0 > y1 ) {
		std::swap( y0, y1 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y1 ) {
		return 0;
	}
	if( checkOnCurve( test, points[0], points[1] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y1 ) {
		return 0;
	}
	float cross = (x1 - x0) * (test.y - points[0].y) - dy * ( test.x - x0 );

	if( ! cross ) {
		// zero cross means the point is on the line, and since the case where
		// y of the query point is at the end point is handled above, we can be
		// sure that we're on the line (excluding the end point) here
		if( test.x != x1 || test.y != points[1].y ) {
			*onCurveCount += 1;
		}
		dir = 0;
	}
	else if( signAsInt(cross) == dir ) {
		dir = 0;
	}

	return dir;
}

int windingMonoQuad( const vec2 pts[], const vec2 &test, int* onCurveCount )
{
	float y0 = pts[0].y;
	float y2 = pts[2].y;

	int dir = 1;
	if( y0 > y2 ) {
		std::swap( y0, y2 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y2 ) {
		return 0;
	}
	if( checkOnCurve( test, pts[0], pts[2] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y2 ) {
		return 0;
	}

	float roots[2];
	int n = findUnitQuadRoots( pts[0].y - 2 * pts[1].y + pts[2].y,
				2 * (pts[1].y - pts[0].y),
				pts[0].y - test.y,
				roots);

	float xt;
	if( 0 == n ) {
		// zero roots are returned only when y0 == y
		// Need [0] if dir == 1
		// and  [2] if dir == -1
		xt = pts[1 - dir].x;
	}
	else {
		float t = roots[0];
		float C = pts[0].x;
		float A = pts[2].x - 2 * pts[1].x + C;
		float B = 2 * (pts[1].x - C);
		xt = (A * t + B) * t + C;
	}
	if( fabs( xt - test.x ) < ( 1.0f / ( 1 << 12 ) ) ) {
		if( test.x != pts[2].x || test.y != pts[2].y ) {  // don't test end points; they're start points
			*onCurveCount += 1;
			return 0;
		}
	}
	return xt < test.x ? dir : 0;
}

int windingQuad( const vec2 points[], const vec2 &test, int *onCurveCount )
{
	vec2 dst[5];
	int n = 0;

	if( ! isMonoQuad( points[0].y, points[1].y, points[2].y ) ) {
		n = chopQuadAtYExtrema( points, dst );
		points = dst;
	}
	int w = windingMonoQuad( points, test, onCurveCount );
	if( n > 0 ) {
		w += windingMonoQuad( &points[2], test, onCurveCount );
	}

	return w;
}

int windingMonoCubic( const vec2 pts[], const vec2 &test, int *onCurveCount )
{
	float y0 = pts[0].y;
	float y3 = pts[3].y;

	int dir = 1;
	if( y0 > y3 ) {
		std::swap( y0, y3 );
		dir = -1;
	}
	if( test.y < y0 || test.y > y3 ) {
		return 0;
	}
	if( checkOnCurve( test, pts[0], pts[3] ) ) {
		*onCurveCount += 1;
		return 0;
	}
	if( test.y == y3 ) {
		return 0;
	}

	// quickreject or quickaccept
	float minX, maxX;
	findMinMaxX<4>( pts, &minX, &maxX );
	if( test.x < minX ) {
		return 0;
	}
	if( test.x > maxX ) {
		return dir;
	}

	// compute the actual x(t) value
	float t;
	if( ! chopMonoAtY( pts, test.y, &t ) ) {
		return 0;
	}
	float xt = evalCubicPts( pts[0].x, pts[1].x, pts[2].x, pts[3].x, t );
	if( fabsf( xt - test.x ) < ( 1.0f / ( 1 << 12 ) ) ) {
		if( test.x != pts[3].x || test.y != pts[3].y ) {  // don't test end points; they're start points
			*onCurveCount += 1;
			return 0;
		}
	}
	return xt < test.x ? dir : 0;
}

int windingCubic( const vec2 pts[], const vec2 &test, int *onCurveCount )
{
	vec2 dst[10];
	int n = chopCubicAtYExtrema( pts, dst );
	int w = 0;
	for( int i = 0; i <= n; ++i )
		w += windingMonoCubic( &dst[i * 3], test, onCurveCount );

	return w;
}
} // anonymous namespace Path2d::contains() helpers

int Path2d::calcWinding( const ci::vec2 &pt, int *onCurveCount ) const
{
	int w = 0;
	size_t firstPoint = 0;
	for( size_t s = 0; s < getSegments().size(); ++s ) {
		switch( getSegmentType( s ) ) {
			case Path2d::LINETO:
				w += windingLine( &mPoints[firstPoint], pt, onCurveCount );
			break;
			case Path2d::QUADTO:
				w += windingQuad( &mPoints[firstPoint], pt, onCurveCount );
			break;
			case Path2d::CUBICTO:
				w += windingCubic( &(mPoints[firstPoint]), pt, onCurveCount );
			break;
			case Path2d::CLOSE: // closed is always assumed and is handled below
			break;
			default:
				throw Path2dExc();
			break;
		}

		firstPoint += sSegmentTypePointCounts[getSegments()[s]];
	}

	// handle close
	vec2 temp[2] = { mPoints[getNumPoints() - 1], mPoints[0] };
	w += windingLine( temp, pt, onCurveCount );

	return w;
}

bool Path2d::contains( const vec2 &pt, bool evenOddFill ) const
{
	int onCurveCount = 0;
	int w = calcWinding( pt, &onCurveCount );

	if( evenOddFill )
		w &= 1;
	if( w )
		return true;

	if( onCurveCount <= 1 )
		return onCurveCount > 0;
	if( (onCurveCount & 1) || evenOddFill )
		return (onCurveCount & 1) > 0;

	return false;
}

float Path2d::calcDistance( const vec2 &pt ) const
{
	float distance = FLT_MAX;

	size_t firstPoint = 0;
	for( size_t s = 0; s < mSegments.size(); ++s ) {
		distance = glm::min( calcDistance( pt, s, firstPoint ), distance );
		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	}

	return distance;
}

float Path2d::calcDistance( const vec2 &pt, size_t segment, size_t firstPoint ) const
{
	return glm::distance( pt, calcClosestPoint( pt, segment, firstPoint ) );
}

float Path2d::calcDistance( const vec2 &pt, size_t segment ) const
{
	return calcDistance( pt, segment, 0 );
}

float Path2d::calcSignedDistance( const vec2 &pt ) const
{
	if( contains( pt ) )
		return -calcDistance( pt );
	else
		return calcDistance( pt );
}


vec2 Path2d::calcClosestPoint( const vec2 &pt ) const
{
	vec2 result;
	float distance2 = FLT_MAX;

	size_t firstPoint = 0;
	for( size_t s = 0; s < mSegments.size(); ++s ) {
		vec2 p = calcClosestPoint( pt, s, firstPoint );
		float d = glm::distance2( pt, p );
		if( d < distance2 ) {
			result = p;
			distance2 = d;
		}
		firstPoint += sSegmentTypePointCounts[mSegments[s]];
	}

	return result;
}

vec2 Path2d::calcClosestPoint( const vec2 &pt, size_t segment, size_t firstPoint ) const
{
	if( firstPoint == 0 ) {
		for( size_t s = 0; s < segment; ++s )
			firstPoint += sSegmentTypePointCounts[mSegments[s]];
	}

	switch( mSegments[segment] ) {
		case CUBICTO:
			return getClosestPointCubic( &mPoints[firstPoint], pt );
		case QUADTO:
			return getClosestPointQuadratic( &mPoints[firstPoint], pt );
		case LINETO:
			return getClosestPointLinear( &mPoints[firstPoint], pt );
		case CLOSE:
			return getClosestPointLinear( mPoints[firstPoint], mPoints[0], pt );
		default:
			return vec2();
	}
}

float Path2d::calcLength() const
{
	float result = 0;

	size_t firstPoint = 0;
	for( size_t s = 0; s < mSegments.size(); ++s ) {
		switch( mSegments[s] ) {
			case CUBICTO:
				result += gaussLegendreIntegral7<float>( 0, 1, std::bind( calcCubicBezierSpeed, &mPoints[firstPoint], std::placeholders::_1 ) );
			break;
			case QUADTO:
				result += gaussLegendreIntegral7<float>( 0, 1, std::bind( calcQuadraticBezierSpeed, &mPoints[firstPoint], std::placeholders::_1 ) );
			break;
			case LINETO:
				result += distance( mPoints[firstPoint], mPoints[firstPoint + 1] );
			break;
			case CLOSE:
				result += distance( mPoints[firstPoint], mPoints[0] );
			break;
			default:
				;
		}

		firstPoint += Path2d::sSegmentTypePointCounts[mSegments[s]];
	}

	return result;
}

float Path2d::calcSegmentLength( size_t segment, float minT, float maxT ) const
{
	if( segment >= mSegments.size() )
		return 0;

	size_t firstPoint = 0;
	for( size_t s = 0; s < segment; ++s )
		firstPoint += sSegmentTypePointCounts[mSegments[s]];

	switch( mSegments[segment] ) {
		case CUBICTO:
			return gaussLegendreIntegral7<float>( minT, maxT, std::bind( calcCubicBezierSpeed, &mPoints[firstPoint], std::placeholders::_1 ) );
		break;
		case QUADTO:
			return gaussLegendreIntegral7<float>( minT, maxT, std::bind( calcQuadraticBezierSpeed, &mPoints[firstPoint], std::placeholders::_1 ) );
		break;
		case LINETO:
			return distance( mPoints[firstPoint], mPoints[firstPoint + 1] ) * ( maxT - minT );
		break;
		case CLOSE:
			return distance( mPoints[firstPoint], mPoints[0] ) * ( maxT - minT );
		break;
		default:
			return 0;
	}
}

float Path2d::calcNormalizedTime( float relativeTime, bool wrap, float tolerance, int maxIterations ) const
{
	if( mSegments.empty() )
		return 0;

	// Wrap relative time if necessary
	if( relativeTime >= 1 ) {
		if( wrap )
			relativeTime = math<float>::fmod( relativeTime, 1.0f );
		else
			return 1.0f;
	}
	else if( relativeTime < 0 ) {
		if( wrap )
			relativeTime = 1.0f - math<float>::fmod( math<float>::abs( relativeTime ), 1.0f );
		else
			return 0.0f;
	}

	float targetLength = calcLength() * math<float>::clamp( relativeTime, 0.0f, 1.0f );
	// test for 0-length Path2d
	if( targetLength < 0.0001f )
		return 0;

	int currentSegment = 0;
	float currentSegmentLength = calcSegmentLength( 0 );
	while( targetLength > currentSegmentLength ) {
		targetLength -= currentSegmentLength;
		currentSegmentLength = calcSegmentLength( ++currentSegment );
	}

	return segmentSolveTimeForDistance( currentSegment, currentSegmentLength, targetLength, tolerance, maxIterations );
}

float Path2d::calcTimeForDistance( float distance, bool wrap, float tolerance, int maxIterations ) const
{
	if( mSegments.empty() )
		return 0;

	float totalLength = calcLength();
	if( distance > totalLength ) {
		if( wrap )
			distance = fmodf( distance, totalLength );
		else
			return 1.0f;
	}

	// Iterate the segments to find the segment defining the range containing our targetLength
	int currentSegment = 0;
	float currentSegmentLength = calcSegmentLength( 0 );
	while( distance > currentSegmentLength ) {
		distance -= currentSegmentLength;
		currentSegmentLength = calcSegmentLength(++currentSegment);
	}

	return segmentSolveTimeForDistance( currentSegment, currentSegmentLength, distance, tolerance, maxIterations );
}

// Helper for calcTimeForDistance() that uses pre-computed arc lengths (for performance)
float Path2d::calcTimeForDistanceCached( float distance, float totalLength, const std::vector<float>& segmentLengths, bool wrap, float tolerance, int maxIterations ) const
{
	if( mSegments.empty() || segmentLengths.empty() )
		return 0;

	if( distance > totalLength ) {
		if( wrap )
			distance = fmodf( distance, totalLength );
		else
			return 1.0f;
	}

	// Iterate the segments using cached lengths
	int currentSegment = 0;
	float currentSegmentLength = segmentLengths[0];
	while( distance > currentSegmentLength && currentSegment < (int)segmentLengths.size() - 1 ) {
		distance -= currentSegmentLength;
		currentSegmentLength = segmentLengths[++currentSegment];
	}

	return segmentSolveTimeForDistance( currentSegment, currentSegmentLength, distance, tolerance, maxIterations );
}

float Path2d::segmentSolveTimeForDistance( size_t segment, float segmentLength, float segmentRelativeDistance, float tolerance, int maxIterations ) const
{
	// initialize bisection endpoints
	float a = 0, b = 1;
	float p = segmentRelativeDistance / segmentLength;    // make first guess

	// we want to calculate a value 'p' such that segmentLength( mCurrentSegment, mCurrentT, mCurrentT + p ) == lengthIncrement

	// iterate and look for zeros
	float lastArcLength = 0;
	float currentT = 0;
	for( int i = 0; i < maxIterations; ++i ) {
		// compute function value and test against zero
		lastArcLength = calcSegmentLength( segment, currentT, currentT + p );
		float delta = lastArcLength - segmentRelativeDistance;
		if( math<float>::abs( delta ) < tolerance ) {
			break;
		}

		 // update bisection endpoints
		if( delta < 0 )
			a = p;
		else
			b = p;

		// get speed along curve
		const float speed = length( getSegmentTangent( segment, currentT + p ) );

		// if result will lie outside [a,b]
		if( ((p-a)*speed - delta)*((p-b)*speed - delta) > -tolerance )
			p = 0.5f*(a+b);	// do bisection
		else
			p -= delta/speed; // otherwise Newton-Raphson
	}
	// If we failed to converge, hopefully 'p' is close enough

	return ( p + segment ) / (float)mSegments.size();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Path2dCalcCache
Path2dCalcCache::Path2dCalcCache( const Path2d &path )
	: mPath( path ), mLength( path.calcLength() )
{
	for( size_t i = 0; i < mPath.getNumSegments(); ++i )
		mSegmentLengths.push_back( mPath.calcSegmentLength( i ) );
}

float Path2dCalcCache::calcNormalizedTime( float relativeTime, bool wrap, float tolerance, int maxIterations ) const
{
	if( mPath.mSegments.empty() )
		return 0;

	// Wrap relative time if necessary
	if( relativeTime >= 1 ) {
		if( wrap )
			relativeTime = math<float>::fmod( relativeTime, 1.0f );
		else
			return 1.0f;
	}
	else if( relativeTime < 0 ) {
		if( wrap )
			relativeTime = 1.0f - math<float>::fmod( math<float>::abs( relativeTime ), 1.0f );
		else
			return 0.0f;
	}

	// We're looking for a length that is relativeTime * totalPathLength
	float targetLength = mLength * math<float>::clamp( relativeTime, 0.0f, 1.0f );

	// Iterate the segments to find the segment defining the range containing our targetLength
	int currentSegment = 0;
	float currentSegmentLength = mSegmentLengths[0];
	while( targetLength > currentSegmentLength ) {
		targetLength -= currentSegmentLength;
		currentSegmentLength = mSegmentLengths[++currentSegment];
	}

	return mPath.segmentSolveTimeForDistance( currentSegment, currentSegmentLength, targetLength, tolerance, maxIterations );
}

float Path2dCalcCache::calcTimeForDistance( float distance, bool wrap, float tolerance, int maxIterations ) const
{
	if( mPath.mSegments.empty() || mLength == 0 )
		return 0;

	if( distance > mLength ) {
		if( wrap )
			distance = fmodf( distance, mLength );
		else
			return 1.0f;
	}

	// Iterate the segments to find the segment defining the range containing our targetLength
	int currentSegment = 0;
	float currentSegmentLength = mSegmentLengths[0];
	while( distance > currentSegmentLength ) {
		distance -= currentSegmentLength;
		currentSegmentLength = mSegmentLengths[++currentSegment];
	}

	return mPath.segmentSolveTimeForDistance( currentSegment, currentSegmentLength, distance, tolerance, maxIterations );
}

// ============================================================================
// OFFSET CURVE IMPLEMENTATION
// Based on Raph Levien's approach: https://raphlinus.github.io/curves/2022/09/09/parallel-beziers.html
// ============================================================================

namespace {

// Calculate the normal vector (perpendicular, rotated 90° clockwise) for a 2D vector
inline vec2 calcOffsetNormal( const vec2& tangent )
{
	return vec2( tangent.y, -tangent.x );
}

// Safe tangent calculation that handles degenerate cases (zero-length segments)
inline bool calcSafeTangent( const vec2& p0, const vec2& p1, vec2& outTangent )
{
	vec2 diff = p1 - p0;
	float lenSq = diff.x * diff.x + diff.y * diff.y;
	if( lenSq > 1e-8f ) {  // ~0.0001^2
		float len = std::sqrt( lenSq );
		outTangent = diff / len;
		return true;
	}
	return false;
}

// Offset a linear segment - this is straightforward
bool offsetLinearSegment( const vec2& p0, const vec2& p1, float distance, vec2& out0, vec2& out1 )
{
	vec2 tangent;
	if( !calcSafeTangent( p0, p1, tangent ) ) {
		return false;  // Degenerate segment
	}

	vec2 normal = calcOffsetNormal( tangent );
	vec2 offset = normal * distance;

	out0 = p0 + offset;
	out1 = p1 + offset;
	return true;
}

// ==================================================================================
// KURBO OFFSET ALGORITHM IMPLEMENTATION
// Based on Raph Levien's parallel Bezier curve algorithm
// https://raphlinus.github.io/curves/2022/09/09/parallel-beziers.html
// ==================================================================================

namespace {
	const int MAX_OFFSET_DEPTH = 8;
	const int N_LSE = 12;  // Number of sample points for least-squares refinement (increased from 8)
	const float BLEND = 1e-3f;  // Blending factor for tangent vs. normal error

	// Helper struct for cubic offset computation (Kurbo algorithm)
	struct CubicOffsetHelper {
		vec2 c[4];    // Original cubic control points
		vec2 q[3];    // Derivative (quadratic)
		float d;      // Offset distance
		float c0, c1, c2;  // Cusp detection coefficients
		float tolerance;

		CubicOffsetHelper(const vec2* controlPoints, float distance, float tol)
			: d(distance), tolerance(tol)
		{
			// Copy control points
			for(int i = 0; i < 4; ++i)
				c[i] = controlPoints[i];

			// Compute derivative (quadratic)
			q[0] = 3.0f * (c[1] - c[0]);
			q[1] = 3.0f * (c[2] - c[1]);
			q[2] = 3.0f * (c[3] - c[2]);

			// Compute cusp detection coefficients (matching Kurbo)
			// Use the derivative control points q[0], q[1], q[2] directly
			float p1xp0 = q[1].x * q[0].y - q[1].y * q[0].x;
			float p2xp0 = q[2].x * q[0].y - q[2].y * q[0].x;
			float p2xp1 = q[2].x * q[1].y - q[2].y * q[1].x;

			// NOTE: Kurbo uses left-handed normals, we use right-handed.
			// Negate distance to compensate for the sign flip.
			float d2 = -2.0f * distance;
			c0 = d2 * p1xp0;
			c1 = d2 * (p2xp0 - 2.0f * p1xp0);
			c2 = d2 * (p2xp1 - p2xp0 + p1xp0);
		}

		// Evaluate the cusp sign at parameter t
		// Zero crossing indicates a cusp
		float cuspSign(float t) const
		{
			// Evaluate derivative at t
			float s = 1.0f - t;
			vec2 deriv = s*s * q[0] + 2.0f*s*t * q[1] + t*t * q[2];
			float ds2 = glm::dot(deriv, deriv);

			if(ds2 < 1e-12f) return 1.0f;  // Degenerate case

			// Cusp value: ((c2*t + c1)*t + c0) / (ds2 * sqrt(ds2)) + 1.0
			return ((c2 * t + c1) * t + c0) / (ds2 * std::sqrt(ds2)) + 1.0f;
		}

		// Evaluate the offset curve at parameter t
		vec2 evalOffset(float t) const
		{
			// Evaluate original curve
			float s = 1.0f - t;
			vec2 point = s*s*s * c[0] + 3.0f*s*s*t * c[1] + 3.0f*s*t*t * c[2] + t*t*t * c[3];

			// Evaluate derivative
			vec2 deriv = s*s * q[0] + 2.0f*s*t * q[1] + t*t * q[2];
			float derivLen = glm::length(deriv);

			if(derivLen < 1e-6f) return point;

			// Offset normal
			vec2 normal = calcOffsetNormal(deriv / derivLen);
			return point + normal * d;
		}

		// Recursive offset computation
		void offsetRec(float t0, float t1, const vec2& utan0, const vec2& utan1,
		               Path2d& result, int depth = 0)
		{
			// Check for cusps in the interval
			float sign0 = cuspSign(t0);
			float sign1 = cuspSign(t1);

			if(sign0 * sign1 < 0.0f && depth < MAX_OFFSET_DEPTH) {
				// Cusp detected - subdivide at the cusp
				float tMid = (t0 + t1) * 0.5f;

				// Bisect to find exact cusp location
				float tLow = t0, tHigh = t1;
				for(int i = 0; i < 10; ++i) {
					float t = (tLow + tHigh) * 0.5f;
					float sign = cuspSign(t);
					if(sign * sign0 > 0.0f)
						tLow = t;
					else
						tHigh = t;
				}
				float tCusp = (tLow + tHigh) * 0.5f;

				// Subdivide at cusp
				vec2 utanCusp = evalUnitTangent(tCusp);
				offsetRec(t0, tCusp, utan0, utanCusp, result, depth + 1);
				offsetRec(tCusp, t1, utanCusp, utan1, result, depth + 1);
				return;
			}

			// No cusp - try to fit a cubic
			// 1. Compute endpoint offset positions
			vec2 p0 = evalOffset(t0);
			vec2 p3 = evalOffset(t1);

			// 2. Use arc drawing for initial approximation
			//    Arc drawing: use angle bisection between endpoint tangents
			float angle0 = std::atan2(utan0.y, utan0.x);
			float angle1 = std::atan2(utan1.y, utan1.x);

			// Normalize angle difference
			float angleDiff = angle1 - angle0;
			while(angleDiff > (float)M_PI) angleDiff -= 2.0f * (float)M_PI;
			while(angleDiff < -(float)M_PI) angleDiff += 2.0f * (float)M_PI;

			// Arc radius estimation from chord length
			float chordLen = glm::length(p3 - p0);
			float radius = (std::abs(angleDiff) > 0.001f) ?
				chordLen / (2.0f * std::sin(std::abs(angleDiff) / 2.0f)) :
				chordLen;

			// Initial control arm lengths (tangential and normal)
			float a = radius * 4.0f * std::tan(angleDiff / 4.0f) / 3.0f;
			float b = a;
			float a_perp = 0.0f;  // Normal component for p1
			float b_perp = 0.0f;  // Normal component for p2

			// Unit normals at endpoints
			vec2 unorm0 = vec2(-utan0.y, utan0.x);
			vec2 unorm1 = vec2(-utan1.y, utan1.x);

			vec2 p1 = p0 + utan0 * a + unorm0 * a_perp;
			vec2 p2 = p3 - utan1 * b + unorm1 * b_perp;

			// 3. Refine using least-squares with both tangent and normal degrees of freedom
			float tSamples[N_LSE];
			for(int i = 0; i < N_LSE; ++i)
				tSamples[i] = t0 + (t1 - t0) * ((float)(i + 1) / (N_LSE + 1));

			// Perform 2 iterations of least-squares refinement (now 4x4 system)
			for(int iter = 0; iter < 2; ++iter) {
				// 4x4 matrix: [a, a_perp, b, b_perp]
				float m00 = 0.0f, m01 = 0.0f, m02 = 0.0f, m03 = 0.0f;
				float m11 = 0.0f, m12 = 0.0f, m13 = 0.0f;
				float m22 = 0.0f, m23 = 0.0f;
				float m33 = 0.0f;
				float r0 = 0.0f, r1 = 0.0f, r2 = 0.0f, r3 = 0.0f;

				for(int i = 0; i < N_LSE; ++i) {
					float t = tSamples[i];
					float ta = (float)(i + 1) / (N_LSE + 1);

					// True offset point
					vec2 pTrue = evalOffset(t);

					// Approximation point
					float sa = 1.0f - ta;
					vec2 pApprox = sa*sa*sa * p0 + 3.0f*sa*sa*ta * p1 +
					               3.0f*sa*ta*ta * p2 + ta*ta*ta * p3;

					// Error vector
					vec2 errVec = pApprox - pTrue;

					// Jacobian: how does pApprox change with a, a_perp, b, b_perp?
					float coeff_p1 = 3.0f * sa*sa * ta;
					float coeff_p2 = 3.0f * sa * ta*ta;

					vec2 dp_da = coeff_p1 * utan0;
					vec2 dp_da_perp = coeff_p1 * unorm0;
					vec2 dp_db = -coeff_p2 * utan1;  // Negative because p2 = p3 - ...
					vec2 dp_db_perp = coeff_p2 * unorm1;  // Positive for normal component

					// Accumulate normal equations: J^T * J and J^T * err
					m00 += glm::dot(dp_da, dp_da);
					m01 += glm::dot(dp_da, dp_da_perp);
					m02 += glm::dot(dp_da, dp_db);
					m03 += glm::dot(dp_da, dp_db_perp);
					m11 += glm::dot(dp_da_perp, dp_da_perp);
					m12 += glm::dot(dp_da_perp, dp_db);
					m13 += glm::dot(dp_da_perp, dp_db_perp);
					m22 += glm::dot(dp_db, dp_db);
					m23 += glm::dot(dp_db, dp_db_perp);
					m33 += glm::dot(dp_db_perp, dp_db_perp);

					r0 += glm::dot(dp_da, errVec);
					r1 += glm::dot(dp_da_perp, errVec);
					r2 += glm::dot(dp_db, errVec);
					r3 += glm::dot(dp_db_perp, errVec);
				}

				// Solve 4x4 symmetric system using Cholesky decomposition
				// A = [[m00, m01, m02, m03],
				//      [m01, m11, m12, m13],
				//      [m02, m12, m22, m23],
				//      [m03, m13, m23, m33]]
				// Solve A * delta = -r

				// Simplified Gaussian elimination for 4x4
				float A[4][5] = {
					{m00, m01, m02, m03, -r0},
					{m01, m11, m12, m13, -r1},
					{m02, m12, m22, m23, -r2},
					{m03, m13, m23, m33, -r3}
				};

				// Forward elimination
				for(int k = 0; k < 4; ++k) {
					// Find pivot
					float pivot = A[k][k];
					if(std::abs(pivot) < 1e-10f) break;

					// Eliminate below
					for(int i = k + 1; i < 4; ++i) {
						float factor = A[i][k] / pivot;
						for(int j = k; j < 5; ++j) {
							A[i][j] -= factor * A[k][j];
						}
					}
				}

				// Back substitution
				float delta[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				for(int i = 3; i >= 0; --i) {
					delta[i] = A[i][4];
					for(int j = i + 1; j < 4; ++j) {
						delta[i] -= A[i][j] * delta[j];
					}
					if(std::abs(A[i][i]) > 1e-10f) {
						delta[i] /= A[i][i];
					}
				}

				// Update parameters
				a += delta[0];
				a_perp += delta[1];
				b += delta[2];
				b_perp += delta[3];

				// Update control points
				p1 = p0 + utan0 * a + unorm0 * a_perp;
				p2 = p3 - utan1 * b + unorm1 * b_perp;
			}

			// 4. Evaluate error
			float maxErrSq = 0.0f;
			for(int i = 0; i < N_LSE; ++i) {
				float ta = (float)(i + 1) / (N_LSE + 1);
				vec2 pTrue = evalOffset(tSamples[i]);

				float sa = 1.0f - ta;
				vec2 pApprox = sa*sa*sa * p0 + 3.0f*sa*sa*ta * p1 +
				               3.0f*sa*ta*ta * p2 + ta*ta*ta * p3;

				float errSq = glm::length2(pApprox - pTrue);
				maxErrSq = std::max(maxErrSq, errSq);
			}

			float maxErr = std::sqrt(maxErrSq);

			// 5. Subdivide if error exceeds tolerance
			if(maxErr > tolerance && depth < MAX_OFFSET_DEPTH) {
				float tMid = (t0 + t1) * 0.5f;
				vec2 utanMid = evalUnitTangent(tMid);
				offsetRec(t0, tMid, utan0, utanMid, result, depth + 1);
				offsetRec(tMid, t1, utanMid, utan1, result, depth + 1);
			}
			else {
				// Accept this cubic approximation
				result.curveTo(p1, p2, p3);
			}
		}

		vec2 evalUnitTangent(float t) const
		{
			float s = 1.0f - t;
			vec2 deriv = s*s * q[0] + 2.0f*s*t * q[1] + t*t * q[2];
			float len = glm::length(deriv);
			return (len > 1e-6f) ? (deriv / len) : vec2(1, 0);
		}
	};

} // anonymous namespace

// Forward declarations
void offsetBezierSegmentCubic( const vec2* controlPoints, float distance, float tolerance, Path2d& result );
void offsetBezierSegmentQuadratic( const vec2* controlPoints, float distance, float tolerance, Path2d& result );

// Main offset function - dispatches to cubic or quadratic
void offsetBezierSegment( const vec2* controlPoints, int degree, float distance,
                          float tolerance, Path2d& result )
{
	if( degree == 3 ) {
		offsetBezierSegmentCubic( controlPoints, distance, tolerance, result );
	}
	else if( degree == 2 ) {
		offsetBezierSegmentQuadratic( controlPoints, distance, tolerance, result );
	}
}

// Cubic offset using Kurbo algorithm
void offsetBezierSegmentCubic( const vec2* controlPoints, float distance, float tolerance, Path2d& result )
{
	CubicOffsetHelper helper(controlPoints, distance, tolerance);

	vec2 utan0 = helper.evalUnitTangent(0.0f);
	vec2 utan1 = helper.evalUnitTangent(1.0f);

	// Don't call moveTo here - the calling code handles that
	helper.offsetRec(0.0f, 1.0f, utan0, utan1, result, 0);
}

// Quadratic offset using degree elevation to cubic
void offsetBezierSegmentQuadratic( const vec2* controlPoints, float distance, float tolerance, Path2d& result )
{
	// Degree-elevate quadratic to cubic, then use the cubic offset algorithm
	// This ensures quadratics use the same tolerance-driven recursive approach
	//
	// Quadratic: P0, P1, P2
	// Cubic: Q0, Q1, Q2, Q3
	//
	// Degree elevation formula:
	//   Q0 = P0
	//   Q1 = P0 + 2/3*(P1 - P0) = P0/3 + 2*P1/3
	//   Q2 = P2 + 2/3*(P1 - P2) = 2*P1/3 + P2/3
	//   Q3 = P2

	vec2 cubic[4];
	cubic[0] = controlPoints[0];
	cubic[1] = controlPoints[0] / 3.0f + controlPoints[1] * (2.0f / 3.0f);
	cubic[2] = controlPoints[1] * (2.0f / 3.0f) + controlPoints[2] / 3.0f;
	cubic[3] = controlPoints[2];

	// Now use the cubic offset algorithm
	offsetBezierSegmentCubic( cubic, distance, tolerance, result );
}

// Add a join between two segments at a corner
void addOffsetJoin( const vec2& point, const vec2& tangent1, const vec2& tangent2,
                    float distance, const Path2d::OffsetOptions& options, Path2d& result )
{
	vec2 normal1 = calcOffsetNormal( tangent1 );
	vec2 normal2 = calcOffsetNormal( tangent2 );

	// Check if normals are very similar (smooth join not needed)
	float dot = glm::dot( normal1, normal2 );
	if( dot > 0.9999f ) {
		return;  // Nearly parallel, no join needed
	}

	vec2 offset1 = point + normal1 * distance;
	vec2 offset2 = point + normal2 * distance;

	// Determine if this is an outer or inner corner
	// Cross product tells us the turn direction
	float cross = tangent1.x * tangent2.y - tangent1.y * tangent2.x;
	bool isOuterCorner = (distance > 0.0f) ? (cross < 0.0f) : (cross > 0.0f);

	if( !isOuterCorner ) {
		// Inner corner - simple bevel (line to next offset point)
		result.lineTo( offset2 );
		return;
	}

	// Outer corner - apply join style
	switch( options.joinStyle ) {
		case Path2d::OffsetOptions::ROUND: {
			// Add circular arc from offset1 to offset2
			float angle1 = std::atan2( normal1.y, normal1.x );
			float angle2 = std::atan2( normal2.y, normal2.x );
			float angleDiff = angle2 - angle1;

			// Normalize to [-π, π]
			while( angleDiff > M_PI ) angleDiff -= 2.0f * static_cast<float>(M_PI);
			while( angleDiff < -M_PI ) angleDiff += 2.0f * static_cast<float>(M_PI);

			// Only add arc if angle is significant
			if( std::abs(angleDiff) > 0.01f ) {
				// Calculate steps based on tolerance
				// For circular arc of radius R, max deviation is R(1 - cos(θ/2))
				// We want: R(1 - cos(θ/2)) ≤ tolerance
				// Use double precision to avoid float precision issues with large radii
				double radius = std::abs(distance);
				double maxAnglePerStep;
				if( options.tolerance > 0.0001 && radius > 0.0001 ) {
					// Calculate angle step from tolerance
					double ratio = std::max( 1e-6, std::min( 2.0, (double)options.tolerance / radius ) );
					maxAnglePerStep = 2.0 * std::acos( 1.0 - ratio );

					// Guard against very small angles that could cause huge step counts
					maxAnglePerStep = std::max( 0.001, maxAnglePerStep );  // Min ~0.06 degrees
				}
				else {
					// Fallback to reasonable default
					maxAnglePerStep = M_PI / 8.0;  // 22.5 degrees
				}

				int steps = std::max( 2, std::min( 1000, (int)std::ceil(std::abs(angleDiff) / maxAnglePerStep) ) );
				for( int i = 1; i <= steps; ++i ) {
					float t = (float)i / steps;
					float a = angle1 + angleDiff * t;
					vec2 n = vec2( std::cos(a), std::sin(a) );
					result.lineTo( point + n * (float)radius );
				}
			}
			else {
				result.lineTo( offset2 );
			}
		}
		break;

		case Path2d::OffsetOptions::MITER: {
			// Calculate miter point (intersection of offset lines)
			// Line 1: offset1 + t * tangent1
			// Line 2: offset2 + s * tangent2
			// Solve for intersection

			vec2 diff = offset2 - offset1;
			float denom = tangent1.x * tangent2.y - tangent1.y * tangent2.x;

			if( std::abs(denom) > 0.0001f ) {
				float t = (diff.x * tangent2.y - diff.y * tangent2.x) / denom;
				vec2 miterPoint = offset1 + tangent1 * t;

				// Check miter limit
				float miterLength = glm::length( miterPoint - point );
				float miterRatio = miterLength / std::abs(distance);

				if( miterRatio <= options.miterLimit ) {
					// Miter within limit
					result.lineTo( miterPoint );
					result.lineTo( offset2 );
				}
				else {
					// Miter exceeds limit, fall back to bevel
					result.lineTo( offset2 );
				}
			}
			else {
				// Nearly parallel, use bevel
				result.lineTo( offset2 );
			}
		}
		break;

		case Path2d::OffsetOptions::BEVEL:
			// Simple line to next offset point
			result.lineTo( offset2 );
		break;
	}
}

// Helper function to emit a circular arc approximated with cubic Bezier
void emitArc( Path2d& path, const vec2& center, float angle0, float angle1, float radius, float tolerance )
{
	// Normalize angle range
	float angleDiff = angle1 - angle0;
	while( angleDiff > (float)M_PI ) angleDiff -= 2.0f * (float)M_PI;
	while( angleDiff < -(float)M_PI ) angleDiff += 2.0f * (float)M_PI;

	// Subdivide arc if needed to keep error within tolerance
	// Maximum angle for single cubic with given tolerance:
	// For radius r and angle θ, max error ≈ r * (1 - cos(θ/2)) / cos²(θ/4)
	// We use a conservative estimate: subdivide if |θ| > π/2
	const float maxAngle = (float)M_PI / 2.0f;
	int numSegments = (int)std::ceil( std::abs(angleDiff) / maxAngle );
	float segmentAngle = angleDiff / numSegments;

	// Control point arm length for arc approximation
	// See: http://pomax.github.io/bezierinfo/#circles_cubic
	float armLength = radius * 4.0f * std::tan(segmentAngle / 4.0f) / 3.0f;

	for( int i = 0; i < numSegments; ++i ) {
		float a0 = angle0 + i * segmentAngle;
		float a1 = a0 + segmentAngle;

		vec2 p0 = center + vec2(std::cos(a0), std::sin(a0)) * radius;
		vec2 p3 = center + vec2(std::cos(a1), std::sin(a1)) * radius;

		vec2 tangent0 = vec2(-std::sin(a0), std::cos(a0));
		vec2 tangent1 = vec2(-std::sin(a1), std::cos(a1));

		vec2 p1 = p0 + tangent0 * armLength;
		vec2 p2 = p3 - tangent1 * armLength;

		if( i == 0 && path.empty() ) {
			path.moveTo( p0 );
		}
		path.curveTo( p1, p2, p3 );
	}
}

// Helper function to add offset cap at path endpoints
// Uses Kurbo's transform-based approach for stable, consistent caps
void addOffsetCap( Path2d& path, const vec2& center, const vec2& tangent,
                   const vec2& offsetStart, const vec2& offsetEnd,
                   float distance, Path2d::StrokeOptions::CapStyle capStyle, float tolerance )
{
	vec2 normal = calcOffsetNormal( tangent );

	switch( capStyle ) {
		case Path2d::StrokeOptions::CAP_BUTT:
			// Straight line connecting the two offset edges
			path.lineTo( offsetEnd );
			break;

		case Path2d::StrokeOptions::CAP_ROUND: {
			// Round cap using Kurbo's transformation approach
			// Instead of calculating angles and deciding on sweep direction,
			// we build a transformation matrix and always draw the arc the same way

			// The normal points perpendicular to the tangent
			// For a cap, we want to draw a semicircular arc from one side to the other
			// We'll draw the arc in "normal space" and transform it to world space

			float radius = std::abs(distance);
			vec2 norm = normal * distance;  // Points from center to offsetStart
			vec2 tang = tangent * radius;   // Perpendicular direction, also scaled by radius

			// Build affine transformation matrix like Kurbo:
			// The arc goes from angle 0 to π in local space
			// Transform: p_world = center + norm * cos(θ) + tang * sin(θ)
			// This is equivalent to: p_world = A * p_local + center
			// where A = [norm.x  tang.x]
			//           [norm.y  tang.y]

			// Generate the semicircular arc from 0 to π
			// Subdivide into segments (π/2 per segment)
			const int numSegments = 2;
			float segmentAngle = (float)M_PI / numSegments;
			float armLength = radius * 4.0f * std::tan(segmentAngle / 4.0f) / 3.0f;

			for( int i = 0; i < numSegments; ++i ) {
				float a0 = i * segmentAngle;
				float a1 = (i + 1) * segmentAngle;

				// Points in local space (unit circle from 0 to π)
				vec2 p0_local( std::cos(a0), std::sin(a0) );
				vec2 p3_local( std::cos(a1), std::sin(a1) );

				// Tangents in local space
				vec2 t0_local( -std::sin(a0), std::cos(a0) );
				vec2 t1_local( -std::sin(a1), std::cos(a1) );

				// Control points in local space
				vec2 p1_local = p0_local + t0_local * armLength / radius;
				vec2 p2_local = p3_local - t1_local * armLength / radius;

				// Transform to world space
				// p_world = center + norm * p.x + tang * p.y
				auto transform = [&]( const vec2& p ) {
					return center + norm * p.x + tang * p.y;
				};

				vec2 p1_world = transform( p1_local );
				vec2 p2_world = transform( p2_local );
				vec2 p3_world = transform( p3_local );

				path.curveTo( p1_world, p2_world, p3_world );
			}
			break;
		}

		case Path2d::StrokeOptions::CAP_SQUARE: {
			// Square cap extending beyond endpoint by distance
			vec2 extend = glm::normalize(tangent) * std::abs(distance);
			vec2 corner0 = offsetStart + extend;
			// corner1 should be perpendicular to extend, not extending offsetEnd
			vec2 corner1 = corner0 + (offsetEnd - offsetStart);

			path.lineTo( corner0 );
			path.lineTo( corner1 );
			path.lineTo( offsetEnd );
			break;
		}
	}
}

} // anonymous namespace

Path2d Path2d::calcOffsetCurve( float distance, float tolerance ) const
{
	OffsetOptions options;
	options.tolerance = tolerance;
	return calcOffsetCurve( distance, options );
}

Path2d Path2d::calcOffsetCurve( float distance, const OffsetOptions& options ) const
{
	Path2d result;

	if( mSegments.empty() || std::abs(distance) < 0.0001f ) {
		return result;
	}

	size_t firstPoint = 0;
	vec2 prevTangent;
	bool hasPrevTangent = false;

	// Track contour start/end for caps and closed paths
	vec2 contourFirstTangent;
	vec2 contourStartPoint;
	vec2 contourStartOffset;
	vec2 contourLastPoint;
	vec2 contourLastOffset;
	vec2 contourLastTangent;
	bool hasContourStart = false;
	bool contourClosed = false;

	// Helper lambda to finalize current contour (simplified - no caps)
	auto finalizeContour = [&]() {
		if( !hasContourStart ) return;

		// calcOffsetCurve now returns open paths only - no caps, no return path
		// The path is left open for open contours, closed for closed contours

		// Reset for next contour
		hasPrevTangent = false;
		hasContourStart = false;
		contourClosed = false;
	};

	for( size_t s = 0; s < mSegments.size(); ++s ) {
		switch( mSegments[s] ) {
			case MOVETO: {
				// Finalize previous contour before starting new one
				finalizeContour();
			}
			break;

			case LINETO: {
				vec2 p0 = mPoints[firstPoint];
				vec2 p1 = mPoints[firstPoint + 1];

				// Calculate tangent safely
				vec2 tangent;
				if( !calcSafeTangent( p0, p1, tangent ) ) {
					// Degenerate segment, skip it
					break;
				}

				vec2 off0, off1;
				if( !offsetLinearSegment( p0, p1, distance, off0, off1 ) ) {
					break;  // Degenerate segment
				}

				if( !hasContourStart ) {
					// First segment of contour
					result.moveTo( off0 );
					contourStartPoint = p0;
					contourStartOffset = off0;
					contourFirstTangent = tangent;
					hasContourStart = true;
				}
				else if( hasPrevTangent ) {
					addOffsetJoin( p0, prevTangent, tangent, distance, options, result );
				}

				result.lineTo( off1 );
				prevTangent = tangent;
				hasPrevTangent = true;

				// Track last point for end cap
				contourLastPoint = p1;
				contourLastOffset = off1;
				contourLastTangent = tangent;
			}
			break;

			case QUADTO: {
				vec2 cp[3] = { mPoints[firstPoint], mPoints[firstPoint + 1], mPoints[firstPoint + 2] };

				// Calculate start tangent safely
				vec2 tangent;
				if( !calcSafeTangent( cp[0], cp[1], tangent ) ) {
					// Try using end point if control point is degenerate
					if( !calcSafeTangent( cp[0], cp[2], tangent ) ) {
						break;  // Completely degenerate
					}
				}

				if( !hasContourStart ) {
					// First segment of contour
					vec2 startOffset = cp[0] + calcOffsetNormal(tangent) * distance;
					result.moveTo( startOffset );
					contourStartPoint = cp[0];
					contourStartOffset = startOffset;
					contourFirstTangent = tangent;
					hasContourStart = true;
				}
				else if( hasPrevTangent ) {
					addOffsetJoin( cp[0], prevTangent, tangent, distance, options, result );
				}

				offsetBezierSegment( cp, 2, distance, options.tolerance, result );

				// Calculate end tangent safely
				vec2 endTangent;
				if( calcSafeTangent( cp[1], cp[2], endTangent ) || calcSafeTangent( cp[0], cp[2], endTangent ) ) {
					prevTangent = endTangent;
					hasPrevTangent = true;

					// Track last point for end cap
					contourLastPoint = cp[2];
					contourLastOffset = cp[2] + calcOffsetNormal(endTangent) * distance;
					contourLastTangent = endTangent;
				}
			}
			break;

			case CUBICTO: {
				vec2 cp[4] = { mPoints[firstPoint], mPoints[firstPoint + 1],
				               mPoints[firstPoint + 2], mPoints[firstPoint + 3] };

				// Calculate start tangent safely
				vec2 tangent;
				if( !calcSafeTangent( cp[0], cp[1], tangent ) ) {
					// Try using control point 2 or end point
					if( !calcSafeTangent( cp[0], cp[2], tangent ) ) {
						if( !calcSafeTangent( cp[0], cp[3], tangent ) ) {
							break;  // Completely degenerate
						}
					}
				}

				if( !hasContourStart ) {
					// First segment of contour
					vec2 startOffset = cp[0] + calcOffsetNormal(tangent) * distance;
					result.moveTo( startOffset );
					contourStartPoint = cp[0];
					contourStartOffset = startOffset;
					contourFirstTangent = tangent;
					hasContourStart = true;
				}
				else if( hasPrevTangent ) {
					addOffsetJoin( cp[0], prevTangent, tangent, distance, options, result );
				}

				offsetBezierSegment( cp, 3, distance, options.tolerance, result );

				// Calculate end tangent safely
				vec2 endTangent;
				if( calcSafeTangent( cp[2], cp[3], endTangent ) ||
				    calcSafeTangent( cp[1], cp[3], endTangent ) ||
				    calcSafeTangent( cp[0], cp[3], endTangent ) ) {
					prevTangent = endTangent;
					hasPrevTangent = true;

					// Track last point for end cap
					contourLastPoint = cp[3];
					contourLastOffset = cp[3] + calcOffsetNormal(endTangent) * distance;
					contourLastTangent = endTangent;
				}
			}
			break;

			case CLOSE: {
				// Generate join between last tangent and first tangent of contour
				if( hasContourStart && hasPrevTangent ) {
					addOffsetJoin( contourStartPoint, prevTangent, contourFirstTangent, distance, options, result );
					// Note: close() will automatically add the closing segment
				}

				if( hasContourStart && !result.empty() ) {
					result.close();
				}

				// Mark contour as closed (no caps needed)
				contourClosed = true;

				// Reset for next contour
				hasPrevTangent = false;
				hasContourStart = false;
			}
			break;
		}

		firstPoint += Path2d::sSegmentTypePointCounts[mSegments[s]];
	}

	// Finalize any remaining open contour
	finalizeContour();

	return result;
}

Path2d Path2d::calcStroke( const StrokeOptions& options ) const
{
	Path2d result;

	if( mSegments.empty() || options.width <= 0.0f ) {
		return result;
	}

	// Warn if dash pattern is specified - user should use calcStrokeAsShape() instead
	if( !options.dashPattern.empty() ) {
		CI_LOG_W( "calcStroke() called with dash pattern - use calcStrokeAsShape() instead for proper multi-contour output" );
		// Continue with non-dashed stroke
	}

	// Half width for bilateral expansion
	float halfWidth = options.width * 0.5f;

	// Create OffsetOptions from StrokeOptions
	OffsetOptions offsetOpts;
	offsetOpts.tolerance = options.tolerance;
	offsetOpts.joinStyle = static_cast<OffsetOptions::JoinStyle>(options.joinStyle);
	offsetOpts.miterLimit = options.miterLimit;

	// Check if path is closed
	bool isClosed = !mSegments.empty() && mSegments.back() == CLOSE;

	if( isClosed ) {
		// For closed paths, just offset both sides and close
		Path2d outer = (*this).calcOffsetCurve( halfWidth, offsetOpts );
		Path2d inner = (*this).calcOffsetCurve( -halfWidth, offsetOpts );

		// Reverse inner path
		inner.reverse();

		// Combine: outer + inner (reversed)
		if( !outer.empty() && !inner.empty() ) {
			result = outer;

			// Append inner path segments
			const auto& innerSegments = inner.getSegments();
			const auto& innerPoints = inner.getPoints();

			size_t pointIndex = 1; // Skip moveto
			for( size_t s = 0; s < innerSegments.size(); ++s ) {
				SegmentType segType = innerSegments[s];

				switch( segType ) {
					case MOVETO:
						break; // Skip
					case LINETO:
						result.lineTo( innerPoints[pointIndex] );
						pointIndex += 1;
						break;
					case QUADTO:
						result.quadTo( innerPoints[pointIndex], innerPoints[pointIndex + 1] );
						pointIndex += 2;
						break;
					case CUBICTO:
						result.curveTo( innerPoints[pointIndex], innerPoints[pointIndex + 1],
						               innerPoints[pointIndex + 2] );
						pointIndex += 3;
						break;
					case CLOSE:
						break; // Skip
				}
			}

			result.close();
		}
	}
	else {
		// For open paths, offset both sides and add caps
		Path2d outer = (*this).calcOffsetCurve( halfWidth, offsetOpts );
		Path2d inner = (*this).calcOffsetCurve( -halfWidth, offsetOpts );

		if( outer.empty() || inner.empty() ) {
			return result;
		}

		// Get tangents at start and end for caps
		vec2 startPoint = (*this).mPoints[0];
		vec2 endPoint = (*this).mPoints[(*this).mPoints.size() - 1];

		// Calculate start tangent
		vec2 startTangent;
		if( (*this).mSegments.size() > 0 ) {
			if( (*this).mSegments[0] == LINETO ) {
				calcSafeTangent( (*this).mPoints[0], (*this).mPoints[1], startTangent );
			}
			else if( (*this).mSegments[0] == QUADTO ) {
				if( !calcSafeTangent( (*this).mPoints[0], (*this).mPoints[1], startTangent ) ) {
					calcSafeTangent( (*this).mPoints[0], (*this).mPoints[2], startTangent );
				}
			}
			else if( (*this).mSegments[0] == CUBICTO ) {
				if( !calcSafeTangent( (*this).mPoints[0], (*this).mPoints[1], startTangent ) ) {
					if( !calcSafeTangent( (*this).mPoints[0], (*this).mPoints[2], startTangent ) ) {
						calcSafeTangent( (*this).mPoints[0], (*this).mPoints[3], startTangent );
					}
				}
			}
		}

		// Calculate end tangent
		vec2 endTangent;
		size_t lastSegIdx = (*this).mSegments.size() - 1;
		if( (*this).mSegments[lastSegIdx] == LINETO ) {
			size_t lastPtIdx = (*this).mPoints.size() - 1;
			calcSafeTangent( (*this).mPoints[lastPtIdx - 1], (*this).mPoints[lastPtIdx], endTangent );
		}
		else if( (*this).mSegments[lastSegIdx] == QUADTO ) {
			size_t lastPtIdx = (*this).mPoints.size() - 1;
			if( !calcSafeTangent( (*this).mPoints[lastPtIdx - 1], (*this).mPoints[lastPtIdx], endTangent ) ) {
				calcSafeTangent( (*this).mPoints[lastPtIdx - 2], (*this).mPoints[lastPtIdx], endTangent );
			}
		}
		else if( (*this).mSegments[lastSegIdx] == CUBICTO ) {
			size_t lastPtIdx = (*this).mPoints.size() - 1;
			if( !calcSafeTangent( (*this).mPoints[lastPtIdx - 1], (*this).mPoints[lastPtIdx], endTangent ) ) {
				if( !calcSafeTangent( (*this).mPoints[lastPtIdx - 2], (*this).mPoints[lastPtIdx], endTangent ) ) {
					calcSafeTangent( (*this).mPoints[lastPtIdx - 3], (*this).mPoints[lastPtIdx], endTangent );
				}
			}
		}

		// Start with outer path
		result = outer;

		// Add end cap
		vec2 outerEnd = outer.getPoints().back();
		vec2 innerEnd = inner.getPoints().back();
		addOffsetCap( result, endPoint, endTangent, outerEnd, innerEnd, halfWidth,
		             options.endCap, options.tolerance );

		// Reverse inner path
		inner.reverse();

		// Append inner path segments
		const auto& innerSegments = inner.getSegments();
		const auto& innerPoints = inner.getPoints();

		size_t pointIndex = 1; // Skip moveto
		for( size_t s = 0; s < innerSegments.size(); ++s ) {
			SegmentType segType = innerSegments[s];

			switch( segType ) {
				case MOVETO:
					break; // Skip
				case LINETO:
					result.lineTo( innerPoints[pointIndex] );
					pointIndex += 1;
					break;
				case QUADTO:
					result.quadTo( innerPoints[pointIndex], innerPoints[pointIndex + 1] );
					pointIndex += 2;
					break;
				case CUBICTO:
					result.curveTo( innerPoints[pointIndex], innerPoints[pointIndex + 1],
					               innerPoints[pointIndex + 2] );
					pointIndex += 3;
					break;
				case CLOSE:
					break; // Skip
			}
		}

		// Add start cap
		// After reversing, the last point of the reversed path is the start of the original path
		vec2 innerStart = inner.getPoints()[inner.getPoints().size() - 1];  // Last point of reversed = start of original
		vec2 outerStart = outer.getPoints()[0];
		addOffsetCap( result, startPoint, -startTangent, innerStart, outerStart, halfWidth,
		             options.startCap, options.tolerance );

		// Close the stroked path
		result.close();
	}

	return result;
}

Shape2d Path2d::applyDashPatternAsShape( const std::vector<float>& dashPattern, float dashOffset ) const
{
	Shape2d result;

	// Validate pattern
	if( dashPattern.empty() ) {
		// Empty pattern, return original path as single contour
		result.appendContour( *this );
		return result;
	}

	// Dash patterns should have even length (alternating on/off), but we can handle odd by duplicating last value
	std::vector<float> pattern = dashPattern;
	if( pattern.size() % 2 != 0 ) {
		pattern.push_back( pattern.back() ); // Duplicate last value to make even
	}

	// Check for all-zero or negative pattern
	float patternTotal = 0.0f;
	for( float len : pattern ) {
		if( len < 0.0f ) {
			result.appendContour( *this );
			return result;
		}
		patternTotal += len;
	}
	if( patternTotal <= 0.0f ) {
		return result; // Empty result for all-zero pattern
	}

	// PRE-COMPUTE arc lengths once to avoid repeated expensive integrations
	// This dramatically improves performance for paths with many dashes
	float totalLength = calcLength();
	if( totalLength <= 0.0f ) {
		CI_LOG_W( "applyDashPatternAsShape: path has zero length" );
		return result; // Empty path
	}

	// Cache segment lengths for fast lookup during dashing
	std::vector<float> segmentLengths;
	segmentLengths.reserve( getNumSegments() );
	for( size_t i = 0; i < getNumSegments(); ++i ) {
		segmentLengths.push_back( calcSegmentLength( i ) );
	}

	CI_LOG_I( "applyDashPatternAsShape: totalLength=" << totalLength << ", pattern size=" << pattern.size() << ", patternTotal=" << patternTotal );

	// Normalize dash offset (wrap to pattern length)
	dashOffset = std::fmod( dashOffset, patternTotal );
	if( dashOffset < 0.0f ) {
		dashOffset += patternTotal;
	}

	// Find starting position in pattern
	size_t patternIndex = 0;
	float remainingInSegment = pattern[0];
	bool isDash = true; // First segment is always "on"

	// Adjust for starting mid-pattern
	if( dashOffset > 0.0f ) {
		float consumed = 0.0f;
		for( size_t i = 0; i < pattern.size(); ++i ) {
			if( consumed + pattern[i] > dashOffset ) {
				patternIndex = i;
				remainingInSegment = pattern[i] - (dashOffset - consumed);
				isDash = (i % 2 == 0);
				break;
			}
			consumed += pattern[i];
		}
	}

	// Walk along path applying pattern
	float distance = 0.0f;
	float dashStart = isDash ? 0.0f : remainingInSegment;

	while( distance < totalLength ) {
		float segmentEnd = std::min( distance + remainingInSegment, totalLength );

		if( isDash ) {
			// Extract sub-path for this dash segment
			// Use cached arc lengths for massive performance improvement (avoids repeated expensive integrations)
			float startT = calcTimeForDistanceCached( dashStart, totalLength, segmentLengths, true, 0.1f, 16 );
			float endT = calcTimeForDistanceCached( segmentEnd, totalLength, segmentLengths, true, 0.1f, 16 );

			CI_LOG_I( "Extracting dash: startT=" << startT << ", endT=" << endT << ", dashStart=" << dashStart << ", segmentEnd=" << segmentEnd );

			// Extract the sub-path
			Path2d dashSegment;
			try {
				dashSegment = getSubPath( startT, endT );
			}
			catch( const std::exception& e ) {
				CI_LOG_E( "getSubPath failed: " << e.what() );
				distance = segmentEnd;
				patternIndex = (patternIndex + 1) % pattern.size();
				remainingInSegment = pattern[patternIndex];
				isDash = !isDash;
				if( isDash ) dashStart = distance;
				continue;
			}

			// Add dash segment as a separate contour
			if( !dashSegment.empty() && dashSegment.getNumPoints() > 0 ) {
				result.appendContour( dashSegment );
			}
		}

		distance = segmentEnd;

		// Move to next pattern segment
		patternIndex = (patternIndex + 1) % pattern.size();
		remainingInSegment = pattern[patternIndex];
		isDash = !isDash;

		if( isDash ) {
			dashStart = distance;
		}
	}

	CI_LOG_I( "applyDashPatternAsShape: created " << result.getNumContours() << " dash segments" );

	return result;
}

Shape2d Path2d::calcStrokeAsShape( const StrokeOptions& options ) const
{
	Shape2d result;

	if( mSegments.empty() || options.width <= 0.0f ) {
		return result;
	}

	// Check if dashing is enabled
	if( !options.dashPattern.empty() ) {
		// Apply dash pattern to get Shape2d with multiple contours
		Shape2d dashedShape = applyDashPatternAsShape( options.dashPattern, options.dashOffset );

		if( dashedShape.empty() ) {
			return result; // No visible dashes
		}

		CI_LOG_I( "calcStrokeAsShape: dashedShape has " << dashedShape.getNumContours() << " contours" );

		// Stroke each contour independently
		StrokeOptions dashStrokeOpts = options;
		dashStrokeOpts.dashPattern.clear(); // Don't re-dash

		for( size_t i = 0; i < dashedShape.getNumContours(); ++i ) {
			const Path2d& contour = dashedShape.getContour( i );
			float segmentLength = contour.calcLength();

			// Only skip truly degenerate segments (numerical noise)
			if( segmentLength < 0.01f ) {
				CI_LOG_W( "Skipping degenerate dash segment " << i << " (length " << segmentLength << ")" );
				continue;
			}

			try {
				// Each dash segment gets caps at both ends
				// Round caps will naturally form circles for very short dashes
				StrokeOptions segmentOpts = dashStrokeOpts;

				Path2d strokedContour = contour.calcStroke( segmentOpts );

				if( !strokedContour.empty() ) {
					result.appendContour( strokedContour );
				}
			}
			catch( const std::exception& e ) {
				CI_LOG_E( "Failed to stroke dash segment " << i << " (length=" << segmentLength << "): " << e.what() );
			}
		}
	}
	else {
		// No dashing - just stroke normally
		Path2d stroked = calcStroke( options );
		if( !stroked.empty() ) {
			result.appendContour( stroked );
		}
	}

	return result;
}

} // namespace cinder
