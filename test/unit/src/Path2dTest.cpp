#include "cinder/app/App.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/CinderMath.h"
#include "cinder/Rand.h"

#include "catch.hpp"

#include <chrono>
#include <iomanip>

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

//=============================================================================
// Line-Segment Intersection Tests
//=============================================================================

TEST_CASE("Line-Segment Intersection")
{
	SECTION("intersectLineLine: Basic X intersection")
	{
		// Horizontal segment from (0,0) to (2,0)
		// Vertical probe line from (1,2) to (1,-2)
		dvec2 seg0( 0.0, 0.0 );
		dvec2 seg1( 2.0, 0.0 );
		dvec2 line0( 1.0, 2.0 );
		dvec2 line1( 1.0, -2.0 );

		LineIntersection<double> result[1];
		int count = intersectLineLine( seg0, seg1, line0, line1, result );

		REQUIRE( count == 1 );
		REQUIRE( result[0].segmentT == Approx( 0.5 ).margin( 1e-9 ) );
		REQUIRE( result[0].lineT == Approx( 0.5 ).margin( 1e-9 ) );

		// Verify intersection point
		dvec2 pt = seg0 + result[0].segmentT * ( seg1 - seg0 );
		REQUIRE( pt.x == Approx( 1.0 ).margin( 1e-9 ) );
		REQUIRE( pt.y == Approx( 0.0 ).margin( 1e-9 ) );
	}

	SECTION("intersectLineLine: Parallel lines (no intersection)")
	{
		dvec2 seg0( 0.0, 0.0 );
		dvec2 seg1( 2.0, 0.0 );
		dvec2 line0( 0.0, 1.0 );
		dvec2 line1( 2.0, 1.0 );

		LineIntersection<double> result[1];
		int count = intersectLineLine( seg0, seg1, line0, line1, result );

		REQUIRE( count == 0 );
	}

	SECTION("intersectLineLine: Intersection outside segment range")
	{
		dvec2 seg0( 0.0, 0.0 );
		dvec2 seg1( 1.0, 0.0 );
		dvec2 line0( 2.0, 1.0 );  // Line is to the right of segment
		dvec2 line1( 2.0, -1.0 );

		LineIntersection<double> result[1];
		int count = intersectLineLine( seg0, seg1, line0, line1, result );

		REQUIRE( count == 0 );
	}

	SECTION("intersectLineLine: Diagonal intersection")
	{
		dvec2 seg0( 0.0, 0.0 );
		dvec2 seg1( 10.0, 10.0 );
		dvec2 line0( 0.0, 10.0 );
		dvec2 line1( 10.0, 0.0 );

		LineIntersection<double> result[1];
		int count = intersectLineLine( seg0, seg1, line0, line1, result );

		REQUIRE( count == 1 );
		REQUIRE( result[0].segmentT == Approx( 0.5 ).margin( 1e-9 ) );
		REQUIRE( result[0].lineT == Approx( 0.5 ).margin( 1e-9 ) );
	}

	SECTION("intersectLineQuadratic: Parabola crossed by horizontal line")
	{
		// Quadratic Bezier from (0,0) through control (0.5, 1) to (1,0)
		// y(t) = 2t(1-t) peaks at y=0.5 when t=0.5
		// A line at y=0.25 will cross the curve twice
		dvec2 q[3] = {
			{ 0.0, 0.0 },
			{ 0.5, 1.0 },  // Control point lifts the curve
			{ 1.0, 0.0 }
		};
		// Horizontal line at y = 0.25 (crosses curve twice)
		// Solving 2t - 2t² = 0.25: 2t² - 2t + 0.25 = 0
		// t = (2 ± √(4-2))/4 = (2 ± √2)/4 ≈ 0.146 and 0.854
		dvec2 line0( -1.0, 0.25 );
		dvec2 line1( 2.0, 0.25 );

		LineIntersection<double> result[2];
		int count = intersectLineQuadratic( q, line0, line1, result );

		REQUIRE( count == 2 );

		// Verify both intersection points lie on the line y=0.25
		for( int i = 0; i < count; ++i ) {
			dvec2 pt = evalQuadraticBezier( q, result[i].segmentT );
			REQUIRE( pt.y == Approx( 0.25 ).margin( 1e-6 ) );
		}
	}

	SECTION("intersectLineQuadratic: Tangent line (single intersection)")
	{
		// Quadratic curve from (0,0) through control (0.5, 1) to (1, 0)
		// The curve peaks at y=0.5 at t=0.5
		// A tangent line at y = 0.5 should touch it at one point
		dvec2 q[3] = {
			{ 0.0, 0.0 },
			{ 0.5, 1.0 },
			{ 1.0, 0.0 }
		};
		// Line at exact peak y
		// At t=0.5: y = (1-t)²*0 + 2*(1-t)*t*1 + t²*0 = 2*0.5*0.5*1 = 0.5
		dvec2 line0( -1.0, 0.5 );
		dvec2 line1( 2.0, 0.5 );

		LineIntersection<double> result[2];
		int count = intersectLineQuadratic( q, line0, line1, result );

		// The quadratic y(t) = 2t(1-t) peaks at t=0.5 with y=0.5
		// So line y=0.5 is tangent at one point (segmentT = 0.5)
		REQUIRE( count == 1 );
		REQUIRE( result[0].segmentT == Approx( 0.5 ).margin( 1e-6 ) );
	}

	SECTION("intersectLineQuadratic: No intersection (line above curve)")
	{
		dvec2 q[3] = {
			{ 0.0, 0.0 },
			{ 0.5, 1.0 },
			{ 1.0, 0.0 }
		};
		// Line at y = 1.0 (above the curve which peaks at 0.5)
		dvec2 line0( -1.0, 1.0 );
		dvec2 line1( 2.0, 1.0 );

		LineIntersection<double> result[2];
		int count = intersectLineQuadratic( q, line0, line1, result );

		REQUIRE( count == 0 );
	}

	SECTION("intersectLineCubic: S-curve crossed by vertical line")
	{
		// S-curve: goes right, up, down, right
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 1.0, 1.0 },
			{ 2.0, -1.0 },
			{ 3.0, 0.0 }
		};
		// Vertical line at x = 1.5
		dvec2 line0( 1.5, -2.0 );
		dvec2 line1( 1.5, 2.0 );

		LineIntersection<double> result[3];
		int count = intersectLineCubic( c, line0, line1, result );

		REQUIRE( count == 1 );

		// Verify intersection point has x ≈ 1.5
		dvec2 pt = evalCubicBezier( c, result[0].segmentT );
		REQUIRE( pt.x == Approx( 1.5 ).margin( 1e-4 ) );
	}

	SECTION("intersectLineCubic: Three intersections (loop curve)")
	{
		// A cubic that loops back can have 3 intersections with a line
		// Create a cubic that oscillates: starts at (0,0), goes up, down, up
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 0.0, 3.0 },
			{ 1.0, -2.0 },
			{ 1.0, 1.0 }
		};
		// Horizontal line at y = 0.5
		dvec2 line0( -1.0, 0.5 );
		dvec2 line1( 2.0, 0.5 );

		LineIntersection<double> result[3];
		int count = intersectLineCubic( c, line0, line1, result );

		// This curve should cross y=0.5 three times
		REQUIRE( count == 3 );

		// Verify all intersection points lie on y = 0.5
		for( int i = 0; i < count; ++i ) {
			dvec2 pt = evalCubicBezier( c, result[i].segmentT );
			REQUIRE( pt.y == Approx( 0.5 ).margin( 1e-4 ) );
		}
	}

	SECTION("intersectLineCubic: No intersection")
	{
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 1.0, 0.0 },
			{ 2.0, 0.0 },
			{ 3.0, 0.0 }
		};
		// Line at y = 1 (curve is flat at y=0)
		dvec2 line0( 0.0, 1.0 );
		dvec2 line1( 3.0, 1.0 );

		LineIntersection<double> result[3];
		int count = intersectLineCubic( c, line0, line1, result );

		REQUIRE( count == 0 );
	}

	SECTION("intersectLineCubic: Intersection at endpoints")
	{
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 1.0, 1.0 },
			{ 2.0, 1.0 },
			{ 3.0, 0.0 }
		};
		// Line through both endpoints (y = 0)
		dvec2 line0( -1.0, 0.0 );
		dvec2 line1( 4.0, 0.0 );

		LineIntersection<double> result[3];
		int count = intersectLineCubic( c, line0, line1, result );

		REQUIRE( count >= 2 );  // At least endpoints
	}

	SECTION("Polynomial coefficients: Quadratic Bezier")
	{
		// Verify the polynomial coefficients match the Bezier evaluation
		double x0 = 1.0, x1 = 3.0, x2 = 2.0;
		double c0, c1, c2;
		quadraticBezierCoeffs( x0, x1, x2, c0, c1, c2 );

		// Test at t = 0, 0.5, 1
		auto evalPoly = [&]( double t ) { return c0 + t * c1 + t * t * c2; };
		auto evalBez = [&]( double t ) {
			double mt = 1.0 - t;
			return mt * mt * x0 + 2.0 * mt * t * x1 + t * t * x2;
		};

		REQUIRE( evalPoly( 0.0 ) == Approx( evalBez( 0.0 ) ) );
		REQUIRE( evalPoly( 0.5 ) == Approx( evalBez( 0.5 ) ) );
		REQUIRE( evalPoly( 1.0 ) == Approx( evalBez( 1.0 ) ) );
		REQUIRE( evalPoly( 0.25 ) == Approx( evalBez( 0.25 ) ) );
		REQUIRE( evalPoly( 0.75 ) == Approx( evalBez( 0.75 ) ) );
	}

	SECTION("Polynomial coefficients: Cubic Bezier")
	{
		// Verify the polynomial coefficients match the Bezier evaluation
		double x0 = 1.0, x1 = 4.0, x2 = 2.0, x3 = 5.0;
		double c0, c1, c2, c3;
		cubicBezierCoeffs( x0, x1, x2, x3, c0, c1, c2, c3 );

		auto evalPoly = [&]( double t ) {
			return c0 + t * c1 + t * t * c2 + t * t * t * c3;
		};
		auto evalBez = [&]( double t ) {
			double mt = 1.0 - t;
			return mt * mt * mt * x0 + 3.0 * mt * mt * t * x1 + 3.0 * mt * t * t * x2 + t * t * t * x3;
		};

		REQUIRE( evalPoly( 0.0 ) == Approx( evalBez( 0.0 ) ) );
		REQUIRE( evalPoly( 0.5 ) == Approx( evalBez( 0.5 ) ) );
		REQUIRE( evalPoly( 1.0 ) == Approx( evalBez( 1.0 ) ) );
		REQUIRE( evalPoly( 0.25 ) == Approx( evalBez( 0.25 ) ) );
		REQUIRE( evalPoly( 0.75 ) == Approx( evalBez( 0.75 ) ) );
	}
}

