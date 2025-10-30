#include "cinder/app/App.h"
#include "cinder/Path2d.h"
#include "cinder/CinderMath.h"
#include "cinder/Rand.h"

#include "catch.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

bool subPathHelper( const Path2d &p, float start, float end )
{
	float targetLength = ( end - start ) * p.calcLength();
	Path2d sub = p.getSubPath( p.calcNormalizedTime( start, false ), p.calcNormalizedTime( end, false ) );
	float subLength = sub.calcLength();
	return abs( targetLength - subLength ) <= (0.01f * targetLength);
}

TEST_CASE("Path2d")
{
	// getSubPath()
	SECTION("getSubPath: Single Segment")
	{
		Path2d line;
		line.moveTo( 50, 50 ); line.lineTo( 150, 150 );
		REQUIRE( subPathHelper( line, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( line, 0.0f, 1.0f ) );
		
		Path2d quad;
		quad.moveTo( 50, 50 ); quad.quadTo( 75, 123, 150, 147 );
		REQUIRE( subPathHelper( quad, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( quad, 0.0f, 1.0f ) );

		Path2d cubic;
		cubic.moveTo( 50, 50 ); cubic.curveTo( 75, 123, 150, 147, 200, 233 );
		REQUIRE( subPathHelper( cubic, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( cubic, 0.0f, 1.0f ) );
	}
	
	SECTION("getSubPath: Multi Segment")
	{
		Path2d p1;
		p1.moveTo( 50, 50 ); p1.lineTo( 123, 345 );
		REQUIRE( subPathHelper( p1, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( p1, 0.2f, 1.0f ) );
		REQUIRE( subPathHelper( p1, 0.0f, 0.7f ) );
		
		Path2d p2;
		p2.moveTo( 50, 50 ); p2.lineTo( 123, 345 ); p2.quadTo( 77, 88, 111, 121 );
		REQUIRE( subPathHelper( p2, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( p2, 0.2f, 1.0f ) );
		REQUIRE( subPathHelper( p2, 0.0f, 0.7f ) );

		Path2d p2b;
		p2b.moveTo( 50, 50 ); p2b.lineTo( 123, 345 ); p2b.lineTo( 77, 88 ); p2b.close();
		REQUIRE( subPathHelper( p2, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( p2, 0.2f, 1.0f ) );
		REQUIRE( subPathHelper( p2, 0.0f, 0.7f ) );

		Path2d p3;
		p3.moveTo( 50, 50 ); p3.lineTo( 123, 345 ); p3.curveTo( 77, 88, 111, 121, 99, 144 );
		REQUIRE( subPathHelper( p3, 0.3f, 0.7f ) );
		REQUIRE( subPathHelper( p3, 0.2f, 1.0f ) );
		REQUIRE( subPathHelper( p3, 0.0f, 0.7f ) );
		
		Rand r1;
		for( int p = 0; p < 50; ++p ) {
			Path2d p4;
			p4.moveTo( 123, 345 );
			int count = r1.nextInt( 20 );
			for( int i = 0; i < count; ++i ) {
				switch( r1.nextInt() % 3 ) {
					case 0:
						p4.lineTo( r1.nextFloat( 500 ), r1.nextFloat( 500 ) );
					break;
					case 1:
						p4.quadTo( r1.nextFloat( 500 ), r1.nextFloat( 500 ), r1.nextFloat( 500 ), r1.nextFloat( 500 ) );
					break;
					case 2:
						p4.curveTo( r1.nextFloat( 500 ), r1.nextFloat( 500 ), r1.nextFloat( 500 ), r1.nextFloat( 500 ),
							r1.nextFloat( 500 ), r1.nextFloat( 500 ) );
					break;
				}
			}
			if( r1.nextBool() )
				p4.close();
			console() << p4 << std::endl;  
			REQUIRE( subPathHelper( p4, 0.3f, 0.7f ) );
			REQUIRE( subPathHelper( p4, 0.2f, 1.0f ) );
			REQUIRE( subPathHelper( p4, 0.0f, 0.7f ) );			
		}
		
	}

	// Distance
	SECTION("Distance: Vertical line")
	{
		vector<vec2> input({
			vec2( 0, 100 ),
			vec2( 0, 200 )
		});

		Path2d p;
		p.moveTo( input[0] );
		p.lineTo( input[1] );

		REQUIRE( p.calcDistance( vec2( 50, 150 ) ) == Approx( 50 ) ); // center, right 50
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 50, 150 ) ), vec2( 0, 150 ) ) == Approx( 0 ) ); // center, right 50
		REQUIRE( p.calcDistance( vec2( 0, 150 ) ) == Approx( 0 ) ); // along line
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 0, 150 ) ), vec2( 0, 150 ) ) == Approx( 0 ) ); // along line
		REQUIRE( p.calcDistance( vec2( 0, 250 ) ) == Approx( 50 ) ); // directly below line 50
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 0, 250 ) ), vec2( 0, 200 ) ) == Approx( 0 ) ); // directly below line 50
		REQUIRE( p.calcDistance( vec2( 0, 100 ) ) == Approx( 0 ) ); // co-sited with top
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 0, 100 ) ), vec2( 0, 100 ) ) == Approx( 0 ) ); // co-sited with top*/
	}

	SECTION("Distance: Triangle")
	{
		vector<vec2> input({
			vec2( 0, 100 ),
			vec2( 0, 200 ),
			vec2( 50, 150 )
		});

		Path2d p;
		p.moveTo( input[0] );
		p.lineTo( input[1] );
		p.lineTo( input[2] );
		
		REQUIRE( p.calcDistance( vec2( 50, 150 ) ) == Approx( 0 ) ); // co-sited with middle point
		REQUIRE( p.calcDistance( vec2( 50, 150 ), 0 ) == Approx( 50 ) ); // co-sited with righmost point, but test only the first segment		
		REQUIRE( p.calcDistance( vec2( -1, 150 ) ) == Approx( 1 ) ); // exterior, directly left 1
		REQUIRE( p.calcSignedDistance( vec2( -1, 150 ) ) == Approx( 1 ) ); // interior, one unit right of the left vertical line
		REQUIRE( p.calcDistance( vec2( 1, 150 ) ) == Approx( 1 ) ); // interior, one unit right of the left vertical line
		REQUIRE( p.calcSignedDistance( vec2( 1, 150 ) ) == Approx( -1 ) ); // interior, one unit right of the left vertical line
	}

	SECTION("Distance: Quadratic")
	{
		Path2d p; // shape matches Path2d guide from the docs
		p.moveTo( vec2( 300.0f, 270.0f ) );
		p.quadTo( vec2( 300.0f, 70.0f ), vec2( 500.0f, 70.0f ) );
		
		REQUIRE( p.calcDistance( vec2( 300.0f, 270.0f ) ) == Approx( 0 ) ); // co-sited with first point
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 300.0f, 270.0f ) ), vec2( 300.0f, 270.0f ) ) == Approx( 0 ) ); // co-sited with first point
		REQUIRE( p.calcDistance( vec2( 300.0f, 70.0f ) ) == Approx( 70.71 ) ); // middle control point; closest is ( 350, 120 ); sqrt(50*50 + 50*50) 
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 300.0f, 70.0f ) ), vec2( 350.0f, 120.0f ) ) == Approx( 0 ) ); // middle control point
	}

	SECTION("Distance: Cubic")
	{
		Path2d p; // shape matches Path2d guide from the docs
		p.moveTo( vec2( 300.0f, 270.0f ) );
		p.curveTo( vec2( 400.0f, 270.0f ), vec2( 400.0f, 70.0f ), vec2( 500.0f, 70.0f ) );
				
		REQUIRE( p.calcDistance( vec2( 300.0f, 270.0f ) ) == Approx( 0 ) ); // co-sited with first point
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 300.0f, 270.0f ) ), vec2( 300.0f, 270.0f ) ) == Approx( 0 ) ); // co-sited with first point
		REQUIRE( p.calcDistance( vec2( 400.0f, 270.0f ) ) == Approx( 51.2251847f ) ); // second control point; closest is ( 360.310f, 237.616f );
		auto closest =  p.calcClosestPoint( vec2( 400.0f, 270.0f ) );
		REQUIRE( glm::distance( p.calcClosestPoint( vec2( 400.0f, 270.0f ) ), vec2( 360.310f, 237.616f ) ) == Approx( 0 ).margin( 0.001 ) ); // second control point
	}

	SECTION("calcNormalizedTime")
	{
		// test pathological case of 0-length
		Path2d p;
		p.moveTo( 50, 50 );
		p.lineTo( 50, 50 );
		float t = p.calcNormalizedTime( 0.5f );
		REQUIRE( glm::distance( p.getPosition( t ), vec2( 50, 50 ) ) == Approx( 0 ).epsilon( 0.001 ) );
	}
	
	SECTION("translate")
	{
		Path2d p;
		p.moveTo( 0, 0 );
		p.lineTo( 10, 10 );
		Path2d q = p;
		p.translate( vec2( 1, 2 ) );
		REQUIRE( glm::distance( p.getPosition( 0 ), vec2( 1, 2 ) ) == Approx( 0 ).epsilon( 0.001 ) );
		REQUIRE( glm::distance( p.getPosition( 1.0 ), vec2( 11, 12 ) ) == Approx( 0 ).epsilon( 0.001 ) );
	}
}

