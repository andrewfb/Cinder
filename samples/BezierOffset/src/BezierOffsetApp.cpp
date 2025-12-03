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
#include "cinder/CanvasUi.h"
#include "cinder/CinderImGui.h"
#include "cinder/Font.h"

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
	SHARP_ZIGZAG,
	GLYPH_A,
	GLYPH_C
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
	void drawContent();
	void drawIntersections();
	void findAllIntersections();

	CanvasUi    mCanvas;

	// Multi-path support
	struct PathEntry {
		Path2d path;
		Color color;
		bool visible = true;
	};
	std::vector<PathEntry>	mPaths;
	int						mActivePathIndex = 0;

	Path2d&     getActivePath() { return mPaths[mActivePathIndex].path; }
	const Path2d& getActivePath() const { return mPaths[mActivePathIndex].path; }

	Shape2d     mInputShape;   // For multi-contour inputs like glyphs
	bool        mUseShape = false;
	Shape2d     mResult;
	int         mTrackedPoint;
	int         mHoveredPoint = -1;
	int         mHoveredPathIndex = -1;  // Which path is being hovered (-1 = none)
	bool        mHoveringPath = false;   // True if hovering over any path stroke
	bool        mDraggingPoint = false;
	bool        mDraggingPath = false;
	vec2        mDragStartPos;

	// Path-to-path intersections
	std::vector<Path2d::Intersection>	mPathIntersections;
	bool								mShowPathIntersections = true;

	// Mode selection
	enum class Mode { OFFSET, STROKE };
	Mode        mMode = Mode::STROKE;

	// Offset parameters
	float       mOffsetDistance = 20.0f;
	float       mTolerance = 0.25f;
	int         mNumOffsetCurves = 1;
	bool        mRemoveSelfIntersections = false;

	// Stroke parameters
	float       mStrokeWidth = 40.0f;
	int         mJoinStyle = 2;      // 0=Bevel, 1=Miter, 2=Round
	float       mMiterLimit = 4.0f;
	int         mStartCapStyle = 2;  // 0=Butt, 1=Square, 2=Round
	int         mEndCapStyle = 2;    // 0=Butt, 1=Square, 2=Round

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
	bool        mShowOriginalControlPoints = true;
	bool        mShowResultControlPoints = false;
	Color       mOriginalColor = Color( 1.0f, 0.5f, 0.25f );
	Color       mResultColor = Color( 0.25f, 0.6f, 0.9f );
};

void BezierOffsetApp::setup()
{
	ImGui::Initialize();

	// Setup canvas for pan/zoom
	mCanvas.connect( getWindow() );
	mCanvas.setZoomLimits( 0.1f, 10.0f );

	// Initialize with first path
	mPaths.push_back( { Path2d(), Color( 1.0f, 0.5f, 0.25f ), true } );

	// Start with an S-curve
	loadPresetShape( PresetShape::OPEN_S_CURVE );
}

void BezierOffsetApp::mouseDown( MouseEvent event )
{
	if( event.isLeftDown() && !event.isAltDown() && !ImGui::GetIO().WantCaptureMouse ) {
		vec2 pos = mCanvas.toContent( event.getPos() );

		if( mHoveredPoint >= 0 ) {
			mTrackedPoint = mHoveredPoint;
			mDraggingPoint = true;
			return;
		}

		// Click on a path/shape to select it and start dragging
		if( mHoveringPath && mHoveredPathIndex >= 0 ) {
			// For shapes, just start dragging (no path selection needed)
			if( !mUseShape && mHoveredPathIndex != mActivePathIndex ) {
				mActivePathIndex = mHoveredPathIndex;
				mHoveredPoint = -1;
				mTrackedPoint = -1;
				updateResult();
			}
			mDraggingPath = true;
			mDragStartPos = pos;
			return;
		}

		Path2d& path = getActivePath();
		if( path.empty() ) {
			path.moveTo( pos );
			mTrackedPoint = 0;
		}
		else {
			path.lineTo( pos );
		}
		updateResult();
	}
}