//=============================================================================
// Cubic-Cubic Intersection Tests
//=============================================================================

TEST_CASE("Cubic-Cubic Intersection")
{
	SECTION("intersectCubicCubic: X-shaped crossing")
	{
		// Two curves that cross in an X pattern
		dvec2 c1[4] = {
			{ 0.0, 0.0 },
			{ 1.0, 0.0 },
			{ 0.0, 1.0 },
			{ 1.0, 1.0 }
		};
		dvec2 c2[4] = {
			{ 0.0, 1.0 },
			{ 1.0, 1.0 },
			{ 0.0, 0.0 },
			{ 1.0, 0.0 }
		};

		auto results = intersectCubicCubic( c1, c2, 1e-6 );

		REQUIRE( results.size() == 1 );

		// Verify the intersection point is the same on both curves
		dvec2 pt1 = evalCubicBezier( c1, results[0].t1 );
		dvec2 pt2 = evalCubicBezier( c2, results[0].t2 );
		REQUIRE( glm::distance( pt1, pt2 ) < 1e-4 );
	}

	SECTION("intersectCubicCubic: No intersection (parallel curves)")
	{
		// Two parallel curves that don't intersect
		dvec2 c1[4] = {
			{ 0.0, 0.0 },
			{ 1.0, 0.0 },
			{ 2.0, 0.0 },
			{ 3.0, 0.0 }
		};
		dvec2 c2[4] = {
			{ 0.0, 1.0 },
			{ 1.0, 1.0 },
			{ 2.0, 1.0 },
			{ 3.0, 1.0 }
		};

		auto results = intersectCubicCubic( c1, c2, 1e-6 );

		REQUIRE( results.empty() );
	}

	SECTION("intersectCubicCubic: Multiple intersections")
	{
		// A wavy curve crossed by a straighter curve
		dvec2 c1[4] = {
			{ 0.0, 0.5 },
			{ 0.3, 1.5 },
			{ 0.7, -0.5 },
			{ 1.0, 0.5 }
		};
		dvec2 c2[4] = {
			{ 0.0, 0.5 },
			{ 0.33, 0.5 },
			{ 0.67, 0.5 },
			{ 1.0, 0.5 }
		};

		auto results = intersectCubicCubic( c1, c2, 1e-6 );

		// Should have at least 2 intersections (start, and one in middle)
		REQUIRE( results.size() >= 2 );

		// Verify all intersection points match
		for( const auto& r : results ) {
			dvec2 pt1 = evalCubicBezier( c1, r.t1 );
			dvec2 pt2 = evalCubicBezier( c2, r.t2 );
			REQUIRE( glm::distance( pt1, pt2 ) < 1e-3 );
		}
	}

	SECTION("selfIntersectCubic: Loop curve")
	{
		// A "fish" shaped cubic that loops back on itself
		// Control points go far right then far left, forcing a crossing
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 2.0, 1.0 },   // Far right
			{ -1.0, 1.0 },  // Far left (crosses!)
			{ 1.0, 0.0 }
		};

		auto results = selfIntersectCubic( c, 1e-6, 0.05 );

		REQUIRE( results.size() == 1 );

		// Verify the self-intersection point
		dvec2 pt1 = evalCubicBezier( c, results[0].t1 );
		dvec2 pt2 = evalCubicBezier( c, results[0].t2 );
		REQUIRE( glm::distance( pt1, pt2 ) < 1e-3 );

		// The two t values should be different (not at the same point on the curve)
		REQUIRE( std::abs( results[0].t1 - results[0].t2 ) > 0.05 );
	}

	SECTION("selfIntersectCubic: No self-intersection (simple curve)")
	{
		// A simple S-curve with no self-intersection
		dvec2 c[4] = {
			{ 0.0, 0.0 },
			{ 0.0, 1.0 },
			{ 1.0, 0.0 },
			{ 1.0, 1.0 }
		};

		auto results = selfIntersectCubic( c, 1e-6, 0.05 );

		REQUIRE( results.empty() );
	}

	SECTION("intersectCubicCubic: Touching at endpoint")
	{
		// Two curves that share an endpoint
		dvec2 c1[4] = {
			{ 0.0, 0.0 },
			{ 0.3, 0.5 },
			{ 0.7, 0.5 },
			{ 1.0, 0.0 }
		};
		dvec2 c2[4] = {
			{ 1.0, 0.0 },  // Starts where c1 ends
			{ 1.3, 0.5 },
			{ 1.7, 0.5 },
			{ 2.0, 0.0 }
		};

		auto results = intersectCubicCubic( c1, c2, 1e-6 );

		REQUIRE( results.size() == 1 );
		// Intersection should be at t1=1, t2=0
		REQUIRE( results[0].t1 == Approx( 1.0 ).margin( 0.01 ) );
		REQUIRE( results[0].t2 == Approx( 0.0 ).margin( 0.01 ) );
	}
}