TEST_CASE("CinderMath: solveQuadraticStable")
{
	SECTION("Root ordering: positive a")
	{
		float result[2];
		int count = solveQuadraticStable( 1.0f, 0.0f, -1.0f, result );
		REQUIRE( count == 2 );
		REQUIRE( result[0] == Approx( -1.0f ) );
		REQUIRE( result[1] == Approx( 1.0f ) );
		REQUIRE( result[0] <= result[1] ); // Verify ascending order
	}

	SECTION("Root ordering: negative a")
	{
		float result[2];
		int count = solveQuadraticStable( -1.0f, 0.0f, 1.0f, result );
		REQUIRE( count == 2 );
		REQUIRE( result[0] == Approx( -1.0f ) );
		REQUIRE( result[1] == Approx( 1.0f ) );
		REQUIRE( result[0] <= result[1] ); // Verify ascending order even for negative a
	}

	SECTION("Numerical stability: large b² >> 4ac")
	{
		// Case where b² >> 4ac causes cancellation in naive formula
		float result[2];
		int count = solveQuadraticStable( 1.0f, 1000.0f, 1.0f, result );
		REQUIRE( count == 2 );
		// roots should be approximately -1000 and -0.001
		REQUIRE( result[0] == Approx( -1000.001f ).margin( 0.01f ) );
		REQUIRE( result[1] == Approx( -0.001f ).margin( 0.0001f ) );
		REQUIRE( result[0] <= result[1] );
	}

	SECTION("Double root")
	{
		float result[2];
		int count = solveQuadraticStable( 1.0f, -2.0f, 1.0f, result ); // (x-1)² = 0
		REQUIRE( count == 1 );
		REQUIRE( result[0] == Approx( 1.0f ) );
	}

	SECTION("No real roots")
	{
		float result[2];
		int count = solveQuadraticStable( 1.0f, 0.0f, 1.0f, result );
		REQUIRE( count == 0 );
	}

	SECTION("Comparison with legacy solver")
	{
		Rand r;
		for( int i = 0; i < 100; ++i ) {
			float a = r.nextFloat( -10.0f, 10.0f );
			float b = r.nextFloat( -10.0f, 10.0f );
			float c = r.nextFloat( -10.0f, 10.0f );
			if( fabs(a) < 0.01f ) continue; // Skip near-zero a

			float resultOld[2], resultNew[2];
			int countOld = solveQuadratic( a, b, c, resultOld );
			int countNew = solveQuadraticStable( a, b, c, resultNew );

			REQUIRE( countOld == countNew );
			if( countOld > 0 ) {
				REQUIRE( resultNew[0] == Approx( resultOld[0] ).margin( 0.001f ) );
			}
			if( countOld > 1 ) {
				REQUIRE( resultNew[1] == Approx( resultOld[1] ).margin( 0.001f ) );
			}
		}
	}
}

