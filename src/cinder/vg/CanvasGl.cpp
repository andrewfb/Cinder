/*
 Copyright (c) 2024, The Cinder Project
 */

#include "cinder/vg/CanvasGl.h"
#include "cinder/Log.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/scoped.h"
#include "cinder/gl/Context.h"
#include "cinder/svg/Svg.h"

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
// Helper functions (GL-specific)
// ------------------------------------------------------------------------------------------------

// Helper to convert Cinder mat3 to Rive Mat2D
static Mat2D toRiveMat( const mat3 &m )
{
    // Cinder mat3 is column-major, Rive Mat2D is [xx, xy, yx, yy, tx, ty]
    return Mat2D( m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1] );
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

// GlyphCache implementation is now in Canvas.cpp

// ------------------------------------------------------------------------------------------------
// CanvasGl implementation
// ------------------------------------------------------------------------------------------------

CanvasGlRef CanvasGl::create()
{
    // Query window's MSAA sample count
    GLint samples = 0;
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );  // Ensure default framebuffer is bound
    glGetIntegerv( GL_SAMPLES, &samples );

    bool windowHasMsaa = samples > 1;
    CI_LOG_I( "Window MSAA samples: " << samples << " (MSAA " << (windowHasMsaa ? "enabled" : "disabled") << ")" );

    auto canvas = CanvasGlRef( new CanvasGl( RenderMode::Window ) );
    canvas->mWindowHasMsaa = windowHasMsaa;
    return canvas;
}

CanvasGlRef CanvasGl::create( int width, int height )
{
    CI_ASSERT_MSG( width > 0 && height > 0, "Offscreen canvas dimensions must be positive" );

    auto canvas = CanvasGlRef( new CanvasGl( RenderMode::Offscreen, width, height ) );
    return canvas;
}

CanvasGl::CanvasGl( RenderMode mode, int fboWidth, int fboHeight )
    : mRenderMode( mode )
    , mOffscreenSize( fboWidth, fboHeight )
{
    // Determine if we should force MSAA mode (disable atomic)
    // - Window mode with MSAA: force MSAA mode to avoid double-coverage artifacts
    // - macOS: always force MSAA mode (GL 4.1 doesn't support PLS/FSI)
    // - Offscreen mode: use atomic mode for analytical AA
    bool forceMsaaMode = false;
#if defined( CINDER_MAC )
    forceMsaaMode = true;  // macOS GL 4.1 doesn't support PLS/FSI
#else
    if( mode == RenderMode::Window ) {
        // Will be set properly after MSAA detection in create()
        // For now, query it here as well
        GLint samples = 0;
        glGetIntegerv( GL_SAMPLES, &samples );
        forceMsaaMode = samples > 1;
        mWindowHasMsaa = forceMsaaMode;
    }
#endif

    initializeGl( forceMsaaMode );

    // Create offscreen FBO if in offscreen mode
    if( mode == RenderMode::Offscreen && fboWidth > 0 && fboHeight > 0 ) {
        gl::Fbo::Format format;
        format.colorTexture( gl::Texture::Format()
            .internalFormat( GL_RGBA16F )
            .minFilter( GL_LINEAR )
            .magFilter( GL_LINEAR ) );
        mOffscreenFbo = gl::Fbo::create( fboWidth, fboHeight, format );
        CI_LOG_I( "Created offscreen RGBA16F FBO: " << fboWidth << "x" << fboHeight );
    }
}

CanvasGl::~CanvasGl()
{
    if( mInFrame ) {
        CI_LOG_W( "CanvasGl destroyed while still in frame" );
    }
    mRiveRenderer = nullptr;  // Clear base class pointer
    mOwnedRiveRenderer.reset();
    mRiveContext.reset();
}