void BezierOffsetApp::mouseDrag( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse )
		return;

	// Let CanvasUi handle pan if Alt is held
	if( event.isAltDown() )
		return;

	vec2 pos = mCanvas.toContent( event.getPos() );
	Path2d& path = getActivePath();

	if( mDraggingPoint && mTrackedPoint >= 0 ) {
		path.setPoint( mTrackedPoint, pos );
		updateResult();
		return;
	}

	// Handle path/shape dragging - translate all points
	if( mDraggingPath ) {
		vec2 delta = pos - mDragStartPos;
		if( mUseShape ) {
			mInputShape.translate( delta );
		}
		else {
			for( size_t i = 0; i < path.getNumPoints(); ++i ) {
				path.setPoint( i, path.getPoint( i ) + delta );
			}
		}
		mDragStartPos = pos;
		updateResult();
		return;
	}

	if( mTrackedPoint >= 0 ) {
		path.setPoint( mTrackedPoint, pos );
	}
	else if( path.getNumSegments() > 0 ) {
		vec2 endPt = path.getPoint( path.getNumPoints() - 1 );
		path.removeSegment( path.getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( path.getNumSegments() == 0 )
			? Path2d::MOVETO
			: path.getSegmentType( path.getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			path.quadTo( pos, endPt );
		}
		else {
			vec2 tan1;
			if( prevType == Path2d::CUBICTO ) {
				vec2 prevDelta = path.getPoint( path.getNumPoints() - 2 ) - path.getPoint( path.getNumPoints() - 1 );
				tan1 = path.getPoint( path.getNumPoints() - 1 ) - prevDelta;
			}
			else if( prevType == Path2d::QUADTO ) {
				vec2 quadTangent = path.getPoint( path.getNumPoints() - 2 );
				vec2 quadEnd = path.getPoint( path.getNumPoints() - 1 );
				vec2 prevDelta = ( quadTangent + ( quadEnd - quadTangent ) / 3.0f ) - quadEnd;
				tan1 = quadEnd - prevDelta;
			}
			else {
				tan1 = path.getPoint( path.getNumPoints() - 1 );
			}
			path.curveTo( tan1, pos, endPt );
		}
		mTrackedPoint = (int)path.getNumPoints() - 2;
	}
	updateResult();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
	mDraggingPoint = false;
	mDraggingPath = false;
}

void BezierOffsetApp::mouseMove( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse ) {
		mHoveredPoint = -1;
		mHoveringPath = false;
		mHoveredPathIndex = -1;
		return;
	}

	vec2 pos = mCanvas.toContent( event.getPos() );
	// Scale hover radius by zoom so it feels consistent at different zoom levels
	float hoverRadius = 12.0f / mCanvas.getZoom();

	// First check for control point hover on active path (skip for shapes - no point editing)
	float closestDist = hoverRadius;
	int closestPoint = -1;
	if( !mUseShape ) {
		const Path2d& activePath = getActivePath();
		for( size_t i = 0; i < activePath.getNumPoints(); ++i ) {
			float dist = glm::distance( pos, activePath.getPoint( i ) );
			if( dist < closestDist ) {
				closestDist = dist;
				closestPoint = (int)i;
			}
		}
	}
	mHoveredPoint = closestPoint;

	// Check if hovering over any path stroke (when not near a control point)
	mHoveringPath = false;
	mHoveredPathIndex = -1;
	if( mHoveredPoint < 0 ) {
		float closestPathDist = hoverRadius;

		// If using a shape, check its contours
		if( mUseShape && !mInputShape.empty() ) {
			float shapeDist = mInputShape.calcDistance( pos );
			if( shapeDist < closestPathDist ) {
				closestPathDist = shapeDist;
				mHoveredPathIndex = 0;  // Treat shape as path index 0
				mHoveringPath = true;
			}
		}
		else {
			for( int pi = 0; pi < (int)mPaths.size(); ++pi ) {
				const PathEntry& entry = mPaths[pi];
				if( !entry.visible || entry.path.empty() || entry.path.getNumSegments() == 0 )
					continue;

				float pathDist = entry.path.calcDistance( pos );
				if( pathDist < closestPathDist ) {
					closestPathDist = pathDist;
					mHoveredPathIndex = pi;
					mHoveringPath = true;
				}
			}
		}
	}
}

