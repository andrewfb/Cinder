#include "cinder/Font.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ShapeTestApp : public App {
  public:
	void setup() override;
	void draw() override;
void drawDebugShape( const Shape2d &s, float radius );

	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;

	void setRandomFont();
	void setRandomGlyph();

	void generateSDF();

	void reposition( const vec2 &point );
	void calculate();
	void calculateModelMatrix();

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
	mFont = Font( "Times", (float)mFontSize );

	setRandomGlyph();
}

void ShapeTestApp::setRandomFont()
{
	// select a random font from those available on the system
	mFont = Font( mFontNames[rand() % mFontNames.size()], (float)mFontSize );
	setRandomGlyph();
}

void ShapeTestApp::setRandomGlyph()
{
	size_t glyphIndex = rand() % mFont.getNumGlyphs();
glyphIndex = mFont.getGlyphChar( ',' );
	try {
		mShape = mFont.getGlyphShape( glyphIndex );
	}
	catch( FontGlyphFailureExc & ) {
		console() << "Looks like glyph " << glyphIndex << " doesn't exist in this font." << std::endl;
	}

console() << mShape.getContour( 0 ) << std::endl;

static bool firstTime = true;
if( firstTime ) {
Path2d p;	
p.moveTo( vec2( 304.5f, -677.5f ) );
p.quadTo( vec2( 72, -614 ), vec2( 14, -658.5f ) );
p.lineTo( vec2( 14, -677.5f ) );

mShape = Shape2d();
mShape.appendContour( p );
//
testPoints.push_back( vec2( 14.000f, -640.202f ) ); 
testPoints.push_back( vec2( 80.5f, -30 ) );
testPoints.push_back( vec2( 79.5f, -30 ) );
for( auto &pt : testPoints )
	std::cout << pt << " : " << p.contains( pt ) << std::endl;
firstTime = false;
p.contains( vec2( 14.000, -640.202 ) );
}

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
	mChannel = Channel32f( mBounds.getWidth(), mBounds.getHeight() );

	Timer t( true );

	// For each texel, calculate the signed distance, normalize it and store it.
	const float kRange = 40.0f;

float maxX = 0;
	auto itr = mChannel.getIter();
	while( itr.line() ) {
		while( itr.pixel() ) {
			auto pt = vec2( itr.getPos() ) + mBounds.getUpperLeft();
//			auto dist = mShape.calcSignedDistance( pt ) / kRange;
			//itr.v() = glm::clamp( dist * 0.5f + 0.5f, 0.0f, 1.0f );
			itr.v() = (float)( mShape.contains( pt + vec2( 0, 0.5f ) ) );
			if( itr.v() > 0 && pt.x > maxX )
				maxX = pt.x;
		}
	}
cout << "MaxX: " << maxX;
	t.stop();
	console() << "Generate:" << t.getSeconds() << std::endl;

	// Create the texture.
	mTexture = gl::Texture2d::create( mChannel, gl::Texture2d::Format().magFilter( GL_NEAREST ) );
}

void ShapeTestApp::drawDebugShape( const Shape2d &s, float radius )
{
	Path2d p = s.getContour( 0 );

	size_t firstPoint = 0;
	for( size_t s = 0; s < p.getNumSegments(); ++s ) {
		switch( p.getSegmentType( s ) ) {
			case Path2d::CUBICTO:
//				crossings += cubicBezierCrossings( &(mPoints[firstPoint]), pt );
			break;
			case Path2d::QUADTO:
				gl::color( 1, 0.5, 0.25f );
				gl::drawSolidCircle( p.getPoint(firstPoint+0), radius );
				gl::color( 0.25, 0.5, 1.0f );
				gl::drawSolidCircle( p.getPoint(firstPoint+1), radius ); 
				gl::color( 1, 0.5, 0.25f );
				gl::drawSolidCircle( p.getPoint(firstPoint+2), radius );
			break;
			case Path2d::LINETO:
				gl::color( 0, 1, 0 );
				gl::drawStrokedCircle( p.getPoint(firstPoint+0), radius );
				gl::drawStrokedCircle( p.getPoint(firstPoint+1), radius );
			break;
			case Path2d::CLOSE: // ignore - we always assume closed
				gl::color( 1, 0, 0 );
				gl::drawStrokedCircle( p.getPoint(firstPoint+0), radius );
			break;
			default:
				;//throw Path2dExc();
		}
		
		firstPoint += Path2d::sSegmentTypePointCounts[p.getSegmentType(s)];
	}
	
	gl::color( 0, 1, 0 );
	for( auto &pt : testPoints )
		gl::drawStrokedCircle( pt, radius );
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

	calculate();
std::cout << "Mouse: " << event.getPos() << " : " << mLocal << std::endl;
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
	case KeyEvent::KEY_s:
		generateSDF();
		break;
	case KeyEvent::KEY_g:
		setRandomGlyph();
		break;
	}
}

void ShapeTestApp::reposition( const vec2 &mouse )
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
	mIsInside = mShape.contains( mLocal );
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