void CanvasGl::initializeGl( bool forceMsaaMode )
{
    CI_LOG_I( "Creating Rive GL context with forceMsaaMode=" << forceMsaaMode );

    // Initialize Rive's GLAD loader - this sets up GLAD_GL_version_major etc.
#if defined( CINDER_MSW )
    gladLoadCustomLoader( (GLADloadfunc)winGlGetProcAddress );
#else
    gladLoadCustomLoader( (GLADloadfunc)glfwGetProcAddress );
#endif

    RenderContextGLImpl::ContextOptions riveOptions;
    riveOptions.disablePixelLocalStorage = forceMsaaMode;
    riveOptions.disableFragmentShaderInterlock = forceMsaaMode;

    mRiveContext = RenderContextGLImpl::MakeContext( riveOptions );

    if( ! mRiveContext ) {
        throw vg::Exc( "Failed to create Rive GL RenderContext" );
    }

    // Store the GL-specific pointer for invalidateGLState()
    mRiveContextGl = mRiveContext->static_impl_cast<RenderContextGLImpl>();

    // Log platform features
    const auto& features = mRiveContext->platformFeatures();
    CI_LOG_I( "CanvasGl created - supportsRasterOrderingMode=" << features.supportsRasterOrderingMode
              << " supportsAtomicMode=" << features.supportsAtomicMode
              << " supportsClockwiseAtomicMode=" << features.supportsClockwiseAtomicMode );

    // CRITICAL: Clean up Rive's initialization state
    // Rive's context creation modifies VAOs, VBOs, textures, shaders, etc.
    // unbindGLInternalResources() uses raw GL calls, so we must invalidate
    // Cinder's caches so it knows to re-query/re-bind state.
    if( mRiveContextGl ) {
        mRiveContextGl->unbindGLInternalResources();
    }

    // Invalidate Cinder's cached knowledge of GL state that Rive touched
    auto* ctx = gl::context();
    if( ctx ) {
        // Buffer bindings - Rive uses VBOs and UBOs
        ctx->invalidateBufferBindingCache( GL_ARRAY_BUFFER );
        ctx->invalidateBufferBindingCache( GL_ELEMENT_ARRAY_BUFFER );
        ctx->invalidateBufferBindingCache( GL_UNIFORM_BUFFER );

        // VAO - Rive creates its own VAOs
        ctx->restoreInvalidatedVao();

        // Textures/samplers - push/pop to force Cinder to re-query
        for( uint8_t i = 0; i < 8; ++i ) {
            ctx->pushTextureBinding( GL_TEXTURE_2D, i );
            ctx->popTextureBinding( GL_TEXTURE_2D, i, true );
#if defined( CINDER_GL_HAS_SAMPLERS )
            ctx->pushSamplerBinding( i, 0 );
            ctx->popSamplerBinding( i, true );
#endif
        }

        // Shader program
        ctx->pushGlslProg();
        ctx->popGlslProg( true );

        // Framebuffer
        ctx->pushFramebuffer();
        ctx->popFramebuffer( true );
    }
}

void CanvasGl::invalidateState()
{
    if( mRiveContextGl ) {
        mRiveContextGl->invalidateGLState();
    }
}

void CanvasGl::saveHostState()
{
    auto* ctx = gl::context();
    if( ! ctx ) return;

    // === Push all GL state that Rive may modify ===

    // VAO and program
    ctx->pushVao();
    ctx->pushGlslProg();

    // Buffer bindings (Rive uses VBO, UBO, potentially EBO)
    ctx->pushBufferBinding( GL_ARRAY_BUFFER );
    ctx->pushBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
    ctx->pushBufferBinding( GL_UNIFORM_BUFFER );

    // Framebuffer (no argument = push current binding for GL_FRAMEBUFFER target)
    ctx->pushFramebuffer();

    // Save active texture BEFORE the loop (since pushTextureBinding modifies active texture)
    ctx->pushActiveTexture();

    // Texture bindings (Rive uses multiple texture units)
    for( uint8_t i = 0; i < 8; ++i ) {
        ctx->pushTextureBinding( GL_TEXTURE_2D, i );
#if defined( CINDER_GL_HAS_SAMPLERS )
        ctx->pushSamplerBinding( i, ctx->getSamplerBinding( i ) );
#endif
    }

    // Bool states Rive modifies
    ctx->pushBoolState( GL_BLEND );
    ctx->pushBoolState( GL_DEPTH_TEST );
    ctx->pushBoolState( GL_STENCIL_TEST );
    ctx->pushBoolState( GL_SCISSOR_TEST );
    ctx->pushBoolState( GL_CULL_FACE );

    // Blend function and equation
    ctx->pushBlendFuncSeparate();
    ctx->pushBlendEquationSeparate();

    // Masks
    ctx->pushColorMask( ctx->getColorMask().r, ctx->getColorMask().g, ctx->getColorMask().b, ctx->getColorMask().a );
    ctx->pushDepthMask( ctx->getDepthMask() );
    ctx->pushStencilMask( ctx->getStencilMask().x, ctx->getStencilMask().y );

    // Stencil function and operation
    ctx->pushStencilFunc( GL_ALWAYS, 0, 0xFF );  // Push current state - Cinder will query GL if stack is empty
    ctx->pushStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

    // Front face (Rive uses CW, Cinder expects CCW)
    ctx->pushFrontFace();

    // Cull face mode
    ctx->pushCullFace();

    // Depth function
    ctx->pushDepthFunc();

    // Line width (wireframe mode)
    ctx->pushLineWidth();

    // Viewport and scissor
    ctx->pushViewport();
    ctx->pushScissor();
}

