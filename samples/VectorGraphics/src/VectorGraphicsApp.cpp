/*
 VectorGraphics Sample

 Demonstrates the Rive-based vector graphics renderer integrated with Cinder.
 Uses the CanvasGl and Paint API.
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Surface.h"
#include "cinder/ip/Resize.h"
#include "cinder/ImageIo.h"

#include "cinder/vg/CanvasGl.h"
#include "cinder/vg/Paint.h"

using namespace ci;
using namespace ci::app;

class VectorGraphicsApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyDown( KeyEvent event ) override;
    void saveScreenshot();

private:
    vg::CanvasGlRef mCanvas;
    bool mVgInitialized = false;
    bool mSaveScreenshot = false;
    int mFrameCount = 0;
    float mAnimTime = 0;
};

void VectorGraphicsApp::setup()
{
    CI_LOG_I( "VectorGraphics sample starting..." );
    CI_LOG_I( "OpenGL Version: " << glGetString( GL_VERSION ) );
    CI_LOG_I( "OpenGL Renderer: " << glGetString( GL_RENDERER ) );

    try {
        // Create the canvas with options
        vg::CanvasOptions options;
        #if defined( CINDER_MAC )
        // macOS GL 4.1 doesn't support PLS or FSI - force fallback rendering mode
        options.disablePixelLocalStorage = true;
        options.disableFragmentShaderInterlock = true;
        #endif

        // Create canvas for screen rendering (no FBO)
        mCanvas = vg::createCanvasGl( options );
        mVgInitialized = true;

        CI_LOG_I( "Vector graphics initialized successfully!" );
    }
    catch( const vg::VgExc &exc ) {
        CI_LOG_E( "Failed to initialize vector graphics: " << exc.what() );
    }
    catch( const std::exception &exc ) {
        CI_LOG_E( "Exception during setup: " << exc.what() );
    }
}

void VectorGraphicsApp::update()
{
    mFrameCount++;
    mAnimTime += 0.016f; // ~60 FPS

    // Auto-save screenshot on frame 5 to capture after rendering stabilizes
    if( mFrameCount == 5 ) {
        mSaveScreenshot = true;
    }
}

void VectorGraphicsApp::saveScreenshot()
{
    try {
        // Capture the screen
        Surface8u screenshot = copyWindowSurface();

        // Resize to 320x240
        Surface8u resized = ip::resizeCopy( screenshot, screenshot.getBounds(), ivec2( 320, 240 ) );

        // Save as JPEG to /tmp
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
    gl::setMatricesWindow( getWindowSize() );
    gl::viewport( 0, 0, getWindowWidth(), getWindowHeight() );

    if( ! mVgInitialized ) {
        // Draw fallback if VG not initialized
        gl::color( 1.0, 1.0, 1.0 );
        gl::drawStringCentered( "Vector Graphics not initialized - check console for errors",
                                 getWindowCenter(),
                                 Color::white() );
        return;
    }

    // === Begin Rive vector graphics frame ===
    mCanvas->beginFrame( getWindowSize() );

    // --- Row 1: Basic shapes with solid colors ---

    // Red rectangle
    vg::Paint redPaint;
    redPaint.setColor( ColorAf( 1.0f, 0.2f, 0.2f, 1.0f ) );
    mCanvas->fillRect( Rectf( 30, 60, 130, 130 ), redPaint );

    // Green circle
    vg::Paint greenPaint;
    greenPaint.setColor( ColorAf( 0.2f, 0.9f, 0.3f, 1.0f ) );
    mCanvas->fillCircle( vec2( 200, 95 ), 40, greenPaint );

    // Blue ellipse
    vg::Paint bluePaint;
    bluePaint.setColor( ColorAf( 0.3f, 0.5f, 1.0f, 1.0f ) );
    mCanvas->fillEllipse( vec2( 310, 95 ), vec2( 50, 35 ), bluePaint );

    // Rounded rectangle with transparency
    vg::Paint purplePaint;
    purplePaint.setColor( ColorAf( 0.7f, 0.3f, 0.9f, 0.7f ) );
    mCanvas->fillRoundedRect( Rectf( 380, 60, 480, 130 ), 15, purplePaint );

    // --- Row 2: Strokes ---

    // Stroked rectangle
    vg::Paint strokePaint;
    strokePaint.setColor( ColorAf( 1.0f, 0.8f, 0.2f, 1.0f ) )
               .setStrokeWidth( 4.0f );
    mCanvas->strokeRect( Rectf( 30, 160, 130, 230 ), strokePaint );

    // Stroked circle with round caps
    vg::Paint circleStroke;
    circleStroke.setColor( ColorAf( 0.2f, 0.9f, 0.9f, 1.0f ) )
                .setStrokeWidth( 5.0f )
                .setLineCap( vg::LineCap::Round );
    mCanvas->strokeCircle( vec2( 200, 195 ), 35, circleStroke );

    // Line with different caps
    vg::Paint linePaint;
    linePaint.setColor( ColorAf( 1.0f, 0.5f, 0.3f, 1.0f ) )
             .setStrokeWidth( 8.0f )
             .setLineCap( vg::LineCap::Round );
    mCanvas->drawLine( vec2( 270, 170 ), vec2( 350, 220 ), linePaint );

    // Thick stroked rounded rect
    vg::Paint thickStroke;
    thickStroke.setColor( ColorAf( 0.9f, 0.3f, 0.6f, 1.0f ) )
               .setStrokeWidth( 6.0f )
               .setLineJoin( vg::LineJoin::Round );
    mCanvas->strokeRoundedRect( Rectf( 380, 160, 480, 230 ), 12, thickStroke );

    // --- Row 3: Gradients ---

    // Linear gradient
    vg::Paint linearGrad;
    linearGrad.setLinearGradient( vec2( 30, 280 ), vec2( 130, 340 ),
                                   ColorAf( 1.0f, 0.2f, 0.2f, 1.0f ),
                                   ColorAf( 0.2f, 0.2f, 1.0f, 1.0f ) );
    mCanvas->fillRect( Rectf( 30, 270, 130, 340 ), linearGrad );

    // Radial gradient
    vg::Paint radialGrad;
    radialGrad.setRadialGradient( vec2( 200, 305 ), 45,
                                   ColorAf( 1.0f, 1.0f, 0.3f, 1.0f ),
                                   ColorAf( 0.2f, 0.5f, 0.1f, 1.0f ) );
    mCanvas->fillCircle( vec2( 200, 305 ), 40, radialGrad );

    // Multi-stop linear gradient
    vg::Paint multiGrad;
    std::vector<vg::GradientStop> stops = {
        vg::GradientStop( 0.0f, ColorAf( 1.0f, 0.0f, 0.0f, 1.0f ) ),
        vg::GradientStop( 0.5f, ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ) ),
        vg::GradientStop( 1.0f, ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ) )
    };
    multiGrad.setLinearGradient( vec2( 270, 270 ), vec2( 360, 340 ), stops );
    mCanvas->fillRoundedRect( Rectf( 270, 270, 360, 340 ), 10, multiGrad );

    // --- Row 4: Feathering (soft edges) ---

    // Feathered circle
    vg::Paint featherPaint;
    featherPaint.setColor( ColorAf( 0.3f, 0.7f, 1.0f, 1.0f ) )
                .setFeather( 15.0f );
    mCanvas->fillCircle( vec2( 80, 415 ), 45, featherPaint );

    // Feathered rectangle
    vg::Paint featherRect;
    featherRect.setColor( ColorAf( 1.0f, 0.5f, 0.8f, 1.0f ) )
               .setFeather( 10.0f );
    mCanvas->fillRect( Rectf( 150, 380, 250, 450 ), featherRect );

    // --- Row 5: Transforms ---
    mCanvas->save();
    mCanvas->translate( vec2( 400, 400 ) );
    mCanvas->rotate( mAnimTime * 0.5f );

    vg::Paint spinPaint;
    spinPaint.setColor( ColorAf( 0.9f, 0.6f, 0.2f, 1.0f ) );
    mCanvas->fillRect( Rectf( -30, -30, 30, 30 ), spinPaint );

    mCanvas->restore();

    // Scaled shape
    mCanvas->save();
    mCanvas->translate( vec2( 550, 400 ) );
    float scaleAmt = 0.7f + 0.3f * std::sin( mAnimTime * 2.0f );
    mCanvas->scale( vec2( scaleAmt, scaleAmt ) );

    vg::Paint scalePaint;
    scalePaint.setColor( ColorAf( 0.5f, 0.9f, 0.5f, 1.0f ) );
    mCanvas->fillCircle( vec2( 0, 0 ), 30, scalePaint );

    mCanvas->restore();

    // --- Path2d example ---
    Path2d starPath;
    vec2 starCenter( 700, 120 );
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

    vg::Paint starPaint;
    starPaint.setColor( ColorAf( 1.0f, 0.85f, 0.0f, 1.0f ) );
    mCanvas->fillPath( starPath, starPaint );

    // Stroked path
    vg::Paint starStroke;
    starStroke.setColor( ColorAf( 0.6f, 0.4f, 0.0f, 1.0f ) )
              .setStrokeWidth( 2.0f );
    mCanvas->strokePath( starPath, starStroke );

    // === End frame ===
    mCanvas->endFrame();

    // Draw Cinder content AFTER Rive to verify GL state is properly restored
    {
        gl::ScopedGlslProg scpShader( gl::getStockShader( gl::ShaderDef().color() ) );
        gl::ScopedColor scpColor( 1.0f, 1.0f, 1.0f, 1.0f );

        // Draw a white rectangle to verify Cinder Batch drawing works after Rive
        auto rect = geom::Rect().rect( Rectf( 720, 180, 780, 240 ) );
        auto batch = gl::Batch::create( rect, gl::getStockShader( gl::ShaderDef().color() ) );
        batch->draw();

        // Draw a white circle to verify gl::drawSolidCircle() works after Rive
        gl::drawSolidCircle( vec2( 750, 300 ), 25 );
    }

    // Draw labels with Cinder's built-in text renderer
    gl::drawString( "CanvasGl + Paint API Demo", vec2( 10, 10 ), Color::white() );
    gl::drawString( "Row 1: Solid fills (rect, circle, ellipse, rounded rect)", vec2( 10, 35 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Row 2: Strokes", vec2( 10, 145 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Row 3: Gradients", vec2( 10, 255 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Row 4: Feathering", vec2( 10, 365 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Path2d star", vec2( 660, 165 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Cinder GL", vec2( 715, 325 ), Color( 0.7f, 0.7f, 0.7f ) );
    gl::drawString( "Transforms (animated)", vec2( 380, 365 ), Color( 0.7f, 0.7f, 0.7f ) );

    gl::drawString( "Press 's' for screenshot, 'q' to quit", vec2( 10, getWindowHeight() - 20 ), Color::white() );

    // Save screenshot if requested (after drawing completes)
    if( mSaveScreenshot ) {
        mSaveScreenshot = false;
        saveScreenshot();
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
}

void prepareSettings( VectorGraphicsApp::Settings* settings )
{
    settings->setTitle( "VectorGraphics - CanvasGl Demo" );
    settings->setWindowSize( 800, 500 );
}

#if defined( CINDER_MAC )
// macOS: Request GL 4.1 core profile
CINDER_APP( VectorGraphicsApp, RendererGl( RendererGl::Options().version(4,1).coreProfile() ), prepareSettings )
#else
// Windows/Linux: Request GL 4.3+ for full feature set
CINDER_APP( VectorGraphicsApp, RendererGl( RendererGl::Options().version(4,3).coreProfile() ), prepareSettings )
#endif
