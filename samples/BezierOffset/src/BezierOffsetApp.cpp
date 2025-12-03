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
	EMPTY,
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

// Names for the preset combo box
static const char* sPresetNames[] = {
	"Empty (draw your own)",
	"Line",
	"Curve",
	"S-Curve",
	"Rectangle",
	"Circle",
	"Star",
	"Zigzag",
	"Glyph 'a'",
	"Glyph 'C'"
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
	Shape2d createPresetShape( PresetShape preset );
	void drawContent();
	void drawIntersections();
	void findAllIntersections();

	CanvasUi    mCanvas;

	// Mode for offset/stroke
	enum class Mode { NONE, OFFSET, STROKE };

	// Multi-shape support - each entry is a Shape2d with its own parameters
	struct ShapeEntry {
		Shape2d shape;
		Shape2d result;  // Computed offset/stroke result
		Color color;
		bool visible = true;
		bool editable = true;  // Single-contour paths are editable, glyphs are not

		// Per-shape mode and parameters
		Mode mode = Mode::STROKE;
		float distance = 20.0f;          // Offset distance or stroke width
		int joinStyle = 2;               // 0=Bevel, 1=Miter, 2=Round
		float miterLimit = 4.0f;
		int startCapStyle = 2;           // 0=Butt, 1=Square, 2=Round
		int endCapStyle = 2;             // 0=Butt, 1=Square, 2=Round
		float tolerance = 0.25f;
		bool removeSelfIntersections = false;

		// Multiple offset curves (evenly spaced up to distance)
		int numOffsetCurves = 1;

		// Dashing
		bool enableDashing = false;
		int dashPreset = 0;
		float dashOn = 20.0f;
		float dashOff = 10.0f;
		float dashOn2 = 5.0f;
		float dashOff2 = 10.0f;
		float dashOffset = 0.0f;
	};
	std::vector<ShapeEntry>	mShapes;
	int						mActiveShapeIndex = 0;
	int                     mSelectedPreset = 3;  // Default to S-Curve

	Shape2d&       getActiveShape() { return mShapes[mActiveShapeIndex].shape; }
	const Shape2d& getActiveShape() const { return mShapes[mActiveShapeIndex].shape; }
	bool           isActiveEditable() const { return mShapes[mActiveShapeIndex].editable; }

	// For editable shapes, get the first (and only) contour
	Path2d*        getEditablePath();
	const Path2d*  getEditablePath() const;

	int         mTrackedPoint;
	int         mHoveredPoint = -1;
	int         mHoveredShapeIndex = -1;  // Which shape is being hovered (-1 = none)
	bool        mHoveringShape = false;   // True if hovering over any shape stroke
	bool        mDraggingPoint = false;
	bool        mDraggingShape = false;
	vec2        mDragStartPos;

	// Path-to-path intersections
	std::vector<Path2d::Intersection>	mPathIntersections;
	bool								mShowPathIntersections = true;

	// Visualization options
	bool        mShowOriginal = true;
	bool        mShowResult = true;
	bool        mShowOriginalControlPoints = true;
	bool        mShowResultControlPoints = false;
	Color       mResultColor = Color( 0.25f, 0.6f, 0.9f );
};

Path2d* BezierOffsetApp::getEditablePath()
{
	if( !isActiveEditable() || getActiveShape().empty() )
		return nullptr;
	return &getActiveShape().getContours()[0];
}

const Path2d* BezierOffsetApp::getEditablePath() const
{
	if( !isActiveEditable() || getActiveShape().empty() )
		return nullptr;
	return &getActiveShape().getContours()[0];
}

void BezierOffsetApp::setup()
{
	ImGui::Initialize();

	// Setup canvas for pan/zoom
	mCanvas.connect( getWindow() );
	mCanvas.setZoomLimits( 0.1f, 10.0f );

	// Start with an S-curve
	Shape2d initialShape = createPresetShape( PresetShape::OPEN_S_CURVE );
	ShapeEntry entry;
	entry.shape = initialShape;
	entry.color = Color( 1.0f, 0.5f, 0.25f );
	entry.visible = true;
	entry.editable = true;
	mShapes.push_back( entry );
	updateResult();
}

