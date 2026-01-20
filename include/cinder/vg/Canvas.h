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

#include "cinder/vg/Paint.h"
#include "cinder/Path2d.h"
#include "cinder/Shape2d.h"
#include "cinder/PolyLine.h"
#include "cinder/Rect.h"
#include "cinder/Exception.h"
#include "cinder/Font.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"

#include <memory>
#include <vector>
#include <span>
#include <unordered_map>

namespace cinder { namespace svg { class Doc; } }

// Forward declarations for Rive types (implementation details, not exposed in API)
namespace rive {
    class RiveRenderer;
    class RawPath;
    class RenderPath;
    class RenderPaint;
    namespace gpu {
        class RenderContext;
    }
}

namespace cinder { namespace vg {

//! Exception type for vector graphics errors
class CI_API Exc : public Exception {
public:
    Exc( const std::string &description ) : Exception( description ) {}
};

//! Rendering mode for CanvasGl
enum class CI_API RenderMode {
    Window,     //!< Render to window (auto-detects MSAA, renders directly)
    Offscreen   //!< Render to offscreen FBO (atomic mode with analytical AA)
};

// Forward declarations
class Canvas;
class CachedPath;
class Image;

using CachedPathRef = std::shared_ptr<CachedPath>;
using ImageRef = std::shared_ptr<Image>;

// ------------------------------------------------------------------------------------------------
// CachedPath - Cached path for efficient repeated drawing
// ------------------------------------------------------------------------------------------------

//! A cached path that can be drawn efficiently multiple times.
//! Created via Canvas::createPath(). Holds internal GPU-friendly representation.
class CI_API CachedPath {
public:
    virtual ~CachedPath() = default;

    //! Get the original source path (for queries)
    const Shape2d& getSourceShape() const { return mSourceShape; }

    //! Get axis-aligned bounding box
    Rectf getBounds() const { return mBounds; }

protected:
    friend class Canvas;
    CachedPath() = default;

    Shape2d mSourceShape;
    Rectf mBounds;
};

//! A GPU image that can be drawn to the canvas.
//! Created via Canvas::createImage(). Wraps a texture for use with Rive renderer.
class CI_API Image {
public:
    virtual ~Image() = default;

    //! Get the image dimensions
    ivec2	getSize() const { return mSize; }
    int		getWidth() const { return mSize.x; }
    int		getHeight() const { return mSize.y; }
	float	getAspectRatio() const { return getWidth() / (float)getHeight(); }
	Area	getBounds() const { return Area( 0, 0, getWidth(), getHeight() ); }

protected:
    friend class Canvas;
    Image() = default;

    ivec2 mSize;
};

// ------------------------------------------------------------------------------------------------
// DisplayList - Recorded drawing commands for efficient replay
// ------------------------------------------------------------------------------------------------

class Canvas;  // Forward declaration

//! A recorded list of drawing commands that can be replayed efficiently.
//! DisplayList captures Canvas draw commands and creates CachedPaths for
//! each path encountered, enabling efficient replay.
//!
//! Use DisplayList when:
//! - Rendering complex static content like SVGs repeatedly
//! - The content doesn't change between frames
//! - Performance profiling shows SVG/path rendering is a bottleneck
//!
//! Example:
//!     // Recording phase (do once when content changes)
//!     auto displayList = canvas->createDisplayList();
//!     displayList->beginRecording( canvas );
//!     canvas->draw( *svgDoc );  // Draw commands are captured
//!     displayList->endRecording();
//!
//!     // Playback phase (do every frame)
//!     canvas->begin( windowSize );
//!     canvas->setTransform( viewMatrix );
//!     displayList->replay( canvas );  // Fast - uses cached paths
//!     canvas->end();
class CI_API DisplayList {
public:
    virtual ~DisplayList() = default;

    //! Begin recording draw commands. The canvas must be in a frame (between begin/end).
    virtual void beginRecording( Canvas* canvas ) = 0;

    //! End recording and freeze all captured paths.
    virtual void endRecording() = 0;

    //! Check if currently recording
    virtual bool isRecording() const = 0;

    //! Replay the recorded commands using frozen paths.
    //! The canvas must be in a frame (between begin/end).
    virtual void replay( Canvas* canvas ) = 0;

    //! Check if the display list is valid (has recorded content)
    virtual bool isValid() const = 0;

    //! Clear the display list
    virtual void clear() = 0;

    //! Get the number of draw commands recorded
    virtual size_t getCommandCount() const = 0;

    //! Get the axis-aligned bounding box of the recorded content
    virtual Rectf getBounds() const = 0;

protected:
    DisplayList() = default;
};

using DisplayListRef = std::shared_ptr<DisplayList>;

// ------------------------------------------------------------------------------------------------
// GlyphCache - Internal glyph shape cache for text rendering
// ------------------------------------------------------------------------------------------------

class GlyphCache {
public:
    CachedPathRef getGlyph( Canvas* canvas, const Font &font, Font::Glyph glyphIndex );
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
// Canvas - Abstract base class for vector graphics rendering
// ------------------------------------------------------------------------------------------------

//! Canvas provides the main drawing interface for vector graphics.
//! This is an abstract base class - use CanvasGl for OpenGL rendering.
//! Provides shared implementation for transforms, instanced drawing, and text.
class CI_API Canvas {
public:
    virtual ~Canvas();