TEST_CASE("CinderMath: Bezier Utilities")
{
	SECTION("evaluateQuadraticBezier correct formula")
	{
		vec2 controlPoints[3] = { vec2(0, 0), vec2(50, 100), vec2(100, 0) };

		for( float t = 0; t <= 1.0f; t += 0.1f ) {
			vec2 result = evaluateQuadraticBezier( controlPoints, t );
			// Compute expected result directly: (1-t)²P₀ + 2t(1-t)P₁ + t²P₂
			float nt = 1.0f - t;
			vec2 expected = controlPoints[0] * (nt*nt) + controlPoints[1] * (2.0f*t*nt) + controlPoints[2] * (t*t);
			REQUIRE( glm::distance( result, expected ) == Approx( 0 ).margin( 0.0001f ) );
		}
	}

	SECTION("evaluateCubicBezier correct formula")
	{
		vec2 controlPoints[4] = { vec2(0, 0), vec2(33, 100), vec2(66, 100), vec2(100, 0) };

		for( float t = 0; t <= 1.0f; t += 0.1f ) {
			vec2 result = evaluateCubicBezier( controlPoints, t );
			// Compute expected result directly: (1-t)³P₀ + 3t(1-t)²P₁ + 3t²(1-t)P₂ + t³P₃
			float nt = 1.0f - t;
			float w0 = nt*nt*nt;
			float w1 = 3.0f*nt*nt*t;
			float w2 = 3.0f*nt*t*t;
			float w3 = t*t*t;
			vec2 expected = controlPoints[0]*w0 + controlPoints[1]*w1 + controlPoints[2]*w2 + controlPoints[3]*w3;
			REQUIRE( glm::distance( result, expected ) == Approx( 0 ).margin( 0.0001f ) );
		}
	}

	SECTION("derivativeQuadraticBezier correct formula")
	{
		vec2 controlPoints[3] = { vec2(0, 0), vec2(50, 100), vec2(100, 0) };

		for( float t = 0; t <= 1.0f; t += 0.1f ) {
			vec2 result = derivativeQuadraticBezier( controlPoints, t );
			// Derivative: 2(1-t)(P₁-P₀) + 2t(P₂-P₁) = 2[(1-t)P₁ - (1-t)P₀ + tP₂ - tP₁]
			//           = 2[-((1-t)P₀ + tP₁) + ((1-t)P₁ + tP₂)]
			// Simplified: -2[(1-t)P₀ - (1-2t)P₁ - tP₂]
			float nt = 1.0f - t;
			vec2 expected = -2.0f * (nt * controlPoints[0] - (1.0f - 2.0f*t) * controlPoints[1] - t * controlPoints[2]);
			REQUIRE( glm::distance( result, expected ) == Approx( 0 ).margin( 0.0001f ) );
		}
	}

	SECTION("derivativeCubicBezier correct formula")
	{
		vec2 controlPoints[4] = { vec2(0, 0), vec2(33, 100), vec2(66, 100), vec2(100, 0) };

		for( float t = 0; t <= 1.0f; t += 0.1f ) {
			vec2 result = derivativeCubicBezier( controlPoints, t );
			// Derivative: 3(1-t)²(P₁-P₀) + 6(1-t)t(P₂-P₁) + 3t²(P₃-P₂)
			// Weights: w0=-3(1-t)², w1=3(1-t)²-6t(1-t), w2=-3t²+6t(1-t), w3=3t²
			float nt = 1.0f - t;
			float w0 = -3.0f*nt*nt;
			float w1 = 3.0f*nt*nt - 6.0f*t*nt;
			float w2 = -3.0f*t*t + 6.0f*t*nt;
			float w3 = 3.0f*t*t;
			vec2 expected = controlPoints[0]*w0 + controlPoints[1]*w1 + controlPoints[2]*w2 + controlPoints[3]*w3;
			REQUIRE( glm::distance( result, expected ) == Approx( 0 ).margin( 0.0001f ) );
		}
	}

	SECTION("subdivideQuadraticBezier produces valid subdivision")
	{
		vec2 src[3] = { vec2(0, 0), vec2(50, 100), vec2(100, 0) };
		vec2 dst[5];
		float t = 0.5f;

		subdivideQuadraticBezier( src, dst, t );

		// First curve should start at src[0]
		REQUIRE( glm::distance( dst[0], src[0] ) == Approx( 0 ).margin( 0.0001f ) );
		// Second curve should end at src[2]
		REQUIRE( glm::distance( dst[4], src[2] ) == Approx( 0 ).margin( 0.0001f ) );
		// Junction point should match evaluation at t
		vec2 junctionPoint = evaluateQuadraticBezier( src, t );
		REQUIRE( glm::distance( dst[2], junctionPoint ) == Approx( 0 ).margin( 0.0001f ) );
	}

	SECTION("subdivideCubicBezier produces valid subdivision")
	{
		vec2 src[4] = { vec2(0, 0), vec2(33, 100), vec2(66, 100), vec2(100, 0) };
		vec2 dst[7];
		float t = 0.5f;

		subdivideCubicBezier( src, dst, t );

		// First curve should start at src[0]
		REQUIRE( glm::distance( dst[0], src[0] ) == Approx( 0 ).margin( 0.0001f ) );
		// Second curve should end at src[3]
		REQUIRE( glm::distance( dst[6], src[3] ) == Approx( 0 ).margin( 0.0001f ) );
		// Junction point should match evaluation at t
		vec2 junctionPoint = evaluateCubicBezier( src, t );
		REQUIRE( glm::distance( dst[3], junctionPoint ) == Approx( 0 ).margin( 0.0001f ) );
	}

	SECTION("findQuadraticBezierExtrema finds correct extrema")
	{
		vec2 controlPoints[3] = { vec2(0, 0), vec2(50, 100), vec2(100, 0) };

		float resultT[2];
		int count = findQuadraticBezierExtrema( controlPoints, resultT );

		// For this curve, derivative is zero when:
		// d/dt[(1-t)²·0 + 2t(1-t)·50 + t²·100] = 0 for x
		// d/dt[(1-t)²·0 + 2t(1-t)·100 + t²·0] = 0 for y
		// y extremum: 2(1-t)·100 - 2t·100 = 0 → t = 0.5
		// x extremum: solving gives t = 0.5
		REQUIRE( count == 1 ); // y has extremum, x is monotonic due to symmetry
		REQUIRE( resultT[0] == Approx( 0.5f ).margin( 0.0001f ) );
	}

	SECTION("findCubicBezierExtrema finds correct extrema")
	{
		vec2 controlPoints[4] = { vec2(0, 0), vec2(33, 100), vec2(66, 100), vec2(100, 0) };

		float resultT[4];
		int count = findCubicBezierExtrema( controlPoints, resultT );

		// This curve has a y extremum at t=0.5 (symmetric control points)
		// Verify at least one extremum exists and is in valid range
		REQUIRE( count >= 1 );
		REQUIRE( count <= 4 );
		for( int i = 0; i < count; ++i ) {
			REQUIRE( resultT[i] > 0.0f );
			REQUIRE( resultT[i] < 1.0f );
		}
		// For this symmetric curve, expect y extremum near t=0.5
		bool hasYExtremum = false;
		for( int i = 0; i < count; ++i ) {
			if( std::abs(resultT[i] - 0.5f) < 0.1f ) {
				hasYExtremum = true;
				break;
			}
		}
		REQUIRE( hasYExtremum );
	}
}
TEST_CASE("CinderMath: Second Derivatives")
{
	SECTION("Quadratic Bezier second derivative is constant") {
		vec2 controlPoints[] = { vec2(0, 0), vec2(50, 100), vec2(100, 0) };

		// Second derivative of quadratic Bezier is constant (no t parameter)
		vec2 d2 = secondDerivativeQuadraticBezier(controlPoints);

		// Verify against formula: 2*(P2 - 2*P1 + P0)
		vec2 expected = 2.0f * (controlPoints[2] - 2.0f * controlPoints[1] + controlPoints[0]);
		REQUIRE( glm::distance(d2, expected) < 0.0001f );
	}
	
	SECTION("Cubic Bezier second derivative") {
		vec2 controlPoints[] = { vec2(0, 0), vec2(33, 100), vec2(66, 100), vec2(100, 0) };
		
		// Second derivative at endpoints
		vec2 d2_0 = secondDerivativeCubicBezier(controlPoints, 0.0f);
		vec2 d2_1 = secondDerivativeCubicBezier(controlPoints, 1.0f);
		
		// Verify against formula at t=0: 6*(P2 - 2*P1 + P0)
		vec2 expected_0 = 6.0f * (controlPoints[2] - 2.0f * controlPoints[1] + controlPoints[0]);
		REQUIRE( glm::distance(d2_0, expected_0) < 0.0001f );
		
		// At t=1: 6*(P3 - 2*P2 + P1)
		vec2 expected_1 = 6.0f * (controlPoints[3] - 2.0f * controlPoints[2] + controlPoints[1]);
		REQUIRE( glm::distance(d2_1, expected_1) < 0.0001f );
	}
}

