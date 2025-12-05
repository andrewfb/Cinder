/*
 Copyright (c) 2024, The Cinder Project
 */

#include "cinder/vg/CanvasGl.h"
#include "cinder/Log.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/scoped.h"
#include "cinder/gl/Context.h"
#include "cinder/Svg.h"

// Rive includes
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include "rive/math/raw_path.hpp"

// For Rive's GLAD loader initialization
#include "glad/glad_custom.h"

#if defined( CINDER_MSW )
    #include <windows.h>
    // Windows-compatible GetProcAddress for GLAD
    static void* winGlGetProcAddress( const char* name ) {
        void* p = (void*)::wglGetProcAddress( name );
        // wglGetProcAddress returns NULL for OpenGL 1.1 functions
        // We need to load them from opengl32.dll instead
        if( p == nullptr || p == (void*)0x1 || p == (void*)0x2 ||
            p == (void*)0x3 || p == (void*)-1 ) {
            static HMODULE opengl32 = LoadLibraryA( "opengl32.dll" );
            if( opengl32 ) {
                p = (void*)GetProcAddress( opengl32, name );
            }
        }
        return p;
    }
#else
    // For GLFW's glfwGetProcAddress on other platforms
    #include "glfw/glfw3.h"
#endif

using namespace rive;
using namespace rive::gpu;

namespace cinder { namespace vg {

// ------------------------------------------------------------------------------------------------
// Internal implementation structs (pimpl)
// ------------------------------------------------------------------------------------------------

struct CachedPathGlImpl {
    rcp<RenderPath> rivePath;
};

struct ImageGlImpl {
    rcp<RiveRenderImage> riveImage;
};

// ------------------------------------------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------------------------------------------

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
// CachedPathGl implementation
// ------------------------------------------------------------------------------------------------

CachedPathGl::CachedPathGl() : mImpl( std::make_unique<CachedPathGlImpl>() ) {}

CachedPathGl::~CachedPathGl() = default;

// ------------------------------------------------------------------------------------------------
// ImageGl implementation
// ------------------------------------------------------------------------------------------------

ImageGl::ImageGl() : mImpl( std::make_unique<ImageGlImpl>() ) {}

ImageGl::~ImageGl() = default;

// ------------------------------------------------------------------------------------------------
// GlyphCache implementation
// ------------------------------------------------------------------------------------------------

CachedPathRef GlyphCache::getGlyph( CanvasGl* canvas, const Font &font, Font::Glyph glyphIndex )
{
    GlyphKey key{ font.getName(), font.getSize(), glyphIndex };

    auto it = mCache.find( key );
    if( it != mCache.end() ) {
        return it->second;
    }

    // Cache miss - create the glyph path
    try {
        Shape2d glyphShape = font.getGlyphShape( glyphIndex );

        // Skip empty glyphs (like spaces)
        if( glyphShape.getContours().empty() ) {
            mCache[key] = nullptr;
            return nullptr;
        }

        auto cachedPath = canvas->createPath( glyphShape );
        mCache[key] = cachedPath;
        return cachedPath;
    }
    catch( const std::exception& e ) {
        CI_LOG_W( "Failed to create glyph path for index " << glyphIndex << ": " << e.what() );
        mCache[key] = nullptr;
        return nullptr;
    }
}

void GlyphCache::clear()
{
    mCache.clear();
}

// ------------------------------------------------------------------------------------------------
// CanvasGl implementation
// ------------------------------------------------------------------------------------------------

CanvasGl::CanvasGl( const CanvasOptions &options )
    : mFbo( nullptr )
    , mUseFloatingPointBuffer( options.useFloatingPointBuffer )
{
    initializeGl( options );
}

CanvasGl::CanvasGl( const gl::FboRef &fbo, const CanvasOptions &options )
    : mFbo( fbo )
    , mUseFloatingPointBuffer( options.useFloatingPointBuffer )
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
#if defined( CINDER_MSW )
    gladLoadCustomLoader( (GLADloadfunc)winGlGetProcAddress );
#else
    gladLoadCustomLoader( (GLADloadfunc)glfwGetProcAddress );
#endif

    RenderContextGLImpl::ContextOptions riveOptions;
    riveOptions.disablePixelLocalStorage = options.disablePixelLocalStorage;
    riveOptions.disableFragmentShaderInterlock = options.disableFragmentShaderInterlock;

    mRiveContext = RenderContextGLImpl::MakeContext( riveOptions );

    if( ! mRiveContext ) {
        throw VgExc( "Failed to create Rive GL RenderContext" );
    }

    // Store the GL-specific pointer for invalidateGLState()
    mRiveContextGl = mRiveContext->static_impl_cast<RenderContextGLImpl>();

