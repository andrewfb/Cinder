/*
 VectorGraphicsDemo - Interactive demo framework using Cinder's built-in ImGui
*/

// Toggle between direct-to-window rendering (0) and offscreen FBO rendering (1)
// - Direct to window: Uses MSAA from RendererGl, renders directly to screen
// - Offscreen FBO: Uses atomic mode with analytical AA, blits result to screen
#define USE_OFFSCREEN_FBO 0

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/CanvasUi.h"
#include "cinder/CinderImGui.h"

#include "cinder/vg/CanvasGl.h"
#include "cinder/vg/Paint.h"

#include <set>

#include "Demo.h"

using namespace ci;
using namespace ci::app;

// ============================================================================
// Demo 1: Primitives
// ============================================================================
class PrimitivesDemo : public Demo {
  public:
	std::string getName() const override { return "Primitives"; }
	std::string getDescription() const override { return "Shapes with fills, strokes, caps, joins"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 500, 420 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		mFont = Font( "Arial", 14 );
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		vg::Paint textPaint;
		textPaint.setColor( ColorAf( 0.7f, 0.7f, 0.7f, 1 ) );

		// Row 1: Filled shapes (smaller)
		vg::Paint fill;
		fill.setColor( mFillColor );
		canvas->fillRect( Rectf( 50, 30, 120, 100 ), fill );
		canvas->fillCircle( vec2( 185, 65 ), 35, fill );
		canvas->fillRoundedRect( Rectf( 255, 30, 325, 100 ), mCornerRadius, fill );

		// Row 2: Stroked shapes (smaller)
		vg::Paint stroke;
		stroke.setColor( mStrokeColor ).setStrokeWidth( mStrokeWidth ).setLineCap( mLineCap ).setLineJoin( mLineJoin );
		canvas->strokeRect( Rectf( 50, 130, 120, 200 ), stroke );
		canvas->strokeCircle( vec2( 185, 165 ), 35, stroke );
		canvas->strokeRoundedRect( Rectf( 255, 130, 325, 200 ), mCornerRadius, stroke );

		// Row 3: Cap styles comparison
		float	  capY = 255;
		vg::Paint capDemo;
		capDemo.setColor( ColorAf( 0.5f, 0.8f, 0.5f, 1 ) ).setStrokeWidth( 14 );

		capDemo.setLineCap( vg::LineCap::Butt );
		canvas->drawLine( vec2( 50, capY ), vec2( 130, capY ), capDemo );
		canvas->drawString( "Butt", vec2( 70, capY + 25 ), mFont, textPaint );

		capDemo.setLineCap( vg::LineCap::Round );
		canvas->drawLine( vec2( 180, capY ), vec2( 260, capY ), capDemo );
		canvas->drawString( "Round", vec2( 195, capY + 25 ), mFont, textPaint );

		capDemo.setLineCap( vg::LineCap::Square );
		canvas->drawLine( vec2( 310, capY ), vec2( 390, capY ), capDemo );
		canvas->drawString( "Square", vec2( 322, capY + 25 ), mFont, textPaint );

		// Row 4: Join styles comparison using triangles
		float	  joinY = 320;
		vg::Paint joinDemo;
		joinDemo.setColor( ColorAf( 0.8f, 0.5f, 0.8f, 1 ) ).setStrokeWidth( 10 );

		// Triangle with Miter join
		joinDemo.setLineJoin( vg::LineJoin::Miter );
		Path2d tri1;
		tri1.moveTo( vec2( 50, joinY + 40 ) );
		tri1.lineTo( vec2( 90, joinY ) );
		tri1.lineTo( vec2( 130, joinY + 40 ) );
		tri1.close();
		canvas->strokePath( canvas->createPath( tri1 ), joinDemo );
		canvas->drawString( "Miter", vec2( 68, joinY + 60 ), mFont, textPaint );

		// Triangle with Round join
		joinDemo.setLineJoin( vg::LineJoin::Round );
		Path2d tri2;
		tri2.moveTo( vec2( 180, joinY + 40 ) );
		tri2.lineTo( vec2( 220, joinY ) );
		tri2.lineTo( vec2( 260, joinY + 40 ) );
		tri2.close();
		canvas->strokePath( canvas->createPath( tri2 ), joinDemo );
		canvas->drawString( "Round", vec2( 195, joinY + 60 ), mFont, textPaint );

		// Triangle with Bevel join
		joinDemo.setLineJoin( vg::LineJoin::Bevel );
		Path2d tri3;
		tri3.moveTo( vec2( 310, joinY + 40 ) );
		tri3.lineTo( vec2( 350, joinY ) );
		tri3.lineTo( vec2( 390, joinY + 40 ) );
		tri3.close();
		canvas->strokePath( canvas->createPath( tri3 ), joinDemo );
		canvas->drawString( "Bevel", vec2( 328, joinY + 60 ), mFont, textPaint );
	}

	void drawUI() override
	{
		ImGui::ColorEdit4( "Fill", &mFillColor.r );
		ImGui::ColorEdit4( "Stroke", &mStrokeColor.r );
		ImGui::SliderFloat( "Width", &mStrokeWidth, 1, 20 );
		ImGui::SliderFloat( "Corner", &mCornerRadius, 0, 30 );
	}

  private:
	ColorAf		 mFillColor{ 0.3f, 0.6f, 1.0f, 1.0f };
	ColorAf		 mStrokeColor{ 1.0f, 0.4f, 0.4f, 1.0f };
	float		 mStrokeWidth = 4, mCornerRadius = 12;
	vg::LineCap	 mLineCap = vg::LineCap::Round;
	vg::LineJoin mLineJoin = vg::LineJoin::Round;
	Font		 mFont;
};

// ============================================================================
// Demo 2: Gradients
// ============================================================================
class GradientsDemo : public Demo {
  public:
	std::string getName() const override { return "Gradients"; }
	std::string getDescription() const override { return "Linear and radial gradients with controls"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 500, 400 ); }

	void draw( vg::CanvasGlRef canvas ) override
	{
		// Linear gradient with angle control
		float	  angleRad = mAngle * M_PI / 180.0f;
		vec2	  center1( 125, 100 );
		vec2	  dir( cos( angleRad ), sin( angleRad ) );
		vec2	  p1 = center1 - dir * 75.0f;
		vec2	  p2 = center1 + dir * 75.0f;
		vg::Paint linear;
		linear.setLinearGradient( p1, p2, mColor1, mColor2 ).setFeather( mFeather );
		canvas->fillRect( Rectf( 50, 25, 200, 175 ), linear );

		// Radial gradient with offset center
		vec2	  radialCenter( 350, 100 );
		vec2	  focalOffset( mFocalX, mFocalY );
		float	  circleRadius = 70; // Fixed circle size
		vg::Paint radial;
		radial.setRadialGradient( radialCenter + focalOffset, mGradientRadius, mColor1, mColor2 ).setFeather( mFeather );
		canvas->fillCircle( radialCenter, circleRadius, radial );

		// Multi-stop gradient bar
		std::vector<vg::GradientStop> stops;
		for( int i = 0; i <= mStops; i++ ) {
			float	t = i / (float)mStops;
			ColorAf c = mColor1.lerp( t, mColor2 );
			if( mRainbow ) {
				c = ColorAf( ci::hsvToRgb( vec3( t, mSaturation, 1.0f ) ), mAlpha );
			}
			stops.push_back( { t, c } );
		}
		vg::Paint multi;
		multi.setLinearGradient( vec2( 50, 220 ), vec2( 450, 220 ), stops ).setFeather( mFeather );
		canvas->fillRoundedRect( Rectf( 50, 200, 450, 280 ), 15, multi );

		// Gradient with alpha
		vg::Paint alphaGrad;
		ColorAf	  c1 = mColor1;
		c1.a = mAlpha;
		ColorAf c2 = mColor2;
		c2.a = 0.0f;
		alphaGrad.setLinearGradient( vec2( 50, 310 ), vec2( 450, 310 ), c1, c2 );
		canvas->fillRoundedRect( Rectf( 50, 300, 450, 370 ), 10, alphaGrad );
	}

	void drawUI() override
	{
		ImGui::ColorEdit3( "Color 1", &mColor1.r );
		ImGui::ColorEdit3( "Color 2", &mColor2.r );
		ImGui::Separator();
		ImGui::SliderFloat( "Angle", &mAngle, 0, 360 );
		ImGui::SliderFloat( "Grad Radius", &mGradientRadius, 10, 150 );
		ImGui::SliderFloat( "Focal X", &mFocalX, -50, 50 );
		ImGui::SliderFloat( "Focal Y", &mFocalY, -50, 50 );
		ImGui::Separator();
		ImGui::SliderInt( "Stops", &mStops, 2, 12 );
		ImGui::Checkbox( "Rainbow", &mRainbow );
		if( mRainbow )
			ImGui::SliderFloat( "Saturation", &mSaturation, 0, 1 );
		ImGui::SliderFloat( "Alpha", &mAlpha, 0, 1 );
		ImGui::SliderFloat( "Feather", &mFeather, 0, 30 );
	}

  private:
	ColorAf mColor1{ 1, 0.3f, 0.3f, 1 }, mColor2{ 0.3f, 0.3f, 1, 1 };
	int		mStops = 4;
	float	mGradientRadius = 70, mAngle = 45, mFocalX = 0, mFocalY = 0;
	float	mAlpha = 1.0f, mSaturation = 0.8f, mFeather = 0;
	bool	mRainbow = false;
};