Shape2d BezierOffsetApp::createPresetShape( PresetShape preset )
{
	Shape2d result;
	vec2 center = getWindowCenter();

	switch( preset ) {
		case PresetShape::EMPTY:
			// Return empty shape - user will draw
			break;

		case PresetShape::OPEN_LINE:
			result.moveTo( center + vec2( -150, 0 ) );
			result.lineTo( center + vec2( 150, 0 ) );
			break;

		case PresetShape::OPEN_CURVE:
			result.moveTo( center + vec2( -150, 0 ) );
			result.curveTo( center + vec2( -50, -100 ), center + vec2( 50, -100 ), center + vec2( 150, 0 ) );
			break;

		case PresetShape::OPEN_S_CURVE:
			result.moveTo( center + vec2( -150, 50 ) );
			result.curveTo( center + vec2( -50, -100 ), center + vec2( 50, 200 ), center + vec2( 150, 50 ) );
			break;

		case PresetShape::CLOSED_RECT:
			result.moveTo( center + vec2( -100, -80 ) );
			result.lineTo( center + vec2( 100, -80 ) );
			result.lineTo( center + vec2( 100, 80 ) );
			result.lineTo( center + vec2( -100, 80 ) );
			result.close();
			break;

		case PresetShape::CLOSED_CIRCLE: {
			float radius = 100.0f;
			float k = 0.5522847498f;
			float kr = k * radius;
			result.moveTo( center + vec2( 0, -radius ) );
			result.curveTo( center + vec2( kr, -radius ), center + vec2( radius, -kr ), center + vec2( radius, 0 ) );
			result.curveTo( center + vec2( radius, kr ), center + vec2( kr, radius ), center + vec2( 0, radius ) );
			result.curveTo( center + vec2( -kr, radius ), center + vec2( -radius, kr ), center + vec2( -radius, 0 ) );
			result.curveTo( center + vec2( -radius, -kr ), center + vec2( -kr, -radius ), center + vec2( 0, -radius ) );
			result.close();
			break;
		}

		case PresetShape::CLOSED_STAR: {
			float outerRadius = 100.0f;
			float innerRadius = 40.0f;
			int points = 5;
			result.moveTo( center + vec2( 0, -outerRadius ) );
			for( int i = 1; i < points * 2; ++i ) {
				float angle = (float)i * (float)M_PI / points - (float)M_PI / 2.0f;
				float r = (i % 2 == 0) ? outerRadius : innerRadius;
				result.lineTo( center + vec2( std::cos( angle ), std::sin( angle ) ) * r );
			}
			result.close();
			break;
		}

		case PresetShape::SHARP_ZIGZAG:
			result.moveTo( center + vec2( -150, -50 ) );
			result.lineTo( center + vec2( -100, 50 ) );
			result.lineTo( center + vec2( -50, -50 ) );
			result.lineTo( center + vec2( 0, 50 ) );
			result.lineTo( center + vec2( 50, -50 ) );
			result.lineTo( center + vec2( 100, 50 ) );
			result.lineTo( center + vec2( 150, -50 ) );
			break;

		case PresetShape::GLYPH_A: {
			Font arial( "Arial", 400.0f );
			auto glyphs = arial.getGlyphs( "a" );
			if( !glyphs.empty() ) {
				result = arial.getGlyphShape( glyphs[0] );
				Rectf bounds = result.calcBoundingBox();
				mat3 xform = glm::translate( mat3(), center - bounds.getCenter() );
				result.transform( xform );
			}
			break;
		}

		case PresetShape::GLYPH_C: {
			Font arial( "Arial", 400.0f );
			auto glyphs = arial.getGlyphs( "C" );
			if( !glyphs.empty() ) {
				result = arial.getGlyphShape( glyphs[0] );
				Rectf bounds = result.calcBoundingBox();
				mat3 xform = glm::translate( mat3(), center - bounds.getCenter() );
				result.transform( xform );
			}
			break;
		}
	}

	return result;
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

		// Click on a shape to select it and start dragging
		if( mHoveringShape && mHoveredShapeIndex >= 0 ) {
			if( mHoveredShapeIndex != mActiveShapeIndex ) {
				mActiveShapeIndex = mHoveredShapeIndex;
				mHoveredPoint = -1;
				mTrackedPoint = -1;
				updateResult();
			}
			mDraggingShape = true;
			mDragStartPos = pos;
			return;
		}

		// Add points only for editable shapes
		Path2d* path = getEditablePath();
		if( path ) {
			if( path->empty() ) {
				path->moveTo( pos );
				mTrackedPoint = 0;
			}
			else {
				path->lineTo( pos );
			}
			updateResult();
		}
		else if( getActiveShape().empty() && isActiveEditable() ) {
			// Empty editable shape - start a new path
			getActiveShape().moveTo( pos );
			mTrackedPoint = 0;
			updateResult();
		}
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

	if( mDraggingPoint && mTrackedPoint >= 0 ) {
		Path2d* path = getEditablePath();
		if( path ) {
			path->setPoint( mTrackedPoint, pos );
			updateResult();
		}
		return;
	}

	// Handle shape dragging - translate all contours
	if( mDraggingShape ) {
		vec2 delta = pos - mDragStartPos;
		getActiveShape().translate( delta );
		mDragStartPos = pos;
		updateResult();
		return;
	}

	// Curve creation while dragging
	Path2d* path = getEditablePath();
	if( !path )
		return;

	if( mTrackedPoint >= 0 ) {
		path->setPoint( mTrackedPoint, pos );
	}
	else if( path->getNumSegments() > 0 ) {
		vec2 endPt = path->getPoint( path->getNumPoints() - 1 );
		path->removeSegment( path->getNumSegments() - 1 );

		Path2d::SegmentType prevType = ( path->getNumSegments() == 0 )
			? Path2d::MOVETO
			: path->getSegmentType( path->getNumSegments() - 1 );

		if( event.isShiftDown() || prevType == Path2d::MOVETO ) {
			path->quadTo( pos, endPt );
		}
		else {
			vec2 tan1;
			if( prevType == Path2d::CUBICTO ) {
				vec2 prevDelta = path->getPoint( path->getNumPoints() - 2 ) - path->getPoint( path->getNumPoints() - 1 );
				tan1 = path->getPoint( path->getNumPoints() - 1 ) - prevDelta;
			}
			else if( prevType == Path2d::QUADTO ) {
				vec2 quadTangent = path->getPoint( path->getNumPoints() - 2 );
				vec2 quadEnd = path->getPoint( path->getNumPoints() - 1 );
				vec2 prevDelta = ( quadTangent + ( quadEnd - quadTangent ) / 3.0f ) - quadEnd;
				tan1 = quadEnd - prevDelta;
			}
			else {
				tan1 = path->getPoint( path->getNumPoints() - 1 );
			}
			path->curveTo( tan1, pos, endPt );
		}
		mTrackedPoint = (int)path->getNumPoints() - 2;
	}
	updateResult();
}