void CanvasGl::restoreHostState( const ivec2 &size )
{
    // First unbind Rive's internal GL resources
    if( mRiveContextGl ) {
        mRiveContextGl->unbindGLInternalResources();
    }

    auto* ctx = gl::context();
    if( ! ctx ) return;

    // === Pop all GL state with forceRestore=true ===
    // This ensures the actual GL state matches what Cinder had before

    // Viewport and scissor (pop first since they affect rendering)
    ctx->popScissor( true );
    ctx->popViewport( true );

    // Line width
    ctx->popLineWidth( true );

    // Depth function
    ctx->popDepthFunc( true );

    // Cull face mode
    ctx->popCullFace( true );

    // Front face
    ctx->popFrontFace( true );

    // Stencil function and operation
    ctx->popStencilOp( true );
    ctx->popStencilFunc( true );

    // Masks
    ctx->popStencilMask( true );
    ctx->popDepthMask( true );
    ctx->popColorMask( true );

    // Blend equation and function
    ctx->popBlendEquationSeparate( true );
    ctx->popBlendFuncSeparate( true );

    // Bool states
    ctx->popBoolState( GL_CULL_FACE, true );
    ctx->popBoolState( GL_SCISSOR_TEST, true );
    ctx->popBoolState( GL_STENCIL_TEST, true );
    ctx->popBoolState( GL_DEPTH_TEST, true );
    ctx->popBoolState( GL_BLEND, true );

    // Texture bindings (pop in reverse order - LIFO)
    for( int i = 7; i >= 0; --i ) {
#if defined( CINDER_GL_HAS_SAMPLERS )
        ctx->popSamplerBinding( i, true );
#endif
        ctx->popTextureBinding( GL_TEXTURE_2D, i, true );
    }
    // Pop active texture AFTER texture bindings (matches push order: active texture first, then bindings)
    ctx->popActiveTexture( true );

    // Framebuffer
    ctx->popFramebuffer( GL_FRAMEBUFFER );

    // Buffer bindings - first restore the invalidated bindings from cache
    ctx->restoreInvalidatedBufferBinding( GL_UNIFORM_BUFFER );
    ctx->restoreInvalidatedBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
    ctx->restoreInvalidatedBufferBinding( GL_ARRAY_BUFFER );
    ctx->popBufferBinding( GL_UNIFORM_BUFFER );
    ctx->popBufferBinding( GL_ELEMENT_ARRAY_BUFFER );
    ctx->popBufferBinding( GL_ARRAY_BUFFER );

    // VAO - restore invalidated state first, then pop
    ctx->restoreInvalidatedVao();
    ctx->popGlslProg( true );
    ctx->popVao();

    // Set matrices for the size
    gl::setMatricesWindow( size );
}

gl::Texture2dRef CanvasGl::getTexture() const
{
    if( mRenderMode == RenderMode::Offscreen && mOffscreenFbo ) {
        return mOffscreenFbo->getColorTexture();
    }
    return nullptr;
}

ivec2 CanvasGl::getFboSize() const
{
    if( mRenderMode == RenderMode::Offscreen ) {
        return mOffscreenSize;
    }
    return ivec2( 0, 0 );
}

void CanvasGl::begin()
{
    // Offscreen mode - use FBO dimensions
    CI_ASSERT_MSG( mRenderMode == RenderMode::Offscreen, "begin() without size argument is only valid for offscreen canvases. Use begin(size) for window mode." );
    CI_ASSERT_MSG( mOffscreenFbo, "Offscreen FBO not created" );
    begin( mOffscreenSize );
}

