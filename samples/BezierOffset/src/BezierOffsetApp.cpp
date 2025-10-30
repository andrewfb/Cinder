#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Path2d.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderImGui.h"
#include "cinder/Log.h"

#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

enum class PresetShape {
	OPEN_LINE,
	OPEN_CURVE,
	OPEN_S_CURVE,
	CLOSED_RECT,
	CLOSED_CIRCLE,
	CLOSED_STAR
};

class BezierOffsetApp : public App {
  public:
	BezierOffsetApp() : mTrackedPoint( -1 ) {}

	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void draw() override;

  private:
	void drawImGuiControls();
	void calculateOffsetCurve();
	void loadPresetShape( PresetShape shape );

	Path2d	mPath;
	Path2d	mOffsetPath;
	int		mTrackedPoint;

	// Offset parameters
	float	mOffsetDistance = 20.0f;
	float	mTolerance = 0.5f;
	int		mJoinStyle = 0; // 0=ROUND, 1=MITER, 2=BEVEL
	float	mMiterLimit = 4.0f;
	bool	mCloseOffsetCurve = true;  // If true, add caps to close open paths
	int		mCapStyle = 1;  // 0=BUTT, 1=ROUND, 2=SQUARE (only used if mCloseOffsetCurve is true)
	int		mNumOffsetCurves = 1;  // Number of offset curves to draw (from 0 to mOffsetDistance)

	// Visualization options
	bool	mShowOriginal = true;
	bool	mShowOffset = true;
	bool	mShowControlPoints = true;
	bool	mShowTangents = false;
	bool	mShowBoundingBox = false;
	Color	mOriginalColor = Color( 1.0f, 0.5f, 0.25f );
	Color	mOffsetColor = Color( 0.25f, 0.8f, 1.0f );

	// Info display
	bool	mShowInfo = true;
	float	mOriginalLength = 0.0f;
	float	mOffsetLength = 0.0f;

	// Conversion tracking
	bool	mIsConverted = false;

	// Path editing
	bool	mPathClosed = false;
};

void BezierOffsetApp::setup()
{
	CI_LOG_I( "Initializing BezierOffset Application" );

	// Initialize ImGui
	ImGui::Initialize( ImGui::Options().window( getWindow() ).enableKeyboard( true ) );
	ImGui::GetStyle().ScaleAllSizes( getWindowContentScale() );
	ImGui::GetStyle().FontScaleMain = getWindowContentScale();

	// Start with a simple example curve
	mPath.moveTo( vec2( 100, 300 ) );
	mPath.curveTo( vec2( 150, 100 ), vec2( 450, 100 ), vec2( 500, 300 ) );
	mPath.curveTo( vec2( 550, 500 ), vec2( 150, 500 ), vec2( 200, 300 ) );

	calculateOffsetCurve();

	CI_LOG_I( "BezierOffset ready" );
}

void BezierOffsetApp::mouseDown( MouseEvent event )
{
	if( event.isLeftDown() && !ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) ) {
		if( mPath.empty() ) {
			mPath.moveTo( event.getPos() );
			mTrackedPoint = 0;
		}
		else {
			mPath.lineTo( event.getPos() );
		}

		mIsConverted = false;  // Reset conversion flag when modifying path
		calculateOffsetCurve();

		console() << "Path updated: " << mPath.getNumSegments() << " segments" << std::endl;
	}
}