    // Log platform features
    const auto& features = mRiveContext->platformFeatures();
    CI_LOG_I( "CanvasGl created - supportsRasterOrderingMode=" << features.supportsRasterOrderingMode
              << " supportsAtomicMode=" << features.supportsAtomicMode
              << " supportsClockwiseAtomicMode=" << features.supportsClockwiseAtomicMode );

    // CRITICAL: Restore host GL state after Rive initialization
    // Rive's context creation modifies VAOs, VBOs, shaders, etc.
    // We need to restore Cinder/ImGui's expected GL state
    restoreHostState( ivec2( 800, 600 ) );  // Use reasonable default size
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

void CanvasGl::begin( const ivec2 &size )
{
    CI_ASSERT_MSG( ! mInFrame, "begin() called while already in frame - did you forget to call end()?" );

    mFrameSize = size;
    mInFrame = true;

    // Create or resize internal floating-point FBO if needed
    if( mUseFloatingPointBuffer && ! mFbo ) {
        if( ! mInternalFbo || mInternalFbo->getWidth() != size.x || mInternalFbo->getHeight() != size.y ) {
            gl::Fbo::Format format;
            format.colorTexture( gl::Texture::Format()
                .internalFormat( GL_RGBA16F )
                .minFilter( GL_LINEAR )
                .magFilter( GL_LINEAR ) );
            mInternalFbo = gl::Fbo::create( size.x, size.y, format );
            CI_LOG_I( "Created internal RGBA16F FBO: " << size.x << "x" << size.y );
        }
    }

    if( mRiveContext ) {
        RenderContext::FrameDescriptor frameDesc;
        frameDesc.renderTargetWidth = size.x;
        frameDesc.renderTargetHeight = size.y;
        frameDesc.loadAction = gpu::LoadAction::preserveRenderTarget;  // Preserve host app's background
        frameDesc.clockwiseFillOverride = true; // Enable feathering for any fill rule

        // Invalidate GL state since Cinder may have modified it
        invalidateState();

        mRiveContext->beginFrame( frameDesc );

        // Create RiveRenderer for this frame
        mRiveRenderer = std::make_unique<RiveRenderer>( mRiveContext.get() );
    }
}

void CanvasGl::end()
{
    CI_ASSERT_MSG( mInFrame, "end() called without begin()" );

    // Destroy the renderer before flush
    mRiveRenderer.reset();

    if( mRiveContext ) {
        // Create render target for the appropriate framebuffer
        GLuint framebufferId = 0;
        int sampleCount = 1;

        // Determine the target FBO
        gl::FboRef targetFbo = mFbo;  // User-provided FBO
        if( ! targetFbo && mInternalFbo ) {
            targetFbo = mInternalFbo;  // Use internal floating-point FBO
        }

        if( targetFbo ) {
            framebufferId = targetFbo->getId();
            sampleCount = targetFbo->getFormat().getSamples();
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

        // If using internal FBO, blit to screen
        if( mInternalFbo && ! mFbo ) {
            gl::ScopedViewport vp( mFrameSize );
            gl::ScopedMatrices matrices;
            gl::setMatricesWindow( mFrameSize );
            gl::ScopedBlendPremult blend;
            gl::ScopedColor color( Color::white() );
            gl::draw( mInternalFbo->getColorTexture(), Rectf( 0, 0, (float)mFrameSize.x, (float)mFrameSize.y ) );
        }
    }

    mInFrame = false;
}

void CanvasGl::save()
{
    mTransformStack.push_back( mTransform );
    // Also save Rive's clip state
    if( mInFrame && mRiveRenderer ) {
        mRiveRenderer->save();
    }
}

void CanvasGl::restore()
{
    if( ! mTransformStack.empty() ) {
        mTransform = mTransformStack.back();
        mTransformStack.pop_back();
    }
    // Also restore Rive's clip state
    if( mInFrame && mRiveRenderer ) {
        mRiveRenderer->restore();
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

// ------------------------------------------------------------------------------------------------
// Drawing Primitives
// ------------------------------------------------------------------------------------------------

void CanvasGl::fillRect( const Rectf &rect, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end() - call begin() first" );

    RawPath rawPath;
    rawPath.addRect( AABB{ rect.x1, rect.y1, rect.x2, rect.y2 } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeRect( const Rectf &rect, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.addRect( AABB{ rect.x1, rect.y1, rect.x2, rect.y2 } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillRoundedRect( const Rectf &rect, float radius, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

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
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

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
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radius, center.y - radius,
                          center.x + radius, center.y + radius } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeCircle( const vec2 &center, float radius, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radius, center.y - radius,
                          center.x + radius, center.y + radius } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radii.x, center.y - radii.y,
                          center.x + radii.x, center.y + radii.y } );

    drawPathInternal( std::move( rawPath ), paint, true, false );
}

void CanvasGl::strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.addOval( AABB{ center.x - radii.x, center.y - radii.y,
                          center.x + radii.x, center.y + radii.y } );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath;
    rawPath.moveTo( p0.x, p0.y );
    rawPath.lineTo( p1.x, p1.y );

    drawPathInternal( std::move( rawPath ), paint, false, true );
}

// ------------------------------------------------------------------------------------------------
// Path Drawing (uncached)
// ------------------------------------------------------------------------------------------------

void CanvasGl::fillPath( const Path2d &path, const Paint &paint, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( path );
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

void CanvasGl::strokePath( const Path2d &path, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( path );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillShape( const Shape2d &shape, const Paint &paint, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

void CanvasGl::strokeShape( const Shape2d &shape, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::strokePolyLine( const PolyLine2f &polyline, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( polyline );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

void CanvasGl::fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( polyline );
    rawPath.close();
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

// ------------------------------------------------------------------------------------------------
// Cached Path API
// ------------------------------------------------------------------------------------------------

CachedPathRef CanvasGl::createPath( const Path2d &path )
{
    Shape2d shape;
    shape.appendContour( path );
    return createPath( shape );
}

CachedPathRef CanvasGl::createPath( const Shape2d &shape )
{
    if( ! mRiveContext ) {
        CI_LOG_W( "Cannot create path without valid context" );
        return nullptr;
    }

    // Check for empty shapes (e.g., space characters in fonts)
    if( shape.getContours().empty() ) {
        return nullptr;
    }

    // Check if all contours are empty
    bool hasPoints = false;
    for( const auto& contour : shape.getContours() ) {
        if( contour.getNumPoints() > 0 ) {
            hasPoints = true;
            break;
        }
    }
    if( ! hasPoints ) {
        return nullptr;
    }

    auto cachedPath = std::make_shared<CachedPathGl>();
    cachedPath->mSourceShape = shape;
    cachedPath->mBounds = shape.calcBoundingBox();

    RawPath rawPath = toRivePath( shape );
    cachedPath->mImpl->rivePath = mRiveContext->makeRenderPath( rawPath, rive::FillRule::nonZero );

    return cachedPath;
}

void CanvasGl::fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule )
{
    if( ! path ) return;
    auto glPath = std::dynamic_pointer_cast<CachedPathGl>( path );
    if( glPath ) {
        drawCachedPathInternal( glPath.get(), paint, true, false, rule );
    }
}

void CanvasGl::strokePath( const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return;
    auto glPath = std::dynamic_pointer_cast<CachedPathGl>( path );
    if( glPath ) {
        drawCachedPathInternal( glPath.get(), paint, false, true );
    }
}

// ------------------------------------------------------------------------------------------------
// Image API
// ------------------------------------------------------------------------------------------------

ImageRef CanvasGl::createImage( const gl::Texture2dRef &texture )
{
    if( ! mRiveContextGl || ! texture ) {
        CI_LOG_W( "Cannot create image without valid context or texture" );
        return nullptr;
    }

    auto image = std::make_shared<ImageGl>();
    image->mTexture = texture;
    image->mSize = ivec2( texture->getWidth(), texture->getHeight() );

    // Adopt the GL texture into Rive's texture system
    // Note: We don't want Rive to delete our texture, so we create a copy
    // For now, let's upload the pixels directly
    auto riveTexture = mRiveContextGl->adoptImageTexture(
        texture->getWidth(),
        texture->getHeight(),
        texture->getId()
    );

    image->mImpl->riveImage = make_rcp<RiveRenderImage>( std::move( riveTexture ) );

    return image;
}

ImageRef CanvasGl::createImage( const Surface &surface )
{
    if( ! mRiveContextGl ) {
        CI_LOG_W( "Cannot create image without valid context" );
        return nullptr;
    }

    auto image = std::make_shared<ImageGl>();
    image->mSize = ivec2( surface.getWidth(), surface.getHeight() );

    // Upload surface data to Rive texture
    // Convert to RGBA premultiplied if necessary
    Surface8u rgbaSurface( surface.getWidth(), surface.getHeight(), true, SurfaceChannelOrder::RGBA );

    const uint8_t* srcData = surface.getData();
    uint8_t* dstData = rgbaSurface.getData();

    bool srcHasAlpha = surface.hasAlpha();
    int srcPixelInc = surface.getPixelInc();
    int srcRowBytes = surface.getRowBytes();
    int dstRowBytes = rgbaSurface.getRowBytes();

    for( int y = 0; y < surface.getHeight(); ++y ) {
        const uint8_t* srcRow = srcData + y * srcRowBytes;
        uint8_t* dstRow = dstData + y * dstRowBytes;

        for( int x = 0; x < surface.getWidth(); ++x ) {
            uint8_t r = srcRow[0];
            uint8_t g = srcRow[1];
            uint8_t b = srcRow[2];
            uint8_t a = srcHasAlpha ? srcRow[3] : 255;

            // Premultiply alpha
            dstRow[0] = (r * a) / 255;
            dstRow[1] = (g * a) / 255;
            dstRow[2] = (b * a) / 255;
            dstRow[3] = a;

            srcRow += srcPixelInc;
            dstRow += 4;
        }
    }

    auto riveTexture = mRiveContextGl->makeImageTexture(
        surface.getWidth(),
        surface.getHeight(),
        1, // mip levels
        rgbaSurface.getData()
    );

    image->mImpl->riveImage = make_rcp<RiveRenderImage>( std::move( riveTexture ) );

    // Also create a Cinder texture for getTexture()
    image->mTexture = gl::Texture2d::create( surface );

    return image;
}

void CanvasGl::drawImage( const ImageRef &image, const vec2 &position )
{
    if( ! image ) return;
    Rectf destRect( position, position + vec2( image->getSize() ) );
    drawImage( image, destRect );
}

void CanvasGl::drawImage( const ImageRef &image, const Rectf &destRect )
{
    if( ! mInFrame || ! mRiveRenderer || ! image ) return;

    auto glImage = std::dynamic_pointer_cast<ImageGl>( image );
    if( ! glImage || ! glImage->mImpl || ! glImage->mImpl->riveImage ) return;

    mRiveRenderer->save();
    mRiveRenderer->transform( toRiveMat( mTransform ) );

    // Scale and position the image
    float sx = destRect.getWidth() / image->getWidth();
    float sy = destRect.getHeight() / image->getHeight();
    mRiveRenderer->transform( Mat2D( sx, 0, 0, sy, destRect.x1, destRect.y1 ) );

    mRiveRenderer->drawImage( glImage->mImpl->riveImage.get(), rive::ImageSampler::LinearClamp(), rive::BlendMode::srcOver, 1.0f );
    mRiveRenderer->restore();
}

void CanvasGl::drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect )
{
    if( ! mInFrame || ! mRiveRenderer || ! image ) return;

    auto glImage = std::dynamic_pointer_cast<ImageGl>( image );
    if( ! glImage || ! glImage->mImpl || ! glImage->mImpl->riveImage ) return;

    // Create a mesh for the sub-rectangle
    float u0 = srcRect.x1 / image->getWidth();
    float v0 = srcRect.y1 / image->getHeight();
    float u1 = srcRect.x2 / image->getWidth();
    float v1 = srcRect.y2 / image->getHeight();

    std::vector<vec2> vertices = {
        vec2( destRect.x1, destRect.y1 ),
        vec2( destRect.x2, destRect.y1 ),
        vec2( destRect.x2, destRect.y2 ),
        vec2( destRect.x1, destRect.y2 )
    };

    std::vector<vec2> uvs = {
        vec2( u0, v0 ),
        vec2( u1, v0 ),
        vec2( u1, v1 ),
        vec2( u0, v1 )
    };

    std::vector<uint16_t> indices = { 0, 1, 2, 0, 2, 3 };

    drawImageMesh( image, vertices, uvs, indices, 1.0f );
}

void CanvasGl::drawImageMesh( const ImageRef &image,
                               std::span<const vec2> vertices,
                               std::span<const vec2> uvs,
                               std::span<const uint16_t> indices,
                               float opacity )
{
    if( ! mInFrame || ! mRiveRenderer || ! mRiveContext || ! image ) return;
    if( vertices.size() != uvs.size() ) return;

    auto glImage = std::dynamic_pointer_cast<ImageGl>( image );
    if( ! glImage || ! glImage->mImpl || ! glImage->mImpl->riveImage ) return;

    // Create Rive buffers
    auto vertexBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::vertex,
        RenderBufferFlags::none, vertices.size() * sizeof( float ) * 2 );
    auto uvBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::vertex,
        RenderBufferFlags::none, uvs.size() * sizeof( float ) * 2 );
    auto indexBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::index,
        RenderBufferFlags::none, indices.size() * sizeof( uint16_t ) );

    // Map and fill buffers
    float* vertexData = static_cast<float*>( vertexBuffer->map() );
    float* uvData = static_cast<float*>( uvBuffer->map() );
    uint16_t* indexData = static_cast<uint16_t*>( indexBuffer->map() );

    // Apply current transform to vertices
    for( size_t i = 0; i < vertices.size(); ++i ) {
        vec3 transformed = mTransform * vec3( vertices[i], 1.0f );
        vertexData[i * 2] = transformed.x;
        vertexData[i * 2 + 1] = transformed.y;

        uvData[i * 2] = uvs[i].x;
        uvData[i * 2 + 1] = uvs[i].y;
    }

    std::memcpy( indexData, indices.data(), indices.size() * sizeof( uint16_t ) );

    vertexBuffer->unmap();
    uvBuffer->unmap();
    indexBuffer->unmap();

    mRiveRenderer->drawImageMesh(
        glImage->mImpl->riveImage.get(),
        rive::ImageSampler::LinearClamp(),
        std::move( vertexBuffer ),
        std::move( uvBuffer ),
        std::move( indexBuffer ),
        static_cast<uint32_t>( vertices.size() ),
        static_cast<uint32_t>( indices.size() ),
        rive::BlendMode::srcOver,
        opacity
    );
}

// ------------------------------------------------------------------------------------------------
// Text API
// ------------------------------------------------------------------------------------------------

void CanvasGl::drawString( const std::string &text, const vec2 &position,
                            const Font &font, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer || text.empty() ) return;

    // Get glyphs from the font
    std::vector<Font::Glyph> glyphs = font.getGlyphs( text );

    float x = position.x;
    float y = position.y;

    for( Font::Glyph glyph : glyphs ) {
        // Get cached glyph path
        auto glyphPath = mGlyphCache.getGlyph( this, font, glyph );

        if( glyphPath ) {
            save();
            translate( vec2( x, y ) );
            fillPath( glyphPath, paint );
            restore();
        }

        // Advance position
        Rectf bbox = font.getGlyphBoundingBox( glyph );
        // Use glyph advance (approximate from bounding box for now)
        x += bbox.getWidth() + 2.0f; // Add small spacing
    }
}

CachedPathRef CanvasGl::createTextPath( const std::string &text, const Font &font )
{
    if( text.empty() ) return nullptr;

    Shape2d textShape;
    std::vector<Font::Glyph> glyphs = font.getGlyphs( text );

    float x = 0;
    for( Font::Glyph glyph : glyphs ) {
        Shape2d glyphShape = font.getGlyphShape( glyph );

        // Translate glyph to current position
        mat3 glyphTransform;
        glyphTransform[2][0] = x;

        for( const auto& contour : glyphShape.getContours() ) {
            Path2d transformedContour = contour.transformed( glyphTransform );
            textShape.appendContour( transformedContour );
        }

        Rectf bbox = font.getGlyphBoundingBox( glyph );
        x += bbox.getWidth() + 2.0f;
    }

    return createPath( textShape );
}

// ------------------------------------------------------------------------------------------------
// Instanced Drawing API
// ------------------------------------------------------------------------------------------------

CachedPathRef CanvasGl::getOrCreateCirclePath( float radius )
{
    auto it = mCirclePathCache.find( radius );
    if( it != mCirclePathCache.end() ) {
        return it->second;
    }

    // Create circle path centered at origin
    Path2d circlePath;
    circlePath.arc( vec2( 0 ), radius, 0, float( M_PI * 2 ), true );
    circlePath.close();

    auto cached = createPath( circlePath );
    mCirclePathCache[radius] = cached;
    return cached;
}

CachedPathRef CanvasGl::getOrCreateRectPath( const vec2 &size )
{
    // Pack size into uint64 for map key
    uint32_t w = *reinterpret_cast<const uint32_t*>( &size.x );
    uint32_t h = *reinterpret_cast<const uint32_t*>( &size.y );
    uint64_t key = (uint64_t( w ) << 32) | h;

    auto it = mRectPathCache.find( key );
    if( it != mRectPathCache.end() ) {
        return it->second;
    }

    Path2d rectPath;
    rectPath.moveTo( 0, 0 );
    rectPath.lineTo( size.x, 0 );
    rectPath.lineTo( size.x, size.y );
    rectPath.lineTo( 0, size.y );
    rectPath.close();

    auto cached = createPath( rectPath );
    mRectPathCache[key] = cached;
    return cached;
}

void CanvasGl::drawCircles( std::span<const vec2> positions, float radius, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    auto circlePath = getOrCreateCirclePath( radius );
    if( ! circlePath ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( circlePath, paint );
        restore();
    }
}

void CanvasGl::drawCircles( std::span<const mat3> transforms, float radius, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    auto circlePath = getOrCreateCirclePath( radius );
    if( ! circlePath ) return;

    for( const mat3& xform : transforms ) {
        save();
        transform( xform );
        fillPath( circlePath, paint );
        restore();
    }
}

void CanvasGl::drawRects( std::span<const vec2> positions, const vec2 &size, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    auto rectPath = getOrCreateRectPath( size );
    if( ! rectPath ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( rectPath, paint );
        restore();
    }
}

void CanvasGl::drawPaths( std::span<const vec2> positions, const CachedPathRef &path, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer || ! path ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( path, paint );
        restore();
    }
}

void CanvasGl::drawPaths( std::span<const mat3> transforms, const CachedPathRef &path, const Paint &paint )
{
    if( ! mInFrame || ! mRiveRenderer || ! path ) return;

    for( const mat3& xform : transforms ) {
        save();
        transform( xform );
        fillPath( path, paint );
        restore();
    }
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

void CanvasGl::drawCachedPathInternal( const CachedPathGl* cachedPath, const Paint &paint,
                                        bool fill, bool stroke, FillRule rule )
{
    if( ! mRiveContext || ! mRiveRenderer || ! cachedPath || ! cachedPath->mImpl || ! cachedPath->mImpl->rivePath ) return;

    if( fill ) {
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), false );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( cachedPath->mImpl->rivePath.get(), rivePaint.get() );
        mRiveRenderer->restore();
    }

    if( stroke ) {
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), true );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( cachedPath->mImpl->rivePath.get(), rivePaint.get() );
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

