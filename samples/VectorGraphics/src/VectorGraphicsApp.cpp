/*
 VectorGraphics Sample

 Demonstrates the Rive-based vector graphics renderer integrated with Cinder.
 Showcases:
 - Basic shapes (rect, circle, ellipse, rounded rect)
 - Strokes with different caps and joins
 - Linear and radial gradients
 - Feathering (soft edges)
 - Transform stack (rotate, scale, translate)
 - Path2d/Shape2d rendering
 - CachedPath for efficient repeated drawing
 - Image drawing (drawImage, drawImageMesh)
 - Text rendering with Font
 - Instanced drawing (drawCircles, drawPaths)
 - CanvasUi integration for pan/zoom
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Surface.h"
#include "cinder/ip/Resize.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/CanvasUi.h"

#include "cinder/vg/CanvasGl.h"
#include "cinder/vg/Paint.h"

using namespace ci;
using namespace ci::app;

class VectorGraphicsApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void resize() override;
    void keyDown( KeyEvent event ) override;
    void saveScreenshot();

private:
    void drawVectorContent();
    void createCachedPaths();
    void createTestImage();

    vg::CanvasGlRef mCanvas;
    CanvasUi mCanvasUi;
    bool mVgInitialized = false;
    bool mSaveScreenshot = false;
    int mFrameCount = 0;
    float mAnimTime = 0;
    double mLastFrameTime = 0;
    float mFps = 0;
    float mFpsSmoothed = 60.0f;

    // Cached paths for efficient drawing
    vg::CachedPathRef mStarPath;
    vg::CachedPathRef mHeartPath;
    vg::CachedPathRef mGearPath;

    // Image for testing
    vg::ImageRef mTestImage;

    // Font for text
    Font mFont;

    // Instanced drawing data
    std::vector<vec2> mParticlePositions;
    std::vector<mat3> mParticleTransforms;

    // Content bounds for CanvasUi
    Rectf mContentBounds;
};

void VectorGraphicsApp::setup()
{
    CI_LOG_I( "VectorGraphics sample starting..." );
    CI_LOG_I( "OpenGL Version: " << glGetString( GL_VERSION ) );
    CI_LOG_I( "OpenGL Renderer: " << glGetString( GL_RENDERER ) );

    // Define content bounds
    mContentBounds = Rectf( 0, 0, 1200, 900 );

    // Setup CanvasUi for pan/zoom
    mCanvasUi.setContentBounds( mContentBounds );
    mCanvasUi.connect( getWindow() );

    // Enable left-click panning (default is Alt+Left, Middle, or Right)
/*    mCanvasUi.setPanMouseButtons( {
        { app::MouseEvent::LEFT_DOWN, 0 },      // Left click to pan
        { app::MouseEvent::MIDDLE_DOWN, 0 },    // Middle click to pan
        { app::MouseEvent::RIGHT_DOWN, 0 }      // Right click to pan
    });*/

    mCanvasUi.fitAll();
    mCanvasUi.setZoomLimits( 0.1f, 20.0f );

    // Load font
    mFont = Font( "Arial", 24 );

    // Generate particle positions for instanced drawing
    Rand rnd;
    for( int i = 0; i < 200; ++i ) {
        mParticlePositions.push_back( vec2(
            rnd.nextFloat( 850, 1150 ),
            rnd.nextFloat( 50, 350 )
        ));

        // Also create transform matrices
        mat3 xform( 1.0f );
        xform[2][0] = rnd.nextFloat( 850, 1150 );
        xform[2][1] = rnd.nextFloat( 400, 600 );
        float scale = rnd.nextFloat( 0.5f, 1.5f );
        xform[0][0] = scale;
        xform[1][1] = scale;
        mParticleTransforms.push_back( xform );
    }

    try {
        // Create the canvas with options
        vg::CanvasOptions options;
        #if defined( CINDER_MAC )
        // macOS GL 4.1 doesn't support PLS or FSI - force fallback rendering mode
        options.disablePixelLocalStorage = true;
        options.disableFragmentShaderInterlock = true;
        #endif
        // Use floating-point buffer for higher quality gradients and feathering
        options.useFloatingPointBuffer = true;
        // Create canvas for screen rendering (no FBO)
        mCanvas = vg::createCanvasGl( options );
        mVgInitialized = true;

        // Create cached paths and test image
        createCachedPaths();
        createTestImage();

        CI_LOG_I( "Vector graphics initialized successfully!" );
    }
    catch( const vg::VgExc &exc ) {
        CI_LOG_E( "Failed to initialize vector graphics: " << exc.what() );
    }
    catch( const std::exception &exc ) {
        CI_LOG_E( "Exception during setup: " << exc.what() );
    }
}