void BezierOffsetApp::keyDown( KeyEvent event )
{
	Path2d& path = getActivePath();
	switch( event.getChar() ) {
		case 'x':
		case 'X':
			path.clear();
			mResult.clear();
			findAllIntersections();
			break;
		case 'c':
		case 'C':
			if( !path.empty() && !path.isClosed() ) {
				path.close();
				updateResult();
			}
			break;
		case 'd':
		case 'D':
			// Dump input and output to console as JSON for comparison
			{
				auto dumpPath = []( const Path2d& path, const string& label ) {
					cout << label << endl;
					const auto& pts = path.getPoints();
					size_t ptIdx = 0;
					cout << "[";
					for( size_t i = 0; i < path.getNumSegments(); ++i ) {
						if( i > 0 ) cout << ",";
						auto type = path.getSegmentType( i );
						if( type == Path2d::MOVETO ) {
							cout << "\n  {\"type\":\"M\",\"p\":[" << pts[ptIdx].x << "," << pts[ptIdx].y << "]}";
							ptIdx++;
						} else if( type == Path2d::LINETO ) {
							cout << "\n  {\"type\":\"L\",\"p\":[" << pts[ptIdx].x << "," << pts[ptIdx].y << "]}";
							ptIdx++;
						} else if( type == Path2d::QUADTO ) {
							cout << "\n  {\"type\":\"Q\",\"c\":[" << pts[ptIdx].x << "," << pts[ptIdx].y
								 << "],\"p\":[" << pts[ptIdx+1].x << "," << pts[ptIdx+1].y << "]}";
							ptIdx += 2;
						} else if( type == Path2d::CUBICTO ) {
							cout << "\n  {\"type\":\"C\",\"c1\":[" << pts[ptIdx].x << "," << pts[ptIdx].y
								 << "],\"c2\":[" << pts[ptIdx+1].x << "," << pts[ptIdx+1].y
								 << "],\"p\":[" << pts[ptIdx+2].x << "," << pts[ptIdx+2].y << "]}";
							ptIdx += 3;
						} else if( type == Path2d::CLOSE ) {
							cout << ",\n  {\"type\":\"Z\"}";
						}
					}
					if( path.isClosed() && (path.getNumSegments() == 0 || path.getSegmentType( path.getNumSegments()-1 ) != Path2d::CLOSE) ) {
						cout << ",\n  {\"type\":\"Z\"}";
					}
					cout << "\n]" << endl;
				};

				dumpPath( getActivePath(), "=== INPUT PATH ===" );

				cout << "\n=== OFFSET PARAMS ===" << endl;
				float dist = (mMode == Mode::OFFSET) ? mOffsetDistance : mStrokeWidth / 2.0f;
				cout << "{\"distance\":" << dist << ",\"tolerance\":" << mTolerance << "}" << endl;

				cout << "\n=== OUTPUT SHAPE (" << mResult.getNumContours() << " contours) ===" << endl;
				for( size_t c = 0; c < mResult.getNumContours(); ++c ) {
					dumpPath( mResult.getContour( c ), "Contour " + to_string( c ) + ":" );
				}
			}
			break;
		case 'i':
		case 'I':
			// Dump self-intersection debug info
			{
				cout << "\n========== SELF-INTERSECTION DEBUG ==========" << endl;

				// Get the offset result WITHOUT removeSelfIntersections
				StrokeJoin joinStyle;
				switch( mJoinStyle ) {
					case 0: joinStyle = StrokeJoin::Bevel; break;
					case 1: joinStyle = StrokeJoin::Miter; break;
					default: joinStyle = StrokeJoin::Round; break;
				}

				Shape2d rawOffset = mUseShape
					? offset( mInputShape, mOffsetDistance, joinStyle, mMiterLimit, mTolerance, false )
					: offset( getActivePath(), mOffsetDistance, joinStyle, mMiterLimit, mTolerance, false );

				cout << "Raw offset has " << rawOffset.getNumContours() << " contour(s)" << endl;

				for( size_t c = 0; c < rawOffset.getNumContours(); ++c ) {
					const Path2d& contour = rawOffset.getContour( c );
					cout << "\n--- Contour " << c << " ---" << endl;
					cout << "  Segments: " << contour.getNumSegments() << ", Points: " << contour.getNumPoints() << endl;
					cout << "  isClosed: " << (contour.isClosed() ? "true" : "false") << endl;

					// Find self-intersections
					auto isects = contour.findSelfIntersections();
					cout << "  Self-intersections found: " << isects.size() << endl;

					for( size_t i = 0; i < isects.size(); ++i ) {
						const auto& si = isects[i];
						cout << "    [" << i << "] seg1=" << si.segment1 << " seg2=" << si.segment2
						     << " t1=" << si.t1 << " t2=" << si.t2
						     << " point=(" << si.point.x << "," << si.point.y << ")" << endl;
					}

					// Show what removeSelfIntersections produces
					Shape2d cleanedShape = contour.removeSelfIntersections();
					cout << "  After removeSelfIntersections: " << cleanedShape.getNumContours() << " contours" << endl;
					for( size_t c = 0; c < cleanedShape.getNumContours(); ++c ) {
						const auto& cleaned = cleanedShape.getContour( c );
						cout << "    Contour " << c << ": " << cleaned.getNumSegments() << " segments, "
						     << cleaned.getNumPoints() << " points" << endl;
					}

					// Dump the raw contour path
					cout << "  Raw contour path:" << endl;
					const auto& pts = contour.getPoints();
					size_t ptIdx = 0;
					for( size_t s = 0; s < contour.getNumSegments(); ++s ) {
						auto type = contour.getSegmentType( s );
						if( type == Path2d::MOVETO ) {
							cout << "    M " << pts[ptIdx].x << "," << pts[ptIdx].y << endl;
							ptIdx++;
						} else if( type == Path2d::LINETO ) {
							cout << "    L " << pts[ptIdx].x << "," << pts[ptIdx].y << endl;
							ptIdx++;
						} else if( type == Path2d::QUADTO ) {
							cout << "    Q " << pts[ptIdx].x << "," << pts[ptIdx].y
							     << " " << pts[ptIdx+1].x << "," << pts[ptIdx+1].y << endl;
							ptIdx += 2;
						} else if( type == Path2d::CUBICTO ) {
							cout << "    C " << pts[ptIdx].x << "," << pts[ptIdx].y
							     << " " << pts[ptIdx+1].x << "," << pts[ptIdx+1].y
							     << " " << pts[ptIdx+2].x << "," << pts[ptIdx+2].y << endl;
							ptIdx += 3;
						} else if( type == Path2d::CLOSE ) {
							cout << "    Z" << endl;
						}
					}
				}
				cout << "==============================================" << endl;
			}
			break;
	}
}

