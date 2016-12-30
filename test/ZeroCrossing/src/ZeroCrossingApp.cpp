#include "cinder/Font.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "ZeroCrossings.h"
#include "ZeroCrossings2.h"

using namespace ci;
using namespace ci::app;
using namespace std;

bool gDebugContains = false;

class ShapeTestApp : public App {
  public:
	void setup() override;
	void draw() override;
	
	void drawDebugShape( const Shape2d &s, float radius );
	void echoCurrentShape();

	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;

	void setRandomFont();
	void setRandomGlyph();

	void generateSDF();
	void generateContains();
	void generateContains2();
	void generateCrossings();

	void reposition( vec2 point );
	void calculate();
	void calculateModelMatrix();

	int				mFontIndex, mGlyphIndex;
//	int				mInitialFontIndex = 193, mInitialGlyphIndex = /*3208*/ 4137;
//	int				mInitialFontIndex = 777, mInitialGlyphIndex = 14661;
//	int				mInitialFontIndex = 880, mInitialGlyphIndex = 417;
//	int				mInitialFontIndex = 880, mInitialGlyphIndex = 3361; // fixed
//	int				mInitialFontIndex = 733, mInitialGlyphIndex = 1441; // fixed 25
//	int				mInitialFontIndex = 733, mInitialGlyphIndex = 1050;
	int				mInitialFontIndex = 324, mInitialGlyphIndex = 594;
	Font             mFont;
	Shape2d          mShape;
	vector<string>   mFontNames;
	int              mFontSize;
	float            mZoom;
	float            mDistance;
	vec2             mMouse, mClick;
	vec2             mClosest;
	vec2             mAnchor, mLocal, mPosition, mOriginal;
	mat4             mModelMatrix;
	bool             mIsInside;
	Rectf            mBounds;
	Channel32f       mChannel;
	gl::Texture2dRef mTexture;
	
std::vector<vec2> testPoints;
};

void ShapeTestApp::setup()
{
	mPosition = getWindowCenter() * vec2( 0.8f, 1.2f );
	mDistance = 0;
	mZoom = 1;
	mIsInside = false;

	mFontNames = Font::getNames();
	mFontSize = 1024;
	setRandomFont();
}

void ShapeTestApp::setRandomFont()
{
	// select a random font from those available on the system
	static bool firstCall = true;
	if( firstCall ) {
		mFontIndex = mInitialFontIndex;
		firstCall = false;
	}
	else
		mFontIndex = rand() % mFontNames.size();
	mFont = Font( mFontNames[mFontIndex], (float)mFontSize );
	setRandomGlyph();
}

void ShapeTestApp::setRandomGlyph()
{
	static bool firstTime = false;
	if( firstTime ) {
		Path2d p;	
		p.moveTo( vec2( 350.208 - 40, -642.048 + 40 ) );
		p.lineTo( vec2( 437.248, -642.048 ) );
		p.quadTo( vec2( 423.936, -665.6 ), vec2( 401.92, -683.52 ) );
		p.quadTo( vec2( 379.904, -701.44), vec2( 350.208, -711.168 ) );
		p.close();

		mShape = Shape2d();
		mShape.appendContour( p );
		//
		testPoints.push_back( vec2( 401.92, -603.52 ) ); 
		testPoints.push_back( vec2( 80.5f, -30 ) );
		testPoints.push_back( vec2( 79.5f, -30 ) );
		for( auto &pt : testPoints )
			std::cout << pt << " : " << p.contains( pt ) << std::endl;
		firstTime = false;
		p.contains( vec2( 14.000, -640.202 ) );
	}
	else {
		static bool firstIndex = true;
		if( firstIndex ) {
			mGlyphIndex = mInitialGlyphIndex;
			firstIndex = false;
		}
		else
			mGlyphIndex = rand() % mFont.getNumGlyphs();
		try {
			mShape = mFont.getGlyphShape( mGlyphIndex );
		}
		catch( FontGlyphFailureExc & ) {
			console() << "Looks like glyph " << mGlyphIndex << " doesn't exist in this font." << std::endl;
		}

//mShape.removeContour( 1 );
//mShape.removeContour( 1 );
//mShape.removeContour( 1 );

		console() << "Font: "<< mFontIndex << "(" << mFontNames[mFontIndex] << ") glyph: " << mGlyphIndex << std::endl;
	}

//console() << mShape.getContour( 0 ) << std::endl;
	testPoints.clear();

	mBounds = mShape.calcBoundingBox();
	mAnchor = mBounds.getUpperLeft() + 0.5f * mBounds.getSize();
	mPosition = getWindowCenter();
	mZoom = 0.9f * glm::min( getWindowWidth() / mBounds.getWidth(), getWindowHeight() / mBounds.getHeight() );

	mTexture.reset();

	calculate();
}