// ============================================================================
// Demo 3: Transforms (Solar System)
// ============================================================================
class TransformsDemo : public Demo {
  public:
	std::string getName() const override { return "Transforms"; }
	std::string getDescription() const override { return "Transform stack - Solar System"; }
	Rectf		getContentBounds() const override { return Rectf( -350, -250, 350, 250 ); }

	void update( double dt ) override { mTime += dt * mSpeed; }

	void draw( vg::CanvasGlRef canvas ) override
	{
		vg::Paint sun;
		sun.setRadialGradient( vec2( 0 ), 50, ColorAf( 1, 1, 0.5f, 1 ), ColorAf( 1, 0.6f, 0, 1 ) );
		canvas->fillCircle( vec2( 0 ), 50, sun );

		for( int i = 0; i < mPlanets; i++ ) {
			float orbit = 80 + i * 45;
			float speed = 1.0f / ( 1 + i * 0.5f );
			float size = 12 - i;

			if( mShowOrbits ) {
				vg::Paint op;
				op.setColor( ColorAf( 0.3f, 0.3f, 0.4f, 0.5f ) ).setStrokeWidth( 1 );
				canvas->strokeCircle( vec2( 0 ), orbit, op );
			}

			canvas->save();
			canvas->rotate( mTime * speed );
			canvas->translate( vec2( orbit, 0 ) );
			ColorAf	  pc( 0.3f + 0.7f * ( i % 3 == 0 ), 0.3f + 0.7f * ( i % 3 == 1 ), 0.3f + 0.7f * ( i % 3 == 2 ), 1 );
			vg::Paint pp;
			pp.setColor( pc );
			canvas->fillCircle( vec2( 0 ), size, pp );
			canvas->restore();
		}
	}

	void drawUI() override
	{
		ImGui::SliderFloat( "Speed", &mSpeed, 0, 3 );
		ImGui::SliderInt( "Planets", &mPlanets, 1, 8 );
		ImGui::Checkbox( "Orbits", &mShowOrbits );
	}

  private:
	float mTime = 0, mSpeed = 1;
	int	  mPlanets = 5;
	bool  mShowOrbits = true;
};

// ============================================================================
// Demo 4: Paths
// ============================================================================
class PathsDemo : public Demo {
  public:
	// Minimal shape definition - just what's needed
	struct Shape {
		vec2											  offset;
		std::vector<vec2>								  points;
		std::function<Path2d( const std::vector<vec2>& )> buildPath;
		ColorAf											  fillColor{ 0, 0, 0, 0 };
		ColorAf											  strokeColor{ 0, 0, 0, 0 };
		float											  strokeWidth = 0;
		std::vector<std::pair<int, int>>				  handleLines; // For bezier control handles
	};

	std::string getName() const override { return "Paths"; }
	std::string getDescription() const override { return "Click to select, drag control points to edit"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 700, 400 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		initializeShapes();
	}

	void initializeShapes()
	{
		mShapes.clear();

		// Star - linear path through alternating radii
		Shape star;
		star.offset = vec2( 80, 80 );
		for( int i = 0; i < 10; i++ ) {
			float a = i * M_PI / 5.0f - M_PI / 2;
			float r = ( i % 2 == 0 ) ? 50.0f : 22.0f;
			star.points.push_back( vec2( cos( a ), sin( a ) ) * r );
		}
		star.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			for( size_t i = 0; i < pts.size(); i++ ) {
				if( i == 0 )
					p.moveTo( pts[i] );
				else
					p.lineTo( pts[i] );
			}
			p.close();
			return p;
		};
		star.fillColor = ColorAf( 1, 0.85f, 0, 1 );
		mShapes.push_back( star );