    // === Frame Management ===
    //! Begin rendering at the given size. Must be paired with end().
    virtual void begin( const ivec2 &size ) = 0;

    //! Begin rendering with explicit width/height
    void begin( int width, int height ) { begin( ivec2( width, height ) ); }

    //! End rendering and flush to the GPU
    virtual void end() = 0;

    //! Check if currently between begin() and end()
    virtual bool inFrame() const = 0;

    // === Transform Stack ===
    //! Push current transform and clipping state onto the stack
    void save();

    //! Pop transform and clipping state from the stack
    void restore();

    //! Translate the current transform (base implementation provided)
    virtual void translate( const vec2 &offset );
    void translate( float x, float y ) { translate( vec2( x, y ) ); }

    //! Rotate the current transform (radians) (base implementation provided)
    virtual void rotate( float radians );

    //! Scale the current transform (base implementation provided)
    virtual void scale( const vec2 &s );
    void scale( float sx, float sy ) { scale( vec2( sx, sy ) ); }
    void scale( float s ) { scale( vec2( s, s ) ); }

    //! Concatenate a transform matrix (base implementation provided)
    virtual void transform( const mat3 &m );

    //! Set the transform matrix directly (base implementation provided)
    virtual void setTransform( const mat3 &m );

    //! Reset transform to identity (base implementation provided)
    virtual void resetTransform();

    //! Get the current transform matrix
    mat3 getTransform() const { return mTransform; }

    // === Drawing Primitives ===
    // These have base implementations using fillPath/strokePath, but can be overridden for efficiency
    //! Fill a rectangle
    virtual void fillRect( const Rectf &rect, const Paint &paint );

    //! Stroke a rectangle
    virtual void strokeRect( const Rectf &rect, const Paint &paint );

    //! Fill a rounded rectangle
    virtual void fillRoundedRect( const Rectf &rect, float radius, const Paint &paint );

    //! Stroke a rounded rectangle
    virtual void strokeRoundedRect( const Rectf &rect, float radius, const Paint &paint );

    //! Fill a circle
    virtual void fillCircle( const vec2 &center, float radius, const Paint &paint );

    //! Stroke a circle
    virtual void strokeCircle( const vec2 &center, float radius, const Paint &paint );

    //! Fill an ellipse
    virtual void fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint );

    //! Stroke an ellipse
    virtual void strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint );

    //! Draw a line (stroked only)
    virtual void drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint );

    // === Path Drawing (uncached) ===
    // These are implemented in base class and delegate to implFillShape/implStrokeShape
    //! Fill a Path2d
    virtual void fillPath( const Path2d &path, const Paint &paint, FillRule rule = FillRule::NonZero );

    //! Stroke a Path2d
    virtual void strokePath( const Path2d &path, const Paint &paint );

    //! Fill a Shape2d
    virtual void fillShape( const Shape2d &shape, const Paint &paint, FillRule rule = FillRule::NonZero );

    //! Stroke a Shape2d
    virtual void strokeShape( const Shape2d &shape, const Paint &paint );

    //! Stroke a PolyLine
    virtual void strokePolyLine( const PolyLine2f &polyline, const Paint &paint );