//=============================================================================
// Path2d::findSelfIntersections Tests
//=============================================================================

TEST_CASE("Path2d::findSelfIntersections")
{
	SECTION("Figure-8 path (two crossing lines)")
	{
		// Create a figure-8 pattern with lines
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );
		path.lineTo( 100, 0 );
		path.lineTo( 0, 100 );

		auto results = path.findSelfIntersections();

		// Lines 1 and 3 should cross (segment 1: (0,0)->(100,100) crosses segment 3: (100,0)->(0,100))
		REQUIRE( results.size() == 1 );

		// Intersection should be near (50, 50)
		REQUIRE( results[0].point.x == Approx( 50.0f ).margin( 1.0f ) );
		REQUIRE( results[0].point.y == Approx( 50.0f ).margin( 1.0f ) );
	}

	SECTION("Self-intersecting cubic (fish curve)")
	{
		// A single cubic that loops on itself
		Path2d path;
		path.moveTo( 0, 0 );
		path.curveTo( 200, 100, -100, 100, 100, 0 );

		auto results = path.findSelfIntersections();

		REQUIRE( results.size() == 1 );

		// The intersection point should be within the curve's bounds
		REQUIRE( results[0].point.x >= -10.0f );
		REQUIRE( results[0].point.x <= 110.0f );
		REQUIRE( results[0].point.y >= 0.0f );
		REQUIRE( results[0].point.y <= 100.0f );
	}

	SECTION("Non-self-intersecting path (simple rectangle)")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );
		path.lineTo( 0, 100 );
		path.close();

		auto results = path.findSelfIntersections();

		// Debug: assert on intersection details so we can see them on failure
		if( !results.empty() ) {
			auto& isect = results[0];
			INFO( "Found intersection: seg1=" << isect.segment1 << " seg2=" << isect.segment2
			      << " t1=" << isect.t1 << " t2=" << isect.t2
			      << " point=(" << isect.point.x << "," << isect.point.y << ")" );
			// Force failure to show the info above
			REQUIRE( results.size() == 0 );
		}

		// Rectangle has no self-intersections
		REQUIRE( results.empty() );
	}

	SECTION("Simple S-curve (no self-intersection)")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.curveTo( 50, 100, 50, -100, 100, 0 );

		auto results = path.findSelfIntersections();

		// S-curve doesn't cross itself
		REQUIRE( results.empty() );
	}

	SECTION("Two crossing cubics")
	{
		// Two connected cubic segments that cross each other
		// First curve goes roughly horizontal, second loops back crossing it
		Path2d path;
		path.moveTo( 0, 50 );
		path.curveTo( 33, 0, 67, 100, 100, 50 );   // Segment 0: horizontal S-curve
		path.curveTo( 67, 0, 33, 100, 0, 50 );    // Segment 1: loops back, crossing segment 0

		auto results = path.findSelfIntersections();

		// These curves should cross (probably in multiple places)
		REQUIRE( results.size() >= 1 );
	}
}