		// Heart - two cubic beziers
		Shape heart;
		heart.offset = vec2( 270, 70 );
		float scale = 0.7f, cx = 70, cy = 67.5f;
		heart.points = {
			vec2( ( 70 - cx ) * scale, ( 40 - cy ) * scale ),  // 0: top
			vec2( ( 45 - cx ) * scale, ( 0 - cy ) * scale ),   // 1: ctrl
			vec2( ( 0 - cx ) * scale, ( 60 - cy ) * scale ),   // 2: ctrl
			vec2( ( 70 - cx ) * scale, ( 95 - cy ) * scale ),  // 3: bottom
			vec2( ( 140 - cx ) * scale, ( 60 - cy ) * scale ), // 4: ctrl
			vec2( ( 95 - cx ) * scale, ( 0 - cy ) * scale )	   // 5: ctrl
		};
		heart.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			p.moveTo( pts[0] );
			p.curveTo( pts[1], pts[2], pts[3] );
			p.curveTo( pts[4], pts[5], pts[0] );
			p.close();
			return p;
		};
		heart.fillColor = ColorAf( 1, 0.2f, 0.4f, 1 );
		heart.handleLines = { { 0, 1 }, { 3, 2 }, { 0, 5 }, { 3, 4 } };
		mShapes.push_back( heart );

		// Polygon
		Shape poly;
		poly.offset = vec2( 430, 80 );
		for( int i = 0; i < mSides; i++ ) {
			float a = i * M_PI * 2.0f / mSides - M_PI / 2.0f;
			poly.points.push_back( vec2( cos( a ), sin( a ) ) * 50.0f );
		}
		poly.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			for( size_t i = 0; i < pts.size(); i++ ) {
				if( i == 0 )
					p.moveTo( pts[i] );
				else
					p.lineTo( pts[i] );
			}
			p.close();
			return p;
		};
		poly.fillColor = ColorAf( 0.5f, 0.8f, 0.5f, 1 );
		poly.strokeColor = ColorAf( 0.2f, 0.4f, 0.2f, 1 );
		poly.strokeWidth = 4;
		mShapes.push_back( poly );

		// Rounded Rect - 4 corners with quadratic curves
		Shape rrect;
		rrect.offset = vec2( 550, 30 );
		float w = 100, h = 100, r = 15;
		// 8 points: edge endpoints, then corner control points
		// 0-1: top edge, 2-3: right edge, 4-5: bottom edge, 6-7: left edge
		// Corner control points at actual corners
		rrect.points = {
			vec2( r, 0 ), vec2( w - r, 0 ), // 0,1: top edge
			vec2( w, 0 ),					// 2: TR corner control
			vec2( w, r ), vec2( w, h - r ), // 3,4: right edge
			vec2( w, h ),					// 5: BR corner control
			vec2( w - r, h ), vec2( r, h ), // 6,7: bottom edge
			vec2( 0, h ),					// 8: BL corner control
			vec2( 0, h - r ), vec2( 0, r ), // 9,10: left edge
			vec2( 0, 0 )					// 11: TL corner control
		};
		rrect.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			p.moveTo( pts[0] );
			p.lineTo( pts[1] );
			p.quadTo( pts[2], pts[3] ); // TR corner
			p.lineTo( pts[4] );
			p.quadTo( pts[5], pts[6] ); // BR corner
			p.lineTo( pts[7] );
			p.quadTo( pts[8], pts[9] ); // BL corner
			p.lineTo( pts[10] );
			p.quadTo( pts[11], pts[0] ); // TL corner
			p.close();
			return p;
		};
		rrect.fillColor = ColorAf( 0.3f, 0.7f, 0.9f, 0.8f );
		rrect.strokeColor = ColorAf( 0.1f, 0.3f, 0.5f, 1 );
		rrect.strokeWidth = 3;
		// Mark corner controls as intermediates (connected from edge endpoints)
		rrect.handleLines = { { 1, 2 }, { 4, 5 }, { 7, 8 }, { 10, 11 } };
		mShapes.push_back( rrect );

		// Cubic bezier curve
		Shape cubic;
		cubic.offset = vec2( 80, 280 );
		cubic.points = { vec2( 0, 0 ), vec2( 40, -70 ), vec2( 80, 70 ), vec2( 120, 0 ), vec2( 160, -70 ), vec2( 200, 70 ), vec2( 240, 0 ) };
		cubic.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			p.moveTo( pts[0] );
			p.curveTo( pts[1], pts[2], pts[3] );
			p.curveTo( pts[4], pts[5], pts[6] );
			return p;
		};
		cubic.strokeColor = ColorAf( 0.5f, 0.5f, 1, 1 );
		cubic.strokeWidth = 4;
		cubic.handleLines = { { 0, 1 }, { 3, 2 }, { 3, 4 }, { 6, 5 } };
		mShapes.push_back( cubic );

		// Quadratic bezier curve
		Shape quad;
		quad.offset = vec2( 400, 280 );
		quad.points = { vec2( 0, 0 ), vec2( 60, -80 ), vec2( 120, 0 ), vec2( 180, 80 ), vec2( 240, 0 ) };
		quad.buildPath = []( const std::vector<vec2>& pts ) {
			Path2d p;
			p.moveTo( pts[0] );
			p.quadTo( pts[1], pts[2] );
			p.quadTo( pts[3], pts[4] );
			return p;
		};
		quad.strokeColor = ColorAf( 1, 0.5f, 0.5f, 1 );
		quad.strokeWidth = 4;
		quad.handleLines = { { 0, 1 }, { 2, 1 }, { 2, 3 }, { 4, 3 } };
		mShapes.push_back( quad );
	}

	vec2 toContent( vec2 windowPos ) const { return mCanvasUi ? mCanvasUi->toContent( windowPos ) : windowPos; }

	void drawControlPoints( vg::CanvasGlRef canvas, const Shape& shape, ColorAf color )
	{
		// Handle lines first
		vg::Paint linePaint;
		linePaint.setColor( ColorAf( 0.5f, 0.5f, 0.5f, 0.6f ) ).setStrokeWidth( 1 );
		for( const auto& line : shape.handleLines ) {
			canvas->drawLine( shape.points[line.first], shape.points[line.second], linePaint );
		}
		// Build set of "endpoint" indices (points that are connected by handle lines)
		std::set<int> endpoints;
		for( const auto& line : shape.handleLines ) {
			endpoints.insert( line.first );
		}
		// Control points - white for intermediates, colored for endpoints
		for( size_t i = 0; i < shape.points.size(); i++ ) {
			bool	  isEndpoint = shape.handleLines.empty() || endpoints.count( (int)i ) > 0;
			float	  r = isEndpoint ? 5.0f : 4.0f;
			vg::Paint fill;
			fill.setColor( isEndpoint ? color : ColorAf( 1, 1, 1, 0.9f ) );
			canvas->fillCircle( shape.points[i], r, fill );
			vg::Paint outline;
			outline.setColor( ColorAf( 0.2f, 0.2f, 0.3f, 1 ) ).setStrokeWidth( 1.5f );
			canvas->strokeCircle( shape.points[i], r, outline );
		}
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		// Update polygon if sides changed
		if( mShapes.size() > 2 && mShapes[2].points.size() != (size_t)mSides ) {
			mShapes[2].points.clear();
			for( int i = 0; i < mSides; i++ ) {
				float a = i * M_PI * 2.0f / mSides - M_PI / 2.0f;
				mShapes[2].points.push_back( vec2( cos( a ), sin( a ) ) * 50.0f );
			}
		}

		for( int idx = 0; idx < (int)mShapes.size(); idx++ ) {
			const auto& shape = mShapes[idx];
			Path2d		path = shape.buildPath( shape.points );
			bool		isFilled = shape.fillColor.a > 0;

			canvas->save();
			canvas->translate( shape.offset );

			// Hover glow
			if( mHovered == idx ) {
				vg::Paint glow;
				glow.setColor( ColorAf( 1, 1, 1, 0.25f ) ).setFeather( 8 ).setStrokeWidth( isFilled ? 6 : 10 );
				canvas->strokePath( canvas->createPath( path ), glow );
			}

			// Fill
			if( isFilled ) {
				vg::Paint fill;
				fill.setColor( shape.fillColor );
				canvas->fillPath( canvas->createPath( path ), fill );
			}

			// Stroke
			if( shape.strokeColor.a > 0 && shape.strokeWidth > 0 ) {
				vg::Paint stroke;
				stroke.setColor( shape.strokeColor ).setStrokeWidth( shape.strokeWidth );
				canvas->strokePath( canvas->createPath( path ), stroke );
			}

			// Control points when selected
			if( mSelected == idx && ! shape.points.empty() ) {
				ColorAf handleColor = isFilled ? shape.fillColor : shape.strokeColor;
				handleColor.a = 1.0f;
				drawControlPoints( canvas, shape, handleColor );
			}

			canvas->restore();
		}
	}

	int hitTestShapes( vec2 contentPos, float threshold = 15.0f )
	{
		struct Hit {
			int	  idx;
			float dist;
		};
		std::vector<Hit> hits;

		for( int idx = 0; idx < (int)mShapes.size(); idx++ ) {
			const auto& shape = mShapes[idx];
			Path2d		path = shape.buildPath( shape.points );
			vec2		local = contentPos - shape.offset;
			bool		isFilled = shape.fillColor.a > 0;

			if( isFilled && path.contains( local ) )
				return idx;
			float dist = path.calcDistance( local );
			if( dist < threshold )
				hits.push_back( { idx, dist } );
		}

		if( hits.empty() )
			return -1;
		std::sort( hits.begin(), hits.end(), []( const Hit& a, const Hit& b ) { return a.dist < b.dist; } );
		return hits[0].idx;
	}

	int hitTestControlPoints( vec2 contentPos, int shapeIdx, float radius = 12.0f )
	{
		if( shapeIdx < 0 || shapeIdx >= (int)mShapes.size() )
			return -1;
		const auto& shape = mShapes[shapeIdx];
		for( size_t i = 0; i < shape.points.size(); i++ ) {
			if( glm::distance( contentPos, shape.points[i] + shape.offset ) < radius )
				return (int)i;
		}
		return -1;
	}

	bool onMouseMove( ci::app::MouseEvent event ) override
	{
		mHovered = hitTestShapes( toContent( vec2( event.getPos() ) ) );
		return false;
	}

	bool onMouseDown( ci::app::MouseEvent event ) override
	{
		if( ! event.isLeftDown() )
			return false;
		vec2 contentPos = toContent( vec2( event.getPos() ) );

		if( mSelected >= 0 ) {
			int ptIdx = hitTestControlPoints( contentPos, mSelected );
			if( ptIdx >= 0 ) {
				mDragShape = mSelected;
				mDragIndex = ptIdx;
				return true;
			}
		}

		int hit = hitTestShapes( contentPos );
		mSelected = hit;
		return hit >= 0;
	}

	bool onMouseDrag( ci::app::MouseEvent event ) override
	{
		if( mDragIndex < 0 || mDragShape < 0 )
			return false;
		vec2 contentPos = toContent( vec2( event.getPos() ) );
		if( mDragShape < (int)mShapes.size() && mDragIndex < (int)mShapes[mDragShape].points.size() ) {
			mShapes[mDragShape].points[mDragIndex] = contentPos - mShapes[mDragShape].offset;
		}
		return true;
	}

	bool onMouseUp( ci::app::MouseEvent event ) override
	{
		bool wasDragging = mDragIndex >= 0;
		mDragIndex = -1;
		mDragShape = -1;
		return wasDragging;
	}

	void drawUI() override
	{
		ImGui::SliderInt( "Polygon Sides", &mSides, 3, 12 );
		if( ImGui::Button( "Reset All" ) )
			initializeShapes();
	}

  private:
	std::vector<Shape> mShapes;
	int				   mHovered = -1, mSelected = -1, mDragShape = -1, mDragIndex = -1;
	int				   mSides = 6;
};

// ============================================================================
// Demo 5: Feathering
// ============================================================================
class FeatheringDemo : public Demo {
  public:
	std::string getName() const override { return "Feathering"; }
	std::string getDescription() const override { return "Soft edges on shapes"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 550, 300 ); }

	void draw( vg::CanvasGlRef canvas ) override
	{
		// Circles with progressively more red and more feathering
		for( int i = 0; i < 5; i++ ) {
			float	t = i / 4.0f; // 0 to 1
			ColorAf color = mColor;
			color.r = glm::mix( mColor.r, 1.0f, t ); // More red as feathering increases
			color.g = glm::mix( mColor.g, 0.2f, t );
			color.b = glm::mix( mColor.b, 0.2f, t );
			vg::Paint p;
			p.setColor( color ).setFeather( mBase + i * mIncrement );
			canvas->fillCircle( vec2( 60 + i * 100, 100 ), 40, p );
		}

		vg::Paint r;
		r.setColor( ColorAf( 0.3f, 0.8f, 0.4f, mOpacity ) ).setFeather( mFeather );
		canvas->fillRect( Rectf( 50, 180, 200, 280 ), r );

		vg::Paint g;
		g.setLinearGradient( vec2( 250, 180 ), vec2( 400, 280 ), ColorAf( 1, 0.5f, 0.2f, 1 ), ColorAf( 0.2f, 0.5f, 1, 1 ) ).setFeather( mFeather );
		canvas->fillRoundedRect( Rectf( 250, 180, 400, 280 ), 15, g );
	}

	void drawUI() override
	{
		ImGui::SliderFloat( "Feather", &mFeather, 0, 100 );
		ImGui::SliderFloat( "Base", &mBase, 0, 20 );
		ImGui::SliderFloat( "Inc", &mIncrement, 1, 20 );
		ImGui::SliderFloat( "Opacity", &mOpacity, 0, 1 );
		ImGui::ColorEdit4( "Base Color", &mColor.r );
	}

  private:
	float	mFeather = 12, mBase = 0, mIncrement = 8, mOpacity = 1;
	ColorAf mColor{ 0.4f, 0.6f, 1, 1 };
};

// ============================================================================
// Demo 6: Instancing
// ============================================================================
class InstancingDemo : public Demo {
  public:
	std::string getName() const override { return "Instancing"; }
	std::string getDescription() const override { return "Batch drawing many shapes"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 800, 600 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		regenerate();
		rebuildStar();
	}

	void rebuildStar()
	{
		if( ! mCanvas )
			return;
		Path2d star;
		for( int i = 0; i < 10; i++ ) {
			float a = i * M_PI / 5.0f - M_PI / 2;
			float r = ( i % 2 == 0 ) ? 1.0f : 0.4f;
			if( i == 0 )
				star.moveTo( vec2( cos( a ), sin( a ) ) * r * mSize );
			else
				star.lineTo( vec2( cos( a ), sin( a ) ) * r * mSize );
		}
		star.close();
		mStar = mCanvas->createPath( star );
	}

