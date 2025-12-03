/*
 Copyright (c) 2024, The Cinder Project
 */

#include "cinder/vg/CanvasGl.h"
#include "cinder/Log.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/scoped.h"
#include "cinder/gl/Context.h"

// Rive includes
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/math/raw_path.hpp"

// For Rive's GLAD loader initialization
#include "glad_custom.h"

// For GLFW's glfwGetProcAddress
#include "GLFW/glfw3.h"

using namespace rive;
using namespace rive::gpu;

namespace cinder { namespace vg {

// Helper to convert Cinder mat3 to Rive Mat2D
static Mat2D toRiveMat( const mat3 &m )
{
    // Cinder mat3 is column-major, Rive Mat2D is [xx, xy, yx, yy, tx, ty]
    return Mat2D( m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1] );
}

// Convert FillRule to Rive FillRule
static rive::FillRule toRiveFillRule( FillRule rule )
{
    return rule == FillRule::EvenOdd ? rive::FillRule::evenOdd : rive::FillRule::nonZero;
}

// ------------------------------------------------------------------------------------------------
// CanvasGl implementation
// ------------------------------------------------------------------------------------------------

CanvasGl::CanvasGl( const CanvasOptions &options )
    : mFbo( nullptr )
{
    initializeGl( options );
}

CanvasGl::CanvasGl( const gl::FboRef &fbo, const CanvasOptions &options )
    : mFbo( fbo )
{
    initializeGl( options );
}

CanvasGl::~CanvasGl()
{
    if( mInFrame ) {
        CI_LOG_W( "CanvasGl destroyed while still in frame" );
    }
    mRiveRenderer.reset();
    mRiveContext.reset();
}

void CanvasGl::initializeGl( const CanvasOptions &options )
{
    CI_LOG_I( "Creating Rive GL context with disablePLS=" << options.disablePixelLocalStorage
              << " disableFSI=" << options.disableFragmentShaderInterlock );

    // Initialize Rive's GLAD loader - this sets up GLAD_GL_version_major etc.
    gladLoadCustomLoader( (GLADloadfunc)glfwGetProcAddress );

    RenderContextGLImpl::ContextOptions riveOptions;
    riveOptions.disablePixelLocalStorage = options.disablePixelLocalStorage;
    riveOptions.disableFragmentShaderInterlock = options.disableFragmentShaderInterlock;

    mRiveContext = RenderContextGLImpl::MakeContext( riveOptions );

    if( ! mRiveContext ) {
        throw VgExc( "Failed to create Rive GL RenderContext" );
    }

    // Store the GL-specific pointer for invalidateGLState()
    mRiveContextGl = mRiveContext->static_impl_cast<RenderContextGLImpl>();

    CI_LOG_I( "CanvasGl created successfully" );
}

void CanvasGl::invalidateState()
{
    if( mRiveContextGl ) {
        mRiveContextGl->invalidateGLState();
    }
}

void CanvasGl::restoreHostState( const ivec2 &size )
{
    // First unbind Rive's internal GL resources
    if( mRiveContextGl ) {
        mRiveContextGl->unbindGLInternalResources();
    }

    // Clear any pending GL errors
    while( glGetError() != GL_NO_ERROR ) {}

    // === Reset raw GL state ===

    // Buffer bindings
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_UNIFORM_BUFFER, 0 );

    // Framebuffer - restore to screen or FBO
    if( mFbo ) {
        mFbo->bindFramebuffer();
    } else {
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    }

    // Reset all texture units and samplers
    for( int i = 0; i < 8; ++i ) {
        glActiveTexture( GL_TEXTURE0 + i );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glBindSampler( i, 0 );
    }
    glActiveTexture( GL_TEXTURE0 );

    // Rive sets GL_CW winding, Cinder expects GL_CCW (default)
    glFrontFace( GL_CCW );