void VectorGraphicsApp::createCachedPaths()
{
    // Star path (reusable)
    Path2d starPath;
    vec2 starCenter( 0, 0 );
    float outerR = 40, innerR = 18;
    for( int i = 0; i < 10; ++i ) {
        float angle = i * M_PI / 5.0f - M_PI / 2.0f;
        float r = ( i % 2 == 0 ) ? outerR : innerR;
        vec2 pt = starCenter + vec2( std::cos( angle ), std::sin( angle ) ) * r;
        if( i == 0 )
            starPath.moveTo( pt );
        else
            starPath.lineTo( pt );
    }
    starPath.close();
    mStarPath = mCanvas->createPath( starPath );

    // Heart path - using bezier curves (arcs with forward=false cause issues)
    Path2d heartPath;
    float s = 30.0f; // Scale
    heartPath.moveTo( 0, s * 0.3f );  // Bottom point
    // Left side curve
    heartPath.curveTo( vec2( -s * 0.5f, -s * 0.1f ), vec2( -s * 0.5f, -s * 0.4f ), vec2( 0, -s * 0.15f ) );
    // Right side curve
    heartPath.curveTo( vec2( s * 0.5f, -s * 0.4f ), vec2( s * 0.5f, -s * 0.1f ), vec2( 0, s * 0.3f ) );
    heartPath.close();
    Shape2d heartShape;
    heartShape.appendContour( heartPath );
    mHeartPath = mCanvas->createPath( heartShape );

    // Gear path
    Path2d gearPath;
    float gearOuterR = 35, gearInnerR = 25;
    int teeth = 8;
    for( int i = 0; i < teeth * 2; ++i ) {
        float angle = i * M_PI / teeth;
        float r = ( i % 2 == 0 ) ? gearOuterR : gearInnerR;
        vec2 pt = vec2( std::cos( angle ), std::sin( angle ) ) * r;
        if( i == 0 )
            gearPath.moveTo( pt );
        else
            gearPath.lineTo( pt );
    }
    gearPath.close();
    mGearPath = mCanvas->createPath( gearPath );
}

void VectorGraphicsApp::createTestImage()
{
    // Create a simple test image (checkerboard pattern)
    Surface8u surface( 64, 64, true );
    auto iter = surface.getIter();
    while( iter.line() ) {
        while( iter.pixel() ) {
            int x = iter.x();
            int y = iter.y();
            bool isWhite = ((x / 8) + (y / 8)) % 2 == 0;
            if( isWhite ) {
                iter.r() = 255; iter.g() = 200; iter.b() = 100; iter.a() = 255;
            } else {
                iter.r() = 100; iter.g() = 150; iter.b() = 255; iter.a() = 255;
            }
        }
    }
    mTestImage = mCanvas->createImage( surface );
}

void VectorGraphicsApp::update()
{
    mFrameCount++;

    // Calculate FPS
    double currentTime = getElapsedSeconds();
    double deltaTime = currentTime - mLastFrameTime;
    mLastFrameTime = currentTime;

    if( deltaTime > 0 ) {
        mFps = 1.0f / deltaTime;
        // Smooth FPS with exponential moving average
        mFpsSmoothed = mFpsSmoothed * 0.95f + mFps * 0.05f;
    }

    mAnimTime += deltaTime;

    // Auto-save screenshot on frame 5 to capture after rendering stabilizes
    if( mFrameCount == 5 ) {
        mSaveScreenshot = true;
    }
}

void VectorGraphicsApp::resize()
{
    // CanvasUi automatically handles resize via its window connection
}

void VectorGraphicsApp::saveScreenshot()
{
    try {
        Surface8u screenshot = copyWindowSurface();
        Surface8u resized = ip::resizeCopy( screenshot, screenshot.getBounds(), ivec2( 320, 240 ) );
        fs::path path = "/tmp/vg_screenshot.jpg";
        writeImage( path, resized );
        CI_LOG_I( "Screenshot saved to: " << path );
    }
    catch( const std::exception &exc ) {
        CI_LOG_E( "Failed to save screenshot: " << exc.what() );
    }
}

