/*
 * BezierOffset Sample
 * Demonstrates path offsetting and stroke expansion using Cinder's PathStroke2d
 *
 * Controls:
 *   Left-click: Add line segment (drag to create curve)
 *   Shift+drag: Create quadratic curve
 *   Mouse wheel: Adjust offset/stroke width
 *   1-3: Switch mode (1=Offset, 2=Stroke, 3=Dashed Stroke)
 *   J: Cycle join style (Bevel, Miter, Round)
 *   C: Cycle cap style (Butt, Square, Round)
 *   X: Clear path
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/Path2dStroke.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BezierOffsetApp : public App {
public:
	BezierOffsetApp();

	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void draw() override;

private:
	void updateResult();
	string getModeName() const;
	string getJoinName() const;
	string getCapName() const;

	Path2d      mPath;
	Shape2d     mResult;
	int         mTrackedPoint;

	// Parameters
	float       mWidth;        // Offset distance or stroke width
	int         mMode;         // 0=Offset, 1=Stroke, 2=Dashed Stroke
	StrokeJoin  mJoin;
	StrokeCap   mCap;
	float       mTolerance;
};

BezierOffsetApp::BezierOffsetApp()
	: mTrackedPoint( -1 )
	, mWidth( 20.0f )
	, mMode( 1 )  // Start with stroke mode
	, mJoin( StrokeJoin::Round )
	, mCap( StrokeCap::Round )
	, mTolerance( 0.25f )
{
}

void BezierOffsetApp::mouseDown( MouseEvent event )
{
	if( event.isLeftDown() ) {
		if( mPath.empty() ) {
			mPath.moveTo( event.getPos() );
			mTrackedPoint = 0;
		}
		else {
			mPath.lineTo( event.getPos() );
		}
		updateResult();
	}
}

void BezierOffsetApp::mouseDrag( MouseEvent event )
{
	if( mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, event.getPos() );
	}
	else if( mPath.getNumSegments() > 0 ) {
		// Convert line to curve
		vec2 endPt = mPath.getPoint( mPath.getNumPoints() - 1 );
		mPath.removeSegment( mPath.getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( mPath.getNumSegments() == 0 )
			? Path2d::MOVETO
			: mPath.getSegmentType( mPath.getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			// Quadratic curve
			mPath.quadTo( event.getPos(), endPt );
		}
		else {
			// Cubic curve with smooth tangent
			vec2 tan1;
			if( prevType == Path2d::CUBICTO ) {
				vec2 prevDelta = mPath.getPoint( mPath.getNumPoints() - 2 ) - mPath.getPoint( mPath.getNumPoints() - 1 );
				tan1 = mPath.getPoint( mPath.getNumPoints() - 1 ) - prevDelta;
			}
			else if( prevType == Path2d::QUADTO ) {
				vec2 quadTangent = mPath.getPoint( mPath.getNumPoints() - 2 );
				vec2 quadEnd = mPath.getPoint( mPath.getNumPoints() - 1 );
				vec2 prevDelta = ( quadTangent + ( quadEnd - quadTangent ) / 3.0f ) - quadEnd;
				tan1 = quadEnd - prevDelta;
			}
			else {
				tan1 = mPath.getPoint( mPath.getNumPoints() - 1 );
			}
			mPath.curveTo( tan1, event.getPos(), endPt );
		}
		mTrackedPoint = mPath.getNumPoints() - 2;
	}
	updateResult();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
}

void BezierOffsetApp::mouseWheel( MouseEvent event )
{
	mWidth += event.getWheelIncrement() * 2.0f;
	mWidth = glm::max( 1.0f, mWidth );
	updateResult();
}

void BezierOffsetApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'x':
		case 'X':
			mPath.clear();
			mResult.clear();
			break;
		case '1':
			mMode = 0;  // Offset
			updateResult();
			break;
		case '2':
			mMode = 1;  // Stroke
			updateResult();
			break;
		case '3':
			mMode = 2;  // Dashed stroke
			updateResult();
			break;
		case 'j':
		case 'J':
			// Cycle join style
			if( mJoin == StrokeJoin::Bevel ) mJoin = StrokeJoin::Miter;
			else if( mJoin == StrokeJoin::Miter ) mJoin = StrokeJoin::Round;
			else mJoin = StrokeJoin::Bevel;
			updateResult();
			break;
		case 'c':
		case 'C':
			// Cycle cap style
			if( mCap == StrokeCap::Butt ) mCap = StrokeCap::Square;
			else if( mCap == StrokeCap::Square ) mCap = StrokeCap::Round;
			else mCap = StrokeCap::Butt;
			updateResult();
			break;
	}
}

void BezierOffsetApp::updateResult()
{
	if( mPath.empty() || mPath.getNumSegments() == 0 ) {
		mResult.clear();
		return;
	}

	switch( mMode ) {
		case 0: // Offset
			mResult = offset( mPath, mWidth, mTolerance );
			break;
		case 1: { // Stroke
			StrokeStyle style( mWidth );
			style.withJoin( mJoin ).withCaps( mCap );
			mResult = stroke( mPath, style, mTolerance );
			break;
		}
		case 2: { // Dashed stroke
			StrokeStyle style( mWidth );
			style.withJoin( mJoin ).withCaps( mCap );
			style.withDashes( 0.0f, { mWidth * 2, mWidth } );  // Dash = 2x width, gap = 1x width
			mResult = stroke( mPath, style, mTolerance );
			break;
		}
	}
}

string BezierOffsetApp::getModeName() const
{
	switch( mMode ) {
		case 0: return "Offset";
		case 1: return "Stroke";
		case 2: return "Dashed Stroke";
	}
	return "";
}

string BezierOffsetApp::getJoinName() const
{
	switch( mJoin ) {
		case StrokeJoin::Bevel: return "Bevel";
		case StrokeJoin::Miter: return "Miter";
		case StrokeJoin::Round: return "Round";
	}
	return "";
}

string BezierOffsetApp::getCapName() const
{
	switch( mCap ) {
		case StrokeCap::Butt: return "Butt";
		case StrokeCap::Square: return "Square";
		case StrokeCap::Round: return "Round";
	}
	return "";
}

void BezierOffsetApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.15f ) );

	// Draw the result (offset or stroke)
	if( !mResult.empty() ) {
		gl::color( ColorA( 0.3f, 0.6f, 0.9f, 0.7f ) );
		gl::draw( mResult );

		// Draw outline of result
		gl::color( ColorA( 0.5f, 0.8f, 1.0f, 0.5f ) );
		for( const auto& contour : mResult.getContours() ) {
			gl::draw( contour );
		}
	}

	// Draw the original path
	if( !mPath.empty() ) {
		gl::color( Color( 1.0f, 0.5f, 0.2f ) );
		gl::lineWidth( 2.0f );
		gl::draw( mPath );
		gl::lineWidth( 1.0f );

		// Draw control points
		gl::color( Color( 1.0f, 1.0f, 0.3f ) );
		for( size_t i = 0; i < mPath.getNumPoints(); ++i ) {
			gl::drawSolidCircle( mPath.getPoint( i ), 4.0f );
		}
	}

	// Draw UI text
	gl::color( Color::white() );

	string info = "Mode: " + getModeName() + " [1-3]";
	info += "  |  Width: " + to_string( (int)mWidth ) + " [wheel]";
	if( mMode > 0 ) {
		info += "  |  Join: " + getJoinName() + " [J]";
		info += "  |  Cap: " + getCapName() + " [C]";
	}
	info += "  |  Clear [X]";

	gl::drawString( info, vec2( 10, 10 ), Color::white() );
	gl::drawString( "Click to add points, drag to create curves", vec2( 10, 30 ), Color( 0.7f, 0.7f, 0.7f ) );
}

CINDER_APP( BezierOffsetApp, RendererGl, []( App::Settings *settings ) {
	settings->setWindowSize( 1024, 768 );
	settings->setTitle( "Bezier Offset & Stroke Demo" );
})