void BezierOffsetApp::mouseDrag( MouseEvent event )
{
	if( ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) )
		return;

	if( mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, event.getPos() );
	}
	else {
		// First bit of dragging, so switch our line to a cubic or a quad if Shift is down
		vec2 endPt = mPath.getPoint( mPath.getNumPoints() - 1 );
		mPath.removeSegment( mPath.getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( mPath.getNumSegments() == 0 ) ? Path2d::MOVETO : mPath.getSegmentType( mPath.getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			// Add a quadratic curve segment
			mPath.quadTo( event.getPos(), endPt );
		}
		else {
			// Add a cubic curve segment
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

	mIsConverted = false;  // Reset conversion flag when modifying path
	calculateOffsetCurve();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
}

void BezierOffsetApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'x' ) {
		mPath.clear();
		mOffsetPath.clear();
		mOriginalLength = 0.0f;
		mOffsetLength = 0.0f;
		mIsConverted = false;
		mPathClosed = false;
	}
	else if( event.getChar() == 'c' ) {
		// Toggle close/open
		mPathClosed = !mPathClosed;
		calculateOffsetCurve();
	}
	else if( event.getChar() == 'g' ) {
		mShowInfo = !mShowInfo;
	}
}

void BezierOffsetApp::loadPresetShape( PresetShape shape )
{
	mPath.clear();
	vec2 center = getWindowCenter();

	switch( shape ) {
		case PresetShape::OPEN_LINE:
			mPath.moveTo( center + vec2( -150, 0 ) );
			mPath.lineTo( center + vec2( 150, 0 ) );
			mPathClosed = false;
			break;

		case PresetShape::OPEN_CURVE:
			mPath.moveTo( center + vec2( -150, 0 ) );
			mPath.curveTo( center + vec2( -50, -100 ), center + vec2( 50, -100 ), center + vec2( 150, 0 ) );
			mPathClosed = false;
			break;

		case PresetShape::OPEN_S_CURVE:
			mPath.moveTo( center + vec2( -150, -50 ) );
			mPath.curveTo( center + vec2( -50, -150 ), center + vec2( 50, 50 ), center + vec2( 150, 150 ) );
			mPathClosed = false;
			break;

		case PresetShape::CLOSED_RECT:
			mPath.moveTo( center + vec2( -100, -80 ) );
			mPath.lineTo( center + vec2( 100, -80 ) );
			mPath.lineTo( center + vec2( 100, 80 ) );
			mPath.lineTo( center + vec2( -100, 80 ) );
			mPathClosed = true;
			break;

		case PresetShape::CLOSED_CIRCLE: {
			float radius = 100.0f;
			// Approximate circle with 4 cubic beziers
			float k = 0.5522847498f;  // 4/3 * tan(π/8)
			float kr = k * radius;
			mPath.moveTo( center + vec2( 0, -radius ) );
			mPath.curveTo( center + vec2( kr, -radius ), center + vec2( radius, -kr ), center + vec2( radius, 0 ) );
			mPath.curveTo( center + vec2( radius, kr ), center + vec2( kr, radius ), center + vec2( 0, radius ) );
			mPath.curveTo( center + vec2( -kr, radius ), center + vec2( -radius, kr ), center + vec2( -radius, 0 ) );
			mPath.curveTo( center + vec2( -radius, -kr ), center + vec2( -kr, -radius ), center + vec2( 0, -radius ) );
			mPathClosed = true;
			break;
		}

		case PresetShape::CLOSED_STAR: {
			float outerRadius = 100.0f;
			float innerRadius = 40.0f;
			int points = 5;
			mPath.moveTo( center + vec2( 0, -outerRadius ) );
			for( int i = 0; i < points * 2; ++i ) {
				float angle = (float)i * (float)M_PI / points - (float)M_PI / 2.0f;
				float radius = (i % 2 == 0) ? outerRadius : innerRadius;
				vec2 pt = center + vec2( std::cos( angle ), std::sin( angle ) ) * radius;
				mPath.lineTo( pt );
			}
			mPath.close();  // Actually close the star path
			mPathClosed = true;
			break;
		}
	}

	mIsConverted = false;
	calculateOffsetCurve();
}

