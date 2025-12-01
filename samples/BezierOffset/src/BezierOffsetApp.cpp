/*
 * BezierOffset Sample
 * Demonstrates path offsetting and stroke expansion using Path2dStroke
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/Path2dStroke.h"
#include "cinder/CinderImGui.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

enum class PresetShape {
	OPEN_LINE,
	OPEN_CURVE,
	OPEN_S_CURVE,
	CLOSED_RECT,
	CLOSED_CIRCLE,
	CLOSED_STAR,
	SHARP_ZIGZAG
};

class BezierOffsetApp : public App {
public:
	BezierOffsetApp() : mTrackedPoint( -1 ) {}

	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void draw() override;

private:
	void drawImGuiControls();
	void updateResult();
	void loadPresetShape( PresetShape shape );

	Path2d      mPath;
	Shape2d     mResult;
	int         mTrackedPoint;
	int         mHoveredPoint = -1;
	bool        mDraggingPoint = false;

	// Mode selection
	enum class Mode { OFFSET, STROKE };
	Mode        mMode = Mode::STROKE;

	// Offset parameters
	float       mOffsetDistance = 20.0f;
	float       mTolerance = 0.25f;
	int         mNumOffsetCurves = 1;

	// Stroke parameters
	float       mStrokeWidth = 40.0f;
	int         mJoinStyle = 2;    // 0=Bevel, 1=Miter, 2=Round
	float       mMiterLimit = 4.0f;
	int         mCapStyle = 2;     // 0=Butt, 1=Square, 2=Round

	// Dash pattern parameters
	bool        mEnableDashing = false;
	int         mDashPreset = 0;
	float       mDashOn = 20.0f;
	float       mDashOff = 10.0f;
	float       mDashOn2 = 5.0f;
	float       mDashOff2 = 10.0f;
	float       mDashOffset = 0.0f;

	// Visualization options
	bool        mShowOriginal = true;
	bool        mShowResult = true;
	bool        mShowControlPoints = true;
	bool        mShowFilled = true;
	bool        mShowOutline = true;
	Color       mOriginalColor = Color( 1.0f, 0.5f, 0.25f );
	Color       mResultColor = Color( 0.25f, 0.6f, 0.9f );
};

void BezierOffsetApp::setup()
{
	CI_LOG_I( "Initializing BezierOffset Application" );

	ImGui::Initialize();

	// Start with an S-curve
	loadPresetShape( PresetShape::OPEN_S_CURVE );

	CI_LOG_I( "BezierOffset ready" );
}

void BezierOffsetApp::mouseDown( MouseEvent event )
{
	if( event.isLeftDown() && !ImGui::GetIO().WantCaptureMouse ) {
		if( mHoveredPoint >= 0 ) {
			mTrackedPoint = mHoveredPoint;
			mDraggingPoint = true;
			return;
		}

		vec2 pos = event.getPos();
		if( mPath.empty() ) {
			mPath.moveTo( pos );
			mTrackedPoint = 0;
		}
		else {
			mPath.lineTo( pos );
		}
		updateResult();
	}
}

void BezierOffsetApp::mouseDrag( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse )
		return;

	vec2 pos = event.getPos();

	if( mDraggingPoint && mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, pos );
		updateResult();
		return;
	}

	if( mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, pos );
	}
	else if( mPath.getNumSegments() > 0 ) {
		vec2 endPt = mPath.getPoint( mPath.getNumPoints() - 1 );
		mPath.removeSegment( mPath.getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( mPath.getNumSegments() == 0 )
			? Path2d::MOVETO
			: mPath.getSegmentType( mPath.getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			mPath.quadTo( pos, endPt );
		}
		else {
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
			mPath.curveTo( tan1, pos, endPt );
		}
		mTrackedPoint = (int)mPath.getNumPoints() - 2;
	}
	updateResult();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
	mDraggingPoint = false;
}

void BezierOffsetApp::mouseMove( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse ) {
		mHoveredPoint = -1;
		return;
	}

	vec2 pos = event.getPos();
	float hoverRadius = 8.0f;
	float closestDist = hoverRadius;
	int closestPoint = -1;

	for( size_t i = 0; i < mPath.getNumPoints(); ++i ) {
		float dist = glm::distance( pos, mPath.getPoint( i ) );
		if( dist < closestDist ) {
			closestDist = dist;
			closestPoint = (int)i;
		}
	}
	mHoveredPoint = closestPoint;
}

void BezierOffsetApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'x':
		case 'X':
			mPath.clear();
			mResult.clear();
			break;
		case 'c':
		case 'C':
			if( !mPath.empty() && !mPath.isClosed() ) {
				mPath.close();
				updateResult();
			}
			break;
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
			break;

		case PresetShape::OPEN_CURVE:
			mPath.moveTo( center + vec2( -150, 0 ) );
			mPath.curveTo( center + vec2( -50, -100 ), center + vec2( 50, -100 ), center + vec2( 150, 0 ) );
			break;

		case PresetShape::OPEN_S_CURVE:
			mPath.moveTo( center + vec2( -150, 50 ) );
			mPath.curveTo( center + vec2( -50, -100 ), center + vec2( 50, 200 ), center + vec2( 150, 50 ) );
			break;

		case PresetShape::CLOSED_RECT:
			mPath.moveTo( center + vec2( -100, -80 ) );
			mPath.lineTo( center + vec2( 100, -80 ) );
			mPath.lineTo( center + vec2( 100, 80 ) );
			mPath.lineTo( center + vec2( -100, 80 ) );
			mPath.close();
			break;

		case PresetShape::CLOSED_CIRCLE: {
			float radius = 100.0f;
			float k = 0.5522847498f;
			float kr = k * radius;
			mPath.moveTo( center + vec2( 0, -radius ) );
			mPath.curveTo( center + vec2( kr, -radius ), center + vec2( radius, -kr ), center + vec2( radius, 0 ) );
			mPath.curveTo( center + vec2( radius, kr ), center + vec2( kr, radius ), center + vec2( 0, radius ) );
			mPath.curveTo( center + vec2( -kr, radius ), center + vec2( -radius, kr ), center + vec2( -radius, 0 ) );
			mPath.curveTo( center + vec2( -radius, -kr ), center + vec2( -kr, -radius ), center + vec2( 0, -radius ) );
			mPath.close();
			break;
		}

		case PresetShape::CLOSED_STAR: {
			float outerRadius = 100.0f;
			float innerRadius = 40.0f;
			int points = 5;
			mPath.moveTo( center + vec2( 0, -outerRadius ) );
			for( int i = 1; i < points * 2; ++i ) {
				float angle = (float)i * (float)M_PI / points - (float)M_PI / 2.0f;
				float r = (i % 2 == 0) ? outerRadius : innerRadius;
				mPath.lineTo( center + vec2( std::cos( angle ), std::sin( angle ) ) * r );
			}
			mPath.close();
			break;
		}

		case PresetShape::SHARP_ZIGZAG:
			mPath.moveTo( center + vec2( -150, -50 ) );
			mPath.lineTo( center + vec2( -100, 50 ) );
			mPath.lineTo( center + vec2( -50, -50 ) );
			mPath.lineTo( center + vec2( 0, 50 ) );
			mPath.lineTo( center + vec2( 50, -50 ) );
			mPath.lineTo( center + vec2( 100, 50 ) );
			mPath.lineTo( center + vec2( 150, -50 ) );
			break;
	}

	updateResult();
}

void BezierOffsetApp::updateResult()
{
	if( mPath.empty() || mPath.getNumSegments() == 0 ) {
		mResult.clear();
		return;
	}

	StrokeJoin joinStyle;
	switch( mJoinStyle ) {
		case 0: joinStyle = StrokeJoin::Bevel; break;
		case 1: joinStyle = StrokeJoin::Miter; break;
		default: joinStyle = StrokeJoin::Round; break;
	}

	StrokeCap capStyle;
	switch( mCapStyle ) {
		case 0: capStyle = StrokeCap::Butt; break;
		case 1: capStyle = StrokeCap::Square; break;
		default: capStyle = StrokeCap::Round; break;
	}

	if( mMode == Mode::OFFSET ) {
		mResult = offset( mPath, mOffsetDistance, mTolerance );
	}
	else {
		StrokeStyle style( mStrokeWidth );
		style.withJoin( joinStyle ).withMiterLimit( mMiterLimit ).withCaps( capStyle );

		if( mEnableDashing ) {
			vector<float> pattern;
			switch( mDashPreset ) {
				case 0: pattern = { mDashOn, mDashOff }; break;
				case 1: pattern = { mDashOn, mDashOff }; break;
				case 2: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff }; break;
				case 3: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff, mDashOn2, mDashOff }; break;
				case 4: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff2 }; break;
			}
			style.withDashes( mDashOffset, pattern );
		}

		mResult = stroke( mPath, style, mTolerance );
	}
}

void BezierOffsetApp::drawImGuiControls()
{
	ImGui::Begin( "Bezier Offset Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize );

	// Mode selection
	ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.2f, 1.0f ), "Mode" );
	ImGui::Separator();

	const char* modes[] = { "Offset", "Stroke" };
	int modeIdx = (mMode == Mode::OFFSET) ? 0 : 1;
	if( ImGui::Combo( "##mode", &modeIdx, modes, 2 ) ) {
		mMode = (modeIdx == 0) ? Mode::OFFSET : Mode::STROKE;
		updateResult();
	}

	ImGui::Spacing();
	ImGui::Separator();

	if( mMode == Mode::OFFSET ) {
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Offset Parameters" );
		ImGui::Separator();

		if( ImGui::SliderFloat( "Distance", &mOffsetDistance, -100.0f, 100.0f, "%.1f" ) ) {
			updateResult();
		}

		if( ImGui::SliderFloat( "Tolerance", &mTolerance, 0.01f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) ) {
			updateResult();
		}

		ImGui::SliderInt( "Num Curves", &mNumOffsetCurves, 1, 10 );
	}
	else {
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Stroke Parameters" );
		ImGui::Separator();

		if( ImGui::SliderFloat( "Width", &mStrokeWidth, 1.0f, 100.0f, "%.1f" ) ) {
			updateResult();
		}

		if( ImGui::SliderFloat( "Tolerance", &mTolerance, 0.01f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) ) {
			updateResult();
		}

		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Join Style" );
		ImGui::Separator();

		const char* joinStyles[] = { "Bevel", "Miter", "Round" };
		if( ImGui::Combo( "##join", &mJoinStyle, joinStyles, 3 ) ) {
			updateResult();
		}

		if( mJoinStyle == 1 ) {
			if( ImGui::SliderFloat( "Miter Limit", &mMiterLimit, 1.0f, 10.0f, "%.1f" ) ) {
				updateResult();
			}
		}

		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Cap Style" );
		ImGui::Separator();

		const char* capStyles[] = { "Butt", "Square", "Round" };
		if( ImGui::Combo( "##cap", &mCapStyle, capStyles, 3 ) ) {
			updateResult();
		}

		// Dash Pattern
		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Dash Pattern" );
		ImGui::Separator();

		if( ImGui::Checkbox( "Enable Dashing", &mEnableDashing ) ) {
			updateResult();
		}

		ImGui::BeginDisabled( !mEnableDashing );

		const char* dashPresets[] = { "Dashed", "Dotted", "Dash-Dot", "Dash-Dot-Dot", "Custom" };
		if( ImGui::Combo( "Preset", &mDashPreset, dashPresets, 5 ) ) {
			switch( mDashPreset ) {
				case 0: mDashOn = 20.0f; mDashOff = 10.0f; break;
				case 1: mDashOn = 2.0f; mDashOff = 8.0f; break;
				case 2: mDashOn = 20.0f; mDashOff = 10.0f; mDashOn2 = 2.0f; break;
				case 3: mDashOn = 20.0f; mDashOff = 8.0f; mDashOn2 = 2.0f; break;
			}
			updateResult();
		}

		if( mDashPreset <= 1 ) {
			if( ImGui::SliderFloat( "On", &mDashOn, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "Off", &mDashOff, 1.0f, 50.0f ) ) updateResult();
		}
		else if( mDashPreset <= 3 ) {
			if( ImGui::SliderFloat( "Dash", &mDashOn, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "Gap", &mDashOff, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "Dot", &mDashOn2, 1.0f, 20.0f ) ) updateResult();
		}
		else {
			if( ImGui::SliderFloat( "On 1", &mDashOn, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "Off 1", &mDashOff, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "On 2", &mDashOn2, 1.0f, 50.0f ) ) updateResult();
			if( ImGui::SliderFloat( "Off 2", &mDashOff2, 1.0f, 50.0f ) ) updateResult();
		}

		if( ImGui::SliderFloat( "Offset", &mDashOffset, 0.0f, 100.0f ) ) {
			updateResult();
		}

		// Pattern preview
		ImGui::Text( "Preview:" );
		ImVec2 previewPos = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		float previewWidth = ImGui::GetContentRegionAvail().x - 10.0f;
		float previewHeight = 16.0f;

		drawList->AddRectFilled( previewPos,
			ImVec2( previewPos.x + previewWidth, previewPos.y + previewHeight ),
			IM_COL32( 40, 40, 40, 255 ) );

		vector<float> pattern;
		switch( mDashPreset ) {
			case 0: case 1: pattern = { mDashOn, mDashOff }; break;
			case 2: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff }; break;
			case 3: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff, mDashOn2, mDashOff }; break;
			case 4: pattern = { mDashOn, mDashOff, mDashOn2, mDashOff2 }; break;
		}

		float patternTotal = 0;
		for( float v : pattern ) patternTotal += v;

		if( patternTotal > 0 ) {
			float scale = min( 1.0f, previewWidth / (patternTotal * 3.0f) );
			float x = previewPos.x - mDashOffset * scale;
			bool on = true;

			for( int repeat = 0; repeat < 10 && x < previewPos.x + previewWidth; ++repeat ) {
				for( float len : pattern ) {
					float scaledLen = len * scale;
					if( on && x + scaledLen > previewPos.x ) {
						drawList->AddRectFilled(
							ImVec2( max( x, previewPos.x ), previewPos.y + 2.0f ),
							ImVec2( min( x + scaledLen, previewPos.x + previewWidth ), previewPos.y + previewHeight - 2.0f ),
							IM_COL32( 100, 180, 255, 255 ) );
					}
					x += scaledLen;
					on = !on;
					if( x > previewPos.x + previewWidth ) break;
				}
			}
		}
		ImGui::Dummy( ImVec2( previewWidth, previewHeight ) );

		ImGui::EndDisabled();
	}

	// Presets
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.2f, 1.0f ), "Presets" );

	if( ImGui::Button( "Line" ) ) loadPresetShape( PresetShape::OPEN_LINE );
	ImGui::SameLine();
	if( ImGui::Button( "Curve" ) ) loadPresetShape( PresetShape::OPEN_CURVE );
	ImGui::SameLine();
	if( ImGui::Button( "S-Curve" ) ) loadPresetShape( PresetShape::OPEN_S_CURVE );

	if( ImGui::Button( "Rect" ) ) loadPresetShape( PresetShape::CLOSED_RECT );
	ImGui::SameLine();
	if( ImGui::Button( "Circle" ) ) loadPresetShape( PresetShape::CLOSED_CIRCLE );
	ImGui::SameLine();
	if( ImGui::Button( "Star" ) ) loadPresetShape( PresetShape::CLOSED_STAR );

	if( ImGui::Button( "Zigzag (Join Test)" ) ) {
		loadPresetShape( PresetShape::SHARP_ZIGZAG );
	}

	// Visualization
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 1.0f, 0.8f, 0.2f, 1.0f ), "Visualization" );
	ImGui::Separator();

	ImGui::Checkbox( "Show Original", &mShowOriginal );
	if( mShowOriginal ) {
		ImGui::SameLine();
		ImGui::ColorEdit3( "##origcolor", &mOriginalColor[0], ImGuiColorEditFlags_NoInputs );
	}

	ImGui::Checkbox( "Show Result", &mShowResult );
	if( mShowResult ) {
		ImGui::SameLine();
		ImGui::ColorEdit3( "##resultcolor", &mResultColor[0], ImGuiColorEditFlags_NoInputs );
	}

	ImGui::Checkbox( "Control Points", &mShowControlPoints );
	ImGui::Checkbox( "Filled", &mShowFilled );
	ImGui::Checkbox( "Outline", &mShowOutline );

	// Path info
	if( !mPath.empty() ) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored( ImVec4( 0.2f, 1.0f, 0.2f, 1.0f ), "Path Info" );
		ImGui::Separator();

		int lineCount = 0, quadCount = 0, cubicCount = 0;
		for( size_t i = 0; i < mPath.getNumSegments(); ++i ) {
			auto type = mPath.getSegmentType( i );
			if( type == Path2d::LINETO ) lineCount++;
			else if( type == Path2d::QUADTO ) quadCount++;
			else if( type == Path2d::CUBICTO ) cubicCount++;
		}

		ImGui::Text( "Original: %zu segments, %zu points", mPath.getNumSegments(), mPath.getNumPoints() );
		ImGui::Text( "  Lines: %d, Quads: %d, Cubics: %d", lineCount, quadCount, cubicCount );
		ImGui::Text( "  Closed: %s", mPath.isClosed() ? "Yes" : "No" );

		ImGui::Spacing();
		ImGui::Text( "Result: %zu contours", mResult.getNumContours() );
		size_t totalPts = 0;
		for( const auto& c : mResult.getContours() ) totalPts += c.getNumPoints();
		ImGui::Text( "  Total points: %zu", totalPts );
	}

	// Instructions
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Controls" );
	ImGui::Separator();
	ImGui::BulletText( "Click to add points" );
	ImGui::BulletText( "Drag to create curves" );
	ImGui::BulletText( "Shift+Drag for quadratic" );
	ImGui::BulletText( "Drag points to edit" );
	ImGui::BulletText( "'X' to clear, 'C' to close" );

	ImGui::End();
}

void BezierOffsetApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.15f ) );
	gl::enableAlphaBlending();

	// Draw result
	if( mShowResult && !mResult.empty() ) {
		if( mMode == Mode::OFFSET && mNumOffsetCurves > 1 ) {
			// Draw multiple offset curves
			for( int i = 0; i < mNumOffsetCurves; ++i ) {
				float t = (float)(i + 1) / (float)mNumOffsetCurves;
				float distance = mOffsetDistance * t;

				Shape2d offsetResult = offset( mPath, distance, mTolerance );

				Color gradColor = mOriginalColor * (1.0f - t) + mResultColor * t;
				float alpha = (mNumOffsetCurves > 3) ? (0.3f + 0.7f * t) : 0.7f;

				if( mShowFilled ) {
					gl::color( ColorA( gradColor, alpha * 0.5f ) );
					gl::draw( offsetResult );
				}

				if( mShowOutline ) {
					gl::color( ColorA( gradColor, alpha ) );
					for( const auto& contour : offsetResult.getContours() ) {
						gl::draw( contour );
					}
				}
			}
		}
		else {
			if( mShowFilled ) {
				gl::color( ColorA( mResultColor, 0.5f ) );
				gl::draw( mResult );
			}

			if( mShowOutline ) {
				gl::color( ColorA( mResultColor.r * 1.3f, mResultColor.g * 1.3f, mResultColor.b * 1.3f, 0.8f ) );
				for( const auto& contour : mResult.getContours() ) {
					gl::draw( contour );
				}
			}
		}
	}

	// Draw original path
	if( mShowOriginal && !mPath.empty() ) {
		gl::color( mOriginalColor );
		gl::lineWidth( 2.0f );
		gl::draw( mPath );
		gl::lineWidth( 1.0f );

		// Control points
		if( mShowControlPoints ) {
			for( size_t i = 0; i < mPath.getNumPoints(); ++i ) {
				bool isHovered = ((int)i == mHoveredPoint);

				if( isHovered ) {
					gl::color( Color( 1, 1, 1 ) );
					gl::drawSolidCircle( mPath.getPoint( i ), 6.0f );
					gl::color( Color( 0.2f, 1.0f, 0.2f ) );
					gl::drawSolidCircle( mPath.getPoint( i ), 4.5f );
				}
				else {
					gl::color( Color( 1, 1, 0 ) );
					gl::drawSolidCircle( mPath.getPoint( i ), 3.5f );
				}
			}
		}
	}

	drawImGuiControls();
}

CINDER_APP( BezierOffsetApp, RendererGl( RendererGl::Options().msaa( 8 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
	settings->setTitle( "Bezier Offset & Stroke Demo" );
})