void BezierOffsetApp::loadPresetShape( PresetShape shape )
{
	Path2d& path = getActivePath();
	path.clear();
	mInputShape.clear();
	mUseShape = false;
	vec2 center = getWindowCenter();

	switch( shape ) {
		case PresetShape::OPEN_LINE:
			path.moveTo( center + vec2( -150, 0 ) );
			path.lineTo( center + vec2( 150, 0 ) );
			break;

		case PresetShape::OPEN_CURVE:
			path.moveTo( center + vec2( -150, 0 ) );
			path.curveTo( center + vec2( -50, -100 ), center + vec2( 50, -100 ), center + vec2( 150, 0 ) );
			break;

		case PresetShape::OPEN_S_CURVE:
			path.moveTo( center + vec2( -150, 50 ) );
			path.curveTo( center + vec2( -50, -100 ), center + vec2( 50, 200 ), center + vec2( 150, 50 ) );
			break;

		case PresetShape::CLOSED_RECT:
			path.moveTo( center + vec2( -100, -80 ) );
			path.lineTo( center + vec2( 100, -80 ) );
			path.lineTo( center + vec2( 100, 80 ) );
			path.lineTo( center + vec2( -100, 80 ) );
			path.close();
			break;

		case PresetShape::CLOSED_CIRCLE: {
			float radius = 100.0f;
			float k = 0.5522847498f;
			float kr = k * radius;
			path.moveTo( center + vec2( 0, -radius ) );
			path.curveTo( center + vec2( kr, -radius ), center + vec2( radius, -kr ), center + vec2( radius, 0 ) );
			path.curveTo( center + vec2( radius, kr ), center + vec2( kr, radius ), center + vec2( 0, radius ) );
			path.curveTo( center + vec2( -kr, radius ), center + vec2( -radius, kr ), center + vec2( -radius, 0 ) );
			path.curveTo( center + vec2( -radius, -kr ), center + vec2( -kr, -radius ), center + vec2( 0, -radius ) );
			path.close();
			break;
		}

		case PresetShape::CLOSED_STAR: {
			float outerRadius = 100.0f;
			float innerRadius = 40.0f;
			int points = 5;
			path.moveTo( center + vec2( 0, -outerRadius ) );
			for( int i = 1; i < points * 2; ++i ) {
				float angle = (float)i * (float)M_PI / points - (float)M_PI / 2.0f;
				float r = (i % 2 == 0) ? outerRadius : innerRadius;
				path.lineTo( center + vec2( std::cos( angle ), std::sin( angle ) ) * r );
			}
			path.close();
			break;
		}

		case PresetShape::SHARP_ZIGZAG:
			path.moveTo( center + vec2( -150, -50 ) );
			path.lineTo( center + vec2( -100, 50 ) );
			path.lineTo( center + vec2( -50, -50 ) );
			path.lineTo( center + vec2( 0, 50 ) );
			path.lineTo( center + vec2( 50, -50 ) );
			path.lineTo( center + vec2( 100, 50 ) );
			path.lineTo( center + vec2( 150, -50 ) );
			break;

		case PresetShape::GLYPH_A: {
			Font arial( "Arial", 400.0f );
			auto glyphs = arial.getGlyphs( "a" );
			if( !glyphs.empty() ) {
				mInputShape = arial.getGlyphShape( glyphs[0] );
				// Center the glyph
				Rectf bounds = mInputShape.calcBoundingBox();
				mat3 xform = glm::translate( mat3(), center - bounds.getCenter() );
				mInputShape.transform( xform );
				mUseShape = true;
			}
			break;
		}

		case PresetShape::GLYPH_C: {
			Font arial( "Arial", 400.0f );
			auto glyphs = arial.getGlyphs( "C" );
			if( !glyphs.empty() ) {
				mInputShape = arial.getGlyphShape( glyphs[0] );
				// Center the glyph
				Rectf bounds = mInputShape.calcBoundingBox();
				mat3 xform = glm::translate( mat3(), center - bounds.getCenter() );
				mInputShape.transform( xform );
				mUseShape = true;
			}
			break;
		}
	}

	updateResult();
}