	void regenerate()
	{
		mPositions.clear();
		mTransforms.clear();
		mCircleColors.clear();
		Rand rnd;
		for( int i = 0; i < mCount; i++ ) {
			mPositions.push_back( vec2( rnd.nextFloat( 50, 350 ), rnd.nextFloat( 50, 550 ) ) );
			// Complementary colors using golden angle
			float hue = fmod( i * 0.618033988749895f, 1.0f );
			mCircleColors.push_back( ColorAf( ci::hsvToRgb( vec3( hue, 0.7f, 0.9f ) ), 1.0f ) );

			mat3 x( 1 );
			x[2][0] = rnd.nextFloat( 450, 750 );
			x[2][1] = rnd.nextFloat( 50, 550 );
			float s = rnd.nextFloat( 0.5f, 1.5f );
			x[0][0] = s;
			x[1][1] = s;
			mTransforms.push_back( x );
		}
	}

	void update( double dt ) override
	{
		if( mAnimate ) {
			mTime += dt;
			for( size_t i = 0; i < mTransforms.size(); i++ ) {
				float a = mTime * 0.5f + i * 0.1f;
				float s = 0.8f + 0.3f * sin( mTime + i * 0.2f );
				mTransforms[i][0][0] = s * cos( a );
				mTransforms[i][0][1] = s * sin( a );
				mTransforms[i][1][0] = -s * sin( a );
				mTransforms[i][1][1] = s * cos( a );
			}
		}
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		// Draw circles with individual colors
		for( size_t i = 0; i < mPositions.size(); i++ ) {
			vg::Paint cp;
			cp.setColor( ColorAf( mCircleColors[i].r, mCircleColors[i].g, mCircleColors[i].b, mOpacity ) );
			canvas->fillCircle( mPositions[i], mRadius, cp );
		}

		if( mStar ) {
			vg::Paint sp;
			sp.setColor( ColorAf( mStarColor.r, mStarColor.g, mStarColor.b, mOpacity ) );
			canvas->drawPaths( mTransforms, mStar, sp );
		}

		vg::Paint lp;
		lp.setColor( ColorAf( 0.4f, 0.4f, 0.4f, 1 ) ).setStrokeWidth( 2 );
		canvas->drawLine( vec2( 400, 20 ), vec2( 400, 580 ), lp );
	}

	void drawUI() override
	{
		if( ImGui::SliderInt( "Count", &mCount, 10, 1000 ) )
			regenerate();
		ImGui::SliderFloat( "Radius", &mRadius, 5, 30 );
		if( ImGui::SliderFloat( "Star Size", &mSize, 5, 30 ) )
			rebuildStar();
		ImGui::SliderFloat( "Opacity", &mOpacity, 0.1f, 1 );
		ImGui::ColorEdit3( "Stars", &mStarColor.r );
		ImGui::Checkbox( "Animate", &mAnimate );
	}

  private:
	std::vector<vec2>	 mPositions;
	std::vector<ColorAf> mCircleColors;
	std::vector<mat3>	 mTransforms;
	vg::CachedPathRef	 mStar;
	int					 mCount = 100;
	float				 mRadius = 15, mSize = 15, mOpacity = 0.7f, mTime = 0;
	ColorAf				 mStarColor{ 1, 0.8f, 0.2f, 1 };
	bool				 mAnimate = true;
};

// ============================================================================
// Demo 7: Blend Modes
// ============================================================================
class BlendModesDemo : public Demo {
  public:
	std::string getName() const override { return "Blend Modes"; }
	std::string getDescription() const override { return "All 15 blend modes in action"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 600, 500 ); }

	void setup( vg::CanvasGlRef canvas ) override { mFont = Font( "Arial", 11 ); }

	void update( double dt ) override
	{
		if( mAnimate )
			mTime += dt * mSpeed;
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		const char*	  names[] = { "SrcOver", "Screen", "Overlay", "Darken", "Lighten", "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference", "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity" };
		vg::BlendMode modes[] = { vg::BlendMode::SrcOver, vg::BlendMode::Screen, vg::BlendMode::Overlay, vg::BlendMode::Darken, vg::BlendMode::Lighten, vg::BlendMode::ColorDodge, vg::BlendMode::ColorBurn, vg::BlendMode::HardLight,
			vg::BlendMode::SoftLight, vg::BlendMode::Difference, vg::BlendMode::Exclusion, vg::BlendMode::Multiply, vg::BlendMode::Hue, vg::BlendMode::Saturation, vg::BlendMode::Color, vg::BlendMode::Luminosity };

		vg::Paint textPaint;
		textPaint.setColor( ColorAf( 0.7f, 0.7f, 0.7f, 1 ) );

		int	  cols = 4;
		float cellW = 140, cellH = 115;

		for( int i = 0; i < 16; i++ ) {
			int	  col = i % cols;
			int	  row = i / cols;
			float x = 20 + col * cellW;
			float y = 20 + row * cellH;

			// Background rectangle (base layer)
			vg::Paint bg;
			bg.setColor( mBaseColor );
			canvas->fillRect( Rectf( x, y, x + 100, y + 70 ), bg );

			// Animated circle position
			float circleX = x + 50 + sin( mTime + i * 0.5f ) * 20;
			float circleY = y + 35 + cos( mTime * 0.7f + i * 0.3f ) * 15;

			// Blended circle with radial gradient and feather
			vg::Paint fg;
			fg.setRadialGradient( vec2( circleX, circleY ), 30, mBlendColor, ColorAf( mBlendColor.r * 0.6f, mBlendColor.g * 0.6f, mBlendColor.b * 0.6f, mBlendColor.a ) ).setBlendMode( modes[i] ).setFeather( mFeather );
			canvas->fillCircle( vec2( circleX, circleY ), 30, fg );

			// Label
			canvas->drawString( names[i], vec2( x + 50 - strlen( names[i] ) * 3, y + 80 ), mFont, textPaint );
		}

		// Large interactive demo - positioned to the right of the grid
		float	  demoX = 20 + 4 * 140 + 20, demoY = 20;
		vg::Paint demoBg;
		demoBg.setLinearGradient( vec2( demoX, demoY ), vec2( demoX + 180, demoY + 100 ), mBaseColor, ColorAf( mBaseColor.r * 0.5f, mBaseColor.g * 0.5f, mBaseColor.b * 0.5f, 1 ) );
		canvas->fillRoundedRect( Rectf( demoX, demoY, demoX + 180, demoY + 100 ), 10, demoBg );

		// Yellow-to-orange gradient for the blended circle
		vg::Paint demoFg;
		demoFg
			.setRadialGradient( vec2( demoX + 90, demoY + 50 ), 40, ColorAf( 1.0f, 0.9f, 0.2f, 0.9f ), // bright yellow center
				ColorAf( 1.0f, 0.4f, 0.1f, 0.9f ) )													   // orange edge
			.setBlendMode( modes[mSelectedMode] )
			.setFeather( mFeather );
		canvas->fillCircle( vec2( demoX + 90, demoY + 50 ), 40, demoFg );

		canvas->drawString( names[mSelectedMode], vec2( demoX + 40, demoY + 115 ), mFont, textPaint );
	}

	void drawUI() override
	{
		const char* names[] = { "SrcOver", "Screen", "Overlay", "Darken", "Lighten", "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference", "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity" };
		ImGui::Combo( "Mode", &mSelectedMode, names, 16 );
		ImGui::ColorEdit4( "Base", &mBaseColor.r );
		ImGui::ColorEdit4( "Blend", &mBlendColor.r );
		ImGui::SliderFloat( "Feather", &mFeather, 0, 20 );
		ImGui::Separator();
		ImGui::Checkbox( "Animate", &mAnimate );
		if( mAnimate )
			ImGui::SliderFloat( "Speed", &mSpeed, 0.1f, 3.0f );
	}

  private:
	Font	mFont;
	ColorAf mBaseColor{ 0.2f, 0.5f, 0.9f, 1.0f };
	ColorAf mBlendColor{ 1.0f, 0.4f, 0.2f, 0.8f };
	int		mSelectedMode = 0;
	float	mFeather = 0;
	float	mTime = 0, mSpeed = 1.0f;
	bool	mAnimate = true;
};

// ============================================================================
// Demo 8: Clipping
// ============================================================================
class ClippingDemo : public Demo {
  public:
	std::string getName() const override { return "Clipping"; }
	std::string getDescription() const override { return "Intersecting clips: circle AND triangle"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 500, 400 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		mFont = Font( "Arial", 14 );

		// Create circle path
		Path2d circle;
		circle.arc( vec2( 0 ), mCircleRadius, 0, M_PI * 2 );
		circle.close();
		mCirclePath = canvas->createPath( circle );

		// Create triangle path
		Path2d tri;
		for( int i = 0; i < 3; i++ ) {
			float a = i * M_PI * 2 / 3 - M_PI / 2;
			vec2  pt = vec2( cos( a ), sin( a ) ) * mTriangleRadius;
			if( i == 0 )
				tri.moveTo( pt );
			else
				tri.lineTo( pt );
		}
		tri.close();
		mTrianglePath = canvas->createPath( tri );
	}