void BezierOffsetApp::mouseUp( MouseEvent event )
{
	mTrackedPoint = -1;
	mDraggingPoint = false;
	mDraggingShape = false;
}

void BezierOffsetApp::mouseMove( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse ) {
		mHoveredPoint = -1;
		mHoveringShape = false;
		mHoveredShapeIndex = -1;
		return;
	}

	vec2 pos = mCanvas.toContent( event.getPos() );
	float hoverRadius = 12.0f / mCanvas.getZoom();

	// First check for control point hover on active editable shape
	float closestDist = hoverRadius;
	int closestPoint = -1;
	const Path2d* editPath = getEditablePath();
	if( editPath ) {
		for( size_t i = 0; i < editPath->getNumPoints(); ++i ) {
			float dist = glm::distance( pos, editPath->getPoint( i ) );
			if( dist < closestDist ) {
				closestDist = dist;
				closestPoint = (int)i;
			}
		}
	}
	mHoveredPoint = closestPoint;

	// Check if hovering over any shape stroke (when not near a control point)
	mHoveringShape = false;
	mHoveredShapeIndex = -1;
	if( mHoveredPoint < 0 ) {
		float closestShapeDist = hoverRadius;

		for( int si = 0; si < (int)mShapes.size(); ++si ) {
			const ShapeEntry& entry = mShapes[si];
			if( !entry.visible || entry.shape.empty() )
				continue;

			float shapeDist = entry.shape.calcDistance( pos );
			if( shapeDist < closestShapeDist ) {
				closestShapeDist = shapeDist;
				mHoveredShapeIndex = si;
				mHoveringShape = true;
			}
		}
	}
}

