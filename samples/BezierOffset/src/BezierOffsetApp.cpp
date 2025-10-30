#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderImGui.h"
#include "cinder/CanvasUi.h"
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
	void mouseMove( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void draw() override;

  private:
	void drawImGuiControls();
	void calculateOffsetCurve();
	void loadPresetShape( PresetShape shape );

	Path2d	mPath;
	Path2d	mOffsetPath;
	Shape2d	mStrokedShape;  // For dashed strokes (multi-contour)
	int		mTrackedPoint;
	int		mHoveredPoint = -1;  // Index of hovered control point (-1 = none)
	bool	mDraggingPoint = false;  // True when dragging an existing point

	// Canvas pan/zoom UI
	CanvasUi mCanvas;

	// Mode selection
	enum class Mode { OFFSET, STROKE };
	Mode	mMode = Mode::OFFSET;

	// Offset parameters
	float	mOffsetDistance = 20.0f;
	float	mTolerance = 0.5f;
	int		mJoinStyle = 0; // 0=ROUND, 1=MITER, 2=BEVEL
	float	mMiterLimit = 4.0f;
	int		mNumOffsetCurves = 1;  // Number of offset curves to draw (from 0 to mOffsetDistance)

	// Stroke parameters
	float	mStrokeWidth = 40.0f;
	int		mStrokeJoinStyle = 1; // 0=ROUND, 1=MITER (SVG default), 2=BEVEL
	float	mStrokeMiterLimit = 4.0f;
	int		mStartCapStyle = 0;  // 0=BUTT (SVG default), 1=ROUND, 2=SQUARE
	int		mEndCapStyle = 0;    // 0=BUTT (SVG default), 1=ROUND, 2=SQUARE

	// Dash pattern parameters
	bool	mEnableDashing = false;
	int		mDashPreset = 0; // 0=Dashed, 1=Dotted, 2=Dash-Dot, 3=Dash-Dot-Dot, 4=Custom
	float	mDashOn = 20.0f;
	float	mDashOff = 10.0f;
	float	mDashOn2 = 5.0f;   // For complex patterns
	float	mDashOff2 = 10.0f; // For complex patterns
	float	mDashOffset = 0.0f;

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
	ImGui::Initialize();
	ImGui::GetStyle().ScaleAllSizes( getWindowContentScale() );
	ImGui::GetStyle().FontScaleMain = getWindowContentScale();

	// Start with a simple example curve
	mPath.moveTo( vec2( 100, 300 ) );
	mPath.curveTo( vec2( 150, 100 ), vec2( 450, 100 ), vec2( 500, 300 ) );
	mPath.curveTo( vec2( 550, 500 ), vec2( 150, 500 ), vec2( 200, 300 ) );

	calculateOffsetCurve();

	// Initialize CanvasUi for pan/zoom
	mCanvas.connect( getWindow() );
	mCanvas.setUnbounded();  // Allow unbounded panning

	CI_LOG_I( "BezierOffset ready" );
}