	void update( double dt ) override
	{
		if( mAnimate )
			mTime += dt * mSpeed;
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		vec2 center( 250, 200 );

		// Draw shape outlines for reference
		vg::Paint outlinePaint;
		outlinePaint.setColor( ColorAf( 0.5f, 0.5f, 0.6f, 0.4f ) ).setStrokeWidth( 2 );

		// Circle outline
		canvas->save();
		canvas->translate( center );
		canvas->strokePath( mCirclePath, outlinePaint );
		canvas->restore();

		// Triangle outline (offset by 50%)
		vec2 triOffset = vec2( cos( mTime * 0.5f ), sin( mTime * 0.5f ) ) * mTriangleRadius * 0.5f;
		canvas->save();
		canvas->translate( center + triOffset );
		canvas->rotate( mTime * 0.3f );
		canvas->strokePath( mTrianglePath, outlinePaint );
		canvas->restore();

		// ========================================
		// KEY DEMO: Two clipPath calls intersected
		// ========================================
		canvas->save();
		canvas->translate( center );

		// First clip: circle at center
		if( mClipCircle ) {
			canvas->clipPath( mCirclePath );
		}

		// Second clip: triangle offset 50% (clips are intersected!)
		canvas->translate( triOffset );
		canvas->rotate( mTime * 0.3f );
		if( mClipTriangle ) {
			canvas->clipPath( mTrianglePath );
		}

		// Draw a big gradient rect - only the intersection is visible
		vg::Paint gradient;
		gradient.setRadialGradient( vec2( 0 ), 150, ColorAf( 1.0f, 0.9f, 0.2f, 1.0f ), // yellow center
			ColorAf( 1.0f, 0.3f, 0.1f, 1.0f ) );									   // orange edge
		canvas->fillRect( Rectf( -200, -200, 200, 200 ), gradient );

		canvas->restore();

		// Labels
		vg::Paint textPaint;
		textPaint.setColor( ColorAf( 0.8f, 0.8f, 0.8f, 1 ) );
		canvas->drawString( "clipPath(circle)", vec2( 20, 30 ), mFont, textPaint );
		canvas->drawString( "clipPath(triangle)", vec2( 20, 50 ), mFont, textPaint );
		canvas->drawString( "= intersection visible", vec2( 20, 70 ), mFont, textPaint );
	}

	void drawUI() override
	{
#if defined( CINDER_MSW )
		ImGui::TextWrapped( "Use VectorGraphicsDemoD3d12 for full clipping support." );
		ImGui::Separator();
#endif
		ImGui::Checkbox( "Clip Circle", &mClipCircle );
		ImGui::Checkbox( "Clip Triangle", &mClipTriangle );
		ImGui::Separator();
		ImGui::Checkbox( "Animate", &mAnimate );
		if( mAnimate )
			ImGui::SliderFloat( "Speed", &mSpeed, 0.1f, 3.0f );
	}

  private:
	Font			  mFont;
	vg::CachedPathRef mCirclePath, mTrianglePath;
	float			  mCircleRadius = 120;
	float			  mTriangleRadius = 100;
	float			  mTime = 0, mSpeed = 1.0f;
	bool			  mAnimate = true;
	bool			  mClipCircle = true;
	bool			  mClipTriangle = true;
};

// ============================================================================
// Demo 9: Images
// ============================================================================
class ImagesDemo : public Demo {
  public:
	std::string getName() const override { return "Images"; }
	std::string getDescription() const override { return "Image drawing with transforms"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 500, 350 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		Surface8u surf( 64, 64, true );
		auto	  iter = surf.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				bool light = ( ( iter.x() / 8 ) + ( iter.y() / 8 ) ) % 2 == 0;
				iter.r() = light ? 255 : 100;
				iter.g() = light ? 200 : 150;
				iter.b() = light ? 100 : 255;
				iter.a() = 255;
			}
		}
		mImage = canvas->createImage( surf );
	}

	void update( double dt ) override
	{
		if( mAnimate )
			mRot += dt * mSpeed;
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		if( ! mImage )
			return;
		canvas->drawImage( mImage, vec2( 50, 50 ) );
		canvas->drawImage( mImage, Rectf( 50, 150, 50 + 80 * mScale, 150 + 80 * mScale ) );

		canvas->save();
		canvas->translate( vec2( 300, 120 ) );
		canvas->rotate( mRot );
		canvas->translate( vec2( -32, -32 ) );
		canvas->drawImage( mImage, vec2( 0, 0 ) );
		canvas->restore();

		for( int i = 0; i < 4; i++ ) {
			float s = 25 + i * 15;
			canvas->drawImage( mImage, Rectf( 50 + i * 70, 270, 50 + i * 70 + s, 270 + s ) );
		}
	}

	void drawUI() override
	{
		ImGui::SliderFloat( "Scale", &mScale, 0.5f, 3 );
		ImGui::SliderFloat( "Speed", &mSpeed, 0, 5 );
		ImGui::Checkbox( "Animate", &mAnimate );
	}

  private:
	vg::ImageRef mImage;
	float		 mScale = 1, mRot = 0, mSpeed = 1;
	bool		 mAnimate = true;
};

