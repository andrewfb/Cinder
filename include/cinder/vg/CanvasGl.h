/*
 Copyright (c) 2024, The Cinder Project

 This code is intended to be used with the Cinder C++ library, http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and
	   the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	   the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/vg/Canvas.h"
#include "cinder/gl/Fbo.h"

#include <unordered_map>

// Forward declarations for Rive types
namespace rive {
    class RiveRenderer;
    class RawPath;
    class RenderPath;
    class RenderImage;
    class RenderBuffer;
    namespace gpu {
        class RenderContext;
        class RenderContextGLImpl;
    }
}

namespace cinder { namespace vg {

// Forward declare CanvasGl for GlyphCache
class CanvasGl;

// ------------------------------------------------------------------------------------------------
// CachedPathGl - GL implementation of CachedPath
// ------------------------------------------------------------------------------------------------

// Forward declare internal struct defined in cpp file
struct CachedPathGlImpl;

class CI_API CachedPathGl : public CachedPath {
public:
    CachedPathGl();
    ~CachedPathGl() override;

private:
    friend class CanvasGl;

    std::unique_ptr<CachedPathGlImpl> mImpl;
};

// ------------------------------------------------------------------------------------------------
// ImageGl - GL implementation of Image
// ------------------------------------------------------------------------------------------------

// Forward declare internal struct defined in cpp file
struct ImageGlImpl;

class CI_API ImageGl : public Image {
public:
    ImageGl();
    ~ImageGl() override;

    //! Get the underlying GL texture
    gl::Texture2dRef getTexture() const { return mTexture; }

private:
    friend class CanvasGl;

    gl::Texture2dRef mTexture;
    std::unique_ptr<ImageGlImpl> mImpl;
};

// ------------------------------------------------------------------------------------------------
// GlyphCache - Internal glyph shape cache for text rendering
// ------------------------------------------------------------------------------------------------

class GlyphCache {
public:
    CachedPathRef getGlyph( CanvasGl* canvas, const Font &font, Font::Glyph glyphIndex );
    void clear();

private:
    // Key: font name + size + glyph index
    struct GlyphKey {
        std::string fontName;
        float fontSize;
        Font::Glyph glyphIndex;

        bool operator==( const GlyphKey &other ) const {
            return fontName == other.fontName && fontSize == other.fontSize && glyphIndex == other.glyphIndex;
        }
    };

    struct GlyphKeyHash {
        size_t operator()( const GlyphKey &k ) const {
            size_t h1 = std::hash<std::string>{}( k.fontName );
            size_t h2 = std::hash<float>{}( k.fontSize );
            size_t h3 = std::hash<Font::Glyph>{}( k.glyphIndex );
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    std::unordered_map<GlyphKey, CachedPathRef, GlyphKeyHash> mCache;
};

// ------------------------------------------------------------------------------------------------
// CanvasGl - OpenGL implementation of Canvas
// ------------------------------------------------------------------------------------------------

//! OpenGL implementation of Canvas using Rive's PLS renderer.
//! Supports rendering to screen (default framebuffer) or to an FBO.
class CI_API CanvasGl : public Canvas {
public:
    //! Create a canvas for rendering to the screen (default framebuffer)
    explicit CanvasGl( const CanvasOptions &options = CanvasOptions() );

    //! Create a canvas for rendering to an FBO
    CanvasGl( const gl::FboRef &fbo, const CanvasOptions &options = CanvasOptions() );

    ~CanvasGl() override;

    // Non-copyable
    CanvasGl( const CanvasGl& ) = delete;
    CanvasGl& operator=( const CanvasGl& ) = delete;

    // === Frame Management ===
    void begin( const ivec2 &size ) override;
    void end() override;
    bool inFrame() const override { return mInFrame; }

    // === FBO Management ===
    //! Set a new FBO target (or nullptr for screen rendering)
    void setFbo( const gl::FboRef &fbo );

    //! Get the current FBO target (nullptr if rendering to screen)
    gl::FboRef getFbo() const { return mFbo; }

    //! Returns true if rendering to an FBO
    bool isRenderingToFbo() const { return mFbo != nullptr; }

    // === Transform Stack ===
    void save() override;
    void restore() override;
    void translate( const vec2 &offset ) override;
    void rotate( float radians ) override;
    void scale( const vec2 &s ) override;
    void transform( const mat3 &m ) override;
    void setTransform( const mat3 &m ) override;
    void resetTransform() override;
    mat3 getTransform() const override { return mTransform; }

    // === Drawing Primitives ===
    void fillRect( const Rectf &rect, const Paint &paint ) override;
    void strokeRect( const Rectf &rect, const Paint &paint ) override;
    void fillRoundedRect( const Rectf &rect, float radius, const Paint &paint ) override;
    void strokeRoundedRect( const Rectf &rect, float radius, const Paint &paint ) override;
    void fillCircle( const vec2 &center, float radius, const Paint &paint ) override;
    void strokeCircle( const vec2 &center, float radius, const Paint &paint ) override;
    void fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint ) override;
    void strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint ) override;
    void drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint ) override;

    // === Path Drawing (uncached) ===
    void fillPath( const Path2d &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const Path2d &path, const Paint &paint ) override;
    void fillShape( const Shape2d &shape, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokeShape( const Shape2d &shape, const Paint &paint ) override;
    void strokePolyLine( const PolyLine2f &polyline, const Paint &paint ) override;
    void fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule = FillRule::NonZero ) override;

    // === Cached Path API ===
    CachedPathRef createPath( const Path2d &path ) override;
    CachedPathRef createPath( const Shape2d &shape ) override;
    void fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const CachedPathRef &path, const Paint &paint ) override;

    // === Image API ===
    ImageRef createImage( const gl::Texture2dRef &texture ) override;
    ImageRef createImage( const Surface &surface ) override;
    void drawImage( const ImageRef &image, const vec2 &position ) override;
    void drawImage( const ImageRef &image, const Rectf &destRect ) override;
    void drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect ) override;
    void drawImageMesh( const ImageRef &image,
                        std::span<const vec2> vertices,
                        std::span<const vec2> uvs,
                        std::span<const uint16_t> indices,
                        float opacity = 1.0f ) override;

    // === Text API ===
    void drawString( const std::string &text, const vec2 &position,
                     const Font &font, const Paint &paint ) override;
    CachedPathRef createTextPath( const std::string &text, const Font &font ) override;

    // === Instanced Drawing API ===
    void drawCircles( std::span<const vec2> positions, float radius, const Paint &paint ) override;
    void drawCircles( std::span<const mat3> transforms, float radius, const Paint &paint ) override;
    void drawRects( std::span<const vec2> positions, const vec2 &size, const Paint &paint ) override;
    void drawPaths( std::span<const vec2> positions, const CachedPathRef &path, const Paint &paint ) override;
    void drawPaths( std::span<const mat3> transforms, const CachedPathRef &path, const Paint &paint ) override;

    // === Clipping API ===
    void clipRect( const Rectf &rect ) override;
    void clipPath( const Path2d &path, FillRule rule = FillRule::NonZero ) override;
    void clipShape( const Shape2d &shape, FillRule rule = FillRule::NonZero ) override;
    void clipPath( const CachedPathRef &path, FillRule rule = FillRule::NonZero ) override;

    // === SVG Rendering ===
    void draw( const svg::Doc &svg ) override;

    // === Advanced ===
    //! Get the underlying Rive RenderContext (for advanced usage)
    rive::gpu::RenderContext* getRiveContext() const { return mRiveContext.get(); }

    //! Clear the glyph cache (call if fonts are unloaded)
    void clearGlyphCache() { mGlyphCache.clear(); }

private:
    void initializeGl( const CanvasOptions &options );
    void invalidateState();
    void restoreHostState( const ivec2 &size );

    // Internal path conversion
    rive::RawPath toRivePath( const Path2d &path );
    rive::RawPath toRivePath( const Shape2d &shape );
    rive::RawPath toRivePath( const PolyLine2f &polyline );

    // Internal drawing
    void drawPathInternal( rive::RawPath path, const Paint &paint,
                           bool fill, bool stroke, FillRule rule = FillRule::NonZero );
    void drawCachedPathInternal( const CachedPathGl* cachedPath, const Paint &paint,
                                  bool fill, bool stroke, FillRule rule = FillRule::NonZero );

    // Internal helpers for primitives
    CachedPathRef getOrCreateCirclePath( float radius );
    CachedPathRef getOrCreateRectPath( const vec2 &size );

    // Rive context (owned)
    std::unique_ptr<rive::gpu::RenderContext> mRiveContext;
    rive::gpu::RenderContextGLImpl* mRiveContextGl = nullptr;  // Raw pointer for GL-specific calls

    // Rive renderer (created per-frame)
    std::unique_ptr<rive::RiveRenderer> mRiveRenderer;

    // FBO target (nullptr = screen)
    gl::FboRef mFbo;

    // Internal floating-point FBO for high-quality rendering
    gl::FboRef mInternalFbo;
    bool mUseFloatingPointBuffer = false;

    // Frame state
    bool mInFrame = false;
    ivec2 mFrameSize;

    // Transform state
    mat3 mTransform = mat3();
    std::vector<mat3> mTransformStack;

    // Caches
    GlyphCache mGlyphCache;
    std::unordered_map<float, CachedPathRef> mCirclePathCache;  // radius -> path
    std::unordered_map<uint64_t, CachedPathRef> mRectPathCache;  // packed size -> path
};

using CanvasGlRef = std::shared_ptr<CanvasGl>;

//! Create a CanvasGl for screen rendering
inline CanvasGlRef createCanvasGl( const CanvasOptions &options = CanvasOptions() ) {
    return std::make_shared<CanvasGl>( options );
}

//! Create a CanvasGl for FBO rendering
inline CanvasGlRef createCanvasGl( const gl::FboRef &fbo, const CanvasOptions &options = CanvasOptions() ) {
    return std::make_shared<CanvasGl>( fbo, options );
}

} } // namespace cinder::vg