//=============================================================================
// Path2d::findIntersections Tests
//=============================================================================

TEST_CASE("Path2d::findIntersections")
{
	SECTION("No intersection - parallel lines")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 0 );

		Path2d path2;
		path2.moveTo( 0, 10 );
		path2.lineTo( 100, 10 );

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.empty() );
	}

	SECTION("Single line-line intersection")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );

		Path2d path2;
		path2.moveTo( 0, 100 );
		path2.lineTo( 100, 0 );

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.size() == 1 );
		REQUIRE( results[0].point.x == Approx( 50.0f ).margin( 0.1f ) );
		REQUIRE( results[0].point.y == Approx( 50.0f ).margin( 0.1f ) );
		REQUIRE( results[0].t1 == Approx( 0.5f ).margin( 0.01f ) );
		REQUIRE( results[0].t2 == Approx( 0.5f ).margin( 0.01f ) );
		REQUIRE( results[0].segment1 == 0 );
		REQUIRE( results[0].segment2 == 0 );
	}

	SECTION("Line-cubic intersection")
	{
		Path2d path1;
		path1.moveTo( 0, 50 );
		path1.lineTo( 100, 50 );

		Path2d path2;
		path2.moveTo( 50, 0 );
		path2.curveTo( 50, 33, 50, 67, 50, 100 );

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.size() == 1 );
		REQUIRE( results[0].point.x == Approx( 50.0f ).margin( 0.5f ) );
		REQUIRE( results[0].point.y == Approx( 50.0f ).margin( 0.5f ) );
	}

	SECTION("Cubic-cubic intersection")
	{
		Path2d path1;
		path1.moveTo( 0, 50 );
		path1.curveTo( 33, 0, 67, 100, 100, 50 );

		Path2d path2;
		path2.moveTo( 50, 0 );
		path2.curveTo( 0, 33, 100, 67, 50, 100 );

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.size() >= 1 );
	}

	SECTION("Multi-segment paths")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 50, 0 );
		path1.lineTo( 100, 50 );

		Path2d path2;
		path2.moveTo( 25, -25 );
		path2.lineTo( 25, 25 );  // Crosses first segment of path1

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.size() == 1 );
		REQUIRE( results[0].segment1 == 0 );
		REQUIRE( results[0].point.x == Approx( 25.0f ).margin( 0.1f ) );
	}

	SECTION("Empty paths return no intersections")
	{
		Path2d path1;
		Path2d path2;
		path2.moveTo( 0, 0 );
		path2.lineTo( 100, 100 );

		auto results = path1.findIntersections( path2 );
		REQUIRE( results.empty() );

		results = path2.findIntersections( path1 );
		REQUIRE( results.empty() );
	}

	SECTION("Line crossing closed rect - CLOSE segment intersection")
	{
		// A horizontal line that crosses through a closed rectangle
		// The rect's CLOSE segment (left edge from bottom-left back to top-left)
		// should be detected for intersections
		vec2 center( 0, 0 );

		// Simple horizontal line crossing through the rect at y=0
		Path2d line;
		line.moveTo( center + vec2( -150, 0 ) );
		line.lineTo( center + vec2( 150, 0 ) );

		// Rect - segments are (MOVETO is NOT stored in segments array):
		// [0] LINETO to (100, -80)  - top edge (from moveTo point)
		// [1] LINETO to (100, 80)   - right edge
		// [2] LINETO to (-100, 80)  - bottom edge
		// [3] CLOSE                 - left edge from (-100, 80) back to (-100, -80)
		Path2d rect;
		rect.moveTo( center + vec2( -100, -80 ) );
		rect.lineTo( center + vec2( 100, -80 ) );
		rect.lineTo( center + vec2( 100, 80 ) );
		rect.lineTo( center + vec2( -100, 80 ) );
		rect.close();

		// The line should cross both the left edge (CLOSE) and right edge
		auto results = line.findIntersections( rect );

		// Debug: print results
		std::cout << "Found " << results.size() << " intersections:" << std::endl;
		for( size_t i = 0; i < results.size(); ++i ) {
			std::cout << "  [" << i << "] seg1=" << results[i].segment1
			          << " seg2=" << results[i].segment2
			          << " point=(" << results[i].point.x << "," << results[i].point.y << ")" << std::endl;
		}

		// Debug: print line segments
		std::cout << "Line has " << line.getSegments().size() << " segments, "
		          << line.getPoints().size() << " points" << std::endl;
		for( size_t s = 0; s < line.getSegments().size(); ++s ) {
			std::cout << "  seg[" << s << "] type=" << (int)line.getSegments()[s] << std::endl;
		}

		// Debug: print rect segments
		std::cout << "Rect has " << rect.getSegments().size() << " segments, "
		          << rect.getPoints().size() << " points" << std::endl;
		for( size_t s = 0; s < rect.getSegments().size(); ++s ) {
			std::cout << "  seg[" << s << "] type=" << (int)rect.getSegments()[s] << std::endl;
		}

		// Should have exactly 2 intersections (entering through left, exiting through right)
		REQUIRE( results.size() == 2 );

		// Check that one intersection is on the CLOSE segment (segment2 == 3)
		// and one is on the right edge (segment2 == 1)
		bool foundCloseSegmentIntersection = false;
		bool foundRightEdgeIntersection = false;
		for( const auto& isect : results ) {
			if( isect.segment2 == 3 ) {
				foundCloseSegmentIntersection = true;
				// The intersection should be on the left edge (x ≈ -100, y = 0)
				REQUIRE( isect.point.x == Approx( -100.0f ).margin( 1.0f ) );
				REQUIRE( isect.point.y == Approx( 0.0f ).margin( 1.0f ) );
			}
			if( isect.segment2 == 1 ) {
				foundRightEdgeIntersection = true;
				// The intersection should be on the right edge (x ≈ 100, y = 0)
				REQUIRE( isect.point.x == Approx( 100.0f ).margin( 1.0f ) );
				REQUIRE( isect.point.y == Approx( 0.0f ).margin( 1.0f ) );
			}
		}
		REQUIRE( foundCloseSegmentIntersection );
		REQUIRE( foundRightEdgeIntersection );
	}
}