// ============================================================================
// Demo 10: Image Mesh
// ============================================================================
class ImageMeshDemo : public Demo {
  public:
	std::string getName() const override { return "Image Mesh"; }
	std::string getDescription() const override { return "drawImageMesh - arbitrary mesh distortions"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 700, 500 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		// Create checkerboard texture
		Surface8u surf( 64, 64, true );
		auto	  iter = surf.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				bool light = ( ( iter.x() / 8 ) + ( iter.y() / 8 ) ) % 2 == 0;
				iter.r() = light ? 255 : 60;
				iter.g() = light ? 220 : 100;
				iter.b() = light ? 100 : 200;
				iter.a() = 255;
			}
		}
		mImage = canvas->createImage( surf );
		rebuildMesh();
	}

	void rebuildMesh()
	{
		mVertices.clear();
		mUvs.clear();
		mIndices.clear();

		// Create NxN grid of vertices
		int n = mGridSize + 1;
		for( int y = 0; y < n; y++ ) {
			for( int x = 0; x < n; x++ ) {
				float u = x / (float)mGridSize;
				float v = y / (float)mGridSize;
				mUvs.push_back( vec2( u, v ) );
				// Position will be computed in update based on warp
				mVertices.push_back( vec2( mMeshRect.x1 + u * mMeshRect.getWidth(), mMeshRect.y1 + v * mMeshRect.getHeight() ) );
			}
		}

		// Create triangle indices (two triangles per cell)
		for( int y = 0; y < mGridSize; y++ ) {
			for( int x = 0; x < mGridSize; x++ ) {
				uint16_t i = y * n + x;
				// First triangle
				mIndices.push_back( i );
				mIndices.push_back( i + 1 );
				mIndices.push_back( i + n );
				// Second triangle
				mIndices.push_back( i + 1 );
				mIndices.push_back( i + n + 1 );
				mIndices.push_back( i + n );
			}
		}
	}

	void update( double dt ) override
	{
		if( mAnimate )
			mTime += dt * mSpeed;

		// Apply warp to vertices
		int n = mGridSize + 1;
		for( int y = 0; y < n; y++ ) {
			for( int x = 0; x < n; x++ ) {
				float u = x / (float)mGridSize;
				float v = y / (float)mGridSize;

				// Base position
				vec2 pos( mMeshRect.x1 + u * mMeshRect.getWidth(), mMeshRect.y1 + v * mMeshRect.getHeight() );

				// Apply warps based on mode
				if( mWarpMode == 0 ) {
					// Wave warp
					float wave = sin( u * M_PI * mWaveFreq + mTime * 2 ) * mWarpAmount;
					pos.y += wave;
					float wave2 = sin( v * M_PI * mWaveFreq + mTime * 1.5f ) * mWarpAmount * 0.5f;
					pos.x += wave2;
				}
				else if( mWarpMode == 1 ) {
					// Bulge/pinch
					vec2  center = mMeshRect.getCenter();
					vec2  toCenter = pos - center;
					float dist = glm::length( toCenter );
					float maxDist = glm::length( mMeshRect.getSize() ) * 0.5f;
					float t = 1.0f - glm::clamp( dist / maxDist, 0.0f, 1.0f );
					float bulge = t * t * mWarpAmount * ( sin( mTime ) * 0.5f + 0.5f );
					pos += glm::normalize( toCenter + vec2( 0.001f ) ) * bulge;
				}
				else if( mWarpMode == 2 ) {
					// Twist
					vec2  center = mMeshRect.getCenter();
					vec2  toCenter = pos - center;
					float dist = glm::length( toCenter );
					float maxDist = glm::length( mMeshRect.getSize() ) * 0.5f;
					float angle = ( 1.0f - dist / maxDist ) * mWarpAmount * 0.02f * sin( mTime );
					float c = cos( angle ), s = sin( angle );
					pos = center + vec2( toCenter.x * c - toCenter.y * s, toCenter.x * s + toCenter.y * c );
				}
				else if( mWarpMode == 3 ) {
					// Flag wave
					float wave = sin( u * M_PI * 3 + mTime * 3 ) * mWarpAmount * ( 1.0f - u * 0.5f );
					pos.y += wave;
				}

				mVertices[y * n + x] = pos;
			}
		}
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		if( ! mImage )
			return;

		// Draw the mesh
		canvas->drawImageMesh( mImage, mVertices, mUvs, mIndices, mOpacity );

		// Draw grid overlay if enabled
		if( mShowGrid ) {
			vg::Paint gridPaint;
			gridPaint.setColor( ColorAf( 0.3f, 0.8f, 0.3f, 0.5f ) ).setStrokeWidth( 1 );
			int n = mGridSize + 1;
			// Horizontal lines
			for( int y = 0; y < n; y++ ) {
				for( int x = 0; x < mGridSize; x++ ) {
					canvas->drawLine( mVertices[y * n + x], mVertices[y * n + x + 1], gridPaint );
				}
			}
			// Vertical lines
			for( int x = 0; x < n; x++ ) {
				for( int y = 0; y < mGridSize; y++ ) {
					canvas->drawLine( mVertices[y * n + x], mVertices[( y + 1 ) * n + x], gridPaint );
				}
			}
		}

		// Draw control point for interactive vertex (corner handles)
		if( mShowHandles ) {
			vg::Paint handlePaint;
			handlePaint.setColor( ColorAf( 1, 0.5f, 0.2f, 0.8f ) );
			int n = mGridSize + 1;
			// Draw corner handles
			canvas->fillCircle( mVertices[0], 6, handlePaint );				// TL
			canvas->fillCircle( mVertices[mGridSize], 6, handlePaint );		// TR
			canvas->fillCircle( mVertices[mGridSize * n], 6, handlePaint ); // BL
			canvas->fillCircle( mVertices[n * n - 1], 6, handlePaint );		// BR
		}

		// Second example: simple quad distortion
		float				  qx = 450, qy = 50;
		std::vector<vec2>	  quadVerts = { vec2( qx, qy ), vec2( qx + 180, qy + sin( mTime ) * 20 ), vec2( qx + 200, qy + 180 + cos( mTime * 1.3f ) * 30 ), vec2( qx - 20 + sin( mTime * 0.7f ) * 15, qy + 160 ) };
		std::vector<vec2>	  quadUvs = { vec2( 0, 0 ), vec2( 1, 0 ), vec2( 1, 1 ), vec2( 0, 1 ) };
		std::vector<uint16_t> quadIndices = { 0, 1, 2, 0, 2, 3 };
		canvas->drawImageMesh( mImage, quadVerts, quadUvs, quadIndices, mOpacity );

		if( mShowGrid ) {
			vg::Paint outlinePaint;
			outlinePaint.setColor( ColorAf( 0.8f, 0.5f, 0.3f, 0.6f ) ).setStrokeWidth( 1 );
			canvas->drawLine( quadVerts[0], quadVerts[1], outlinePaint );
			canvas->drawLine( quadVerts[1], quadVerts[2], outlinePaint );
			canvas->drawLine( quadVerts[2], quadVerts[3], outlinePaint );
			canvas->drawLine( quadVerts[3], quadVerts[0], outlinePaint );
		}

		// Labels
		vg::Paint textPaint;
		textPaint.setColor( ColorAf( 0.7f, 0.7f, 0.7f, 1 ) );
		Font font( "Arial", 12 );
		canvas->drawString( "Grid mesh with warp", vec2( 80, 280 ), font, textPaint );
		canvas->drawString( "Simple quad", vec2( 500, 260 ), font, textPaint );
	}

	void drawUI() override
	{
		const char* warpNames[] = { "Wave", "Bulge", "Twist", "Flag" };
		ImGui::Combo( "Warp Mode", &mWarpMode, warpNames, 4 );
		ImGui::SliderFloat( "Warp Amount", &mWarpAmount, 0, 50 );
		if( mWarpMode == 0 ) {
			ImGui::SliderFloat( "Wave Freq", &mWaveFreq, 1, 5 );
		}
		ImGui::Separator();
		if( ImGui::SliderInt( "Grid Size", &mGridSize, 2, 20 ) ) {
			rebuildMesh();
		}
		ImGui::SliderFloat( "Opacity", &mOpacity, 0, 1 );
		ImGui::Separator();
		ImGui::Checkbox( "Show Grid", &mShowGrid );
		ImGui::Checkbox( "Show Handles", &mShowHandles );
		ImGui::Checkbox( "Animate", &mAnimate );
		if( mAnimate )
			ImGui::SliderFloat( "Speed", &mSpeed, 0.1f, 3.0f );
	}

  private:
	vg::ImageRef		  mImage;
	std::vector<vec2>	  mVertices;
	std::vector<vec2>	  mUvs;
	std::vector<uint16_t> mIndices;

	Rectf mMeshRect{ 50, 50, 300, 250 };
	int	  mGridSize = 8;
	int	  mWarpMode = 0;
	float mWarpAmount = 20;
	float mWaveFreq = 2;
	float mOpacity = 1.0f;
	float mTime = 0, mSpeed = 1.0f;
	bool  mAnimate = true;
	bool  mShowGrid = true;
	bool  mShowHandles = true;
};

// ============================================================================
// Demo 11: Shadow (Dynamic Lighting Simulation)
// ============================================================================
class ShadowDemo : public Demo {
  public:
	std::string getName() const override { return "Shadow"; }
	std::string getDescription() const override { return "Dynamic lighting simulation with feathered shadows"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 600, 500 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		mCenter = vec2( 300, 250 );
		mLightPos = vec2( 450, 100 ); // Start top-right
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		vec2  center = mCenter;
		float sphereRadius = mSphereRadius;
		vec2  center2 = center + vec2( -120, -60 );
		float radius2 = sphereRadius * 0.5f;

		// Safe normalize helper to avoid NaN when light is at sphere center
		auto safeNorm = []( const vec2& v ) {
			float len = glm::length( v );
			return len > 1e-4f ? v / len : vec2( 0, -1 );
		};

		// Per-sphere light directions - each sphere reacts independently!
		vec2 lightDir1 = safeNorm( mLightPos - center );
		vec2 lightDir2 = safeNorm( mLightPos - center2 );

		float lightDist1 = glm::distance( mLightPos, center );
		float lightDist2 = glm::distance( mLightPos, center2 );
		float normalizedDist1 = glm::clamp( lightDist1 / 400.0f, 0.0f, 1.0f );
		float normalizedDist2 = glm::clamp( lightDist2 / 400.0f, 0.0f, 1.0f );

		// Shadow parameters
		float shadowScale1 = 1.0f + normalizedDist1 * mShadowSpread;
		float shadowScale2 = 1.0f + normalizedDist2 * mShadowSpread;
		float shadowAlpha1 = mShadowOpacity * ( 1.0f - normalizedDist1 * 0.3f );
		float shadowAlpha2 = mShadowOpacity * ( 1.0f - normalizedDist2 * 0.3f ) * 0.8f;

		// Draw ground plane hint
		if( mShowGround ) {
			vg::Paint groundPaint;
			groundPaint.setColor( ColorAf( 0.15f, 0.15f, 0.18f, 1.0f ) );
			canvas->fillRect( Rectf( 50, center.y + sphereRadius + 50, 550, center.y + sphereRadius + 55 ), groundPaint );
		}

		// === PASS 1: Draw ALL shadows first ===

		// Main sphere shadow (uses lightDir1)
		{
			vec2	  shadowOffset = -lightDir1 * mShadowOffset * ( 1.0f + normalizedDist1 * 0.5f );
			float	  shadowRadius = sphereRadius * shadowScale1;
			vg::Paint shadowPaint;
			float	  shadowFeather = mShadowFeather * ( 1.0f + normalizedDist1 * 0.5f );
			shadowPaint.setColor( ColorAf( 0, 0, 0, shadowAlpha1 ) ).setFeather( shadowFeather );
			vec2 shadowRadii( shadowRadius * 1.2f, shadowRadius * 0.4f );
			vec2 shadowPos = center + shadowOffset + vec2( 0, sphereRadius * 0.8f );
			canvas->fillEllipse( shadowPos, shadowRadii, shadowPaint );
		}

		// Second sphere shadow (uses lightDir2)
		if( mShowSecondSphere ) {
			vec2	  shadowOffset2 = -lightDir2 * mShadowOffset * 0.7f * ( 1.0f + normalizedDist2 * 0.5f );
			float	  shadowRadius2 = radius2 * shadowScale2;
			vg::Paint shadowPaint2;
			float	  shadowFeather2 = mShadowFeather * 0.7f * ( 1.0f + normalizedDist2 * 0.5f );
			shadowPaint2.setColor( ColorAf( 0, 0, 0, shadowAlpha2 ) ).setFeather( shadowFeather2 );
			vec2 shadowRadii2( shadowRadius2 * 1.2f, shadowRadius2 * 0.4f );
			vec2 shadowPos2 = center2 + shadowOffset2 + vec2( 0, radius2 * 0.8f );
			canvas->fillEllipse( shadowPos2, shadowRadii2, shadowPaint2 );
		}

		// === PASS 2: Draw ALL spheres with layered shading ===
		// Layer order: base lambert -> AO -> subsurface -> rim -> bounce -> wide spec -> tight spec

		// Helper to draw a sphere with full PBR-like layering
		auto drawSphere = [&]( vec2 sphereCenter, float radius, vec2 lightDir, float normDist, const ColorAf& baseColor ) {
			// 1. Base Lambert gradient
			vec2	highlightCenter = sphereCenter + lightDir * radius * 0.4f;
			ColorAf highlightColor = baseColor;
			highlightColor.r = glm::min( 1.0f, highlightColor.r * 1.5f );
			highlightColor.g = glm::min( 1.0f, highlightColor.g * 1.5f );
			highlightColor.b = glm::min( 1.0f, highlightColor.b * 1.5f );
			ColorAf shadowColor = baseColor;
			shadowColor.r *= 0.25f;
			shadowColor.g *= 0.25f;
			shadowColor.b *= 0.25f;
			vg::Paint basePaint;
			basePaint.setRadialGradient( highlightCenter, radius * 1.8f, highlightColor, shadowColor );
			canvas->fillCircle( sphereCenter, radius, basePaint );

			// 2. Ambient occlusion at base
			if( mShowRim ) {
				vec2	  aoCenter = sphereCenter + vec2( 0, radius * 0.7f );
				vg::Paint ao;
				ao.setRadialGradient( aoCenter, radius * 0.9f, ColorAf( 0, 0, 0, 0.3f ), ColorAf( 0, 0, 0, 0.0f ) ).setFeather( radius * 0.3f );
				canvas->fillEllipse( aoCenter, vec2( radius * 0.9f, radius * 0.5f ), ao );
			}

			// 3. Fresnel rim lighting (glow on edge opposite light)
			if( mShowRim ) {
				vec2	  rimCenter = sphereCenter - lightDir * radius * 0.15f;
				vg::Paint rim;
				rim.setRadialGradient( rimCenter, radius * 1.05f, ColorAf( 1, 1, 1, 0.0f ), ColorAf( 1, 1, 1, 0.2f ) ).setFeather( radius * 0.2f );
				canvas->fillCircle( sphereCenter, radius * 1.0f, rim );
			}

			// 5. Ground bounce (faint up-light from below)
			if( mShowRim ) {
				vec2	  bounceCenter = sphereCenter + vec2( 0, radius * 0.5f );
				vg::Paint bounce;
				bounce.setRadialGradient( bounceCenter, radius * 0.9f, ColorAf( 0.5f, 0.45f, 0.4f, 0.12f ), ColorAf( 0.5f, 0.45f, 0.4f, 0.0f ) );
				canvas->fillCircle( sphereCenter, radius * 0.95f, bounce );
			}

			// 6. Wide specular lobe (soft highlight spread)
			if( mShowSpecular ) {
				vec2	  specularPos = sphereCenter + lightDir * radius * 0.5f;
				vg::Paint specWide;
				specWide.setRadialGradient( specularPos, radius * 0.35f, ColorAf( 1, 1, 1, 0.3f ), ColorAf( 1, 1, 1, 0.0f ) );
				canvas->fillCircle( specularPos, radius * 0.35f, specWide );
			}

			// 7. Tight specular hotspot
			if( mShowSpecular ) {
				vec2	  specularPos = sphereCenter + lightDir * radius * 0.55f;
				float	  specRadius = radius * 0.12f;
				vg::Paint specTight;
				specTight.setRadialGradient( specularPos, specRadius, ColorAf( 1, 1, 1, 0.95f ), ColorAf( 1, 1, 1, 0.0f ) );
				canvas->fillCircle( specularPos, specRadius, specTight );
			}
		};

		// Draw main sphere
		drawSphere( center, sphereRadius, lightDir1, normalizedDist1, mSphereColor );

		// Draw second sphere
		if( mShowSecondSphere ) {
			ColorAf color2( 0.3f, 0.7f, 0.9f, 1.0f );
			drawSphere( center2, radius2, lightDir2, normalizedDist2, color2 );
		}

		// Draw light source indicator
		if( mShowLightIndicator ) {
			vg::Paint lightPaint;
			lightPaint.setRadialGradient( mLightPos, 25, ColorAf( 1, 1, 0.8f, 1.0f ), ColorAf( 1, 0.8f, 0.2f, 0.0f ) );
			canvas->fillCircle( mLightPos, 20, lightPaint );

			// Draw light rays
			vg::Paint rayPaint;
			rayPaint.setColor( ColorAf( 1, 1, 0.5f, 0.3f ) ).setStrokeWidth( 2 );
			for( int i = 0; i < 8; i++ ) {
				float angle = i * M_PI / 4.0f;
				vec2  rayStart = mLightPos + vec2( cos( angle ), sin( angle ) ) * 25.0f;
				vec2  rayEnd = mLightPos + vec2( cos( angle ), sin( angle ) ) * 35.0f;
				canvas->drawLine( rayStart, rayEnd, rayPaint );
			}
		}
	}