void BezierOffsetApp::keyDown( KeyEvent event )
{
	Path2d* path = getEditablePath();
	switch( event.getChar() ) {
		case 'x':
		case 'X':
			if( path ) {
				path->clear();
				mShapes[mActiveShapeIndex].result.clear();
				findAllIntersections();
			}
			break;
		case 'c':
		case 'C':
			if( path && !path->empty() && !path->isClosed() ) {
				path->close();
				updateResult();
			}
			break;
		case 'd':
		case 'D':
			// Dump input and output to console as JSON for comparison
			if( path ) {
				auto dumpPath = []( const Path2d& p, const string& label ) {
					cout << label << endl;
					const auto& pts = p.getPoints();
					size_t ptIdx = 0;
					cout << "[";
					for( size_t i = 0; i < p.getNumSegments(); ++i ) {
						if( i > 0 ) cout << ",";
						auto type = p.getSegmentType( i );
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
					if( p.isClosed() && (p.getNumSegments() == 0 || p.getSegmentType( p.getNumSegments()-1 ) != Path2d::CLOSE) ) {
						cout << ",\n  {\"type\":\"Z\"}";
					}
					cout << "\n]" << endl;
				};

				dumpPath( *path, "=== INPUT PATH ===" );

				const ShapeEntry& active = mShapes[mActiveShapeIndex];
				cout << "\n=== OFFSET PARAMS ===" << endl;
				float dist = (active.mode == Mode::OFFSET) ? active.distance : active.distance / 2.0f;
				cout << "{\"distance\":" << dist << ",\"tolerance\":" << active.tolerance << "}" << endl;

				cout << "\n=== OUTPUT SHAPE (" << active.result.getNumContours() << " contours) ===" << endl;
				for( size_t c = 0; c < active.result.getNumContours(); ++c ) {
					dumpPath( active.result.getContour( c ), "Contour " + to_string( c ) + ":" );
				}
			}
			break;
		case 'i':
		case 'I':
			// Dump self-intersection debug info
			if( path && !mShapes.empty() ) {
				const ShapeEntry& active = mShapes[mActiveShapeIndex];
				cout << "\n========== SELF-INTERSECTION DEBUG ==========" << endl;

				StrokeJoin joinStyle;
				switch( active.joinStyle ) {
					case 0: joinStyle = StrokeJoin::Bevel; break;
					case 1: joinStyle = StrokeJoin::Miter; break;
					default: joinStyle = StrokeJoin::Round; break;
				}

				Shape2d rawOffset = offset( *path, active.distance, joinStyle, active.miterLimit, active.tolerance, false );

				cout << "Raw offset has " << rawOffset.getNumContours() << " contour(s)" << endl;

				for( size_t c = 0; c < rawOffset.getNumContours(); ++c ) {
					const Path2d& contour = rawOffset.getContour( c );
					cout << "\n--- Contour " << c << " ---" << endl;
					cout << "  Segments: " << contour.getNumSegments() << ", Points: " << contour.getNumPoints() << endl;
					cout << "  isClosed: " << (contour.isClosed() ? "true" : "false") << endl;

					auto isects = contour.findSelfIntersections();
					cout << "  Self-intersections found: " << isects.size() << endl;

					for( size_t i = 0; i < isects.size(); ++i ) {
						const auto& si = isects[i];
						cout << "    [" << i << "] seg1=" << si.segment1 << " seg2=" << si.segment2
						     << " t1=" << si.t1 << " t2=" << si.t2
						     << " point=(" << si.point.x << "," << si.point.y << ")" << endl;
					}

					Shape2d cleanedShape = contour.removeSelfIntersections();
					cout << "  After removeSelfIntersections: " << cleanedShape.getNumContours() << " contours" << endl;
				}
				cout << "==============================================" << endl;
			}
			break;
	}
}

void BezierOffsetApp::updateResult()
{
	if( mShapes.empty() ) {
		findAllIntersections();
		return;
	}

	ShapeEntry& entry = mShapes[mActiveShapeIndex];
	const Shape2d& shape = entry.shape;

	if( shape.empty() || entry.mode == Mode::NONE ) {
		entry.result.clear();
		findAllIntersections();
		return;
	}

	// Check if shape has any segments
	bool hasSegments = false;
	for( const auto& contour : shape.getContours() ) {
		if( contour.getNumSegments() > 0 ) {
			hasSegments = true;
			break;
		}
	}
	if( !hasSegments ) {
		entry.result.clear();
		findAllIntersections();
		return;
	}

	StrokeJoin joinStyle;
	switch( entry.joinStyle ) {
		case 0: joinStyle = StrokeJoin::Bevel; break;
		case 1: joinStyle = StrokeJoin::Miter; break;
		default: joinStyle = StrokeJoin::Round; break;
	}

	StrokeCap startCap, endCap;
	switch( entry.startCapStyle ) {
		case 0: startCap = StrokeCap::Butt; break;
		case 1: startCap = StrokeCap::Square; break;
		default: startCap = StrokeCap::Round; break;
	}
	switch( entry.endCapStyle ) {
		case 0: endCap = StrokeCap::Butt; break;
		case 1: endCap = StrokeCap::Square; break;
		default: endCap = StrokeCap::Round; break;
	}

	if( entry.mode == Mode::OFFSET ) {
		entry.result = offset( shape, entry.distance, joinStyle, entry.miterLimit, entry.tolerance, entry.removeSelfIntersections );
	}
	else {
		StrokeStyle style( entry.distance );
		style.withJoin( joinStyle ).withMiterLimit( entry.miterLimit );
		style.withStartCap( startCap ).withEndCap( endCap );

		if( entry.enableDashing ) {
			vector<float> pattern;
			switch( entry.dashPreset ) {
				case 0: pattern = { entry.dashOn, entry.dashOff }; break;
				case 1: pattern = { entry.dashOn, entry.dashOff }; break;
				case 2: pattern = { entry.dashOn, entry.dashOff, entry.dashOn2, entry.dashOff }; break;
				case 3: pattern = { entry.dashOn, entry.dashOff, entry.dashOn2, entry.dashOff, entry.dashOn2, entry.dashOff }; break;
				case 4: pattern = { entry.dashOn, entry.dashOff, entry.dashOn2, entry.dashOff2 }; break;
			}
			style.withDashes( entry.dashOffset, pattern );
		}

		entry.result = stroke( shape, style, entry.tolerance );

		if( entry.removeSelfIntersections && !entry.result.empty() ) {
			Shape2d cleaned;
			for( const auto& contour : entry.result.getContours() ) {
				Shape2d contourCleaned = contour.removeSelfIntersections();
				for( const auto& c : contourCleaned.getContours() ) {
					cleaned.appendContour( c );
				}
			}
			entry.result = cleaned;
		}
	}

	findAllIntersections();
}

void BezierOffsetApp::drawImGuiControls()
{
	ImGui::Begin( "Bezier Offset Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize );

	// Shape Management
	ImGui::TextColored( ImVec4( 0.6f, 1.0f, 0.6f, 1.0f ), "Shapes" );
	ImGui::Separator();

	// Preset selector
	ImGui::SetNextItemWidth( 160 );
	ImGui::Combo( "Preset", &mSelectedPreset, sPresetNames, IM_ARRAYSIZE( sPresetNames ) );

	// Add button
	if( ImGui::Button( "Add Shape" ) ) {
		PresetShape preset = static_cast<PresetShape>( mSelectedPreset );
		Shape2d newShape = createPresetShape( preset );

		// Determine if editable (single contour, not a glyph)
		bool editable = (preset != PresetShape::GLYPH_A && preset != PresetShape::GLYPH_C);

		// Generate a distinct color
		static Color colors[] = {
			Color( 1.0f, 0.5f, 0.25f ),
			Color( 0.25f, 0.6f, 0.9f ),
			Color( 0.9f, 0.3f, 0.6f ),
			Color( 0.4f, 0.9f, 0.4f ),
			Color( 0.9f, 0.9f, 0.3f )
		};
		Color newColor = colors[mShapes.size() % 5];

		ShapeEntry newEntry;
		newEntry.shape = newShape;
		newEntry.color = newColor;
		newEntry.visible = true;
		newEntry.editable = editable;
		mShapes.push_back( newEntry );
		mActiveShapeIndex = (int)mShapes.size() - 1;
		mHoveredPoint = -1;
		mTrackedPoint = -1;
		updateResult();
	}

	ImGui::Spacing();

	// List of shapes with remove buttons
	for( int i = 0; i < (int)mShapes.size(); ++i ) {
		ImGui::PushID( i );

		bool isActive = (i == mActiveShapeIndex);
		ShapeEntry& entry = mShapes[i];

		// Selectable row
		string label = "Shape " + to_string( i + 1 );
		if( entry.shape.empty() ) label += " (empty)";
		else {
			size_t totalSegs = 0;
			for( const auto& c : entry.shape.getContours() ) totalSegs += c.getNumSegments();
			label += " (" + to_string( totalSegs ) + " segs)";
		}
		if( !entry.editable ) label += " [glyph]";

		if( ImGui::Selectable( label.c_str(), isActive, ImGuiSelectableFlags_None, ImVec2( 140, 0 ) ) ) {
			mActiveShapeIndex = i;
			mHoveredPoint = -1;
			mTrackedPoint = -1;
			updateResult();
		}

		ImGui::SameLine();
		ImGui::ColorEdit3( "##color", &entry.color[0], ImGuiColorEditFlags_NoInputs );

		ImGui::SameLine();
		ImGui::Checkbox( "##visible", &entry.visible );
		if( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Visible" );

		ImGui::SameLine();
		ImGui::BeginDisabled( mShapes.size() <= 1 );
		if( ImGui::SmallButton( "X" ) ) {
			mShapes.erase( mShapes.begin() + i );
			if( mActiveShapeIndex >= (int)mShapes.size() ) {
				mActiveShapeIndex = (int)mShapes.size() - 1;
			}
			else if( mActiveShapeIndex > i ) {
				mActiveShapeIndex--;
			}
			mHoveredPoint = -1;
			mTrackedPoint = -1;
			findAllIntersections();
			ImGui::PopID();
			ImGui::EndDisabled();
			break;  // Exit loop since we modified the vector
		}
		ImGui::EndDisabled();
		if( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Remove" );

		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Separator();

	// Per-shape mode and parameters (for active shape)
	if( !mShapes.empty() ) {
		ShapeEntry& active = mShapes[mActiveShapeIndex];

		ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.2f, 1.0f ), "Active Shape Settings" );
		ImGui::Separator();

		const char* modes[] = { "None", "Offset", "Stroke" };
		int modeIdx = (active.mode == Mode::NONE) ? 0 : (active.mode == Mode::OFFSET) ? 1 : 2;
		if( ImGui::Combo( "Mode", &modeIdx, modes, 3 ) ) {
			active.mode = (modeIdx == 0) ? Mode::NONE : (modeIdx == 1) ? Mode::OFFSET : Mode::STROKE;
			updateResult();
		}

		ImGui::Spacing();

		if( active.mode == Mode::OFFSET ) {
			if( ImGui::SliderFloat( "Distance", &active.distance, -100.0f, 100.0f, "%.1f" ) ) {
				updateResult();
			}

			if( ImGui::SliderFloat( "Tolerance", &active.tolerance, 0.01f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) ) {
				updateResult();
			}

			const char* joinStyles[] = { "Bevel", "Miter", "Round" };
			if( ImGui::Combo( "Join", &active.joinStyle, joinStyles, 3 ) ) {
				updateResult();
			}

			if( active.joinStyle == 1 ) {
				if( ImGui::SliderFloat( "Miter Limit", &active.miterLimit, 1.0f, 10.0f, "%.1f" ) ) {
					updateResult();
				}
			}

			if( ImGui::Checkbox( "Remove Self-Intersections", &active.removeSelfIntersections ) ) {
				updateResult();
			}

			if( ImGui::SliderInt( "Num Curves", &active.numOffsetCurves, 1, 20 ) ) {
				// No updateResult needed - multiple curves drawn on the fly
			}
		}
		else if( active.mode == Mode::STROKE ) {
			if( ImGui::SliderFloat( "Width", &active.distance, 1.0f, 100.0f, "%.1f" ) ) {
				updateResult();
			}

			if( ImGui::SliderFloat( "Tolerance", &active.tolerance, 0.01f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) ) {
				updateResult();
			}

			const char* joinStyles[] = { "Bevel", "Miter", "Round" };
			if( ImGui::Combo( "Join", &active.joinStyle, joinStyles, 3 ) ) {
				updateResult();
			}

			if( active.joinStyle == 1 ) {
				if( ImGui::SliderFloat( "Miter Limit", &active.miterLimit, 1.0f, 10.0f, "%.1f" ) ) {
					updateResult();
				}
			}

			const char* capStyles[] = { "Butt", "Square", "Round" };
			if( ImGui::Combo( "Start Cap", &active.startCapStyle, capStyles, 3 ) ) {
				updateResult();
			}
			if( ImGui::Combo( "End Cap", &active.endCapStyle, capStyles, 3 ) ) {
				updateResult();
			}

			if( ImGui::Checkbox( "Remove Self-Intersections", &active.removeSelfIntersections ) ) {
				updateResult();
			}
		}

		// Dash Pattern (stroke mode only)
		if( active.mode == Mode::STROKE ) {
			ImGui::Spacing();
			ImGui::TextColored( ImVec4( 0.2f, 0.8f, 1.0f, 1.0f ), "Dash Pattern" );
			ImGui::Separator();

			if( ImGui::Checkbox( "Enable Dashing", &active.enableDashing ) ) {
				updateResult();
			}

			ImGui::BeginDisabled( !active.enableDashing );

			const char* dashPresets[] = { "Dashed", "Dotted", "Dash-Dot", "Dash-Dot-Dot", "Custom" };
			if( ImGui::Combo( "Dash Preset", &active.dashPreset, dashPresets, 5 ) ) {
				switch( active.dashPreset ) {
					case 0: active.dashOn = 20.0f; active.dashOff = 10.0f; break;
					case 1: active.dashOn = 2.0f; active.dashOff = 8.0f; break;
					case 2: active.dashOn = 20.0f; active.dashOff = 10.0f; active.dashOn2 = 2.0f; break;
					case 3: active.dashOn = 20.0f; active.dashOff = 8.0f; active.dashOn2 = 2.0f; break;
				}
				updateResult();
			}

			if( active.dashPreset <= 1 ) {
				if( ImGui::SliderFloat( "On", &active.dashOn, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "Off", &active.dashOff, 1.0f, 50.0f ) ) updateResult();
			}
			else if( active.dashPreset <= 3 ) {
				if( ImGui::SliderFloat( "Dash", &active.dashOn, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "Gap", &active.dashOff, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "Dot", &active.dashOn2, 1.0f, 20.0f ) ) updateResult();
			}
			else {
				if( ImGui::SliderFloat( "On 1", &active.dashOn, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "Off 1", &active.dashOff, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "On 2", &active.dashOn2, 1.0f, 50.0f ) ) updateResult();
				if( ImGui::SliderFloat( "Off 2", &active.dashOff2, 1.0f, 50.0f ) ) updateResult();
			}

			if( ImGui::SliderFloat( "Offset", &active.dashOffset, 0.0f, 100.0f ) ) {
				updateResult();
			}

			ImGui::EndDisabled();
		}

	}

	// Visualization
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 1.0f, 0.8f, 0.2f, 1.0f ), "Visualization" );
	ImGui::Separator();

	ImGui::Checkbox( "Show Original", &mShowOriginal );
	ImGui::Checkbox( "Show Result", &mShowResult );
	ImGui::Checkbox( "Original Control Points", &mShowOriginalControlPoints );
	ImGui::Checkbox( "Result Control Points", &mShowResultControlPoints );

	if( mShapes.size() > 1 ) {
		if( ImGui::Checkbox( "Show Shape Intersections", &mShowPathIntersections ) ) {
			findAllIntersections();
		}
		if( !mPathIntersections.empty() ) {
			ImGui::SameLine();
			ImGui::Text( "(%zu)", mPathIntersections.size() );
		}
	}

	// Shape info
	const Shape2d& shape = getActiveShape();
	if( !shape.empty() ) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored( ImVec4( 0.2f, 1.0f, 0.2f, 1.0f ), "Shape Info" );
		ImGui::Separator();

		size_t totalSegs = 0, totalPts = 0;
		int lineCount = 0, quadCount = 0, cubicCount = 0;
		for( const auto& contour : shape.getContours() ) {
			totalSegs += contour.getNumSegments();
			totalPts += contour.getNumPoints();
			for( size_t i = 0; i < contour.getNumSegments(); ++i ) {
				auto type = contour.getSegmentType( i );
				if( type == Path2d::LINETO ) lineCount++;
				else if( type == Path2d::QUADTO ) quadCount++;
				else if( type == Path2d::CUBICTO ) cubicCount++;
			}
		}
		ImGui::Text( "Original: %zu contours, %zu segments", shape.getNumContours(), totalSegs );
		ImGui::Text( "  Lines: %d, Quads: %d, Cubics: %d", lineCount, quadCount, cubicCount );

		ImGui::Spacing();
		const Shape2d& activeResult = mShapes[mActiveShapeIndex].result;
		size_t resultSegs = 0, resultPts = 0;
		int resultLines = 0, resultQuads = 0, resultCubics = 0;
		for( const auto& c : activeResult.getContours() ) {
			resultSegs += c.getNumSegments();
			resultPts += c.getNumPoints();
			for( size_t i = 0; i < c.getNumSegments(); ++i ) {
				auto type = c.getSegmentType( i );
				if( type == Path2d::LINETO ) resultLines++;
				else if( type == Path2d::QUADTO ) resultQuads++;
				else if( type == Path2d::CUBICTO ) resultCubics++;
			}
		}
		ImGui::Text( "Result: %zu contours, %zu segments", activeResult.getNumContours(), resultSegs );
		ImGui::Text( "  Lines: %d, Quads: %d, Cubics: %d", resultLines, resultQuads, resultCubics );
	}

	// Instructions
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Controls" );
	ImGui::Separator();
	ImGui::BulletText( "Click shape to select & drag" );
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

	{
		gl::ScopedModelMatrix scpMatrix( mCanvas.getModelMatrix() );
		drawContent();
	}

	drawImGuiControls();
}