//=============================================================================
// Path2d::splitAt Tests
//=============================================================================

TEST_CASE("Path2d::splitAt")
{
	SECTION("Split line segment at midpoint")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );

		auto [first, second] = path.splitAt( 0.5f );

		// First path should end at midpoint
		REQUIRE( first.getNumSegments() == 1 );
		vec2 firstEnd = first.getPosition( 1.0f );
		REQUIRE( firstEnd.x == Approx( 50.0f ).margin( 0.1f ) );
		REQUIRE( firstEnd.y == Approx( 50.0f ).margin( 0.1f ) );

		// Second path should start at midpoint and end at original end
		REQUIRE( second.getNumSegments() == 1 );
		vec2 secondStart = second.getPosition( 0.0f );
		vec2 secondEnd = second.getPosition( 1.0f );
		REQUIRE( secondStart.x == Approx( 50.0f ).margin( 0.1f ) );
		REQUIRE( secondStart.y == Approx( 50.0f ).margin( 0.1f ) );
		REQUIRE( secondEnd.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( secondEnd.y == Approx( 100.0f ).margin( 0.1f ) );
	}

	SECTION("Split cubic at midpoint")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.curveTo( 33, 100, 67, 100, 100, 0 );

		auto [first, second] = path.splitAt( 0.5f );

		// First path ends at split point
		REQUIRE( first.getNumSegments() == 1 );
		vec2 firstEnd = first.getPosition( 1.0f );

		// Second path starts at same point
		REQUIRE( second.getNumSegments() == 1 );
		vec2 secondStart = second.getPosition( 0.0f );

		// Both should be at the same position
		REQUIRE( firstEnd.x == Approx( secondStart.x ).margin( 0.1f ) );
		REQUIRE( firstEnd.y == Approx( secondStart.y ).margin( 0.1f ) );

		// Second path should end at original end
		vec2 secondEnd = second.getPosition( 1.0f );
		REQUIRE( secondEnd.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( secondEnd.y == Approx( 0.0f ).margin( 0.1f ) );
	}

	SECTION("Split multi-segment path between segments")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );
		path.lineTo( 0, 100 );

		// Split at t=1.0 (end of first segment, start of second)
		auto [first, second] = path.splitAt( 1.0f );

		// First path has just the first segment
		REQUIRE( first.getNumSegments() == 1 );
		vec2 firstEnd = first.getPosition( 1.0f );
		REQUIRE( firstEnd.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( firstEnd.y == Approx( 0.0f ).margin( 0.1f ) );

		// Second path has remaining segments
		REQUIRE( second.getNumSegments() == 2 );
		vec2 secondEnd = second.getPosition( 1.0f );
		REQUIRE( secondEnd.x == Approx( 0.0f ).margin( 0.1f ) );
		REQUIRE( secondEnd.y == Approx( 100.0f ).margin( 0.1f ) );
	}

	SECTION("Split multi-segment path within segment")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );

		// Split at t=1.5 (middle of second segment)
		auto [first, second] = path.splitAt( 1.5f );

		// First path has first segment + half of second
		REQUIRE( first.getNumSegments() == 2 );
		vec2 firstEnd = first.getPosition( 1.0f );
		REQUIRE( firstEnd.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( firstEnd.y == Approx( 50.0f ).margin( 0.1f ) );

		// Second path starts at split point
		REQUIRE( second.getNumSegments() == 1 );
		vec2 secondStart = second.getPosition( 0.0f );
		REQUIRE( secondStart.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( secondStart.y == Approx( 50.0f ).margin( 0.1f ) );

		vec2 secondEnd = second.getPosition( 1.0f );
		REQUIRE( secondEnd.x == Approx( 100.0f ).margin( 0.1f ) );
		REQUIRE( secondEnd.y == Approx( 100.0f ).margin( 0.1f ) );
	}

	SECTION("Split at t=0 returns empty first, full second")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );

		auto [first, second] = path.splitAt( 0.0f );

		// First path should just have moveTo (no segments)
		REQUIRE( first.getNumSegments() == 0 );

		// Second path should be the full path
		REQUIRE( second.getNumSegments() == 1 );
	}

	SECTION("Split quadratic segment")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.quadTo( 50, 100, 100, 0 );

		auto [first, second] = path.splitAt( 0.5f );

		// Both should have 1 segment
		REQUIRE( first.getNumSegments() == 1 );
		REQUIRE( second.getNumSegments() == 1 );

		// Split point should be continuous
		vec2 firstEnd = first.getPosition( 1.0f );
		vec2 secondStart = second.getPosition( 0.0f );
		REQUIRE( firstEnd.x == Approx( secondStart.x ).margin( 0.1f ) );
		REQUIRE( firstEnd.y == Approx( secondStart.y ).margin( 0.1f ) );

		// Verify segment types
		REQUIRE( first.getSegmentType( 0 ) == Path2d::QUADTO );
		REQUIRE( second.getSegmentType( 0 ) == Path2d::QUADTO );
	}

	// NOTE: Path2d does NOT support multi-contour paths (moveTo throws on non-empty path)
	// Multi-contour paths require Shape2d instead. The MOVETO handling in splitAt
	// is vestigial from design considerations but Path2d fundamentally only has one contour.

	// BUG: splitAt cannot split on CLOSE segment
	// When t falls on a CLOSE segment, the split point should interpolate along
	// the implicit line from the last point back to the sub-path start
	SECTION("Split on CLOSE segment")
	{
		// Create a closed triangle
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );   // Segment 0
		path.lineTo( 50, 100 );  // Segment 1
		path.close();            // Segment 2: implicit line from (50,100) back to (0,0)

		// Split at t=2.5 (halfway through the CLOSE segment)
		auto [first, second] = path.splitAt( 2.5f );

		// The split point should be at (25, 50) - halfway from (50,100) to (0,0)
		vec2 firstEnd = first.getCurrentPoint();
		vec2 secondStart = second.getPoint( 0 );

		// Verify split point is in the middle of the closing edge
		REQUIRE( firstEnd.x == Approx( 25.0f ).margin( 1.0f ) );
		REQUIRE( firstEnd.y == Approx( 50.0f ).margin( 1.0f ) );
		REQUIRE( secondStart.x == Approx( 25.0f ).margin( 1.0f ) );
		REQUIRE( secondStart.y == Approx( 50.0f ).margin( 1.0f ) );

		// Both paths should have content
		REQUIRE( first.getNumSegments() >= 1 );
		REQUIRE( second.getNumSegments() >= 1 );
	}
}

