#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/text/Text.h"
#include "cinder/text/AttrString.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Font.h"
#include "cinder/GeomIo.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TextTestApp : public App {
 public:
	void setup();
	void draw();
	void keyDown( KeyEvent event );
	void testAttrString();

	text::Face		*mFace, *mEmojiFace;
	text::Font		*mFont17, *mEmojiFont, *mFont36;
	
	
	gl::TextureRef 	mTex;
	Channel8u		mChannel;
	Shape2d			mGlyphShape;
};

void printFontNames()
{
	for( vector<string>::const_iterator fontName = Font::getNames().begin(); fontName != Font::getNames().end(); ++fontName )
		console() << *fontName << endl;
}

void printFaceInfo( const text::Face *face )
{
	console() << "Family: '" << face->getFamilyName() << "'  Style: '" << face->getStyleName() << "'  Total Glyphs: " << face->getNumGlyphs()
				<< "  Color: " << (face->hasColor()  ? "true" : "false");
	console() << "  Fixed sizes: { ";
	for( auto sz : face->getFixedSizes() )
		console() << sz << " ";
	console() << "}" << std::endl;
//	console() << "  Ascender: " << face->getAscender() << "  Descender: " << face->getDescender() << "  Height: " << face->getHeight() << std::endl;
}

void printFontInfo( const text::Font *font )
{
	console() << "  Ascender: " << font->getAscender() << "  Descender: " << font->getDescender() << "  Height: " << font->getHeight() << "  'A' index: " << font->getCharIndex( 65 ) << std::endl;
}

void TextTestApp::testAttrString()
{
	text::AttrString as;
/*	as << mFont17;
	as << "Hello";
	as << mFont36;
	as << "Big";
	as << mFont17;
	as << "World";*/
	
	as << mFont17;
	as << "Hello BIG World";
	as.setFont( 10, 15, mFont36 );
	
	auto it = as.iterate();
	while( it.nextRun() ) {
		console() << "Run: '" << it.getRunUtf8() << "' {" << *(it.getRunFont()) << "}" << std::endl;
	}
}

void TextTestApp::setup()
{
	printFontNames();

	//mEmojiFace = text::loadFace( getAssetPath( "Twemoji.ttf" ) );
	mEmojiFace = text::loadFace( "/System/Library/Fonts/Apple Color Emoji.ttc" );
	//mFace = text::loadFace( getResourcePath( "Saint-Andrews Queen.ttf" ) );
	//mFace = text::loadFace( "/System/Library/Fonts/Avenir Next Condensed.ttc" );
	mFace = text::loadFace( "/System/Library/Fonts/Noteworthy.ttc" );
	printFaceInfo( mEmojiFace );
	printFaceInfo( mFace );
	
	mFont17 = text::loadFont( mFace, 24 );
	printFontInfo( mFont17 );
	mFont36 = text::loadFont( mFace, 36 );

	mEmojiFont = text::loadFont( mEmojiFace, 84 );

	//mATex = gl::Texture::create( mFont17->getGlyphBitmap( mFont17->getCharIndex( 'A' ) ) );
//	writeImage( getHomeDirectory() / "out.png", mEmojiFont->getGlyphBitmap( mEmojiFont->getCharIndex( U"😀"[0] ) ) );
//	mATex = gl::Texture::create( mEmojiFont->getGlyphBitmap( mEmojiFont->getCharIndex( U"😀"[0] ) ) );
	
	//mGlyphShape = mFont17->getGlyphShape( mFont17->getCharIndex( 'A' ) );

	testAttrString();
	
	console() << "Width: " << mFont17->calcStringWidth( "Hello World" ) << std::endl;
	vector<uint32_t> indices;
	vector<float> positions;
	mFont17->shapeString( "Héllo World", &indices, &positions );
	
	for( size_t i = 0; i < indices.size(); ++i ) {
		Shape2d temp = mFont17->getGlyphShape( indices[i] );
		temp.translate( vec2( positions[i], 0 ) );
		mGlyphShape.append( temp );
	}

//	mChannel = mFont17->renderString( "Hello World" );
//	mTex = gl::Texture::create( mChannel );

	mChannel = renderString( text::AttrString() << mFont17 << "Hello" << mFont36 << " BIG " << mFont17 << "boi" );
	mTex = gl::Texture::create( mChannel );
}

void TextTestApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 's' ) {
		writeImage( getHomeDirectory() / "textTestOut.png", mChannel );
	}
}

void TextTestApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::clear( Color( 0.5f, 0.5f, 0.5f ) );
	gl::enableAlphaBlending();
	
	gl::color( Color::white() );
	gl::draw( mTex, getWindowCenter() );
	gl::color( Color8u( 255, 128, 64 ) );
	{
		gl::ScopedMatrices s_;
		gl::translate( vec2( 20, 240 ) );
	//	gl::draw( mGlyphShape );
	}
}

CINDER_APP( TextTestApp, RendererGl )