    // Reset enable states
    glDisable( GL_CULL_FACE );
    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_STENCIL_TEST );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glDepthMask( GL_TRUE );

    // Reset line width that Rive's wireframe mode may change
    glLineWidth( 1.0f );

    // Re-enable blending with Cinder's expected blend func
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // === Synchronize Cinder's internal GL state tracking ===
    auto* ctx_gl = gl::context();
    if( ctx_gl ) {
        // Invalidate buffer binding caches
        ctx_gl->invalidateBufferBindingCache( GL_ARRAY_BUFFER );
        ctx_gl->invalidateBufferBindingCache( GL_ELEMENT_ARRAY_BUFFER );
        ctx_gl->invalidateBufferBindingCache( GL_UNIFORM_BUFFER );

        // Reset actual GL state for VAO and program
        glUseProgram( 0 );
        glBindVertexArray( 0 );

        // Tell Cinder its tracking is invalid
        ctx_gl->bindGlslProg( nullptr );
        ctx_gl->bindVao( nullptr );

        // Bind Cinder's default VAO
        gl::Vao* defaultVao = ctx_gl->getDefaultVao();
        if( defaultVao ) {
            ctx_gl->bindVao( defaultVao );
        }

        // Get and bind the stock shader
        auto defaultShader = gl::getStockShader( gl::ShaderDef().color() );
        if( defaultShader ) {
            defaultShader->bind();
        }

        // Unbind textures using Cinder's tracking to keep cache in sync
        for( uint8_t i = 0; i < 8; ++i ) {
            ctx_gl->bindTexture( GL_TEXTURE_2D, 0, i );
            ctx_gl->bindSampler( i, 0 );
        }
        ctx_gl->setActiveTexture( 0 );
    }

    // Reset viewport and matrices AFTER binding shader so uniforms get set
    gl::viewport( 0, 0, size.x, size.y );
    gl::setMatricesWindow( size );
}

void CanvasGl::setFbo( const gl::FboRef &fbo )
{
    if( mInFrame ) {
        CI_LOG_W( "Cannot change FBO while in frame" );
        return;
    }
    mFbo = fbo;
}

void CanvasGl::beginFrame( const ivec2 &size )
{
    if( mInFrame ) {
        CI_LOG_W( "beginFrame called while already in frame" );
        return;
    }

    mFrameSize = size;
    mInFrame = true;

    if( mRiveContext ) {
        RenderContext::FrameDescriptor frameDesc;
        frameDesc.renderTargetWidth = size.x;
        frameDesc.renderTargetHeight = size.y;
        frameDesc.loadAction = gpu::LoadAction::preserveRenderTarget;
        frameDesc.clearColor = 0x00000000; // Not used when preserving
        frameDesc.clockwiseFillOverride = true; // Enable feathering for any fill rule

        // Invalidate GL state since Cinder may have modified it
        invalidateState();

        mRiveContext->beginFrame( frameDesc );

        // Create RiveRenderer for this frame
        mRiveRenderer = std::make_unique<RiveRenderer>( mRiveContext.get() );
    }
}

void CanvasGl::endFrame()
{
    if( ! mInFrame ) {
        CI_LOG_W( "endFrame called without beginFrame" );
        return;
    }

    // Destroy the renderer before flush
    mRiveRenderer.reset();

    if( mRiveContext ) {
        // Create render target for the appropriate framebuffer
        GLuint framebufferId = 0;
        int sampleCount = 1;

        if( mFbo ) {
            framebufferId = mFbo->getId();
            sampleCount = mFbo->getFormat().getSamples();
            if( sampleCount == 0 ) sampleCount = 1;
        }

        auto renderTarget = make_rcp<FramebufferRenderTargetGL>(
            mFrameSize.x, mFrameSize.y,
            framebufferId,
            sampleCount
        );

        mRiveContext->flush( { .renderTarget = renderTarget.get() } );

        // Restore Cinder's expected GL state
        restoreHostState( mFrameSize );
    }

    mInFrame = false;
}

void CanvasGl::save()
{
    mTransformStack.push_back( mTransform );
}

void CanvasGl::restore()
{
    if( ! mTransformStack.empty() ) {
        mTransform = mTransformStack.back();
        mTransformStack.pop_back();
    }
}