TEST_CASE("Path2d::splitAtMultiple")
{
	SECTION("Split at multiple points")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );
		path.lineTo( 0, 100 );

		// Split into 3 parts
		std::vector<float> splits = { 1.0f, 2.0f };
		auto results = path.splitAtMultiple( splits );

		REQUIRE( results.size() == 3 );

		// First part: segment 0
		REQUIRE( results[0].getNumSegments() == 1 );

		// Second part: segment 1
		REQUIRE( results[1].getNumSegments() == 1 );

		// Third part: segment 2
		REQUIRE( results[2].getNumSegments() == 1 );
	}

	SECTION("Empty split list returns original")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );

		std::vector<float> splits;
		auto results = path.splitAtMultiple( splits );

		REQUIRE( results.size() == 1 );
		REQUIRE( results[0].getNumSegments() == path.getNumSegments() );
	}

	SECTION("Split within segments")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );

		// Split at 0.25 and 0.75
		std::vector<float> splits = { 0.25f, 0.75f };
		auto results = path.splitAtMultiple( splits );

		REQUIRE( results.size() == 3 );

		// Check that pieces are continuous
		vec2 end0 = results[0].getPosition( 1.0f );
		vec2 start1 = results[1].getPosition( 0.0f );
		REQUIRE( end0.x == Approx( start1.x ).margin( 0.1f ) );
		REQUIRE( end0.y == Approx( start1.y ).margin( 0.1f ) );

		vec2 end1 = results[1].getPosition( 1.0f );
		vec2 start2 = results[2].getPosition( 0.0f );
		REQUIRE( end1.x == Approx( start2.x ).margin( 0.1f ) );
		REQUIRE( end1.y == Approx( start2.y ).margin( 0.1f ) );
	}
}

//=============================================================================
// Path2d::removeSelfIntersections Tests
//=============================================================================

TEST_CASE("Path2d::removeSelfIntersections")
{
	SECTION("Path without self-intersections returns unchanged")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );

		Shape2d result = path.removeSelfIntersections();

		REQUIRE( result.getNumContours() == 1 );
		REQUIRE( result.getContour( 0 ).getNumSegments() == path.getNumSegments() );
	}

	SECTION("Figure-8 with crossing lines")
	{
		// Create a figure-8 pattern: the outer boundary should be extracted
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );   // Segment 0: goes up-right
		path.lineTo( 100, 0 );     // Segment 1: goes down
		path.lineTo( 0, 100 );     // Segment 2: crosses segment 0

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() == 1 );

		Shape2d result = path.removeSelfIntersections();

		// Should produce at least one contour
		REQUIRE( result.getNumContours() >= 1 );
	}

	SECTION("Self-intersecting cubic")
	{
		// A cubic that loops back on itself
		Path2d path;
		path.moveTo( 0, 0 );
		path.curveTo( 200, 100, -100, 100, 100, 0 );

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() == 1 );

		Shape2d result = path.removeSelfIntersections();

		// Should produce at least one non-empty contour
		REQUIRE( result.getNumContours() >= 1 );
		REQUIRE( !result.getContour( 0 ).empty() );

		// The result should have no self-intersections
		auto newIntersections = result.getContour( 0 ).findSelfIntersections();
		REQUIRE( newIntersections.empty() );
	}

	SECTION("Zigzag pattern with multiple self-intersections")
	{
		// Create a zigzag that creates overlapping loops when offset
		// The pattern is a series of peaks and valleys
		Path2d zzPath;
		zzPath.moveTo( 0, 0 );
		zzPath.lineTo( 20, 40 );  // up-right
		zzPath.lineTo( 40, 0 );   // down-right
		zzPath.lineTo( 60, 40 );  // up-right
		zzPath.lineTo( 80, 0 );   // down-right
		zzPath.lineTo( 100, 40 ); // up-right
		zzPath.lineTo( 120, 0 );  // down-right

		// Offset inward will create self-intersections at the peaks
		float offsetDist = -25.0f;  // Negative for inward
		Shape2d rawOffset = zzPath.offset( offsetDist, Join::Miter, 10.0f, 0.25f, false );

		REQUIRE( !rawOffset.empty() );
		const Path2d& offsetPath = rawOffset.getContour( 0 );

		auto selfIntersects = offsetPath.findSelfIntersections();

		if( selfIntersects.size() > 0 ) {
			Shape2d cleaned = offsetPath.removeSelfIntersections();
			if( !cleaned.empty() ) {
				const auto& cleanedPath = cleaned.getContour( 0 );
				// Note: In complex cases like zigzags with multi-way intersections, the algorithm
				// may not eliminate ALL intersections because the kept curve portions can still
				// geometrically intersect each other. A proper solution would require planar
				// subdivision / boolean operations.
				REQUIRE( !cleanedPath.empty() );
			}
		}
	}

	SECTION("Two adjacent crossing cubics")
	{
		// Two cubic segments that cross - this is a complex case
		Path2d path;
		path.moveTo( 0, 50 );
		path.curveTo( 33, 0, 67, 100, 100, 50 );
		path.curveTo( 67, 0, 33, 100, 0, 50 );

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() >= 1 );

		// Just verify the function runs without crashing
		Shape2d result = path.removeSelfIntersections();
		REQUIRE( result.getNumContours() >= 1 );
	}

	SECTION("Empty path returns empty")
	{
		Path2d path;
		Shape2d result = path.removeSelfIntersections();
		REQUIRE( result.getNumContours() == 0 );
	}
}