    //! Fill a closed PolyLine
    virtual void fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule = FillRule::NonZero );

    // === Cached Path API ===
    //! Create a cached path from a Path2d. Cached paths are more efficient for repeated drawing.
    //! Base implementation converts to Shape2d and calls createPath(Shape2d).
    virtual CachedPathRef createPath( const Path2d &path );

    //! Create a cached path from a Shape2d - must be implemented by subclass
    virtual CachedPathRef createPath( const Shape2d &shape ) = 0;

    //! Fill a cached path - base implementation uses source shape
    virtual void fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule = FillRule::NonZero );

    //! Stroke a cached path - base implementation uses source shape
    virtual void strokePath( const CachedPathRef &path, const Paint &paint );

    // === Image API ===
    //! Create an image from a gl::Texture2d
    virtual ImageRef createImage( const gl::Texture2dRef &texture ) = 0;

    //! Create an image from a Surface (uploads to GPU)
    virtual ImageRef createImage( const Surface &surface ) = 0;

    //! Draw an image at a position (1:1 pixel mapping) - base implementation provided
    virtual void drawImage( const ImageRef &image, const vec2 &position );

    //! Draw an image scaled to fit a destination rectangle
    virtual void drawImage( const ImageRef &image, const Rectf &destRect ) = 0;

    //! Draw a portion of an image (srcRect) to a destination rectangle
    virtual void drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect ) = 0;

    //! Draw an image mapped to arbitrary mesh geometry
    //! @param vertices Vertex positions
    //! @param uvs Texture coordinates (0-1 range)
    //! @param indices Triangle indices
    virtual void drawImageMesh( const ImageRef &image,
                                std::span<const vec2> vertices,
                                std::span<const vec2> uvs,
                                std::span<const uint16_t> indices,
                                float opacity = 1.0f ) = 0;

    // === Text API ===
    //! Draw a string at a position using Cinder's Font - base implementation provided
    virtual void drawString( const std::string &text, const vec2 &position,
                             const Font &font, const Paint &paint );

    //! Create a cached path from text (for custom effects like stroking text) - base implementation provided
    virtual CachedPathRef createTextPath( const std::string &text, const Font &font );

    // === Instanced Drawing API ===
    //! Draw multiple circles at different positions (same radius, same paint) - base implementation provided
    virtual void drawCircles( std::span<const vec2> positions, float radius, const Paint &paint );

    //! Draw multiple circles with full transforms (same radius, same paint) - base implementation provided
    virtual void drawCircles( std::span<const mat3> transforms, float radius, const Paint &paint );

    //! Draw multiple rectangles at different positions (same size, same paint) - base implementation provided
    virtual void drawRects( std::span<const vec2> positions, const vec2 &size, const Paint &paint );

    //! Draw multiple cached paths at different positions (same paint) - base implementation provided
    virtual void drawPaths( std::span<const vec2> positions, const CachedPathRef &path, const Paint &paint );

    //! Draw multiple cached paths with full transforms (same paint) - base implementation provided
    virtual void drawPaths( std::span<const mat3> transforms, const CachedPathRef &path, const Paint &paint );

    // === Clipping API ===
    // Clips are accumulated (intersected) within a save/restore block.
    // Each call to clipPath/clipRect/clipShape intersects with the current clip.
    // Call save() before clipping to preserve the previous clip state, then restore() to revert.

    //! Clip subsequent drawing to a rectangle (intersected with current clip)
    void clipRect( const Rectf &rect );

    //! Clip subsequent drawing to a path (intersected with current clip)
    void clipPath( const Path2d &path, FillRule rule = FillRule::NonZero );

    //! Clip subsequent drawing to a shape (intersected with current clip)
    void clipShape( const Shape2d &shape, FillRule rule = FillRule::NonZero );

    //! Clip subsequent drawing to a cached path (intersected with current clip)
    void clipPath( const CachedPathRef &path, FillRule rule = FillRule::NonZero );

    // === SVG Rendering ===
    //! Render an SVG document (base implementation provided)
    virtual void draw( const svg::Doc &svg );

    // === DisplayList API ===
    //! Create a display list for recording draw commands
    virtual DisplayListRef createDisplayList() = 0;

    //! Clear the glyph cache (call if fonts are unloaded)
    void clearGlyphCache() { mGlyphCache.clear(); }

protected:
    Canvas();

    // === Path conversion (Cinder -> Rive) ===
    //! Convert a Path2d to Rive RawPath
    rive::RawPath toRivePath( const Path2d &path );

    //! Convert a Shape2d to Rive RawPath
    rive::RawPath toRivePath( const Shape2d &shape );

    //! Convert a PolyLine to Rive RawPath
    rive::RawPath toRivePath( const PolyLine2f &polyline );

    // === Internal drawing ===
    //! Draw a path with the given paint (fill and/or stroke)
    void drawPathInternal( rive::RawPath rawPath, const Paint &paint,
                           bool fill, bool stroke, FillRule rule = FillRule::NonZero );

    // === Clipping implementation ===
    //! Called by clip methods to apply a clip path with current transform
    void implClipPath( const Path2d &path, FillRule rule );

    //! Called by clipPath(CachedPathRef) - uses source shape
    void implClipPath( const CachedPathRef &path, FillRule rule );

    //! Fill a shape (converts to Rive path internally)
    void implFillShape( const Shape2d &shape, const Paint &paint, FillRule rule );

    //! Stroke a shape (converts to Rive path internally)
    void implStrokeShape( const Shape2d &shape, const Paint &paint );

    //! Get or create a cached circle path at origin
    CachedPathRef getOrCreateCirclePath( float radius );

    //! Get or create a cached rectangle path at origin
    CachedPathRef getOrCreateRectPath( const vec2 &size );

    // === Rive context (owned by subclass, set during construction) ===
    std::unique_ptr<rive::gpu::RenderContext> mRiveContext;

    // Rive renderer (created per-frame in begin(), destroyed in end())
    std::unique_ptr<rive::RiveRenderer> mOwnedRiveRenderer;
    rive::RiveRenderer* mRiveRenderer = nullptr;

    // Frame state
    bool mInFrame = false;

    // Transform state
    mat3 mTransform;
    std::vector<mat3> mTransformStack;

    // Caches
    GlyphCache mGlyphCache;
    std::unordered_map<float, CachedPathRef> mCirclePathCache;  // radius -> path
    std::unordered_map<uint64_t, CachedPathRef> mRectPathCache;  // packed size -> path
};

using CanvasRef = std::shared_ptr<Canvas>;

} } // namespace cinder::vg
