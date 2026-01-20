/*
 VectorGraphicsBasicD3d12 - Minimal example of the vg::CanvasD3d12 API

 Demonstrates 3 shapes with different fill styles:
 - Circle with solid color
 - Path2d (star) with gradient
 - Rectangle with feathering (soft edges)
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererD3d12.h"

#include "cinder/vg/CanvasD3d12.h"
#include "cinder/vg/Paint.h"

#include <d3d12.h>

using namespace ci;
using namespace ci::app;

class VectorGraphicsBasicD3d12App : public App {
public:
    void setup() override;
    void resize() override;
    void draw() override;

private:
    app::RendererD3d12Ref mRenderer;
    vg::CanvasD3d12Ref    mCanvas;
    vg::CachedPathRef     mStarPath;
};

void VectorGraphicsBasicD3d12App::setup()
{
    mRenderer = std::dynamic_pointer_cast<RendererD3d12>( getRenderer() );
    mCanvas = vg::CanvasD3d12::create( mRenderer );

    // Release render targets before swap chain buffers are recreated
    mRenderer->getSignalSwapChainBuffersWillRecreate().connect( [this]() {
        if( mCanvas )
            mCanvas->releaseRenderTargets();
    } );

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

void VectorGraphicsBasicD3d12App::resize()
{
    // Render target release handled by getSignalSwapChainBuffersWillRecreate() in setup()
}

void VectorGraphicsBasicD3d12App::draw()
{
    if( ! mRenderer || ! mCanvas )
        return;

    // Get back buffer - may be null during resize
    ID3D12Resource* backBuffer = mRenderer->getCurrentBackBuffer();
    if( ! backBuffer )
        return;

    // Use back buffer dimensions (may differ from window size during resize)
    D3D12_RESOURCE_DESC bufferDesc = backBuffer->GetDesc();
    ivec2 bufferSize( static_cast<int>( bufferDesc.Width ), static_cast<int>( bufferDesc.Height ) );

    mCanvas->begin( bufferSize );

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
        Font font( "Arial", 18 );
        vg::Paint textPaint;
        textPaint.setColor( ColorAf( 0.8f, 0.8f, 0.8f, 1.0f ) );

        mCanvas->drawString( "Solid Fill", vec2( 110, 420 ), font, textPaint );
        mCanvas->drawString( "Gradient", vec2( 360, 420 ), font, textPaint );
        mCanvas->drawString( "Feathering", vec2( 605, 420 ), font, textPaint );
    }

    mCanvas->end();
}

CINDER_APP( VectorGraphicsBasicD3d12App, RendererD3d12( RendererD3d12::Options().debugLayer( true ) ),
    []( App::Settings* settings ) {
        settings->setTitle( "VectorGraphicsBasicD3d12" );
        settings->setWindowSize( 880, 600 );
    }
)