void BezierOffsetApp::calculateOffsetCurve()
{
	if( mPath.empty() || mPath.getNumSegments() == 0 ) {
		mOffsetPath.clear();
		return;
	}

	try {
		Path2d::OffsetOptions opts;
		opts.tolerance = mTolerance;

		switch( mJoinStyle ) {
			case 0: opts.joinStyle = Path2d::OffsetOptions::ROUND; break;
			case 1: opts.joinStyle = Path2d::OffsetOptions::MITER; break;
			case 2: opts.joinStyle = Path2d::OffsetOptions::BEVEL; break;
		}

		opts.miterLimit = mMiterLimit;

		// Cap style based on whether user wants to close the offset curve
		if( mCloseOffsetCurve ) {
			switch( mCapStyle ) {
				case 0: opts.capStyle = Path2d::OffsetOptions::CAP_BUTT; break;
				case 1: opts.capStyle = Path2d::OffsetOptions::CAP_ROUND; break;
				case 2: opts.capStyle = Path2d::OffsetOptions::CAP_SQUARE; break;
			}
		}
		else {
			opts.capStyle = Path2d::OffsetOptions::CAP_NONE;
		}

		mOffsetPath = mPath.calcOffsetCurve( mOffsetDistance, opts );
		mOriginalLength = mPath.calcLength();
		mOffsetLength = mOffsetPath.calcLength();
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Error calculating offset: " << e.what() );
		mOffsetPath.clear();
	}
}