	bool onMouseMove( ci::app::MouseEvent event ) override
	{
		if( ! mCanvasUi )
			return false;
		mLightPos = mCanvasUi->toContent( vec2( event.getPos() ) );
		return true;
	}

	bool onMouseDrag( ci::app::MouseEvent event ) override
	{
		if( ! mCanvasUi )
			return false;
		mLightPos = mCanvasUi->toContent( vec2( event.getPos() ) );
		return true;
	}

	void drawUI() override
	{
		ImGui::Text( "Move mouse to control light position" );
		ImGui::Separator();
		ImGui::SliderFloat( "Sphere Radius", &mSphereRadius, 30, 120 );
		ImGui::SliderFloat( "Shadow Offset", &mShadowOffset, 10, 100 );
		ImGui::SliderFloat( "Shadow Feather", &mShadowFeather, 5, 80 );
		ImGui::SliderFloat( "Shadow Opacity", &mShadowOpacity, 0.1f, 1.0f );
		ImGui::SliderFloat( "Shadow Spread", &mShadowSpread, 0.0f, 1.0f );
		ImGui::Separator();
		ImGui::ColorEdit3( "Sphere Color", &mSphereColor.r );
		ImGui::Separator();
		ImGui::Checkbox( "Show Specular", &mShowSpecular );
		ImGui::Checkbox( "Show Rim/AO", &mShowRim );
		ImGui::Checkbox( "Show Light Indicator", &mShowLightIndicator );
		ImGui::Checkbox( "Show Ground", &mShowGround );
		ImGui::Checkbox( "Show Second Sphere", &mShowSecondSphere );
	}

  private:
	vec2	mCenter;
	vec2	mLightPos;
	float	mSphereRadius = 80;
	float	mShadowOffset = 40;
	float	mShadowFeather = 30;
	float	mShadowOpacity = 0.6f;
	float	mShadowSpread = 0.3f;
	ColorAf mSphereColor{ 0.9f, 0.4f, 0.3f, 1.0f };
	bool	mShowSpecular = true;
	bool	mShowRim = true;
	bool	mShowLightIndicator = true;
	bool	mShowGround = true;
	bool	mShowSecondSphere = true;
};

// ============================================================================
// Demo 12: DisplayList
// ============================================================================
class DisplayListDemo : public Demo {
  public:
	std::string getName() const override { return "DisplayList"; }
	std::string getDescription() const override { return "Record and replay drawing commands"; }
	Rectf		getContentBounds() const override { return Rectf( 0, 0, 600, 500 ); }

	void setup( vg::CanvasGlRef canvas ) override
	{
		mCanvas = canvas;
		mFont = Font( "Arial", 14 );

		// Create some cached paths for the demo
		Path2d star;
		for( int i = 0; i < 10; i++ ) {
			float angle = (float)( i * M_PI / 5.0f - M_PI / 2.0f );
			float radius = ( i % 2 == 0 ) ? 40.0f : 18.0f;
			vec2  pt = vec2( cos( angle ), sin( angle ) ) * radius;
			if( i == 0 )
				star.moveTo( pt );
			else
				star.lineTo( pt );
		}
		star.close();
		mStarPath = canvas->createPath( star );

		// Build initial display list
		rebuildDisplayList();
	}

	void rebuildDisplayList()
	{
		if( ! mCanvas )
			return;

		mDisplayList = mCanvas->createDisplayList();
		if( ! mDisplayList )
			return;

		mDisplayList->beginRecording( mCanvas.get() );

		// Draw a grid of shapes
		vg::Paint fill, stroke;

		// Row 1: Colored circles
		for( int i = 0; i < 5; i++ ) {
			fill.setColor( ColorAf( 0.2f + i * 0.15f, 0.5f, 1.0f - i * 0.15f, 0.9f ) );
			mCanvas->fillCircle( vec2( 60 + i * 110, 60 ), 40, fill );
		}

		// Row 2: Stroked rectangles
		stroke.setColor( ColorAf( 1.0f, 0.6f, 0.2f, 1.0f ) ).setStrokeWidth( 3 );
		for( int i = 0; i < 5; i++ ) {
			mCanvas->strokeRect( Rectf( 20 + i * 110, 120, 100 + i * 110, 200 ), stroke );
		}

		// Row 3: Gradient-filled rounded rects
		for( int i = 0; i < 5; i++ ) {
			vg::Paint grad;
			grad.setLinearGradient( vec2( 20 + i * 110, 230 ), vec2( 100 + i * 110, 310 ), ColorAf( 1.0f, 0.3f + i * 0.1f, 0.3f, 1.0f ), ColorAf( 0.3f, 0.3f + i * 0.1f, 1.0f, 1.0f ) );
			mCanvas->fillRoundedRect( Rectf( 20 + i * 110, 230, 100 + i * 110, 310 ), 10, grad );
		}

		// Row 4: Stars with feathering
		for( int i = 0; i < 5; i++ ) {
			vg::Paint starPaint;
			starPaint.setColor( ColorAf( 0.9f, 0.8f, 0.2f, 1.0f ) );
			starPaint.setFeather( 3.0f + i * 2.0f );
			mCanvas->save();
			mCanvas->translate( vec2( 60 + i * 110, 380 ) );
			mCanvas->fillPath( mStarPath, starPaint );
			mCanvas->restore();
		}

		mDisplayList->endRecording();
	}