void BezierOffsetApp::mouseDown( MouseEvent event )
{
	if( event.isLeftDown() && !ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) ) {
		// Check if clicking on an existing control point
		if( mHoveredPoint >= 0 ) {
			mTrackedPoint = mHoveredPoint;
			mDraggingPoint = true;
			return;
		}

		// Otherwise, add new points as before
		vec2 canvasPos = mCanvas.toContent( event.getPos() );

		if( mPath.empty() ) {
			mPath.moveTo( canvasPos );
			mTrackedPoint = 0;
		}
		else {
			mPath.lineTo( canvasPos );
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

	vec2 canvasPos = mCanvas.toContent( event.getPos() );

	// If dragging an existing point, just move it
	if( mDraggingPoint && mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, canvasPos );
		mIsConverted = false;
		calculateOffsetCurve();
		return;
	}

	// Otherwise, handle creating new curve segments
	if( mTrackedPoint >= 0 ) {
		mPath.setPoint( mTrackedPoint, canvasPos );
	}
	else {
		// First bit of dragging, so switch our line to a cubic or a quad if Shift is down
		vec2 endPt = mPath.getPoint( mPath.getNumPoints() - 1 );
		mPath.removeSegment( mPath.getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( mPath.getNumSegments() == 0 ) ? Path2d::MOVETO : mPath.getSegmentType( mPath.getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			// Add a quadratic curve segment
			mPath.quadTo( canvasPos, endPt );
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

			mPath.curveTo( tan1, canvasPos, endPt );
		}

		mTrackedPoint = mPath.getNumPoints() - 2;
	}

	mIsConverted = false;  // Reset conversion flag when modifying path
	calculateOffsetCurve();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
	mDraggingPoint = false;
}

void BezierOffsetApp::mouseMove( MouseEvent event )
{
	if( ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) ) {
		mHoveredPoint = -1;
		return;
	}

	// Convert mouse position from window space to canvas/content space
	vec2 canvasPos = mCanvas.toContent( event.getPos() );

	// Find closest control point
	float hoverRadius = 8.0f / mCanvas.getZoom();  // Scale with zoom
	float closestDist = hoverRadius;
	int closestPoint = -1;

	for( size_t i = 0; i < mPath.getNumPoints(); ++i ) {
		vec2 pt = mPath.getPoint( i );
		float dist = glm::distance( canvasPos, pt );

		if( dist < closestDist ) {
			closestDist = dist;
			closestPoint = (int)i;
		}
	}

	mHoveredPoint = closestPoint;
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
		if( mMode == Mode::OFFSET ) {
			// Single-sided offset (no caps)
			Path2d::OffsetOptions opts;
			opts.tolerance = mTolerance;

			switch( mJoinStyle ) {
				case 0: opts.joinStyle = Path2d::OffsetOptions::ROUND; break;
				case 1: opts.joinStyle = Path2d::OffsetOptions::MITER; break;
				case 2: opts.joinStyle = Path2d::OffsetOptions::BEVEL; break;
			}

			opts.miterLimit = mMiterLimit;

			mOffsetPath = mPath.calcOffsetCurve( mOffsetDistance, opts );
		}
		else {
			// Bilateral stroke expansion with caps
			Path2d::StrokeOptions opts;
			opts.width = mStrokeWidth;
			opts.tolerance = mTolerance;

			switch( mStrokeJoinStyle ) {
				case 0: opts.joinStyle = Path2d::StrokeOptions::ROUND; break;
				case 1: opts.joinStyle = Path2d::StrokeOptions::MITER; break;
				case 2: opts.joinStyle = Path2d::StrokeOptions::BEVEL; break;
			}

			opts.miterLimit = mStrokeMiterLimit;

			switch( mStartCapStyle ) {
				case 0: opts.startCap = Path2d::StrokeOptions::CAP_BUTT; break;
				case 1: opts.startCap = Path2d::StrokeOptions::CAP_ROUND; break;
				case 2: opts.startCap = Path2d::StrokeOptions::CAP_SQUARE; break;
			}

			switch( mEndCapStyle ) {
				case 0: opts.endCap = Path2d::StrokeOptions::CAP_BUTT; break;
				case 1: opts.endCap = Path2d::StrokeOptions::CAP_ROUND; break;
				case 2: opts.endCap = Path2d::StrokeOptions::CAP_SQUARE; break;
			}

			// Build dash pattern if enabled
			if( mEnableDashing ) {
				switch( mDashPreset ) {
					case 0: // Dashed
						opts.dashPattern = { mDashOn, mDashOff };
						break;
					case 1: // Dotted
						opts.dashPattern = { mDashOn, mDashOff };
						break;
					case 2: // Dash-Dot
						opts.dashPattern = { mDashOn, mDashOff, mDashOn2, mDashOff };
						break;
					case 3: // Dash-Dot-Dot
						opts.dashPattern = { mDashOn, mDashOff, mDashOn2, mDashOff, mDashOn2, mDashOff };
						break;
					case 4: // Custom (user can edit all values)
						opts.dashPattern = { mDashOn, mDashOff, mDashOn2, mDashOff2 };
						break;
				}
				opts.dashOffset = mDashOffset;

				// Use calcStrokeAsShape for dashed strokes (returns multi-contour Shape2d)
				mStrokedShape = mPath.calcStrokeAsShape( opts );
				mOffsetPath.clear(); // Clear single-path result
			}
			else {
				// Non-dashed stroke - use regular calcStroke
				mOffsetPath = mPath.calcStroke( opts );
				mStrokedShape = Shape2d(); // Clear multi-contour result
			}
		}

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

	// Mode selection
	ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.2f, 1.0f ), "Mode" );
	ImGui::Separator();

	const char* modes[] = { "Offset", "Stroke" };
	int modeIdx = (mMode == Mode::OFFSET) ? 0 : 1;
	if( ImGui::Combo( "##mode", &modeIdx, modes, 2 ) ) {
		mMode = (modeIdx == 0) ? Mode::OFFSET : Mode::STROKE;
		calculateOffsetCurve();
	}
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Offset: single-sided parallel curve\nStroke: bilateral expansion with caps" );
	}

	ImGui::Spacing();
	ImGui::Separator();

	if( mMode == Mode::OFFSET ) {
		// OFFSET MODE CONTROLS
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

		// Number of offset curves slider (moved above join style)
		ImGui::Spacing();
		if( ImGui::SliderInt( "Num Curves", &mNumOffsetCurves, 1, 10 ) ) {
			// No need to recalculate, just affects drawing
		}
		if( ImGui::IsItemHovered() ) {
			ImGui::SetTooltip( "Draw multiple offset curves from 0 to Distance" );
		}

		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Join Style" );
		ImGui::Separator();

		const char* joinStyles[] = { "ROUND", "MITER", "BEVEL" };
		if( ImGui::Combo( "##join", &mJoinStyle, joinStyles, 3 ) ) {
			calculateOffsetCurve();
		}

		if( mJoinStyle == 1 ) { // MITER
			if( ImGui::SliderFloat( "Miter Limit", &mMiterLimit, 1.0f, 10.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
		}
	}
	else {
		// STROKE MODE CONTROLS
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Stroke Parameters" );
		ImGui::Separator();

		if( ImGui::SliderFloat( "Width", &mStrokeWidth, 1.0f, 100.0f, "%.1f" ) ) {
			calculateOffsetCurve();
		}
		ImGui::SameLine();
		if( ImGui::Button( "Reset##width" ) ) {
			mStrokeWidth = 40.0f;
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
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Join Style" );
		ImGui::Separator();

		const char* joinStyles[] = { "ROUND", "MITER", "BEVEL" };
		if( ImGui::Combo( "##strokejoin", &mStrokeJoinStyle, joinStyles, 3 ) ) {
			calculateOffsetCurve();
		}

		if( mStrokeJoinStyle == 1 ) { // MITER
			if( ImGui::SliderFloat( "Miter Limit", &mStrokeMiterLimit, 1.0f, 10.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
		}

		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Cap Styles" );
		ImGui::Separator();

		const char* capStyles[] = { "BUTT", "ROUND", "SQUARE" };
		if( ImGui::Combo( "Start Cap", &mStartCapStyle, capStyles, 3 ) ) {
			calculateOffsetCurve();
		}
		if( ImGui::Combo( "End Cap", &mEndCapStyle, capStyles, 3 ) ) {
			calculateOffsetCurve();
		}

		// Dash Pattern UI
		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Dash Pattern" );
		ImGui::Separator();

		if( ImGui::Checkbox( "Enable Dashing", &mEnableDashing ) ) {
			calculateOffsetCurve();
		}

		ImGui::BeginDisabled( !mEnableDashing );

		const char* dashPresets[] = { "Dashed", "Dotted", "Dash-Dot", "Dash-Dot-Dot", "Custom" };
		if( ImGui::Combo( "Preset", &mDashPreset, dashPresets, 5 ) ) {
			// Set default values for each preset
			switch( mDashPreset ) {
				case 0: // Dashed
					mDashOn = 20.0f;
					mDashOff = 10.0f;
					break;
				case 1: // Dotted
					mDashOn = 2.0f;
					mDashOff = 8.0f;
					break;
				case 2: // Dash-Dot
					mDashOn = 20.0f;
					mDashOff = 10.0f;
					mDashOn2 = 2.0f;
					break;
				case 3: // Dash-Dot-Dot
					mDashOn = 20.0f;
					mDashOff = 8.0f;
					mDashOn2 = 2.0f;
					break;
				case 4: // Custom
					// Keep current values
					break;
			}
			calculateOffsetCurve();
		}

		// Show appropriate sliders based on preset
		if( mDashPreset <= 1 ) {
			// Dashed or Dotted - simple on/off pattern
			if( ImGui::SliderFloat( "On", &mDashOn, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "Off", &mDashOff, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
		}
		else if( mDashPreset == 2 || mDashPreset == 3 ) {
			// Dash-Dot or Dash-Dot-Dot - complex pattern
			if( ImGui::SliderFloat( "Dash", &mDashOn, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "Gap", &mDashOff, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "Dot", &mDashOn2, 1.0f, 20.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
		}
		else if( mDashPreset == 4 ) {
			// Custom - all controls
			if( ImGui::SliderFloat( "On 1", &mDashOn, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "Off 1", &mDashOff, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "On 2", &mDashOn2, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
			if( ImGui::SliderFloat( "Off 2", &mDashOff2, 1.0f, 50.0f, "%.1f" ) ) {
				calculateOffsetCurve();
			}
		}

		// Dash offset slider
		if( ImGui::SliderFloat( "Offset", &mDashOffset, 0.0f, 100.0f, "%.1f" ) ) {
			calculateOffsetCurve();
		}
		if( ImGui::IsItemHovered() ) {
			ImGui::SetTooltip( "Starting offset into the dash pattern" );
		}

		// Visual preview of dash pattern
		ImGui::Spacing();
		ImGui::Text( "Pattern Preview:" );
		ImVec2 previewPos = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		float previewWidth = ImGui::GetContentRegionAvail().x - 10.0f;
		float previewHeight = 20.0f;

		// Draw background
		drawList->AddRectFilled( previewPos,
		                        ImVec2( previewPos.x + previewWidth, previewPos.y + previewHeight ),
		                        IM_COL32( 40, 40, 40, 255 ) );

		// Draw pattern
		std::vector<float> pattern;
		switch( mDashPreset ) {
			case 0: // Dashed
			case 1: // Dotted
				pattern = { mDashOn, mDashOff };
				break;
			case 2: // Dash-Dot
				pattern = { mDashOn, mDashOff, mDashOn2, mDashOff };
				break;
			case 3: // Dash-Dot-Dot
				pattern = { mDashOn, mDashOff, mDashOn2, mDashOff, mDashOn2, mDashOff };
				break;
			case 4: // Custom
				pattern = { mDashOn, mDashOff, mDashOn2, mDashOff2 };
				break;
		}

		float x = previewPos.x;
		float patternTotal = 0.0f;
		for( float v : pattern ) patternTotal += v;

		if( patternTotal > 0 ) {
			float scale = std::min( 1.0f, previewWidth / (patternTotal * 3.0f) );
			x -= mDashOffset * scale;

			bool on = true;
			for( int repeat = 0; repeat < 10 && x < previewPos.x + previewWidth; ++repeat ) {
				for( float len : pattern ) {
					float scaledLen = len * scale;
					if( on && x + scaledLen > previewPos.x ) {
						drawList->AddRectFilled(
							ImVec2( std::max( x, previewPos.x ), previewPos.y + 2.0f ),
							ImVec2( std::min( x + scaledLen, previewPos.x + previewWidth ), previewPos.y + previewHeight - 2.0f ),
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

	// Canvas controls
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Canvas Controls" );
	ImGui::Separator();

	if( ImGui::Button( "Fit All", ImVec2( -1, 0 ) ) ) {
		mCanvas.fitAll();
	}
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Fit all content in view (F key)" );
	}

	if( ImGui::Button( "Reset View", ImVec2( -1, 0 ) ) ) {
        mCanvas.viewOnetoOne();
	}
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Reset zoom to 100%% and center (Home key)" );
	}

	ImGui::Text( "Zoom: %.0f%%", mCanvas.getZoom() * 100.0f );
	ImGui::BulletText( "Mouse drag to pan" );
	ImGui::BulletText( "Mouse wheel to zoom" );

	ImGui::End();
}

void BezierOffsetApp::draw()
{
	gl::clear( Color( 0.05f, 0.05f, 0.1f ) );

	gl::enableAlphaBlending();

	// Apply canvas transform for pan/zoom
	gl::ScopedModelMatrix scpMatrix( mCanvas.getModelMatrix() );

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

	// Draw multiple offset curves (only in offset mode)
	if( mShowOffset && !mPath.empty() && mMode == Mode::OFFSET ) {
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

	// Draw stroke result (only in stroke mode)
	if( mShowOffset && mMode == Mode::STROKE ) {
		// Draw dashed stroke (Shape2d with multiple contours)
		if( !mStrokedShape.empty() ) {
			gl::color( mOffsetColor );
			gl::lineWidth( 2.0f );

			// Draw each contour (dash segment)
			for( size_t i = 0; i < mStrokedShape.getNumContours(); ++i ) {
				const Path2d& contour = mStrokedShape.getContour( i );
				gl::draw( contour );

				// Draw control points
				if( mShowControlPoints ) {
					gl::color( ColorA( mOffsetColor, 0.5f ) );
					for( size_t p = 0; p < contour.getNumPoints(); ++p )
						gl::drawSolidCircle( contour.getPoint( p ), 2.0f );
					gl::color( mOffsetColor );
				}
			}

			gl::lineWidth( 1.0f );
		}
		// Draw non-dashed stroke (single Path2d)
		else if( !mOffsetPath.empty() ) {
			gl::color( mOffsetColor );
			gl::lineWidth( 2.0f );
			gl::draw( mOffsetPath );
			gl::lineWidth( 1.0f );

			// Draw control points
			if( mShowControlPoints ) {
				gl::color( ColorA( mOffsetColor, 0.5f ) );
				for( size_t p = 0; p < mOffsetPath.getNumPoints(); ++p )
					gl::drawSolidCircle( mOffsetPath.getPoint( p ), 2.0f );
			}
		}
	}

	// Draw the original curve
	if( mShowOriginal && !mPath.empty() ) {
		gl::color( mOriginalColor );
		gl::lineWidth( 2.0f );
		gl::draw( mPath );
		gl::lineWidth( 1.0f );

		// Draw control points
		if( mShowControlPoints ) {
			for( size_t p = 0; p < mPath.getNumPoints(); ++p ) {
				bool isHovered = ( (int)p == mHoveredPoint );

				// Highlight hovered point
				if( isHovered ) {
					// Draw larger white outline
					gl::color( Color( 1, 1, 1 ) );
					gl::drawSolidCircle( mPath.getPoint( p ), 6.0f );
					// Draw inner colored circle
					gl::color( Color( 0.2f, 1.0f, 0.2f ) );
					gl::drawSolidCircle( mPath.getPoint( p ), 4.5f );
				}
				else {
					// Normal yellow control point
					gl::color( Color( 1, 1, 0 ) );
					gl::drawSolidCircle( mPath.getPoint( p ), 3.5f );
				}
			}
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