TEST_CASE("CinderMath: Curvature")
{
	SECTION("Curvature of straight line is zero") {
		vec2 firstDeriv(1, 0);  // Unit tangent
		vec2 secondDeriv(0, 0); // No curvature
		
		float kappa = curvature(firstDeriv, secondDeriv);
		REQUIRE( std::abs(kappa) < 0.0001f );
	}
	
	SECTION("Curvature of circle") {
		// Circle of radius R has curvature 1/R
		float R = 10.0f;
		float t = 0.5f;
		
		// Parametric circle: x=R*cos(t), y=R*sin(t)
		vec2 firstDeriv(-R * std::sin(t), R * std::cos(t));
		vec2 secondDeriv(-R * std::cos(t), -R * std::sin(t));
		
		float kappa = curvature(firstDeriv, secondDeriv);
		float expected = 1.0f / R;
		
		REQUIRE( std::abs(kappa - expected) < 0.01f );
	}
	
	SECTION("Curvature handles zero velocity") {
		vec2 firstDeriv(0, 0);  // Zero velocity
		vec2 secondDeriv(1, 1);
		
		float kappa = curvature(firstDeriv, secondDeriv);
		REQUIRE( kappa == 0.0f );  // Should return 0, not NaN
	}
}

TEST_CASE("CinderMath: deflateCubic")
{
	SECTION("Deflate (x-1)³ by root 1") {
		// (x-1)³ = x³ - 3x² + 3x - 1
		float c0 = -1, c1 = 3, c2 = -3, c3 = 1;
		float root = 1.0f;
		
		float quad[3];
		deflateCubic(c0, c1, c2, c3, root, quad);
		
		// Should give (x-1)² = x² - 2x + 1
		REQUIRE( std::abs(quad[2] - 1.0f) < 0.0001f );   // x² coefficient
		REQUIRE( std::abs(quad[1] - (-2.0f)) < 0.0001f ); // x coefficient
		REQUIRE( std::abs(quad[0] - 1.0f) < 0.0001f );   // constant
	}
	
	SECTION("Deflate x³ - 6x² + 11x - 6 by root 1") {
		// (x-1)(x-2)(x-3) with root 1 -> (x-2)(x-3)
		float c0 = -6, c1 = 11, c2 = -6, c3 = 1;
		float root = 1.0f;
		
		float quad[3];
		deflateCubic(c0, c1, c2, c3, root, quad);
		
		// Should give x² - 5x + 6 = (x-2)(x-3)
		REQUIRE( std::abs(quad[2] - 1.0f) < 0.0001f );
		REQUIRE( std::abs(quad[1] - (-5.0f)) < 0.0001f );
		REQUIRE( std::abs(quad[0] - 6.0f) < 0.0001f );
	}
}