	void draw( vg::CanvasGlRef canvas ) override
	{
		vg::Paint textPaint;
		textPaint.setColor( ColorAf( 0.8f, 0.8f, 0.8f, 1 ) );

		if( mUseDisplayList && mDisplayList && mDisplayList->isValid() ) {
			// Replay the display list
			mDisplayList->replay( canvas.get() );

			canvas->drawString( "Mode: DisplayList replay", vec2( 20, 450 ), mFont, textPaint );
		}
		else {
			// Draw directly (same content as recorded)
			vg::Paint fill, stroke;

			for( int i = 0; i < 5; i++ ) {
				fill.setColor( ColorAf( 0.2f + i * 0.15f, 0.5f, 1.0f - i * 0.15f, 0.9f ) );
				canvas->fillCircle( vec2( 60 + i * 110, 60 ), 40, fill );
			}

			stroke.setColor( ColorAf( 1.0f, 0.6f, 0.2f, 1.0f ) ).setStrokeWidth( 3 );
			for( int i = 0; i < 5; i++ ) {
				canvas->strokeRect( Rectf( 20 + i * 110, 120, 100 + i * 110, 200 ), stroke );
			}

			for( int i = 0; i < 5; i++ ) {
				vg::Paint grad;
				grad.setLinearGradient( vec2( 20 + i * 110, 230 ), vec2( 100 + i * 110, 310 ), ColorAf( 1.0f, 0.3f + i * 0.1f, 0.3f, 1.0f ), ColorAf( 0.3f, 0.3f + i * 0.1f, 1.0f, 1.0f ) );
				canvas->fillRoundedRect( Rectf( 20 + i * 110, 230, 100 + i * 110, 310 ), 10, grad );
			}

			for( int i = 0; i < 5; i++ ) {
				vg::Paint starPaint;
				starPaint.setColor( ColorAf( 0.9f, 0.8f, 0.2f, 1.0f ) );
				starPaint.setFeather( 3.0f + i * 2.0f );
				canvas->save();
				canvas->translate( vec2( 60 + i * 110, 380 ) );
				canvas->fillPath( mStarPath, starPaint );
				canvas->restore();
			}

			canvas->drawString( "Mode: Direct drawing", vec2( 20, 450 ), mFont, textPaint );
		}
	}

	void drawUI() override
	{
		ImGui::Checkbox( "Use DisplayList", &mUseDisplayList );

		if( mDisplayList && mDisplayList->isValid() ) {
			ImGui::TextColored( ImVec4( 0.3f, 0.9f, 0.3f, 1.0f ), "Commands: %zu", mDisplayList->getCommandCount() );
		}
		else {
			ImGui::TextColored( ImVec4( 0.9f, 0.3f, 0.3f, 1.0f ), "DisplayList: Invalid" );
		}

		if( ImGui::Button( "Rebuild DisplayList" ) ) {
			rebuildDisplayList();
		}

		ImGui::Separator();
		ImGui::TextWrapped(
			"DisplayList records drawing commands and replays them. "
			"Toggle to compare direct drawing vs replay." );
	}

  private:
	Font			   mFont;
	vg::CachedPathRef  mStarPath;
	vg::DisplayListRef mDisplayList;
	bool			   mUseDisplayList = true;
};

// ============================================================================
// Main App
// ============================================================================
class VectorGraphicsDemoApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;

  private:
	void switchDemo( int i );

	vg::CanvasGlRef		 mCanvas;
	CanvasUi			 mCanvasUi;
	std::vector<DemoRef> mDemos;
	int					 mDemoIndex = 0;
	DemoRef				 mDemo;
	double				 mLastTime = 0;
	float				 mFps = 60;
	bool				 mDemoDragging = false;
};

void VectorGraphicsDemoApp::setup()
{
	try {
#if USE_OFFSCREEN_FBO
		// Offscreen mode: atomic rendering with analytical AA
		// FBO will be created/resized on first begin(size) call
		mCanvas = vg::CanvasGl::create( 1, 1 );
#else
		// Window mode: direct rendering, auto-detects MSAA
		mCanvas = vg::CanvasGl::create();
#endif
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Canvas init failed: " << e.what() );
		return;
	}

	ImGui::Initialize();

	mDemos = { std::make_shared<PrimitivesDemo>(), std::make_shared<GradientsDemo>(), std::make_shared<TransformsDemo>(), std::make_shared<PathsDemo>(), std::make_shared<FeatheringDemo>(), std::make_shared<InstancingDemo>(),
		std::make_shared<BlendModesDemo>(), std::make_shared<ClippingDemo>(), std::make_shared<ImagesDemo>(), std::make_shared<ImageMeshDemo>(), std::make_shared<ShadowDemo>(), std::make_shared<DisplayListDemo>() };
	for( auto& d : mDemos )
		d->setup( mCanvas );
	switchDemo( 0 );
	mLastTime = getElapsedSeconds();
}

void VectorGraphicsDemoApp::switchDemo( int i )
{
	if( i < 0 || i >= (int)mDemos.size() )
		return;
	mDemoIndex = i;
	mDemo = mDemos[i];
	mDemo->setCanvasUi( &mCanvasUi );
	mCanvasUi.setContentBounds( mDemo->getContentBounds() );
	mCanvasUi.connect( getWindow() );
	mCanvasUi.fitAll();
	mCanvasUi.setZoomLimits( 0.1f, 30.0f );
}

void VectorGraphicsDemoApp::update()
{
	double t = getElapsedSeconds();
	double dt = t - mLastTime;
	mLastTime = t;
	if( dt > 0 )
		mFps = mFps * 0.95f + ( 1.0f / dt ) * 0.05f;
	if( mDemo )
		mDemo->update( dt );
}

void VectorGraphicsDemoApp::draw()
{
	gl::clear( Color( 0.12f, 0.12f, 0.15f ) );
	if( ! mCanvas )
		return;

	// ImGui
	ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 260, 350 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "VectorGraphics Demo" );
	if( ImGui::BeginCombo( "Demo", mDemo ? mDemo->getName().c_str() : "" ) ) {
		for( int i = 0; i < (int)mDemos.size(); i++ ) {
			if( ImGui::Selectable( mDemos[i]->getName().c_str(), i == mDemoIndex ) )
				switchDemo( i );
		}
		ImGui::EndCombo();
	}
	if( mDemo && ! mDemo->getDescription().empty() )
		ImGui::TextWrapped( "%s", mDemo->getDescription().c_str() );
	ImGui::Separator();
	if( mDemo )
		mDemo->drawUI();
	ImGui::Separator();
	ImGui::Text( "FPS: %.0f", mFps );
#if USE_OFFSCREEN_FBO
	ImGui::Text( "Mode: Offscreen FBO (atomic)" );
#else
	ImGui::Text( "Mode: Direct to window" );
#endif
	ImGui::Text( "Keys: 1-9 demos, 0 fit, Q quit" );
	ImGui::End();

	// Canvas rendering - both modes use begin(size) now
	// Offscreen mode will auto-resize FBO if window size changes
	mCanvas->begin( toPixels( getWindowSize() ) );
	mCanvas->setTransform( mCanvasUi.getTransform2d() );

	try {
		if( mDemo ) {
			mDemo->draw( mCanvas );
			vg::Paint bp;
			bp.setColor( ColorAf( 0.3f, 0.3f, 0.4f, 0.5f ) ).setStrokeWidth( 1 );
			//            mCanvas->strokeRect( mDemo->getContentBounds(), bp );
		}
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Demo draw error: " << e.what() );
	}
	mCanvas->end();

#if USE_OFFSCREEN_FBO
	// Blit the offscreen FBO to the window
	gl::draw( mCanvas->getTexture(), getWindowBounds() );
#endif
}

void VectorGraphicsDemoApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE || event.getChar() == 'q' )
		quit();
	else if( event.getChar() >= '1' && event.getChar() <= '7' )
		switchDemo( event.getChar() - '1' );
	else if( event.getChar() == '0' )
		mCanvasUi.fitAll();
}

void VectorGraphicsDemoApp::mouseDown( MouseEvent event )
{
	if( mDemo && mDemo->onMouseDown( event ) ) {
		mDemoDragging = true;
	}
}

void VectorGraphicsDemoApp::mouseDrag( MouseEvent event )
{
	if( mDemoDragging && mDemo ) {
		mDemo->onMouseDrag( event );
	}
}

void VectorGraphicsDemoApp::mouseUp( MouseEvent event )
{
	if( mDemo )
		mDemo->onMouseUp( event );
	mDemoDragging = false;
}

void VectorGraphicsDemoApp::mouseMove( MouseEvent event )
{
	if( mDemo )
		mDemo->onMouseMove( event );
}

void prepareSettings( VectorGraphicsDemoApp::Settings* s )
{
	s->setTitle( "VectorGraphics Demo" );
	s->setWindowSize( 1200, 800 );
}

#if USE_OFFSCREEN_FBO
// Offscreen mode: no MSAA needed (atomic mode uses analytical AA)
CINDER_APP( VectorGraphicsDemoApp, RendererGl, prepareSettings )
#else
// Window mode: use MSAA for hardware antialiasing
CINDER_APP( VectorGraphicsDemoApp, RendererGl( RendererGl::Options().msaa( 16 ) ), prepareSettings )
#endif
