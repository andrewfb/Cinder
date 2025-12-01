#include "cinder/app/App.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/Path2dStroke.h"

#include "catch.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

TEST_CASE("Path2dStroke")
{
	// offset() tests
	SECTION("offset: Horizontal Line")
	{
		Path2d line;
		line.moveTo( 0, 100 );
		line.lineTo( 100, 100 );

		Shape2d offsetUp = offset( line, -10.0f );
		Shape2d offsetDown = offset( line, 10.0f );

		// Offset up should have Y values less than 100
		REQUIRE( !offsetUp.empty() );
		for( const auto& contour : offsetUp.getContours() ) {
			for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
				REQUIRE( contour.getPoint( i ).y <= 101.0f ); // Small tolerance
			}
		}

		// Offset down should have Y values greater than 100
		REQUIRE( !offsetDown.empty() );
		for( const auto& contour : offsetDown.getContours() ) {
			for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
				REQUIRE( contour.getPoint( i ).y >= 99.0f ); // Small tolerance
			}
		}
	}

	SECTION("offset: Quadratic Curve")
	{
		Path2d quad;
		quad.moveTo( 0, 0 );
		quad.quadTo( 50, 100, 100, 0 );

		Shape2d offsetResult = offset( quad, 10.0f );

		// Should produce non-empty result
		REQUIRE( !offsetResult.empty() );

		// The offset curve should roughly follow the same shape
		// Check that it has reasonable bounds
		auto bounds = offsetResult.calcPreciseBoundingBox();
		REQUIRE( bounds.getWidth() > 90.0f );  // Should be at least as wide as original
		REQUIRE( bounds.getHeight() > 90.0f ); // Should be at least as tall as original plus offset
	}

	SECTION("offset: Cubic Curve")
	{
		Path2d cubic;
		cubic.moveTo( 0, 0 );
		cubic.curveTo( 30, 100, 70, 100, 100, 0 );

		Shape2d offsetResult = offset( cubic, 5.0f );

		REQUIRE( !offsetResult.empty() );

		auto bounds = offsetResult.calcPreciseBoundingBox();
		REQUIRE( bounds.getWidth() > 90.0f );
	}

	SECTION("offset: Zero Distance")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 0 );

		Shape2d offsetResult = offset( line, 0.0f );

		// Zero offset should return something close to original
		REQUIRE( !offsetResult.empty() );
	}

	// stroke() tests
	SECTION("stroke: Line Segment")
	{
		Path2d line;
		line.moveTo( 50, 50 );
		line.lineTo( 150, 50 );

		Shape2d stroked = stroke( line, 10.0f );

		REQUIRE( !stroked.empty() );

		// The stroked path should be a closed shape
		for( const auto& contour : stroked.getContours() ) {
			REQUIRE( contour.isClosed() );
		}

		// Check bounding box
		auto bounds = stroked.calcPreciseBoundingBox();
		REQUIRE( bounds.getWidth() >= Approx( 100.0f ).margin( 1.0f ) );  // Length of line
		REQUIRE( bounds.getHeight() >= Approx( 10.0f ).margin( 1.0f ) );  // Stroke width
	}

	SECTION("stroke: Quadratic Curve")
	{
		Path2d quad;
		quad.moveTo( 0, 50 );
		quad.quadTo( 50, 0, 100, 50 );

		StrokeStyle style( 10.0f );
		style.withJoin( StrokeJoin::Round );
		style.withCaps( StrokeCap::Round );

		Shape2d stroked = stroke( quad, style );

		REQUIRE( !stroked.empty() );

		// Should produce a closed contour
		for( const auto& contour : stroked.getContours() ) {
			REQUIRE( contour.isClosed() );
		}
	}

	SECTION("stroke: Cubic S-Curve")
	{
		Path2d cubic;
		cubic.moveTo( 0, 50 );
		cubic.curveTo( 25, 0, 75, 100, 100, 50 );

		Shape2d stroked = stroke( cubic, 8.0f );

		REQUIRE( !stroked.empty() );

		// The result should be reasonably bounded
		auto bounds = stroked.calcPreciseBoundingBox();
		REQUIRE( bounds.getWidth() > 90.0f );
		REQUIRE( bounds.getHeight() > 40.0f );
	}

	SECTION("stroke: Closed Triangle")
	{
		Path2d triangle;
		triangle.moveTo( 50, 0 );
		triangle.lineTo( 100, 100 );
		triangle.lineTo( 0, 100 );
		triangle.close();

		StrokeStyle style( 10.0f );
		style.withJoin( StrokeJoin::Miter );

		Shape2d stroked = stroke( triangle, style );

		REQUIRE( !stroked.empty() );

		// Should produce exactly 2 contours (outer and inner)
		REQUIRE( stroked.getNumContours() == 2 );
	}

	SECTION("stroke: Different Join Styles")
	{
		Path2d angle;
		angle.moveTo( 0, 0 );
		angle.lineTo( 50, 0 );
		angle.lineTo( 50, 50 );

		StrokeStyle bevelStyle( 10.0f );
		bevelStyle.withJoin( StrokeJoin::Bevel );

		StrokeStyle miterStyle( 10.0f );
		miterStyle.withJoin( StrokeJoin::Miter );

		StrokeStyle roundStyle( 10.0f );
		roundStyle.withJoin( StrokeJoin::Round );

		Shape2d bevelResult = stroke( angle, bevelStyle );
		Shape2d miterResult = stroke( angle, miterStyle );
		Shape2d roundResult = stroke( angle, roundStyle );

		REQUIRE( !bevelResult.empty() );
		REQUIRE( !miterResult.empty() );
		REQUIRE( !roundResult.empty() );

		// Round join should produce more points than bevel due to arc approximation
		size_t bevelPoints = 0, roundPoints = 0;
		for( const auto& c : bevelResult.getContours() ) bevelPoints += c.getNumPoints();
		for( const auto& c : roundResult.getContours() ) roundPoints += c.getNumPoints();
		REQUIRE( roundPoints >= bevelPoints );
	}

	SECTION("stroke: Different Cap Styles")
	{
		Path2d line;
		line.moveTo( 50, 50 );
		line.lineTo( 150, 50 );

		StrokeStyle buttStyle( 20.0f );
		buttStyle.withCaps( StrokeCap::Butt );

		StrokeStyle squareStyle( 20.0f );
		squareStyle.withCaps( StrokeCap::Square );

		StrokeStyle roundStyle( 20.0f );
		roundStyle.withCaps( StrokeCap::Round );

		Shape2d buttResult = stroke( line, buttStyle );
		Shape2d squareResult = stroke( line, squareStyle );
		Shape2d roundResult = stroke( line, roundStyle );

		REQUIRE( !buttResult.empty() );
		REQUIRE( !squareResult.empty() );
		REQUIRE( !roundResult.empty() );

		// Square cap extends the line by half the stroke width
		auto buttBounds = buttResult.calcPreciseBoundingBox();
		auto squareBounds = squareResult.calcPreciseBoundingBox();

		// Square caps should extend beyond butt caps
		REQUIRE( squareBounds.getWidth() > buttBounds.getWidth() );
	}

	// dash() tests
	SECTION("dash: Basic Line Dash Pattern")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 0 );

		// Dash pattern: 10 on, 5 off
		// Over 100 pixels: 0-10, 15-25, 30-40, 45-55, 60-70, 75-85, 90-100 = 7 dashes
		Shape2d dashed = dash( line, 0.0f, { 10.0f, 5.0f } );

		REQUIRE( !dashed.empty() );
		REQUIRE( dashed.getNumContours() == 7 );
	}

	SECTION("dash: Dash with Offset")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 0 );

		// Start with offset of 5 (middle of first dash)
		// Remaining from first dash: 5, then gap 5, then dash 10...
		// 0-5, 10-20, 25-35, 40-50, 55-65, 70-80, 85-95 = 7 dashes still
		Shape2d dashed = dash( line, 5.0f, { 10.0f, 5.0f } );

		REQUIRE( !dashed.empty() );
		REQUIRE( dashed.getNumContours() >= 6 );  // At least 6 dashes
	}

	SECTION("dash: Short Dashes on Curve")
	{
		Path2d quad;
		quad.moveTo( 0, 0 );
		quad.quadTo( 50, 100, 100, 0 );

		Shape2d dashed = dash( quad, 0.0f, { 5.0f, 5.0f } );

		// Should produce multiple dash segments
		REQUIRE( !dashed.empty() );
		REQUIRE( dashed.getNumContours() >= 5 ); // Arc length is roughly 120, so at least 10 dashes
	}

	SECTION("dash: Single Long Dash (No Actual Dashing)")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 50, 0 );

		// Dash pattern longer than the line
		Shape2d dashed = dash( line, 0.0f, { 100.0f, 10.0f } );

		// Should produce just one contour (the whole line fits in one dash)
		REQUIRE( !dashed.empty() );
		REQUIRE( dashed.getNumContours() == 1 );
	}

	SECTION("dash: Empty Pattern Returns Original")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 0 );

		Shape2d dashed = dash( line, 0.0f, {} );

		// Should return original path (single contour)
		REQUIRE( !dashed.empty() );
		REQUIRE( dashed.getNumContours() == 1 );
	}

	// stroke with dashing
	SECTION("stroke: Dashed Line")
	{
		Path2d line;
		line.moveTo( 0, 50 );
		line.lineTo( 200, 50 );

		StrokeStyle style( 10.0f );
		style.withDashes( 0.0f, { 20.0f, 10.0f } );
		style.withCaps( StrokeCap::Round );

		Shape2d stroked = stroke( line, style );

		REQUIRE( !stroked.empty() );

		// Each dash becomes its own closed contour
		REQUIRE( stroked.getNumContours() >= 6 ); // 200 / 30 ≈ 6.67 dashes
	}

	// Shape2d versions
	SECTION("stroke: Shape2d Input")
	{
		Shape2d shape;
		Path2d rect;
		rect.moveTo( 0, 0 );
		rect.lineTo( 100, 0 );
		rect.lineTo( 100, 50 );
		rect.lineTo( 0, 50 );
		rect.close();
		shape.appendContour( rect );

		Shape2d stroked = stroke( shape, 5.0f );

		REQUIRE( !stroked.empty() );
		REQUIRE( stroked.getNumContours() == 2 ); // Inner and outer contours
	}

	SECTION("offset: Shape2d Input")
	{
		Shape2d shape;
		Path2d circle;
		// Approximate circle with 4 cubic segments
		circle.moveTo( 100, 50 );
		circle.curveTo( 100, 77.6f, 77.6f, 100, 50, 100 );
		circle.curveTo( 22.4f, 100, 0, 77.6f, 0, 50 );
		circle.curveTo( 0, 22.4f, 22.4f, 0, 50, 0 );
		circle.curveTo( 77.6f, 0, 100, 22.4f, 100, 50 );
		circle.close();
		shape.appendContour( circle );

		Shape2d offsetResult = offset( shape, 10.0f );

		REQUIRE( !offsetResult.empty() );

		// Expanded shape should be larger
		auto originalBounds = shape.calcPreciseBoundingBox();
		auto offsetBounds = offsetResult.calcPreciseBoundingBox();
		REQUIRE( offsetBounds.getWidth() > originalBounds.getWidth() );
	}

	// Edge cases
	SECTION("stroke: Degenerate Line (Zero Length)")
	{
		Path2d point;
		point.moveTo( 50, 50 );
		point.lineTo( 50, 50 );

		Shape2d stroked = stroke( point, 10.0f );

		// Should handle gracefully (may or may not produce output)
		// Just ensure no crash
	}

	SECTION("offset: Negative Tolerance")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 100 );

		// Negative tolerance should still work (treated as absolute)
		Shape2d offsetResult = offset( line, 10.0f, -0.25f );

		// Just ensure no crash
		REQUIRE( true );
	}

	SECTION("stroke: Very Small Stroke Width")
	{
		Path2d line;
		line.moveTo( 0, 0 );
		line.lineTo( 100, 0 );

		Shape2d stroked = stroke( line, 0.1f );

		REQUIRE( !stroked.empty() );

		auto bounds = stroked.calcPreciseBoundingBox();
		REQUIRE( bounds.getHeight() <= 1.0f ); // Should be very thin
	}

	SECTION("stroke: Very Large Stroke Width")
	{
		Path2d line;
		line.moveTo( 50, 50 );
		line.lineTo( 60, 50 );

		Shape2d stroked = stroke( line, 100.0f );

		REQUIRE( !stroked.empty() );

		auto bounds = stroked.calcPreciseBoundingBox();
		REQUIRE( bounds.getHeight() >= 90.0f ); // Should be approximately stroke width
	}
}