void BezierOffsetApp::drawImGuiControls()
{
	ImGui::Begin( "Bezier Offset Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize );

	ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Offset Parameters" );
	ImGui::Separator();

	if( ImGui::SliderFloat( "Distance", &mOffsetDistance, -100.0f, 100.0f, "%.1f" ) ) {
		calculateOffsetCurve();
	}
	ImGui::SameLine();
	if( ImGui::Button( "Reset##dist" ) ) {
		mOffsetDistance = 20.0f;
		calculateOffsetCurve();
	}

	if( ImGui::SliderFloat( "Tolerance", &mTolerance, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) ) {
		calculateOffsetCurve();
	}
	ImGui::SameLine();
	if( ImGui::Button( "Reset##tol" ) ) {
		mTolerance = 0.5f;
		calculateOffsetCurve();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.2f, 1.0f ), "Presets" );

	if( ImGui::Button( "Open Line" ) ) loadPresetShape( PresetShape::OPEN_LINE );
	ImGui::SameLine();
	if( ImGui::Button( "Open Curve" ) ) loadPresetShape( PresetShape::OPEN_CURVE );
	ImGui::SameLine();
	if( ImGui::Button( "Open S-Curve" ) ) loadPresetShape( PresetShape::OPEN_S_CURVE );

	if( ImGui::Button( "Closed Rect" ) ) loadPresetShape( PresetShape::CLOSED_RECT );
	ImGui::SameLine();
	if( ImGui::Button( "Closed Circle" ) ) loadPresetShape( PresetShape::CLOSED_CIRCLE );
	ImGui::SameLine();
	if( ImGui::Button( "Closed Star" ) ) loadPresetShape( PresetShape::CLOSED_STAR );

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Join & Cap Styles" );

	const char* joinStyles[] = { "ROUND", "MITER", "BEVEL" };
	if( ImGui::Combo( "Join Style", &mJoinStyle, joinStyles, 3 ) ) {
		calculateOffsetCurve();
	}

	if( mJoinStyle == 1 ) { // MITER
		if( ImGui::SliderFloat( "Miter Limit", &mMiterLimit, 1.0f, 10.0f, "%.1f" ) ) {
			calculateOffsetCurve();
		}
	}

	ImGui::Spacing();

	// Close offset curve checkbox (for open paths only)
	if( ImGui::Checkbox( "Close Offset Curve", &mCloseOffsetCurve ) ) {
		calculateOffsetCurve();
	}
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Add caps to close open paths (only applies to open paths)" );
	}

	// Cap style combo (only enabled if closing offset curve)
	ImGui::BeginDisabled( !mCloseOffsetCurve );
	const char* capStyles[] = { "BUTT", "ROUND", "SQUARE" };
	if( ImGui::Combo( "Cap Style", &mCapStyle, capStyles, 3 ) ) {
		calculateOffsetCurve();
	}
	ImGui::EndDisabled();

	ImGui::Spacing();

	// Number of offset curves slider
	if( ImGui::SliderInt( "Num Curves", &mNumOffsetCurves, 1, 10 ) ) {
		// No need to recalculate, just affects drawing
	}
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Draw multiple offset curves from 0 to Distance" );
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 1.0f, 0.8f, 0.2f, 1.0f ), "Visualization" );
	ImGui::Separator();

	ImGui::Checkbox( "Show Original", &mShowOriginal );
	if( mShowOriginal ) {
		ImGui::SameLine();
		ImGui::ColorEdit3( "##origcolor", &mOriginalColor[0], ImGuiColorEditFlags_NoInputs );
	}

	ImGui::Checkbox( "Show Offset", &mShowOffset );
	if( mShowOffset ) {
		ImGui::SameLine();
		ImGui::ColorEdit3( "##offsetcolor", &mOffsetColor[0], ImGuiColorEditFlags_NoInputs );
	}

	ImGui::Checkbox( "Control Points", &mShowControlPoints );
	ImGui::Checkbox( "Tangents", &mShowTangents );
	ImGui::Checkbox( "Bounding Box", &mShowBoundingBox );

	if( mShowInfo && !mPath.empty() ) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored( ImVec4( 0.2f, 1.0f, 0.2f, 1.0f ), "Path Info" );
		ImGui::Separator();

		// Count segment types
		int lineCount = 0, quadCount = 0, cubicCount = 0;
		for( size_t i = 0; i < mPath.getNumSegments(); ++i ) {
			if( mPath.getSegmentType(i) == Path2d::LINETO ) lineCount++;
			else if( mPath.getSegmentType(i) == Path2d::QUADTO ) quadCount++;
			else if( mPath.getSegmentType(i) == Path2d::CUBICTO ) cubicCount++;
		}

		ImGui::Text( "Original:" );
		ImGui::BulletText( "Segments: %zu", mPath.getNumSegments() );
		ImGui::BulletText( "Points: %zu", mPath.getNumPoints() );
		ImGui::BulletText( "Lines: %d, Quads: %d, Cubics: %d", lineCount, quadCount, cubicCount );
		ImGui::BulletText( "Length: %.1f", mOriginalLength );

		// Show conversion status
		if( mIsConverted ) {
			ImGui::TextColored( ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ), "  [Converted to Cubics]" );
		}

		// Add Convert to Cubics button
		ImGui::Spacing();
		if( quadCount > 0 && !mIsConverted ) {
			if( ImGui::Button( "Convert to Cubics", ImVec2( -1, 0 ) ) ) {
				mPath.convertQuadraticsToCubics();
				mIsConverted = true;
				calculateOffsetCurve();
				console() << "Converted " << quadCount << " quadratic(s) to cubics" << std::endl;
			}
			if( ImGui::IsItemHovered() ) {
				ImGui::SetTooltip( "Convert all quadratic Bezier segments to cubic Bezier segments\nusing degree elevation (mathematically equivalent)" );
			}
		}
		else if( quadCount == 0 && !mIsConverted ) {
			ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.0f ), "No quadratics to convert" );
		}

		ImGui::Spacing();
		ImGui::Text( "Offset:" );
		ImGui::BulletText( "Segments: %zu", mOffsetPath.getNumSegments() );
		ImGui::BulletText( "Points: %zu", mOffsetPath.getNumPoints() );
		ImGui::BulletText( "Length: %.1f", mOffsetLength );
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Instructions" );
	ImGui::Separator();
	ImGui::BulletText( "Click to add points" );
	ImGui::BulletText( "Drag to create curves" );
	ImGui::BulletText( "Shift+Drag for quadratic" );
	ImGui::BulletText( "Press 'X' to clear" );
	ImGui::BulletText( "Press 'G' to toggle info" );

	ImGui::End();
}