void ShapeTestApp::generateSDF()
{
	// Create a channel large enough for this glyph.
	const int padding = 0;
	mChannel = Channel32f( mBounds.getWidth() + padding * 2, mBounds.getHeight() + padding * 2 );

	Timer t( true );
	
	const float kRange = 40.0f;

	// For each texel, calculate the signed distance, normalize it and store it.
	auto itr = mChannel.getIter();
	vector<vec2> zeros;
	while( itr.line() ) {
		while( itr.pixel() ) {
			auto pt = vec2( itr.getPos() ) + mBounds.getUpperLeft() - vec2( padding, padding );
			auto dist = mShape.calcSignedDistance( pt ) / kRange;
			itr.v() = glm::clamp( dist * 0.5f + 0.5f, 0.0f, 1.0f );
			//itr.v() = (float)( zeros.size() / 7.0f );
		}
	}

	t.stop();
//	console() << "Generate:" << t.getSeconds() << std::endl;

	// Create the texture.
	mTexture = gl::Texture2d::create( mChannel, gl::Texture2d::Format().magFilter( GL_NEAREST ) );
}

void ShapeTestApp::generateContains()
{
	mChannel = Channel32f( mBounds.getWidth(), mBounds.getHeight() );

	// For each texel, calculate the signed distance, normalize it and store it.
	auto itr = mChannel.getIter();
	vector<vec2> zeros;
	while( itr.line() ) {
		vec2 pt;
		while( itr.pixel() ) {
			pt = vec2( itr.getPos() ) + mBounds.getUpperLeft();
			itr.v() = mShape.contains( pt ) ? 1 : 0;
		}
	}

	mTexture = gl::Texture2d::create( mChannel, gl::Texture2d::Format().magFilter( GL_NEAREST ) );
}

void ShapeTestApp::generateContains2()
{
	mChannel = Channel32f( mBounds.getWidth(), mBounds.getHeight() );

	// For each texel, calculate the signed distance, normalize it and store it.
	auto itr = mChannel.getIter();
	vector<vec2> zeros;
	while( itr.line() ) {
		vec2 pt;
		while( itr.pixel() ) {
			pt = vec2( itr.getPos() ) + mBounds.getUpperLeft();
			itr.v() = contains2( mShape, pt ) ? 1 : 0;
		}
	}

	mTexture = gl::Texture2d::create( mChannel, gl::Texture2d::Format().magFilter( GL_NEAREST ) );
}

void ShapeTestApp::generateCrossings()
{
	Surface32f s( mBounds.getWidth(), mBounds.getHeight(), false );

	// For each texel, calculate the signed distance, normalize it and store it.
	auto itr = s.getIter();
	vector<vec2> zeros;
	while( itr.line() ) {
		vec2 pt;
		while( itr.pixel() ) {
			pt = vec2( itr.getPos() ) + mBounds.getUpperLeft();
			findZeroes( mShape, pt, &zeros );
			itr.r() = (float)( zeros.size() / 7.0f );
			itr.g() = ( zeros.size() & 1 ) ? 1.0f : 0.0f;
		}
//		std::cout << pt.y << " ";
	}

	mTexture = gl::Texture2d::create( s, gl::Texture2d::Format().magFilter( GL_NEAREST ) );
}

void ShapeTestApp::drawDebugShape( const Shape2d &s, float radius )
{
	for( int c = 0; c < s.getNumContours(); ++c ) {
		Path2d p = s.getContour( c);

		size_t firstPoint = 0;
		for( size_t s = 0; s < p.getNumSegments(); ++s ) {
			switch( p.getSegmentType( s ) ) {
				case Path2d::CUBICTO:
					gl::color( 0, 1, 1 );
					gl::drawSolidCircle( p.getPoint(firstPoint+0), radius, 3 );
					gl::color( 1, 0, 1 );
					gl::drawSolidCircle( p.getPoint(firstPoint+1), radius ); 
					gl::color( 1, 0, 1 );
					gl::drawSolidCircle( p.getPoint(firstPoint+2), radius );
					gl::color( 0, 1, 1 );
					gl::drawSolidCircle( p.getPoint(firstPoint+3), radius, 3 );
				break;
				case Path2d::QUADTO:
					gl::color( 1, 0.5, 0.25f, 0.5f );
					gl::drawSolidCircle( p.getPoint(firstPoint+0), radius );
					gl::color( 0.25, 0.5, 1.0f );
					gl::drawStrokedRect( Rectf( p.getPoint(firstPoint+1) - vec2( radius ), p.getPoint(firstPoint+1) + vec2( radius ) ) ); 
					gl::color( 1, 0.5, 0.25f, 0.5f );
					gl::drawSolidCircle( p.getPoint(firstPoint+2), radius );
				break;
				case Path2d::LINETO:
					gl::color( 0, 1, 0 );
					gl::drawStrokedCircle( p.getPoint(firstPoint+0), radius );
					gl::drawStrokedCircle( p.getPoint(firstPoint+1), radius );
				break;
				case Path2d::CLOSE: // ignore - we always assume closed
					gl::color( 0.9, 0.1, 0.1 );
					gl::drawStrokedCircle( p.getPoint(firstPoint+0), radius );
				break;
				default:
					;//throw Path2dExc();
			}
			
			firstPoint += Path2d::sSegmentTypePointCounts[p.getSegmentType(s)];
		}
	}
	
	gl::color( 1, 1, 1 );
	for( auto &pt : testPoints )
		gl::drawStrokedCircle( pt, radius );
}