void CanvasGl::begin( const ivec2 &size )
{
    CI_ASSERT_MSG( ! mInFrame, "begin() called while already in frame - did you forget to call end()?" );
    CI_ASSERT_MSG( size.x > 0 && size.y > 0, "begin() size must be positive" );

    // For offscreen mode, resize FBO if needed
    if( mRenderMode == RenderMode::Offscreen && size != mOffscreenSize ) {
        gl::Fbo::Format format;
        format.colorTexture( gl::Texture::Format()
            .internalFormat( GL_RGBA16F )
            .minFilter( GL_LINEAR )
            .magFilter( GL_LINEAR ) );
        mOffscreenFbo = gl::Fbo::create( size.x, size.y, format );
        mOffscreenSize = size;
        CI_LOG_I( "Resized offscreen FBO to: " << size.x << "x" << size.y );
    }

    mFrameSize = size;
    mInFrame = true;

    if( mRiveContext ) {
        // Save host GL state before Rive takes over
        saveHostState();

        RenderContext::FrameDescriptor frameDesc;
        frameDesc.renderTargetWidth = size.x;
        frameDesc.renderTargetHeight = size.y;
        // Clear offscreen FBO, preserve window framebuffer (so app's gl::clear works)
        if( mRenderMode == RenderMode::Offscreen ) {
            frameDesc.loadAction = gpu::LoadAction::clear;
            frameDesc.clearColor = 0x00000000;  // Transparent black
        } else {
            frameDesc.loadAction = gpu::LoadAction::preserveRenderTarget;
        }
        frameDesc.clockwiseFillOverride = true; // Enable feathering for any fill rule

        // Invalidate GL state since Cinder may have modified it
        invalidateState();

        mRiveContext->beginFrame( frameDesc );

        // Create RiveRenderer for this frame and set base class pointer
        mOwnedRiveRenderer = std::make_unique<RiveRenderer>( mRiveContext.get() );
        mRiveRenderer = mOwnedRiveRenderer.get();
    }
}

void CanvasGl::end()
{
    CI_ASSERT_MSG( mInFrame, "end() called without begin()" );

    // Clear base class pointer and destroy the renderer before flush
    mRiveRenderer = nullptr;
    mOwnedRiveRenderer.reset();

    if( mRiveContext ) {
        // Determine target FBO and sample count
        GLuint framebufferId = 0;
        int sampleCount = 1;

        if( mRenderMode == RenderMode::Offscreen && mOffscreenFbo ) {
            // Offscreen: render to our internal FBO
            framebufferId = mOffscreenFbo->getId();
            sampleCount = 1;  // Always single-sample for atomic mode
        } else if( mRenderMode == RenderMode::Window ) {
            // Window: render directly to default framebuffer
            framebufferId = 0;
            // Get the actual sample count from the window
            GLint samples = 0;
            glGetIntegerv( GL_SAMPLES, &samples );
            sampleCount = std::max( 1, (int)samples );
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

// Transform methods (translate, rotate, scale, transform, setTransform, resetTransform)
// are now implemented in Canvas base class

// Drawing Primitives (fillRect, strokeRect, fillCircle, strokeCircle, etc.)
// are now implemented in Canvas base class using Path2d

// implFillShape and implStrokeShape are now implemented in Canvas base class

// ------------------------------------------------------------------------------------------------
// Cached Path API
// ------------------------------------------------------------------------------------------------

// createPath(Path2d) is implemented in Canvas base class - it converts to Shape2d

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

// drawImage(position) is now implemented in Canvas base class

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

// Text API (drawString, createTextPath) is now implemented in Canvas base class

// Instanced Drawing API (drawCircles, drawRects, drawPaths, getOrCreateCirclePath, getOrCreateRectPath)
// is now implemented in Canvas base class

// ------------------------------------------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------------------------------------------

// drawPathInternal is now implemented in Canvas base class

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

// toRivePath implementations are now in Canvas base class
// implClipPath is now implemented in Canvas base class
// SVG rendering is handled by the base Canvas class using SvgRendererVg

} } // namespace cinder::vg