    // Track last point for quad-to-cubic conversion
    vec2 lastPt = startPt;

    for( size_t seg = 0; seg < path.getNumSegments(); ++seg ) {
        switch( path.getSegmentType( seg ) ) {
            case Path2d::LINETO: {
                vec2 p = path.getPoint( pointIndex++ );
                rawPath.lineTo( p.x, p.y );
                lastPt = p;
                break;
            }
            case Path2d::QUADTO: {
                // Convert quadratic bezier to cubic bezier
                // Rive's computeCoarseArea doesn't support quads (hits RIVE_UNREACHABLE)
                // Formula: C1 = P0 + 2/3*(P1-P0), C2 = P2 + 2/3*(P1-P2)
                vec2 p1 = path.getPoint( pointIndex++ );  // Control point
                vec2 p2 = path.getPoint( pointIndex++ );  // End point

                vec2 c1 = lastPt + (2.0f / 3.0f) * (p1 - lastPt);
                vec2 c2 = p2 + (2.0f / 3.0f) * (p1 - p2);

                rawPath.cubicTo( c1.x, c1.y, c2.x, c2.y, p2.x, p2.y );
                lastPt = p2;
                break;
            }
            case Path2d::CUBICTO: {
                vec2 p1 = path.getPoint( pointIndex++ );
                vec2 p2 = path.getPoint( pointIndex++ );
                vec2 p3 = path.getPoint( pointIndex++ );
                rawPath.cubicTo( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y );
                lastPt = p3;
                break;
            }
            case Path2d::CLOSE:
                rawPath.close();
                lastPt = startPt;
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

// ------------------------------------------------------------------------------------------------
// Clipping API
// ------------------------------------------------------------------------------------------------

void CanvasGl::clipRect( const Rectf &rect )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Clipping outside of begin()/end()" );

    // Create a rect path and clip with it
    RawPath rawPath;
    rawPath.moveTo( rect.x1, rect.y1 );
    rawPath.lineTo( rect.x2, rect.y1 );
    rawPath.lineTo( rect.x2, rect.y2 );
    rawPath.lineTo( rect.x1, rect.y2 );
    rawPath.close();

    auto path = mRiveContext->makeRenderPath( rawPath, rive::FillRule::nonZero );

    // Apply transform, clip, then restore transform (but keep clip)
    // Rive clips accumulate within a save/restore block
    mRiveRenderer->transform( toRiveMat( mTransform ) );
    mRiveRenderer->clipPath( path.get() );
    mRiveRenderer->transform( toRiveMat( glm::inverse( mTransform ) ) );
}

void CanvasGl::clipPath( const Path2d &path, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Clipping outside of begin()/end()" );

    RawPath rawPath = toRivePath( path );
    auto rivePath = mRiveContext->makeRenderPath( rawPath, toRiveFillRule( rule ) );

    // Apply transform, clip, then restore transform (but keep clip)
    mRiveRenderer->transform( toRiveMat( mTransform ) );
    mRiveRenderer->clipPath( rivePath.get() );
    mRiveRenderer->transform( toRiveMat( glm::inverse( mTransform ) ) );
}

void CanvasGl::clipShape( const Shape2d &shape, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Clipping outside of begin()/end()" );

    RawPath rawPath = toRivePath( shape );
    auto rivePath = mRiveContext->makeRenderPath( rawPath, toRiveFillRule( rule ) );

    // Apply transform, clip, then restore transform (but keep clip)
    mRiveRenderer->transform( toRiveMat( mTransform ) );
    mRiveRenderer->clipPath( rivePath.get() );
    mRiveRenderer->transform( toRiveMat( glm::inverse( mTransform ) ) );
}

void CanvasGl::clipPath( const CachedPathRef &path, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Clipping outside of begin()/end()" );
    if( ! path ) return;

    auto glPath = std::dynamic_pointer_cast<CachedPathGl>( path );
    if( ! glPath || ! glPath->mImpl || ! glPath->mImpl->rivePath ) return;

    // Apply transform, clip, then restore transform (but keep clip)
    mRiveRenderer->transform( toRiveMat( mTransform ) );
    mRiveRenderer->clipPath( glPath->mImpl->rivePath.get() );
    mRiveRenderer->transform( toRiveMat( glm::inverse( mTransform ) ) );
}

// ------------------------------------------------------------------------------------------------
// SVG Rendering
// ------------------------------------------------------------------------------------------------

// Internal SVG renderer that uses vg::Canvas
class SvgRendererVg : public svg::Renderer {
public:
    SvgRendererVg( CanvasGl* canvas )
        : mCanvas( canvas )
    {
        // Initialize stacks with defaults
        mFillStack.push_back( svg::Paint( Color::black() ) );
        mStrokeStack.push_back( svg::Paint() );
        mFillOpacityStack.push_back( 1.0f );
        mStrokeOpacityStack.push_back( 1.0f );
        mStrokeWidthStack.push_back( 1.0f );
        mFillRuleStack.push_back( svg::FILL_RULE_NONZERO );
        mLineCapStack.push_back( svg::LINE_CAP_BUTT );
        mLineJoinStack.push_back( svg::LINE_JOIN_MITER );
    }

    void pushGroup( const svg::Group & /*group*/, float /*opacity*/ ) override {}
    void popGroup() override {}

    void drawPath( const svg::Path &path ) override {
        drawShapeInternal( path.getShape2d() );
    }

    void drawPolygon( const svg::Polygon &polygon ) override {
        // Convert PolyLine to Path2d for Shape2d
        const PolyLine2f& polyline = polygon.getPolyLine();
        Path2d path;
        if( ! polyline.getPoints().empty() ) {
            path.moveTo( polyline.getPoints()[0] );
            for( size_t i = 1; i < polyline.getPoints().size(); ++i ) {
                path.lineTo( polyline.getPoints()[i] );
            }
            path.close();
        }
        Shape2d shape;
        shape.appendContour( path );
        drawShapeInternal( shape );
    }

    void drawPolyline( const svg::Polyline &polyline ) override {
        // Polyline is stroke-only (not closed)
        if( mStrokeStack.back().isNone() ) return;

        const PolyLine2f& poly = polyline.getPolyLine();
        Paint strokePaint = createStrokePaint();
        mCanvas->strokePolyLine( poly, strokePaint );
    }

    void drawLine( const svg::Line &line ) override {
        if( mStrokeStack.back().isNone() ) return;

        Paint strokePaint = createStrokePaint();
        mCanvas->drawLine( line.getPoint1(), line.getPoint2(), strokePaint );
    }

    void drawRect( const svg::Rect &rect ) override {
        if( ! mFillStack.back().isNone() ) {
            Paint fillPaint = createFillPaint();
            mCanvas->fillRect( rect.getRect(), fillPaint );
        }
        if( ! mStrokeStack.back().isNone() ) {
            Paint strokePaint = createStrokePaint();
            mCanvas->strokeRect( rect.getRect(), strokePaint );
        }
    }

    void drawCircle( const svg::Circle &circle ) override {
        if( ! mFillStack.back().isNone() ) {
            Paint fillPaint = createFillPaint();
            mCanvas->fillCircle( circle.getCenter(), circle.getRadius(), fillPaint );
        }
        if( ! mStrokeStack.back().isNone() ) {
            Paint strokePaint = createStrokePaint();
            mCanvas->strokeCircle( circle.getCenter(), circle.getRadius(), strokePaint );
        }
    }

    void drawEllipse( const svg::Ellipse &ellipse ) override {
        if( ! mFillStack.back().isNone() ) {
            Paint fillPaint = createFillPaint();
            mCanvas->fillEllipse( ellipse.getCenter(), vec2( ellipse.getRadiusX(), ellipse.getRadiusY() ), fillPaint );
        }
        if( ! mStrokeStack.back().isNone() ) {
            Paint strokePaint = createStrokePaint();
            mCanvas->strokeEllipse( ellipse.getCenter(), vec2( ellipse.getRadiusX(), ellipse.getRadiusY() ), strokePaint );
        }
    }

    void drawImage( const svg::Image & /*image*/ ) override {
        // TODO: implement image drawing
    }

    void drawTextSpan( const svg::TextSpan & /*span*/ ) override {
        // TODO: implement text rendering
    }

    void pushMatrix( const mat3 &m ) override {
        mCanvas->save();
        mCanvas->transform( m );
    }

    void popMatrix() override {
        mCanvas->restore();
    }

    void pushFill( const svg::Paint &paint ) override { mFillStack.push_back( paint ); }
    void popFill() override { mFillStack.pop_back(); }
    void pushStroke( const svg::Paint &paint ) override { mStrokeStack.push_back( paint ); }
    void popStroke() override { mStrokeStack.pop_back(); }
    void pushFillOpacity( float opacity ) override { mFillOpacityStack.push_back( opacity ); }
    void popFillOpacity() override { mFillOpacityStack.pop_back(); }
    void pushStrokeOpacity( float opacity ) override { mStrokeOpacityStack.push_back( opacity ); }
    void popStrokeOpacity() override { mStrokeOpacityStack.pop_back(); }
    void pushStrokeWidth( float width ) override { mStrokeWidthStack.push_back( width ); }
    void popStrokeWidth() override { mStrokeWidthStack.pop_back(); }
    void pushFillRule( svg::FillRule rule ) override { mFillRuleStack.push_back( rule ); }
    void popFillRule() override { mFillRuleStack.pop_back(); }
    void pushLineCap( svg::LineCap cap ) override { mLineCapStack.push_back( cap ); }
    void popLineCap() override { mLineCapStack.pop_back(); }
    void pushLineJoin( svg::LineJoin join ) override { mLineJoinStack.push_back( join ); }
    void popLineJoin() override { mLineJoinStack.pop_back(); }

private:
    void drawShapeInternal( const Shape2d &shape ) {
        FillRule rule = ( mFillRuleStack.back() == svg::FILL_RULE_EVENODD ) ? FillRule::EvenOdd : FillRule::NonZero;

        if( ! mFillStack.back().isNone() ) {
            Paint fillPaint = createFillPaint();
            mCanvas->fillShape( shape, fillPaint, rule );
        }
        if( ! mStrokeStack.back().isNone() ) {
            Paint strokePaint = createStrokePaint();
            mCanvas->strokeShape( shape, strokePaint );
        }
    }

    Paint createFillPaint() {
        Paint paint;
        const svg::Paint& svgPaint = mFillStack.back();

        if( svgPaint.isLinearGradient() ) {
            // Create multi-stop gradient
            std::vector<GradientStop> stops;
            for( size_t i = 0; i < svgPaint.getNumColors(); ++i ) {
                stops.push_back( GradientStop( svgPaint.getOffset( i ), ColorAf( svgPaint.getColor( i ) ) ) );
            }
            paint.setLinearGradient( svgPaint.getCoords0(), svgPaint.getCoords1(), stops );
        }
        else if( svgPaint.isRadialGradient() ) {
            std::vector<GradientStop> stops;
            for( size_t i = 0; i < svgPaint.getNumColors(); ++i ) {
                stops.push_back( GradientStop( svgPaint.getOffset( i ), ColorAf( svgPaint.getColor( i ) ) ) );
            }
            paint.setRadialGradient( svgPaint.getCoords0(), svgPaint.getRadius(), stops );
        }
        else {
            ColorAf color( svgPaint.getColor() );
            color.a = mFillOpacityStack.back();
            paint.setColor( color );
        }

        return paint;
    }

    Paint createStrokePaint() {
        Paint paint;
        const svg::Paint& svgPaint = mStrokeStack.back();

        if( svgPaint.isLinearGradient() ) {
            std::vector<GradientStop> stops;
            for( size_t i = 0; i < svgPaint.getNumColors(); ++i ) {
                stops.push_back( GradientStop( svgPaint.getOffset( i ), ColorAf( svgPaint.getColor( i ) ) ) );
            }
            paint.setLinearGradient( svgPaint.getCoords0(), svgPaint.getCoords1(), stops );
        }
        else if( svgPaint.isRadialGradient() ) {
            std::vector<GradientStop> stops;
            for( size_t i = 0; i < svgPaint.getNumColors(); ++i ) {
                stops.push_back( GradientStop( svgPaint.getOffset( i ), ColorAf( svgPaint.getColor( i ) ) ) );
            }
            paint.setRadialGradient( svgPaint.getCoords0(), svgPaint.getRadius(), stops );
        }
        else {
            ColorAf color( svgPaint.getColor() );
            color.a = mStrokeOpacityStack.back();
            paint.setColor( color );
        }

        paint.setStrokeWidth( mStrokeWidthStack.back() );

        // Convert line cap
        switch( mLineCapStack.back() ) {
            case svg::LINE_CAP_ROUND:  paint.setLineCap( LineCap::Round ); break;
            case svg::LINE_CAP_SQUARE: paint.setLineCap( LineCap::Square ); break;
            default:                   paint.setLineCap( LineCap::Butt ); break;
        }

        // Convert line join
        switch( mLineJoinStack.back() ) {
            case svg::LINE_JOIN_ROUND: paint.setLineJoin( LineJoin::Round ); break;
            case svg::LINE_JOIN_BEVEL: paint.setLineJoin( LineJoin::Bevel ); break;
            default:                   paint.setLineJoin( LineJoin::Miter ); break;
        }

        return paint;
    }

    CanvasGl* mCanvas;
    std::vector<svg::Paint> mFillStack;
    std::vector<svg::Paint> mStrokeStack;
    std::vector<float> mFillOpacityStack;
    std::vector<float> mStrokeOpacityStack;
    std::vector<float> mStrokeWidthStack;
    std::vector<svg::FillRule> mFillRuleStack;
    std::vector<svg::LineCap> mLineCapStack;
    std::vector<svg::LineJoin> mLineJoinStack;
};

void CanvasGl::draw( const svg::Doc &svg )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    SvgRendererVg renderer( this );
    svg.render( renderer );
}

} } // namespace cinder::vg