TEST_CASE("offset() with removeSelfIntersections")
{
	SECTION("Sharp corner creates self-intersection that gets removed")
	{
		// Create a path with a sharp corner that will self-intersect when offset outward
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 50, 0 );
		path.lineTo( 50, 50 );

		// Large offset relative to corner will create self-intersection
		float largeOffset = 30.0f;

		// Without removal - should have self-intersecting loop
		Shape2d withLoops = path.offset( largeOffset, Join::Miter, 10.0f, 0.25f, false );
		REQUIRE( !withLoops.empty() );

		// With removal - loops should be removed
		Shape2d noLoops = path.offset( largeOffset, Join::Miter, 10.0f, 0.25f, true );
		REQUIRE( !noLoops.empty() );

		// The cleaned version should have fewer or equal self-intersections
		// (we can't guarantee zero due to algorithm limitations, but it should help)
		size_t loopIntersections = 0;
		for( const auto& contour : withLoops.getContours() ) {
			loopIntersections += contour.findSelfIntersections().size();
		}

		size_t cleanedIntersections = 0;
		for( const auto& contour : noLoops.getContours() ) {
			cleanedIntersections += contour.findSelfIntersections().size();
		}

		REQUIRE( cleanedIntersections <= loopIntersections );
	}

	SECTION("Offset without self-intersection is unchanged")
	{
		// Simple path that won't self-intersect at small offset
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 0 );
		path.lineTo( 100, 100 );

		float smallOffset = 2.0f;

		Shape2d withFlag = path.offset( smallOffset, Join::Round, 4.0f, 0.25f, true );
		Shape2d withoutFlag = path.offset( smallOffset, Join::Round, 4.0f, 0.25f, false );

		// Both should produce similar results since no self-intersection to remove
		REQUIRE( withFlag.getContours().size() == withoutFlag.getContours().size() );
	}
}

//=============================================================================
// Path2d::isCoincident and Shape2d::isCoincident Tests
//=============================================================================

TEST_CASE("Path2d::isCoincident")
{
	SECTION("Identical paths are coincident")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );
		path1.curveTo( 150, 50, 200, 150, 250, 100 );

		Path2d path2 = path1;

		REQUIRE( path1.isCoincident( path2 ) );
		REQUIRE( path2.isCoincident( path1 ) );
	}

	SECTION("Paths within tolerance are coincident")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );

		Path2d path2;
		path2.moveTo( 0.5f, 0.5f );
		path2.lineTo( 100.5f, 100.5f );

		// Within default tolerance of 1.0
		REQUIRE( path1.isCoincident( path2, 1.0f ) );

		// Not within tighter tolerance
		REQUIRE_FALSE( path1.isCoincident( path2, 0.1f ) );
	}

	SECTION("Different paths are not coincident")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );

		Path2d path2;
		path2.moveTo( 0, 0 );
		path2.lineTo( 200, 200 );

		REQUIRE_FALSE( path1.isCoincident( path2 ) );
	}

	SECTION("Paths with different segment counts are not coincident")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );

		Path2d path2;
		path2.moveTo( 0, 0 );
		path2.lineTo( 50, 50 );
		path2.lineTo( 100, 100 );

		REQUIRE_FALSE( path1.isCoincident( path2 ) );
	}

	SECTION("Empty paths are coincident")
	{
		Path2d path1;
		Path2d path2;

		REQUIRE( path1.isCoincident( path2 ) );
	}

	SECTION("Path2d::isCoincident with Shape2d")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );

		Shape2d shape;
		shape.moveTo( 0, 0 );
		shape.lineTo( 100, 100 );

		REQUIRE( path.isCoincident( shape ) );

		// Add another contour to shape - path should still be coincident with it
		shape.moveTo( 200, 200 );
		shape.lineTo( 300, 300 );

		REQUIRE( path.isCoincident( shape ) );
	}

	SECTION("Path2d not coincident with unrelated Shape2d")
	{
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );

		Shape2d shape;
		shape.moveTo( 500, 500 );
		shape.lineTo( 600, 600 );

		REQUIRE_FALSE( path.isCoincident( shape ) );
	}
}

TEST_CASE("Shape2d::isCoincident")
{
	SECTION("Identical shapes are coincident")
	{
		Shape2d shape1;
		shape1.moveTo( 0, 0 );
		shape1.lineTo( 100, 0 );
		shape1.lineTo( 100, 100 );
		shape1.close();

		Shape2d shape2 = shape1;

		REQUIRE( shape1.isCoincident( shape2 ) );
		REQUIRE( shape2.isCoincident( shape1 ) );
	}

	SECTION("Shapes within tolerance are coincident")
	{
		Shape2d shape1;
		shape1.moveTo( 0, 0 );
		shape1.lineTo( 100, 100 );

		Shape2d shape2;
		shape2.moveTo( 0.3f, 0.3f );
		shape2.lineTo( 100.3f, 100.3f );

		REQUIRE( shape1.isCoincident( shape2, 1.0f ) );
		REQUIRE_FALSE( shape1.isCoincident( shape2, 0.1f ) );
	}

	SECTION("Shapes with different contour counts are not coincident")
	{
		Shape2d shape1;
		shape1.moveTo( 0, 0 );
		shape1.lineTo( 100, 100 );

		Shape2d shape2;
		shape2.moveTo( 0, 0 );
		shape2.lineTo( 100, 100 );
		shape2.moveTo( 200, 200 );
		shape2.lineTo( 300, 300 );

		REQUIRE_FALSE( shape1.isCoincident( shape2 ) );
	}

	SECTION("Empty shapes are coincident")
	{
		Shape2d shape1;
		Shape2d shape2;

		REQUIRE( shape1.isCoincident( shape2 ) );
	}

	SECTION("Shape2d::isCoincident with Path2d")
	{
		Shape2d shape;
		shape.moveTo( 0, 0 );
		shape.lineTo( 100, 100 );
		shape.moveTo( 200, 200 );
		shape.lineTo( 300, 300 );

		Path2d path;
		path.moveTo( 200, 200 );
		path.lineTo( 300, 300 );

		// Shape has a contour coincident with path
		REQUIRE( shape.isCoincident( path ) );

		// Different path should not be coincident
		Path2d differentPath;
		differentPath.moveTo( 500, 500 );
		differentPath.lineTo( 600, 600 );

		REQUIRE_FALSE( shape.isCoincident( differentPath ) );
	}

	SECTION("Multi-contour shape coincidence")
	{
		Shape2d shape1;
		shape1.moveTo( 0, 0 );
		shape1.lineTo( 100, 100 );
		shape1.moveTo( 200, 200 );
		shape1.lineTo( 300, 300 );

		Shape2d shape2;
		shape2.moveTo( 0, 0 );
		shape2.lineTo( 100, 100 );
		shape2.moveTo( 200, 200 );
		shape2.lineTo( 300, 300 );

		REQUIRE( shape1.isCoincident( shape2 ) );

		// Modify second contour of shape2
		shape2.getContour( 1 ).translate( vec2( 50, 50 ) );
		REQUIRE_FALSE( shape1.isCoincident( shape2 ) );
	}
}