void BezierOffsetApp::drawContent()
{
	float pointScale = 1.0f / mCanvas.getZoom();

	// Draw results for all visible shapes
	if( mShowResult ) {
		for( int si = 0; si < (int)mShapes.size(); ++si ) {
			const ShapeEntry& entry = mShapes[si];
			if( !entry.visible || entry.result.empty() )
				continue;

			bool isActive = (si == mActiveShapeIndex);
			float alpha = isActive ? 0.5f : 0.3f;

			// Draw multiple offset curves if requested (offset mode only)
			if( entry.mode == Mode::OFFSET && entry.numOffsetCurves > 1 ) {
				StrokeJoin joinStyle;
				switch( entry.joinStyle ) {
					case 0: joinStyle = StrokeJoin::Bevel; break;
					case 1: joinStyle = StrokeJoin::Miter; break;
					default: joinStyle = StrokeJoin::Round; break;
				}

				for( int c = 1; c <= entry.numOffsetCurves; ++c ) {
					float dist = entry.distance * (float)c / (float)entry.numOffsetCurves;
					Shape2d curveResult = offset( entry.shape, dist, joinStyle, entry.miterLimit, entry.tolerance, entry.removeSelfIntersections );

					// Blend from original shape color to result color
					float t = (float)c / (float)entry.numOffsetCurves;
					Color blendColor(
						entry.color.r + t * (mResultColor.r - entry.color.r),
						entry.color.g + t * (mResultColor.g - entry.color.g),
						entry.color.b + t * (mResultColor.b - entry.color.b)
					);
					gl::color( blendColor );
					for( const auto& contour : curveResult.getContours() ) {
						gl::draw( contour );
					}
				}
			}
			else {
				// Single result (original behavior)
				gl::color( ColorA( mResultColor, alpha ) );
				gl::draw( entry.result );

				gl::color( ColorA( mResultColor.r * 1.3f, mResultColor.g * 1.3f, mResultColor.b * 1.3f, isActive ? 0.8f : 0.5f ) );
				for( const auto& contour : entry.result.getContours() ) {
					gl::draw( contour );
				}
			}

			// Result control points (only for active shape)
			if( mShowResultControlPoints && isActive ) {
				gl::color( Color( 0.2f, 0.9f, 0.2f ) );
				for( const auto& contour : entry.result.getContours() ) {
					for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
						gl::drawStrokedCircle( contour.getPoint( i ), 3.0f * pointScale );
					}
				}
			}
		}
	}

	// Draw original shapes
	if( mShowOriginal ) {
		for( int si = 0; si < (int)mShapes.size(); ++si ) {
			const ShapeEntry& entry = mShapes[si];
			if( !entry.visible || entry.shape.empty() )
				continue;

			bool isActive = (si == mActiveShapeIndex);
			bool isHovered = (si == mHoveredShapeIndex) || (isActive && mDraggingShape);
			float alpha = isActive ? 1.0f : 0.6f;

			// Determine draw color
			Color drawColor = entry.color;
			if( isHovered && !isActive ) {
				drawColor = Color( 1.0f, 1.0f, 0.4f );  // Yellow for hover
				alpha = 1.0f;
			}
			else if( isHovered && isActive ) {
				drawColor = Color(
					glm::min( entry.color.r * 1.5f, 1.0f ),
					glm::min( entry.color.g * 1.5f, 1.0f ),
					glm::min( entry.color.b * 1.0f, 1.0f )
				);
			}

			gl::color( ColorA( drawColor, alpha ) );
			gl::lineWidth( isActive ? 2.0f : 1.5f );

			for( const auto& contour : entry.shape.getContours() ) {
				gl::draw( contour );
			}

			// Control points (only for active editable shape)
			if( mShowOriginalControlPoints && isActive && entry.editable ) {
				for( const auto& contour : entry.shape.getContours() ) {
					for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
						bool ptHovered = ((int)i == mHoveredPoint);

						if( ptHovered ) {
							gl::color( Color( 1, 1, 1 ) );
							gl::drawSolidCircle( contour.getPoint( i ), 6.0f * pointScale );
							gl::color( Color( 0.2f, 1.0f, 0.2f ) );
							gl::drawSolidCircle( contour.getPoint( i ), 4.5f * pointScale );
						}
						else {
							gl::color( Color( 1, 1, 0 ) );
							gl::drawSolidCircle( contour.getPoint( i ), 3.5f * pointScale );
						}
					}
				}
			}
			// Show control points for non-editable shapes too (but not hoverable)
			else if( mShowOriginalControlPoints && isActive && !entry.editable ) {
				for( const auto& contour : entry.shape.getContours() ) {
					for( size_t i = 0; i < contour.getNumPoints(); ++i ) {
						gl::color( Color( 1, 1, 0 ) );
						gl::drawSolidCircle( contour.getPoint( i ), 3.5f * pointScale );
					}
				}
			}
		}
		gl::lineWidth( 1.0f );
	}

	drawIntersections();
}

void BezierOffsetApp::findAllIntersections()
{
	mPathIntersections.clear();

	if( !mShowPathIntersections )
		return;

	// Find intersections between all pairs of shapes
	for( size_t i = 0; i < mShapes.size(); ++i ) {
		if( mShapes[i].shape.empty() || !mShapes[i].visible )
			continue;

		for( size_t j = i + 1; j < mShapes.size(); ++j ) {
			if( mShapes[j].shape.empty() || !mShapes[j].visible )
				continue;

			auto isects = mShapes[i].shape.findIntersections( mShapes[j].shape );
			for( const auto& si : isects ) {
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
		gl::color( Color( 1, 1, 1 ) );
		gl::drawSolidCircle( isect.point, 7.0f * pointScale );

		gl::color( Color( 1, 0, 1 ) );
		gl::drawSolidCircle( isect.point, 5.0f * pointScale );
	}
}

CINDER_APP( BezierOffsetApp, RendererGl( RendererGl::Options().msaa( 8 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
	settings->setTitle( "Bezier Offset & Stroke Demo" );
})