TEST_CASE("CinderMath: solveCubicYuksel")
{
	SECTION("Triple root") {
		// (x-1)³ = x³ - 3x² + 3x - 1
		float result[3];
		int n = solveCubicYuksel(1.0f, -3.0f, 3.0f, -1.0f, result);

		REQUIRE( n >= 1 );  // Should find at least one root
		REQUIRE( std::abs(result[0] - 1.0f) < 0.001f );
	}
	
	SECTION("Three distinct roots") {
		// (x-1)(x-2)(x-3) = x³ - 6x² + 11x - 6
		float result[3];
		int n = solveCubicYuksel(1.0f, -6.0f, 11.0f, -6.0f, result);
		
		REQUIRE( n == 3 );
		REQUIRE( std::abs(result[0] - 1.0f) < 0.001f );
		REQUIRE( std::abs(result[1] - 2.0f) < 0.001f );
		REQUIRE( std::abs(result[2] - 3.0f) < 0.001f );
	}
	
	SECTION("One real root") {
		// x³ + x + 1 has one real root around -0.68
		float result[3];
		int n = solveCubicYuksel(1.0f, 0.0f, 1.0f, 1.0f, result);
		
		REQUIRE( n == 1 );
		// Verify it's actually a root
		float x = result[0];
		float val = x*x*x + x + 1.0f;
		REQUIRE( std::abs(val) < 0.001f );
	}
	
	SECTION("Endpoint root") {
		// x³ - x = x(x-1)(x+1), roots at -1, 0, 1
		float result[3];
		int n = solveCubicYuksel(1.0f, 0.0f, -1.0f, 0.0f, result);
		
		REQUIRE( n >= 1 );  // Should find at least the endpoint roots
		// Check that 0 is found
		bool foundZero = false;
		for (int i = 0; i < n; ++i) {
			if (std::abs(result[i]) < 0.001f) {
				foundZero = true;
				break;
			}
		}
		REQUIRE( foundZero );
	}
}