void BezierOffsetApp::updateResult()
{
	const Path2d& path = getActivePath();
	bool hasInput = mUseShape ? !mInputShape.empty() : (!path.empty() && path.getNumSegments() > 0);
	if( !hasInput ) {
		mResult.clear();
		findAllIntersections();
		return;
	}

	StrokeJoin joinStyle;
	switch( mJoinStyle ) {
		case 0: joinStyle = StrokeJoin::Bevel; break;
		case 1: joinStyle = StrokeJoin::Miter; break;
		default: joinStyle = StrokeJoin::Round; break;
	}

	StrokeCap startCap, endCap;
	switch( mStartCapStyle ) {
		case 0: startCap = StrokeCap::Butt; break;
		case 1: startCap = StrokeCap::Square; break;
		default: startCap = StrokeCap::Round; break;
	}
	switch( mEndCapStyle ) {
		case 0: endCap = StrokeCap::Butt; break;
		case 1: endCap = StrokeCap::Square; break;
		default: endCap = StrokeCap::Round; break;
	}

	if( mMode == Mode::OFFSET ) {
		if( mUseShape ) {
			mResult = offset( mInputShape, mOffsetDistance, joinStyle, mMiterLimit, mTolerance, mRemoveSelfIntersections );
		}
		else {
			mResult = offset( path, mOffsetDistance, joinStyle, mMiterLimit, mTolerance, mRemoveSelfIntersections );
		}
	}
	else {
		StrokeStyle style( mStrokeWidth );
		style.withJoin( joinStyle ).withMiterLimit( mMiterLimit );
		style.withStartCap( startCap ).withEndCap( endCap );

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

		if( mUseShape ) {
			mResult = stroke( mInputShape, style, mTolerance );
		}
		else {
			mResult = stroke( path, style, mTolerance );
		}

		// Apply self-intersection removal if enabled
		if( mRemoveSelfIntersections && !mResult.empty() ) {
			Shape2d cleaned;
			for( const auto& contour : mResult.getContours() ) {
				Shape2d contourCleaned = contour.removeSelfIntersections();
				for( const auto& c : contourCleaned.getContours() ) {
					cleaned.appendContour( c );
				}
			}
			mResult = cleaned;
		}
	}

	findAllIntersections();
}

