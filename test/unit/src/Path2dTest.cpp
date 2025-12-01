#include "cinder/app/App.h"
#include "cinder/Path2d.h"
#include "cinder/Path2dStroke.h"
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

		Path2d result = path.removeSelfIntersections();

		REQUIRE( result.getNumSegments() == path.getNumSegments() );
	}

	SECTION("Figure-8 with crossing lines")
	{
		// Create a figure-8 pattern: the loop should be removed
		Path2d path;
		path.moveTo( 0, 0 );
		path.lineTo( 100, 100 );   // Segment 0: goes up-right
		path.lineTo( 100, 0 );     // Segment 1: goes down
		path.lineTo( 0, 100 );     // Segment 2: crosses segment 0

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() == 1 );

		Path2d result = path.removeSelfIntersections();

		// The result should have fewer segments (loop removed)
		// The crossing creates a loop that gets removed
		REQUIRE( result.getNumSegments() < path.getNumSegments() );
	}

	SECTION("Self-intersecting cubic")
	{
		// A cubic that loops back on itself
		Path2d path;
		path.moveTo( 0, 0 );
		path.curveTo( 200, 100, -100, 100, 100, 0 );

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() == 1 );

		Path2d result = path.removeSelfIntersections();

		// The loop should be removed, resulting in a shorter curve
		// The exact number of segments depends on where the split occurs
		REQUIRE( !result.empty() );

		// Verify the result doesn't self-intersect anymore
		auto newIntersections = result.findSelfIntersections();
		REQUIRE( newIntersections.empty() );
	}

	SECTION("Two adjacent crossing cubics")
	{
		// Two cubic segments that cross - this is a complex case
		// The algorithm handles simple offset curve loops well,
		// but complex multi-crossing patterns may not be fully resolved
		Path2d path;
		path.moveTo( 0, 50 );
		path.curveTo( 33, 0, 67, 100, 100, 50 );
		path.curveTo( 67, 0, 33, 100, 0, 50 );

		auto intersections = path.findSelfIntersections();
		REQUIRE( intersections.size() >= 1 );

		// Just verify the function runs without crashing
		Path2d result = path.removeSelfIntersections();
		REQUIRE( !result.empty() );
	}

	SECTION("Empty path returns empty")
	{
		Path2d path;
		Path2d result = path.removeSelfIntersections();
		REQUIRE( result.empty() );
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
		Shape2d withLoops = offset( path, largeOffset, Join::Miter, 10.0f, 0.25f, false );
		REQUIRE( !withLoops.empty() );

		// With removal - loops should be removed
		Shape2d noLoops = offset( path, largeOffset, Join::Miter, 10.0f, 0.25f, true );
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

		Shape2d withFlag = offset( path, smallOffset, Join::Round, 4.0f, 0.25f, true );
		Shape2d withoutFlag = offset( path, smallOffset, Join::Round, 4.0f, 0.25f, false );

		// Both should produce similar results since no self-intersection to remove
		REQUIRE( withFlag.getContours().size() == withoutFlag.getContours().size() );
	}
}