TEST_CASE("CinderMath: solveQuartic")
{
	SECTION("(x-1)⁴ = 0") {
		float result[4];
		int n = solveQuartic(1.0f, -4.0f, 6.0f, -4.0f, 1.0f, result);
		
		REQUIRE( n >= 1 );
		REQUIRE( std::abs(result[0] - 1.0f) < 0.001f );
	}
	
	SECTION("Four distinct roots") {
		// (x-1)(x-2)(x-3)(x-4) = x⁴ - 10x³ + 35x² - 50x + 24
		float result[4];
		int n = solveQuartic(24.0f, -50.0f, 35.0f, -10.0f, 1.0f, result);
		
		REQUIRE( n == 4 );
		REQUIRE( std::abs(result[0] - 1.0f) < 0.001f );
		REQUIRE( std::abs(result[1] - 2.0f) < 0.001f );
		REQUIRE( std::abs(result[2] - 3.0f) < 0.001f );
		REQUIRE( std::abs(result[3] - 4.0f) < 0.001f );
	}
	
	SECTION("Two real roots") {
		// (x²-1)(x²+1) = x⁴ - 1
		float result[4];
		int n = solveQuartic(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, result);
		
		REQUIRE( n == 2 );
		REQUIRE( std::abs(result[0] - (-1.0f)) < 0.001f );
		REQUIRE( std::abs(result[1] - 1.0f) < 0.001f );
	}
	
	SECTION("No real roots") {
		// x⁴ + 1 has no real roots
		float result[4];
		int n = solveQuartic(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, result);

		REQUIRE( n == 0 );
	}
}