void BezierOffsetApp::drawImGuiControls()
{
	ImGui::Begin( "Bezier Offset Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize );

	// Path Management
	ImGui::TextColored( ImVec4( 0.6f, 1.0f, 0.6f, 1.0f ), "Paths" );
	ImGui::Separator();

	// Show active path info (click on canvas to select)
	string activeLabel = "Path " + to_string( mActivePathIndex + 1 );
	if( mPaths[mActivePathIndex].path.empty() ) activeLabel += " (empty)";
	else activeLabel += " (" + to_string( mPaths[mActivePathIndex].path.getNumSegments() ) + " segs)";
	ImGui::Text( "%s", activeLabel.c_str() );
	ImGui::SameLine();
	ImGui::ColorEdit3( "##pathcolor", &mPaths[mActivePathIndex].color[0], ImGuiColorEditFlags_NoInputs );
	ImGui::SameLine();
	ImGui::Checkbox( "##visible", &mPaths[mActivePathIndex].visible );
	if( ImGui::IsItemHovered() ) {
		ImGui::SetTooltip( "Toggle visibility" );
	}

	// Add/Remove buttons
	if( ImGui::Button( "Add Path" ) ) {
		// Generate a distinct color
		static Color colors[] = {
			Color( 1.0f, 0.5f, 0.25f ),
			Color( 0.25f, 0.6f, 0.9f ),
			Color( 0.9f, 0.3f, 0.6f ),
			Color( 0.4f, 0.9f, 0.4f ),
			Color( 0.9f, 0.9f, 0.3f )
		};
		Color newColor = colors[mPaths.size() % 5];
		mPaths.push_back( { Path2d(), newColor, true } );
		mActivePathIndex = (int)mPaths.size() - 1;
		mHoveredPoint = -1;
		mTrackedPoint = -1;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled( mPaths.size() <= 1 );
	if( ImGui::Button( "Remove Path" ) ) {
		mPaths.erase( mPaths.begin() + mActivePathIndex );
		if( mActivePathIndex >= (int)mPaths.size() ) {
			mActivePathIndex = (int)mPaths.size() - 1;
		}
		mHoveredPoint = -1;
		mTrackedPoint = -1;
		findAllIntersections();
	}
	ImGui::EndDisabled();

	ImGui::Spacing();
	ImGui::Separator();

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

		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Join Style" );
		ImGui::Separator();

		const char* joinStyles[] = { "Bevel", "Miter", "Round" };
		if( ImGui::Combo( "##offsetjoin", &mJoinStyle, joinStyles, 3 ) ) {
			updateResult();
		}

		if( mJoinStyle == 1 ) {
			if( ImGui::SliderFloat( "Miter Limit", &mMiterLimit, 1.0f, 10.0f, "%.1f" ) ) {
				updateResult();
			}
		}

		ImGui::Spacing();
		if( ImGui::Checkbox( "Remove Self-Intersections", &mRemoveSelfIntersections ) ) {
			updateResult();
		}
		if( ImGui::IsItemHovered() ) {
			ImGui::SetTooltip( "Remove loops that occur at sharp corners\nwhen the offset distance exceeds the curve radius" );
		}
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

		// Cap Style - applies to both strokes and each dash segment
		ImGui::Spacing();
		ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Cap Style" );
		ImGui::Separator();

		const char* capStyles[] = { "Butt", "Square", "Round" };
		if( ImGui::Combo( "Start Cap", &mStartCapStyle, capStyles, 3 ) ) {
			updateResult();
		}
		if( ImGui::Combo( "End Cap", &mEndCapStyle, capStyles, 3 ) ) {
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

		ImGui::Spacing();
		if( ImGui::Checkbox( "Remove Self-Intersections", &mRemoveSelfIntersections ) ) {
			updateResult();
		}
		if( ImGui::IsItemHovered() ) {
			ImGui::SetTooltip( "Remove loops that occur at sharp corners\nwhen the stroke width exceeds the curve radius" );
		}
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

	if( ImGui::Button( "Glyph 'a'" ) ) {
		loadPresetShape( PresetShape::GLYPH_A );
	}
	ImGui::SameLine();
	if( ImGui::Button( "Glyph 'C'" ) ) {
		loadPresetShape( PresetShape::GLYPH_C );
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

	ImGui::Checkbox( "Original Control Points", &mShowOriginalControlPoints );
	ImGui::Checkbox( "Result Control Points", &mShowResultControlPoints );

	// Path-to-path intersection toggle
	if( mPaths.size() > 1 ) {
		if( ImGui::Checkbox( "Show Path Intersections", &mShowPathIntersections ) ) {
			findAllIntersections();
		}
		if( !mPathIntersections.empty() ) {
			ImGui::SameLine();
			ImGui::Text( "(%zu)", mPathIntersections.size() );
		}
	}

	// Path info
	const Path2d& path = getActivePath();
	bool hasInput = mUseShape ? !mInputShape.empty() : !path.empty();
	if( hasInput ) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored( ImVec4( 0.2f, 1.0f, 0.2f, 1.0f ), "Path Info" );
		ImGui::Separator();

		if( mUseShape ) {
			size_t totalSegs = 0, totalPts = 0;
			int lineCount = 0, quadCount = 0, cubicCount = 0;
			for( const auto& contour : mInputShape.getContours() ) {
				totalSegs += contour.getNumSegments();
				totalPts += contour.getNumPoints();
				for( size_t i = 0; i < contour.getNumSegments(); ++i ) {
					auto type = contour.getSegmentType( i );
					if( type == Path2d::LINETO ) lineCount++;
					else if( type == Path2d::QUADTO ) quadCount++;
					else if( type == Path2d::CUBICTO ) cubicCount++;
				}
			}
			ImGui::Text( "Original: %zu contours, %zu segments, %zu points",
				mInputShape.getNumContours(), totalSegs, totalPts );
			ImGui::Text( "  Lines: %d, Quads: %d, Cubics: %d", lineCount, quadCount, cubicCount );
		}
		else {
			int lineCount = 0, quadCount = 0, cubicCount = 0;
			for( size_t i = 0; i < path.getNumSegments(); ++i ) {
				auto type = path.getSegmentType( i );
				if( type == Path2d::LINETO ) lineCount++;
				else if( type == Path2d::QUADTO ) quadCount++;
				else if( type == Path2d::CUBICTO ) cubicCount++;
			}

			ImGui::Text( "Original: %zu segments, %zu points", path.getNumSegments(), path.getNumPoints() );
			ImGui::Text( "  Lines: %d, Quads: %d, Cubics: %d", lineCount, quadCount, cubicCount );
			ImGui::Text( "  Closed: %s", path.isClosed() ? "Yes" : "No" );
		}

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
	ImGui::BulletText( "Click path to select & drag" );
	ImGui::BulletText( "Click empty to add points" );
	ImGui::BulletText( "Drag to create curves" );
	ImGui::BulletText( "Drag points to edit" );
	ImGui::BulletText( "'X' to clear, 'C' to close" );
	ImGui::Spacing();
	ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Navigation" );
	ImGui::Separator();
	ImGui::BulletText( "Alt+Drag to pan" );
	ImGui::BulletText( "Scroll wheel to zoom" );

	ImGui::End();
}

void BezierOffsetApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.15f ) );
	gl::enableAlphaBlending();

	// Apply canvas transform and draw content
	{
		gl::ScopedModelMatrix scpMatrix( mCanvas.getModelMatrix() );
		drawContent();
	}

	drawImGuiControls();
}

void BezierOffsetApp::drawContent()
{
	// Scale point sizes by inverse zoom so they appear constant on screen
	float pointScale = 1.0f / mCanvas.getZoom();

	// Draw result
	if( mShowResult && !mResult.empty() ) {
		Shape2d lastOffsetResult;  // Track last curve for control points

		if( mMode == Mode::OFFSET && mNumOffsetCurves > 1 ) {
			// Draw multiple offset curves
			Join joinStyle;
			switch( mJoinStyle ) {
				case 0: joinStyle = Join::Bevel; break;
				case 1: joinStyle = Join::Miter; break;
				default: joinStyle = Join::Round; break;
			}

			for( int i = 0; i < mNumOffsetCurves; ++i ) {
				float t = (float)(i + 1) / (float)mNumOffsetCurves;
				float distance = mOffsetDistance * t;

				Shape2d offsetResult = mUseShape
					? offset( mInputShape, distance, joinStyle, mMiterLimit, mTolerance, mRemoveSelfIntersections )
					: offset( getActivePath(), distance, joinStyle, mMiterLimit, mTolerance, mRemoveSelfIntersections );

				Color gradColor = mOriginalColor * (1.0f - t) + mResultColor * t;
				float alpha = (mNumOffsetCurves > 3) ? (0.3f + 0.7f * t) : 0.7f;

				gl::color( ColorA( gradColor, alpha * 0.5f ) );
				gl::draw( offsetResult );

				gl::color( ColorA( gradColor, alpha ) );
				for( const auto& contour : offsetResult.getContours() ) {
					gl::draw( contour );
				}

				// Keep track of the last (outermost) curve
				if( i == mNumOffsetCurves - 1 ) {
					lastOffsetResult = offsetResult;
				}
			}
		}
		else {
			gl::color( ColorA( mResultColor, 0.5f ) );
			gl::draw( mResult );

			gl::color( ColorA( mResultColor.r * 1.3f, mResultColor.g * 1.3f, mResultColor.b * 1.3f, 0.8f ) );
			for( const auto& contour : mResult.getContours() ) {
				gl::draw( contour );
			}

			lastOffsetResult = mResult;
		}

		// Result control points - green outline circles (drawn on last/outermost curve)
		if( mShowResultControlPoints && !lastOffsetResult.empty() ) {
			gl::color( Color( 0.2f, 0.9f, 0.2f ) );
			for( const auto& contour : lastOffsetResult.getContours() ) {
				for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
					gl::drawStrokedCircle( contour.getPoint( i ), 3.0f * pointScale );
				}
			}
		}
	}

	// Draw original path or shape
	if( mShowOriginal ) {
		if( mUseShape && !mInputShape.empty() ) {
			// Use highlighted color when hovering over shape
			Color drawColor = mOriginalColor;
			if( mHoveringPath || mDraggingPath ) {
				drawColor = Color( 1.0f, 1.0f, 0.4f );  // Yellow tint when hovered/dragging
			}
			gl::color( drawColor );
			gl::lineWidth( 2.0f );
			for( const auto& contour : mInputShape.getContours() ) {
				gl::draw( contour );
			}
			gl::lineWidth( 1.0f );

			// Original control points for shape
			if( mShowOriginalControlPoints ) {
				for( const auto& contour : mInputShape.getContours() ) {
					for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
						gl::color( Color( 1, 1, 0 ) );
						gl::drawSolidCircle( contour.getPoint( i ), 3.5f * pointScale );
					}
				}
			}
		}
		else {
			// Draw all paths
			for( int pi = 0; pi < (int)mPaths.size(); ++pi ) {
				const PathEntry& entry = mPaths[pi];
				if( !entry.visible || entry.path.empty() )
					continue;

				bool isActive = ( pi == mActivePathIndex );
				bool isHovered = ( pi == mHoveredPathIndex ) || ( isActive && mDraggingPath );
				float alpha = isActive ? 1.0f : 0.6f;

				// Change color when hovered (gl::lineWidth > 1.0 doesn't work reliably on macOS)
				Color drawColor = entry.color;
				if( isHovered && !isActive ) {
					// Highlight hovered path with yellow tint
					drawColor = Color( 1.0f, 1.0f, 0.4f );
					alpha = 1.0f;
				}
				else if( isHovered && isActive ) {
					// Brighten active path when dragging
					drawColor = Color(
						glm::min( entry.color.r * 1.5f, 1.0f ),
						glm::min( entry.color.g * 1.5f, 1.0f ),
						glm::min( entry.color.b * 1.0f, 1.0f )
					);
				}

				gl::color( ColorA( drawColor, alpha ) );
				gl::lineWidth( isActive ? 2.0f : 1.5f );
				gl::draw( entry.path );

				// Control points (only for active path)
				if( mShowOriginalControlPoints && isActive ) {
					for( size_t i = 0; i < entry.path.getNumPoints(); ++i ) {
						bool ptHovered = ((int)i == mHoveredPoint);

						if( ptHovered ) {
							gl::color( Color( 1, 1, 1 ) );
							gl::drawSolidCircle( entry.path.getPoint( i ), 6.0f * pointScale );
							gl::color( Color( 0.2f, 1.0f, 0.2f ) );
							gl::drawSolidCircle( entry.path.getPoint( i ), 4.5f * pointScale );
						}
						else {
							gl::color( Color( 1, 1, 0 ) );
							gl::drawSolidCircle( entry.path.getPoint( i ), 3.5f * pointScale );
						}
					}
				}
			}
			gl::lineWidth( 1.0f );
		}
	}

	// Draw path-to-path intersections
	drawIntersections();
}