void ShapeTestApp::echoCurrentShape()
{
	for( size_t c = 0; c < mShape.getNumContours(); ++c )
		console() << mShape.getContour( c ) << std::endl;
}

void ShapeTestApp::draw()
{
	gl::clear();

	gl::pushModelMatrix();
	gl::setModelMatrix( mModelMatrix );

	// Draw SDF texture if available.
	if( mTexture ) {
		gl::pushModelMatrix();
		gl::translate( mBounds.getUpperLeft() );

		gl::color( 1, 1, 1 );
		gl::draw( mTexture );

		gl::popModelMatrix();
	}

	// Draw shape outlines.
	gl::color( mIsInside ? Color( 0.4f, 0.8f, 0.0f ) : Color( 0.8f, 0.4f, 0.0f ) );
	gl::draw( mShape );
	drawDebugShape( mShape, 2.0f );

	gl::popModelMatrix();

	// Draw closest point on shape.
//	gl::color( 0, 0.5f, 0.5f );
//	gl::drawSolidCircle( mClosest, 5 );

	// Draw distance as a circle.
	gl::color( 0.5f, 0.5f, 0 );
	gl::drawStrokedCircle( mMouse, mDistance );
}

void ShapeTestApp::mouseMove( MouseEvent event )
{
	mMouse = event.getPos();

	calculate();
}

void ShapeTestApp::mouseDown( MouseEvent event )
{
	reposition( event.getPos() );

	mClick = event.getPos();
	mOriginal = mPosition;

	float closest = 10000000000;
	vec2 closestPt;
	for( auto &c : mShape.getContours() ) {
		for( auto p : c.getPoints() ) {
			if( distance( p, mLocal ) < closest ) {
				closest = distance( p, mLocal );
				closestPt = p; 
			}
		} 
	} 

	console() << "Closest ctrl pt: " << closestPt << std::endl;

gDebugContains = true;
	calculate();
	if( event.isMiddle() || event.isShiftDown() ) {
		findZeroes( mShape, mLocal, &testPoints );
		std::cout << "Zeroes: " << testPoints.size();
	}
	else if( event.isRight() || event.isControlDown() ) {
		findZeroes( mShape, { closestPt.x, mLocal.y }, &testPoints );
		std::cout << "Snapped Zeroes: " << testPoints.size();
		for( auto &tp : testPoints )
			std::cout << tp;
	} 
gDebugContains = false;	
std::cout << "   Mouse: " << event.getPos() << " : " << mLocal << std::endl;
}

void ShapeTestApp::mouseDrag( MouseEvent event )
{
	mPosition = mOriginal + vec2( event.getPos() ) - mClick;
	mMouse = event.getPos();

	calculate();
}

void ShapeTestApp::mouseWheel( MouseEvent event )
{
	reposition( event.getPos() );

	mMouse = event.getPos();
	mZoom *= 1.0f + 0.1f * event.getWheelIncrement();

	calculate();
}

void ShapeTestApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
		break;
		case KeyEvent::KEY_c:
			if( ! mTexture )
				generateContains();
			else
				mTexture.reset();
		break;
		case KeyEvent::KEY_2:
			if( ! mTexture )
				generateContains2();
			else
				mTexture.reset();
		break;
		case KeyEvent::KEY_x:
			if( ! mTexture )
				generateCrossings();
			else
				mTexture.reset();
		break;
		case KeyEvent::KEY_s:
			if( ! mTexture )
				generateSDF();
			else
				mTexture.reset();
		break;
		case KeyEvent::KEY_g:
			setRandomGlyph();
		break;
		case KeyEvent::KEY_f:
			setRandomFont();
		break;
		case KeyEvent::KEY_e:
			echoCurrentShape();
		break;
		
	}
}

void ShapeTestApp::reposition( vec2 mouse )
{
	// Convert mouse to object space.
	mat4 invModelMatrix = glm::inverse( mModelMatrix );
	vec2 anchor = vec2( invModelMatrix * vec4( mouse, 0, 1 ) );

	// Calculate new position, anchor and scale.
	mPosition += vec2( mModelMatrix * vec4( anchor - mAnchor, 0, 0 ) );
	mAnchor = anchor;
}

void ShapeTestApp::calculate()
{
	calculateModelMatrix();

	mLocal = vec2( glm::inverse( mModelMatrix ) * vec4( mMouse, 0, 1 ) );
	mClosest = vec2( mModelMatrix * vec4( mShape.calcClosestPoint( mLocal ), 0, 1 ) );
	mDistance = glm::distance( mClosest, mMouse );
}

void ShapeTestApp::calculateModelMatrix()
{
	// Update model matrix.
	mModelMatrix = glm::translate( vec3( mPosition, 0 ) );
	mModelMatrix *= glm::scale( vec3( mZoom ) );
	mModelMatrix *= glm::translate( vec3( -mAnchor, 0 ) );
}

CINDER_APP( ShapeTestApp, RendererGl )