void VectorGraphicsApp::draw()
{
    // Clear with Cinder first
    gl::clear( Color( 0.15f, 0.15f, 0.18f ) );

    if( ! mVgInitialized ) {
        // Fallback - can't use canvas if not initialized
        return;
    }

    // === Begin Rive vector graphics frame ===
    // Use toPixels for HiDPI/Retina displays - otherwise FBO is at logical size, not physical pixels
    mCanvas->beginFrame( toPixels( getWindowSize() ) );

    // Apply CanvasUi transform to the canvas
    // Convert mat4 to mat3 properly: mat4 has translation in column 3, mat3 needs it in column 2
    const mat4& m4 = mCanvasUi.getModelMatrix();
    mat3 m3;
    m3[0][0] = m4[0][0]; m3[0][1] = m4[0][1]; m3[0][2] = 0;
    m3[1][0] = m4[1][0]; m3[1][1] = m4[1][1]; m3[1][2] = 0;
    m3[2][0] = m4[3][0]; m3[2][1] = m4[3][1]; m3[2][2] = 1;  // Translation from mat4 column 3
    mCanvas->setTransform( m3 );

    // Draw all vector content
    drawVectorContent();

    // === Draw UI overlay (reset transform to screen space) ===
    mCanvas->setTransform( mat3( 1.0f ) );

    // FPS indicator
    {
        Font fpsFont( "Arial", 16 );
        vg::Paint fpsPaint;
        fpsPaint.setColor( ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) );

        char fpsText[32];
        snprintf( fpsText, sizeof(fpsText), "FPS: %.0f", mFpsSmoothed );
        mCanvas->drawString( fpsText, vec2( getWindowWidth() - 80, 20 ), fpsFont, fpsPaint );
    }

    // === End frame ===
    mCanvas->endFrame();

    // Save screenshot if requested
    if( mSaveScreenshot ) {
        mSaveScreenshot = false;
        saveScreenshot();
    }
}