void BezierOffsetApp::findAllIntersections()
{
	mPathIntersections.clear();

	if( !mShowPathIntersections )
		return;

	// Find intersections between all pairs of paths in mPaths
	for( size_t i = 0; i < mPaths.size(); ++i ) {
		if( mPaths[i].path.empty() || !mPaths[i].visible )
			continue;

		for( size_t j = i + 1; j < mPaths.size(); ++j ) {
			if( mPaths[j].path.empty() || !mPaths[j].visible )
				continue;

			auto isects = mPaths[i].path.findIntersections( mPaths[j].path );
			mPathIntersections.insert( mPathIntersections.end(), isects.begin(), isects.end() );
		}
	}

	// Find intersections between mResult (offset/stroke result) and the source
	if( !mResult.empty() ) {
		if( mUseShape && !mInputShape.empty() ) {
			// Find intersections with all contours of the input shape
			for( size_t ci = 0; ci < mInputShape.getNumContours(); ++ci ) {
				const Path2d& contour = mInputShape.getContour( ci );
				if( contour.empty() )
					continue;
				auto shapeIsects = mResult.findIntersections( contour );
				for( const auto& si : shapeIsects ) {
					mPathIntersections.push_back( { si.t1, si.t2, si.point, si.segment1, si.segment2 } );
				}
			}
		}
		else if( !mPaths.empty() && !mPaths[0].path.empty() && mPaths[0].visible ) {
			auto shapeIsects = mResult.findIntersections( mPaths[0].path );
			for( const auto& si : shapeIsects ) {
				mPathIntersections.push_back( { si.t1, si.t2, si.point, si.segment1, si.segment2 } );
			}
		}
	}
}

void BezierOffsetApp::drawIntersections()
{
	if( !mShowPathIntersections || mPathIntersections.empty() )
		return;

	float pointScale = 1.0f / mCanvas.getZoom();

	for( const auto& isect : mPathIntersections ) {
		// Outer ring (white)
		gl::color( Color( 1, 1, 1 ) );
		gl::drawSolidCircle( isect.point, 7.0f * pointScale );

		// Inner circle (magenta)
		gl::color( Color( 1, 0, 1 ) );
		gl::drawSolidCircle( isect.point, 5.0f * pointScale );
	}
}

CINDER_APP( BezierOffsetApp, RendererGl( RendererGl::Options().msaa( 8 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
	settings->setTitle( "Bezier Offset & Stroke Demo" );
})