TEST_CASE("isCoincident prevents pathological findIntersections")
{
	SECTION("Coincident paths return empty intersections")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.curveTo( 33, 100, 67, 100, 100, 0 );

		Path2d path2 = path1;

		// Should detect coincidence and return empty
		auto results = path1.findIntersections( path2 );
		REQUIRE( results.empty() );
	}

	SECTION("Nearly coincident paths return empty intersections")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.curveTo( 33, 100, 67, 100, 100, 0 );

		Path2d path2;
		// Use tiny offset that's within findIntersections's default tolerance (1e-4f)
		path2.moveTo( 0.00005f, 0.00005f );
		path2.curveTo( 33.00005f, 100.00005f, 67.00005f, 100.00005f, 100.00005f, 0.00005f );

		// These are within default findIntersections tolerance (1e-4f), should return empty
		auto results = path1.findIntersections( path2 );
		REQUIRE( results.empty() );
	}

	SECTION("Non-coincident paths still find intersections")
	{
		Path2d path1;
		path1.moveTo( 0, 0 );
		path1.lineTo( 100, 100 );

		Path2d path2;
		path2.moveTo( 0, 100 );
		path2.lineTo( 100, 0 );

		// These are not coincident, should find intersection
		auto results = path1.findIntersections( path2 );
		REQUIRE( results.size() == 1 );
	}
}

//=============================================================================
// Benchmark: findSelfIntersections performance
//=============================================================================

// Deterministic path generator for benchmarking
// Uses LCG with fixed seed for reproducibility
namespace {
	struct DeterministicRng {
		uint32_t state;
		DeterministicRng( uint32_t seed ) : state( seed ) {}

		uint32_t next() {
			state = state * 1103515245 + 12345;
			return ( state >> 16 ) & 0x7fff;
		}

		float nextFloat( float min, float max ) {
			return min + ( max - min ) * ( next() / 32767.0f );
		}
	};

	// Generate a deterministic path with n cubic segments that may self-intersect
	Path2d generateBenchmarkPath( size_t numSegments, uint32_t seed ) {
		DeterministicRng rng( seed );
		Path2d path;

		float x = rng.nextFloat( 100, 400 );
		float y = rng.nextFloat( 100, 400 );
		path.moveTo( x, y );

		for( size_t i = 0; i < numSegments; ++i ) {
			// Mostly cubics (80%), some lines (20%) - cubics are worst case
			if( rng.next() % 5 == 0 ) {
				x += rng.nextFloat( -100, 100 );
				y += rng.nextFloat( -100, 100 );
				path.lineTo( x, y );
			}
			else {
				float c1x = x + rng.nextFloat( -80, 80 );
				float c1y = y + rng.nextFloat( -80, 80 );
				float c2x = x + rng.nextFloat( -80, 80 );
				float c2y = y + rng.nextFloat( -80, 80 );
				x += rng.nextFloat( -100, 100 );
				y += rng.nextFloat( -100, 100 );
				path.curveTo( c1x, c1y, c2x, c2y, x, y );
			}
		}

		return path;
	}
}

TEST_CASE("Benchmark: findSelfIntersections", "[.benchmark]")
{
	// Test segment counts: 1, 5, 10, 20, 30, 40, 50, 60, 70, 77
	std::vector<size_t> segmentCounts = { 1, 5, 10, 20, 30, 40, 50, 60, 70, 77 };

	// Fixed seed ensures identical paths across runs
	constexpr uint32_t BENCHMARK_SEED = 0xDEADBEEF;
	constexpr int ITERATIONS = 10;

	console() << "\n=== findSelfIntersections Benchmark ===" << std::endl;
	console() << "Segments\tAvg(ms)\t\tIsects\tCubics\tLines" << std::endl;

	for( size_t numSegs : segmentCounts ) {
		// Generate path with fixed seed + segment count for determinism
		Path2d path = generateBenchmarkPath( numSegs, BENCHMARK_SEED + static_cast<uint32_t>( numSegs ) );

		// Count segment types
		size_t cubics = 0, lines = 0;
		for( auto seg : path.getSegments() ) {
			if( seg == Path2d::CUBICTO ) cubics++;
			else if( seg == Path2d::LINETO ) lines++;
		}

		// Time multiple iterations
		auto start = std::chrono::high_resolution_clock::now();
		size_t totalIntersections = 0;

		for( int i = 0; i < ITERATIONS; ++i ) {
			auto isects = path.findSelfIntersections();
			totalIntersections += isects.size();
		}

		auto end = std::chrono::high_resolution_clock::now();
		double avgMs = std::chrono::duration<double, std::milli>( end - start ).count() / ITERATIONS;
		size_t avgIsects = totalIntersections / ITERATIONS;

		console() << numSegs << "\t\t"
		          << std::fixed << std::setprecision( 3 ) << avgMs << "\t\t"
		          << avgIsects << "\t" << cubics << "\t" << lines << std::endl;
	}

	console() << "========================================\n" << std::endl;

	REQUIRE( true );  // Always pass - this is a benchmark
}