void VectorGraphicsApp::drawVectorContent()
{
    // === ROW 1: Basic shapes with solid colors ===
    {
        vg::Paint redPaint;
        redPaint.setColor( ColorAf( 1.0f, 0.2f, 0.2f, 1.0f ) );
        mCanvas->fillRect( Rectf( 30, 60, 130, 130 ), redPaint );

        vg::Paint greenPaint;
        greenPaint.setColor( ColorAf( 0.2f, 0.9f, 0.3f, 1.0f ) );
        mCanvas->fillCircle( vec2( 200, 95 ), 40, greenPaint );

        vg::Paint bluePaint;
        bluePaint.setColor( ColorAf( 0.3f, 0.5f, 1.0f, 1.0f ) );
        mCanvas->fillEllipse( vec2( 310, 95 ), vec2( 50, 35 ), bluePaint );

        vg::Paint purplePaint;
        purplePaint.setColor( ColorAf( 0.7f, 0.3f, 0.9f, 0.7f ) );
        mCanvas->fillRoundedRect( Rectf( 380, 60, 480, 130 ), 15, purplePaint );
    }

    // === ROW 2: Strokes ===
    {
        vg::Paint strokePaint;
        strokePaint.setColor( ColorAf( 1.0f, 0.8f, 0.2f, 1.0f ) ).setStrokeWidth( 4.0f );
        mCanvas->strokeRect( Rectf( 30, 160, 130, 230 ), strokePaint );

        vg::Paint circleStroke;
        circleStroke.setColor( ColorAf( 0.2f, 0.9f, 0.9f, 1.0f ) )
                    .setStrokeWidth( 5.0f )
                    .setLineCap( vg::LineCap::Round );
        mCanvas->strokeCircle( vec2( 200, 195 ), 35, circleStroke );

        vg::Paint linePaint;
        linePaint.setColor( ColorAf( 1.0f, 0.5f, 0.3f, 1.0f ) )
                 .setStrokeWidth( 8.0f )
                 .setLineCap( vg::LineCap::Round );
        mCanvas->drawLine( vec2( 270, 170 ), vec2( 350, 220 ), linePaint );

        vg::Paint thickStroke;
        thickStroke.setColor( ColorAf( 0.9f, 0.3f, 0.6f, 1.0f ) )
                   .setStrokeWidth( 6.0f )
                   .setLineJoin( vg::LineJoin::Round );
        mCanvas->strokeRoundedRect( Rectf( 380, 160, 480, 230 ), 12, thickStroke );
    }

    // === ROW 3: Gradients ===
    {
        vg::Paint linearGrad;
        linearGrad.setLinearGradient( vec2( 30, 280 ), vec2( 130, 340 ),
                                       ColorAf( 1.0f, 0.2f, 0.2f, 1.0f ),
                                       ColorAf( 0.2f, 0.2f, 1.0f, 1.0f ) );
        mCanvas->fillRect( Rectf( 30, 270, 130, 340 ), linearGrad );

        vg::Paint radialGrad;
        radialGrad.setRadialGradient( vec2( 200, 305 ), 45,
                                       ColorAf( 1.0f, 1.0f, 0.3f, 1.0f ),
                                       ColorAf( 0.2f, 0.5f, 0.1f, 1.0f ) );
        mCanvas->fillCircle( vec2( 200, 305 ), 40, radialGrad );

        std::vector<vg::GradientStop> stops = {
            vg::GradientStop( 0.0f, ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) ),
            vg::GradientStop( 0.5f, ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) ),
            vg::GradientStop( 1.0f, ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) )
        };
        vg::Paint multiGrad;
        multiGrad.setLinearGradient( vec2( 270, 270 ), vec2( 360, 340 ), stops );
        mCanvas->fillRoundedRect( Rectf( 270, 270, 360, 340 ), 10, multiGrad );
    }

    // === ROW 4: Feathering (soft edges) ===
    {
        vg::Paint featherPaint;
        featherPaint.setColor( ColorAf( 0.3f, 0.7f, 1.0f, 1.0f ) ).setFeather( 15.0f );
        mCanvas->fillCircle( vec2( 80, 415 ), 45, featherPaint );

        vg::Paint featherRect;
        featherRect.setColor( ColorAf( 1.0f, 0.5f, 0.8f, 1.0f ) ).setFeather( 10.0f );
        mCanvas->fillRect( Rectf( 150, 380, 250, 450 ), featherRect );
    }

    // === ROW 5: Transforms (animated) ===
    {
        mCanvas->save();
        mCanvas->translate( vec2( 400, 400 ) );
        mCanvas->rotate( mAnimTime * 0.5f );
        vg::Paint spinPaint;
        spinPaint.setColor( ColorAf( 0.9f, 0.6f, 0.2f, 1.0f ) );
        mCanvas->fillRect( Rectf( -30, -30, 30, 30 ), spinPaint );
        mCanvas->restore();

        mCanvas->save();
        mCanvas->translate( vec2( 550, 400 ) );
        float scaleAmt = 0.7f + 0.3f * std::sin( mAnimTime * 2.0f );
        mCanvas->scale( vec2( scaleAmt, scaleAmt ) );
        vg::Paint scalePaint;
        scalePaint.setColor( ColorAf( 0.5f, 0.9f, 0.5f, 1.0f ) );
        mCanvas->fillCircle( vec2( 0, 0 ), 30, scalePaint );
        mCanvas->restore();
    }

    // === CACHED PATHS (Star, Heart, Gear) ===
    {
        // Star
        mCanvas->save();
        mCanvas->translate( vec2( 700, 95 ) );
        vg::Paint starPaint;
        starPaint.setColor( ColorAf( 1.0f, 0.85f, 0.0f, 1.0f ) );
        mCanvas->fillPath( mStarPath, starPaint );
        vg::Paint starStroke;
        starStroke.setColor( ColorAf( 0.6f, 0.4f, 0.0f, 1.0f ) ).setStrokeWidth( 2.0f );
        mCanvas->strokePath( mStarPath, starStroke );
        mCanvas->restore();

        // Heart
        mCanvas->save();
        mCanvas->translate( vec2( 700, 205 ) );
        vg::Paint heartPaint;
        heartPaint.setColor( ColorAf( 1.0f, 0.2f, 0.4f, 1.0f ) );
        mCanvas->fillPath( mHeartPath, heartPaint );
        mCanvas->restore();

        // Gear (rotating)
        mCanvas->save();
        mCanvas->translate( vec2( 700, 305 ) );
        mCanvas->rotate( mAnimTime * 0.3f );
        vg::Paint gearPaint;
        gearPaint.setColor( ColorAf( 0.5f, 0.5f, 0.6f, 1.0f ) );
        mCanvas->fillPath( mGearPath, gearPaint );
        vg::Paint gearStroke;
        gearStroke.setColor( ColorAf( 0.3f, 0.3f, 0.4f, 1.0f ) ).setStrokeWidth( 2.0f );
        mCanvas->strokePath( mGearPath, gearStroke );
        mCanvas->restore();
    }

    // === IMAGE DRAWING ===
    if( mTestImage ) {
        // Simple image draw
        mCanvas->drawImage( mTestImage, vec2( 550, 50 ) );

        // Scaled image
        mCanvas->drawImage( mTestImage, Rectf( 550, 130, 630, 210 ) );

        // Image with srcRect (show only part)
        mCanvas->drawImage( mTestImage, Rectf( 0, 0, 32, 32 ), Rectf( 550, 230, 610, 290 ) );
    }

    // === TEXT RENDERING ===
    // Note: Text rendering uses Font::getGlyphShape which may not work on all fonts
    // Commenting out for now until we verify glyph extraction works
    /*
    {
        vg::Paint textPaint;
        textPaint.setColor( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
        mCanvas->drawString( "Hello Canvas!", vec2( 30, 500 ), mFont, textPaint );

        vg::Paint gradTextPaint;
        gradTextPaint.setLinearGradient( vec2( 30, 540 ), vec2( 200, 540 ),
                                          ColorAf( 1.0f, 0.5f, 0.0f, 1.0f ),
                                          ColorAf( 1.0f, 1.0f, 0.0f, 1.0f ) );
        mCanvas->drawString( "Gradient Text", vec2( 30, 540 ), mFont, gradTextPaint );

        // Text as path (for stroking)
        auto textPath = mCanvas->createTextPath( "Stroked", mFont );
        if( textPath ) {
            mCanvas->save();
            mCanvas->translate( vec2( 30, 590 ) );
            vg::Paint textStroke;
            textStroke.setColor( ColorAf( 0.2f, 0.8f, 1.0f, 1.0f ) ).setStrokeWidth( 1.5f );
            mCanvas->strokePath( textPath, textStroke );
            mCanvas->restore();
        }
    }
    */

    // === INSTANCED DRAWING ===
    {
        // Draw 200 circles at different positions (same size, same paint)
        vg::Paint particlePaint;
        particlePaint.setColor( ColorAf( 0.3f, 0.8f, 1.0f, 0.6f ) );
        mCanvas->drawCircles( mParticlePositions, 4.0f, particlePaint );

        // Draw paths with full transforms
        vg::Paint starParticlePaint;
        starParticlePaint.setColor( ColorAf( 1.0f, 0.9f, 0.2f, 0.7f ) );
        mCanvas->drawPaths( mParticleTransforms, mStarPath, starParticlePaint );
    }

    // === LABELS (in content space) ===
    {
        vg::Paint labelPaint;
        labelPaint.setColor( ColorAf( 0.7f, 0.7f, 0.7f, 1.0f ) );

        Font smallFont( "Arial", 14 );
        mCanvas->drawString( "Fills", vec2( 10, 45 ), smallFont, labelPaint );
        mCanvas->drawString( "Strokes", vec2( 10, 145 ), smallFont, labelPaint );
        mCanvas->drawString( "Gradients", vec2( 10, 255 ), smallFont, labelPaint );
        mCanvas->drawString( "Feather", vec2( 10, 365 ), smallFont, labelPaint );
        mCanvas->drawString( "Transform", vec2( 380, 365 ), smallFont, labelPaint );
        mCanvas->drawString( "Cached", vec2( 650, 45 ), smallFont, labelPaint );
        mCanvas->drawString( "Images", vec2( 550, 40 ), smallFont, labelPaint );
        mCanvas->drawString( "Instanced", vec2( 850, 40 ), smallFont, labelPaint );
    }

    // === CONTENT BOUNDS OUTLINE ===
    {
        vg::Paint boundsPaint;
        boundsPaint.setColor( ColorAf( 0.3f, 0.3f, 0.4f, 1.0f ) ).setStrokeWidth( 2.0f );
        mCanvas->strokeRect( mContentBounds, boundsPaint );
    }
}

void VectorGraphicsApp::keyDown( KeyEvent event )
{
    if( event.getChar() == 'q' || event.getCode() == KeyEvent::KEY_ESCAPE ) {
        quit();
    }
    else if( event.getChar() == 's' ) {
        mSaveScreenshot = true;
    }
    else if( event.getChar() == '0' ) {
        mCanvasUi.fitAll();
    }
}

void prepareSettings( VectorGraphicsApp::Settings* settings )
{
    settings->setTitle( "VectorGraphics - CanvasGl + CanvasUi Demo" );
    settings->setWindowSize( 1200, 800 );
    settings->setResizable( true );
}

#if defined( CINDER_MAC )
// macOS: Request GL 4.1 core profile
CINDER_APP( VectorGraphicsApp, RendererGl( RendererGl::Options().version(4,1).coreProfile().msaa(16) ), prepareSettings )
#else
// Windows/Linux: Request GL 4.3+ for full feature set
CINDER_APP( VectorGraphicsApp, RendererGl( RendererGl::Options().version(4,3).coreProfile().msaa(8) ), prepareSettings )
#endif
