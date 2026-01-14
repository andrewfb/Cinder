/*
 VectorGraphicsBasic - Minimal example of the vg::CanvasGl API

 Demonstrates 3 shapes with different fill styles:
 - Circle with solid color
 - Path2d (star) with gradient
 - Rectangle with feathering (soft edges)
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/vg/CanvasGl.h"
#include "cinder/vg/Paint.h"

using namespace ci;
using namespace ci::app;

class VectorGraphicsBasicApp : public App {
public:
    void setup() override;
    void draw() override;

private:
    vg::CanvasGlRef     mCanvas;
    vg::CachedPathRef   mStarPath;
};

void VectorGraphicsBasicApp::setup()
{
    mCanvas = vg::CanvasGl::create( getWindowWidth(), getWindowHeight() );
//	mCanvas = vg::CanvasGl::create();

    // Create a star path using Path2d
    Path2d star;
    for( int i = 0; i < 10; i++ ) {
        float angle = i * M_PI / 5.0f - M_PI / 2.0f;
        float radius = (i % 2 == 0) ? 80.0f : 35.0f;
        vec2 pt = vec2( cos( angle ), sin( angle ) ) * radius;
        if( i == 0 )
            star.moveTo( pt );
        else
            star.lineTo( pt );
    }
    star.close();
    mStarPath = mCanvas->createPath( star );
}

void VectorGraphicsBasicApp::draw()
{
    gl::clear( Color( 0.15f, 0.15f, 0.18f ) );

    mCanvas->begin( toPixels( getWindowSize() ) );

    // ========================================
    // 1. CIRCLE WITH SOLID COLOR
    // ========================================
    {
        vg::Paint solidPaint;
        solidPaint.setColor( ColorAf( 0.2f, 0.7f, 1.0f, 1.0f ) );

        mCanvas->fillCircle( vec2( 150, 300 ), 80, solidPaint );
    }

    // ========================================
    // 2. STAR PATH WITH GRADIENT
    // ========================================
    {
        vg::Paint gradientPaint;
        gradientPaint.setLinearGradient(
            vec2( 400 - 80, 300 - 80 ),    // start point
            vec2( 400 + 80, 300 + 80 ),    // end point
            ColorAf( 1.0f, 0.3f, 0.3f, 1.0f ),  // start color (red)
            ColorAf( 1.0f, 0.9f, 0.2f, 1.0f )   // end color (yellow)
        );

        mCanvas->save();
        mCanvas->translate( vec2( 400, 300 ) );
        mCanvas->fillPath( mStarPath, gradientPaint );
        mCanvas->restore();
    }

    // ========================================
    // 3. RECTANGLE WITH FEATHERING
    // ========================================
    {
        vg::Paint featherPaint;
        featherPaint.setColor( ColorAf( 0.4f, 1.0f, 0.5f, 1.0f ) );
        featherPaint.setFeather( 20.0f );

        mCanvas->fillRect( Rectf( 570, 220, 730, 380 ), featherPaint );
    }

    // ========================================
    // LABELS
    // ========================================
    {
        Font font( "Arial", 48 );
        vg::Paint textPaint;
        textPaint.setColor( ColorAf( 0.8f, 0.8f, 0.8f, 1.0f ) );

        mCanvas->drawString( "Solid Fill", vec2( 110, 420 ), font, textPaint );
        mCanvas->drawString( "Gradient", vec2( 360, 420 ), font, textPaint );
        mCanvas->drawString( "Feathering", vec2( 605, 420 ), font, textPaint );
    }

    mCanvas->end();
}

CINDER_APP( VectorGraphicsBasicApp, RendererGl( RendererGl::Options().msaa( 0 ) ),
    []( App::Settings* settings ) {
        settings->setTitle( "VectorGraphicsBasic" );
        settings->setWindowSize( 880, 600 );
    }
)