void CanvasGl::translate( const vec2 &offset )
{
    mat3 t( 1.0f );
    t[2][0] = offset.x;
    t[2][1] = offset.y;
    mTransform = mTransform * t;
}

void CanvasGl::rotate( float radians )
{
    float c = std::cos( radians );
    float s = std::sin( radians );
    mat3 r( 1.0f );
    r[0][0] = c;  r[0][1] = s;
    r[1][0] = -s; r[1][1] = c;
    mTransform = mTransform * r;
}

void CanvasGl::scale( const vec2 &s )
{
    mat3 sc( 1.0f );
    sc[0][0] = s.x;
    sc[1][1] = s.y;
    mTransform = mTransform * sc;
}

void CanvasGl::transform( const mat3 &m )
{
    mTransform = mTransform * m;
}

void CanvasGl::setTransform( const mat3 &m )
{
    mTransform = m;
}

void CanvasGl::resetTransform()
{
    mTransform = mat3();
}

void CanvasGl::fillRect( const Rectf &rect, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) {
        CI_LOG_W( "fillRect called outside of frame" );
        return;
    }

    RawPath rawPath;
    rawPath.addRect( AABB{ rect.x1, rect.y1, rect.x2, rect.y2 } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeRect( const Rectf &rect, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.addRect( AABB{ rect.x1, rect.y1, rect.x2, rect.y2 } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillRoundedRect( const Rectf &rect, float radius, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    // Build rounded rect path manually
    RawPath rawPath;
    float x1 = rect.x1, y1 = rect.y1, x2 = rect.x2, y2 = rect.y2;
    float r = std::min( radius, std::min( rect.getWidth(), rect.getHeight() ) / 2.0f );

    rawPath.moveTo( x1 + r, y1 );
    rawPath.lineTo( x2 - r, y1 );
    rawPath.cubicTo( x2 - r * 0.45f, y1, x2, y1 + r * 0.45f, x2, y1 + r );
    rawPath.lineTo( x2, y2 - r );
    rawPath.cubicTo( x2, y2 - r * 0.45f, x2 - r * 0.45f, y2, x2 - r, y2 );
    rawPath.lineTo( x1 + r, y2 );
    rawPath.cubicTo( x1 + r * 0.45f, y2, x1, y2 - r * 0.45f, x1, y2 - r );
    rawPath.lineTo( x1, y1 + r );
    rawPath.cubicTo( x1, y1 + r * 0.45f, x1 + r * 0.45f, y1, x1 + r, y1 );
    rawPath.close();

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeRoundedRect( const Rectf &rect, float radius, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    float x1 = rect.x1, y1 = rect.y1, x2 = rect.x2, y2 = rect.y2;
    float r = std::min( radius, std::min( rect.getWidth(), rect.getHeight() ) / 2.0f );

    rawPath.moveTo( x1 + r, y1 );
    rawPath.lineTo( x2 - r, y1 );
    rawPath.cubicTo( x2 - r * 0.45f, y1, x2, y1 + r * 0.45f, x2, y1 + r );
    rawPath.lineTo( x2, y2 - r );
    rawPath.cubicTo( x2, y2 - r * 0.45f, x2 - r * 0.45f, y2, x2 - r, y2 );
    rawPath.lineTo( x1 + r, y2 );
    rawPath.cubicTo( x1 + r * 0.45f, y2, x1, y2 - r * 0.45f, x1, y2 - r );
    rawPath.lineTo( x1, y1 + r );
    rawPath.cubicTo( x1, y1 + r * 0.45f, x1 + r * 0.45f, y1, x1 + r, y1 );
    rawPath.close();

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillCircle( const vec2 &center, float radius, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radius, center.y - radius,
                          center.x + radius, center.y + radius } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeCircle( const vec2 &center, float radius, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radius, center.y - radius,
                          center.x + radius, center.y + radius } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radii.x, center.y - radii.y,
                          center.x + radii.x, center.y + radii.y } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radii.x, center.y - radii.y,
                          center.x + radii.x, center.y + radii.y } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath;
    rawPath.moveTo( p0.x, p0.y );
    rawPath.lineTo( p1.x, p1.y );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillPath( const Path2d &path, const Paint &paint, FillRule rule )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( path );
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

void CanvasGl::strokePath( const Path2d &path, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( path );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillShape( const Shape2d &shape, const Paint &paint, FillRule rule )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

void CanvasGl::strokeShape( const Shape2d &shape, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::strokePolyLine( const PolyLine2f &polyline, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( polyline );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule )
{
    if( ! mInFrame || ! mRiveRenderer ) return;

    RawPath rawPath = toRivePath( polyline );
    rawPath.close();
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

// ------------------------------------------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------------------------------------------

void CanvasGl::drawPathInternal( RawPath rawPath, const Paint &paint,
                                  bool fill, bool stroke, FillRule rule )
{
    if( ! mRiveContext ) return;

    auto path = mRiveContext->makeRenderPath( rawPath, toRiveFillRule( rule ) );

    if( fill ) {
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), false );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( path.get(), rivePaint.get() );
        mRiveRenderer->restore();
    }

    if( stroke ) {
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), true );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( path.get(), rivePaint.get() );
        mRiveRenderer->restore();
    }
}

RawPath CanvasGl::toRivePath( const Path2d &path )
{
    RawPath rawPath;

    if( path.getNumPoints() == 0 )
        return rawPath;

    // In Cinder Path2d:
    // - Point 0 is always the moveTo point (no corresponding segment)
    // - Segments (LINETO, QUADTO, etc.) consume points starting at index 1
    // - sSegmentTypePointCounts = { 0, 1, 2, 3, 0 } for MOVETO, LINETO, QUADTO, CUBICTO, CLOSE

    // Start with the moveTo point
    vec2 startPt = path.getPoint( 0 );
    rawPath.moveTo( startPt.x, startPt.y );

    // Track point index starting at 1 (after moveTo)
    size_t pointIndex = 1;

    for( size_t seg = 0; seg < path.getNumSegments(); ++seg ) {
        switch( path.getSegmentType( seg ) ) {
            case Path2d::LINETO: {
                vec2 p = path.getPoint( pointIndex++ );
                rawPath.lineTo( p.x, p.y );
                break;
            }
            case Path2d::QUADTO: {
                vec2 p1 = path.getPoint( pointIndex++ );
                vec2 p2 = path.getPoint( pointIndex++ );
                rawPath.quadTo( p1.x, p1.y, p2.x, p2.y );
                break;
            }
            case Path2d::CUBICTO: {
                vec2 p1 = path.getPoint( pointIndex++ );
                vec2 p2 = path.getPoint( pointIndex++ );
                vec2 p3 = path.getPoint( pointIndex++ );
                rawPath.cubicTo( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y );
                break;
            }
            case Path2d::CLOSE:
                rawPath.close();
                break;
            default:
                break;
        }
    }

    if( path.isClosed() && path.getNumSegments() > 0 &&
        path.getSegmentType( path.getNumSegments() - 1 ) != Path2d::CLOSE ) {
        rawPath.close();
    }

    return rawPath;
}

RawPath CanvasGl::toRivePath( const Shape2d &shape )
{
    RawPath rawPath;

    for( const auto& contour : shape.getContours() ) {
        RawPath contourPath = toRivePath( contour );
        rawPath.addPath( contourPath, nullptr ); // nullptr = identity transform
    }

    return rawPath;
}

RawPath CanvasGl::toRivePath( const PolyLine2f &polyline )
{
    RawPath rawPath;

    const auto& points = polyline.getPoints();
    if( points.empty() ) return rawPath;

    rawPath.moveTo( points[0].x, points[0].y );
    for( size_t i = 1; i < points.size(); ++i ) {
        rawPath.lineTo( points[i].x, points[i].y );
    }

    if( polyline.isClosed() ) {
        rawPath.close();
    }

    return rawPath;
}

} } // namespace cinder::vg