TEST_CASE("Path2d::convertQuadraticsToCubics")
{
	SECTION("Single quadratic conversion") {
		// Test that a simple quadratic is converted correctly
		Path2d path;
		path.moveTo( 0, 0 );
		path.quadTo( 50, 100, 100, 0 );

		// Should have 1 segment (QUADTO) - moveTo is implicit
		REQUIRE( path.getNumSegments() == 1 );
		REQUIRE( path.getSegmentType( 0 ) == Path2d::QUADTO );
		REQUIRE( path.getNumPoints() == 3 ); // (0,0), (50,100), (100,0)

		path.convertQuadraticsToCubics();

		// Should now have 1 segment (CUBICTO)
		REQUIRE( path.getNumSegments() == 1 );
		REQUIRE( path.getSegmentType( 0 ) == Path2d::CUBICTO );
		REQUIRE( path.getNumPoints() == 4 ); // (0,0), Q1, Q2, (100,0)
	}

	SECTION("No quadratics - no change") {
		// Path with no quadratics should remain unchanged
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );
		path.lineTo( 100, 0 );
		path.close();

		size_t origSegments = path.getNumSegments();
		size_t origPoints = path.getNumPoints();

		path.convertQuadraticsToCubics();

		// Should be unchanged
		REQUIRE( path.getNumSegments() == origSegments );
		REQUIRE( path.getNumPoints() == origPoints );
	}

	SECTION("Multiple quadratics") {
		// Test path with multiple quadratics
		Path2d path;
		path.moveTo( 0, 0 );
		path.quadTo( 25, 50, 50, 0 );
		path.quadTo( 75, -50, 100, 0 );

		// Should have 2 QUADTO segments (moveTo is implicit)
		REQUIRE( path.getNumSegments() == 2 );
		int quadCount = 0;
		for( size_t i = 0; i < path.getNumSegments(); ++i ) {
			if( path.getSegmentType( i ) == Path2d::QUADTO ) quadCount++;
		}
		REQUIRE( quadCount == 2 );

		path.convertQuadraticsToCubics();

		// All quadratics should be converted to cubics
		int cubicCount = 0;
		quadCount = 0;
		for( size_t i = 0; i < path.getNumSegments(); ++i ) {
			if( path.getSegmentType( i ) == Path2d::CUBICTO ) cubicCount++;
			if( path.getSegmentType( i ) == Path2d::QUADTO ) quadCount++;
		}
		REQUIRE( quadCount == 0 );
		REQUIRE( cubicCount == 2 );
	}

	SECTION("Closed path with quadratics") {
		// Test closed path
		Path2d path;
		path.moveTo( 0, 0 );
		path.quadTo( 50, 50, 100, 0 );
		path.lineTo( 100, 100 );
		path.close();

		Path2d converted = path.convertedToCubics();

		// Should have no quadratics
		for( size_t i = 0; i < converted.getNumSegments(); ++i ) {
			REQUIRE( converted.getSegmentType( i ) != Path2d::QUADTO );
		}

		// Should still be closed
		REQUIRE( converted.isClosed() );
	}

	SECTION("In-place vs copy conversion equivalence") {
		// Test that both methods produce the same result
		Path2d path;
		path.moveTo( 0, 0 );
		path.quadTo( 25, 50, 50, 0 );
		path.lineTo( 75, 25 );
		path.quadTo( 85, -25, 100, 0 );

		Path2d inPlace = path;
		inPlace.convertQuadraticsToCubics();

		Path2d copy = path.convertedToCubics();

		// Should have same structure
		REQUIRE( inPlace.getNumSegments() == copy.getNumSegments() );
		REQUIRE( inPlace.getNumPoints() == copy.getNumPoints() );

		// All segment types should match
		for( size_t i = 0; i < inPlace.getNumSegments(); ++i ) {
			REQUIRE( inPlace.getSegmentType( i ) == copy.getSegmentType( i ) );
		}
	}
}