void BezierOffsetApp::draw()
{
	gl::clear( Color( 0.05f, 0.05f, 0.1f ) );

	gl::enableAlphaBlending();

	// Draw bounding boxes if enabled
	if( mShowBoundingBox && mPath.getNumSegments() > 1 ) {
		if( mShowOriginal ) {
			gl::color( ColorA( mOriginalColor, 0.15f ) );
			gl::drawSolidRect( mPath.calcPreciseBoundingBox() );
		}

		if( mShowOffset && !mOffsetPath.empty() ) {
			gl::color( ColorA( mOffsetColor, 0.15f ) );
			gl::drawSolidRect( mOffsetPath.calcPreciseBoundingBox() );
		}
	}

	// Draw multiple offset curves
	if( mShowOffset && !mPath.empty() ) {
		for( int i = 0; i < mNumOffsetCurves; ++i ) {
			float t = (float)(i + 1) / (float)mNumOffsetCurves;
			float distance = mOffsetDistance * t;

			// Calculate offset for this distance
			Path2d::OffsetOptions opts;
			opts.tolerance = mTolerance;
			switch( mJoinStyle ) {
				case 0: opts.joinStyle = Path2d::OffsetOptions::ROUND; break;
				case 1: opts.joinStyle = Path2d::OffsetOptions::MITER; break;
				case 2: opts.joinStyle = Path2d::OffsetOptions::BEVEL; break;
			}
			opts.miterLimit = mMiterLimit;

			// Set cap style
			if( mCloseOffsetCurve ) {
				switch( mCapStyle ) {
					case 0: opts.capStyle = Path2d::OffsetOptions::CAP_BUTT; break;
					case 1: opts.capStyle = Path2d::OffsetOptions::CAP_ROUND; break;
					case 2: opts.capStyle = Path2d::OffsetOptions::CAP_SQUARE; break;
				}
			}
			else {
				opts.capStyle = Path2d::OffsetOptions::CAP_NONE;
			}

			Path2d offsetCurve = mPath.calcOffsetCurve( distance, opts );

			// Fade color for intermediate curves
			ColorA curveColor( mOffsetColor, 1.0f - (1.0f - t) * 0.5f );
			gl::color( curveColor );
			gl::lineWidth( (i == mNumOffsetCurves - 1) ? 3.0f : 2.0f );
			gl::draw( offsetCurve );

			// Draw control points for final curve only
			if( i == mNumOffsetCurves - 1 && mShowControlPoints ) {
				gl::color( ColorA( mOffsetColor, 0.5f ) );
				for( size_t p = 0; p < offsetCurve.getNumPoints(); ++p )
					gl::drawSolidCircle( offsetCurve.getPoint( p ), 2.0f );
			}
		}
		gl::lineWidth( 1.0f );
	}

	// Draw the original curve
	if( mShowOriginal && !mPath.empty() ) {
		gl::color( mOriginalColor );
		gl::lineWidth( 2.0f );
		gl::draw( mPath );
		gl::lineWidth( 1.0f );

		// Draw control points
		if( mShowControlPoints ) {
			gl::color( Color( 1, 1, 0 ) );
			for( size_t p = 0; p < mPath.getNumPoints(); ++p )
				gl::drawSolidCircle( mPath.getPoint( p ), 3.5f );
		}

		// Draw tangents
		if( mShowTangents && mPath.getNumSegments() > 1 ) {
			gl::color( Color( 0.2f, 0.9f, 0.2f ) );
			for( float t = 0; t < 1; t += 0.1f ) {
				vec2 pos = mPath.getPosition( t );
				vec2 tan = normalize( mPath.getTangent( t ) );
				gl::drawLine( pos, pos + tan * 50.0f );
			}
		}
	}

	// Draw ImGui interface
	drawImGuiControls();
}

CINDER_APP( BezierOffsetApp, RendererGl( RendererGl::Options().msaa( 16 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
	settings->setTitle( "Bezier Offset - Interactive Demo" );
} )
