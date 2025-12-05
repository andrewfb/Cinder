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

namespace cinder { namespace svg { class Doc; } }

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

//! Canvas provides the main drawing interface for vector graphics.
//! This is an abstract base class - use CanvasGl for OpenGL rendering.
class CI_API Canvas {
public:
    virtual ~Canvas() = default;

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
    virtual void save() = 0;

    //! Pop transform and clipping state from the stack
    virtual void restore() = 0;

    //! Translate the current transform
    virtual void translate( const vec2 &offset ) = 0;
    void translate( float x, float y ) { translate( vec2( x, y ) ); }

    //! Rotate the current transform (radians)
    virtual void rotate( float radians ) = 0;

    //! Scale the current transform
    virtual void scale( const vec2 &s ) = 0;
    void scale( float sx, float sy ) { scale( vec2( sx, sy ) ); }
    void scale( float s ) { scale( vec2( s, s ) ); }

    //! Concatenate a transform matrix
    virtual void transform( const mat3 &m ) = 0;

    //! Set the transform matrix directly
    virtual void setTransform( const mat3 &m ) = 0;

    //! Reset transform to identity
    virtual void resetTransform() = 0;

    //! Get the current transform matrix
    virtual mat3 getTransform() const = 0;

    // === Drawing Primitives ===
    //! Fill a rectangle
    virtual void fillRect( const Rectf &rect, const Paint &paint ) = 0;

    //! Stroke a rectangle
    virtual void strokeRect( const Rectf &rect, const Paint &paint ) = 0;

    //! Fill a rounded rectangle
    virtual void fillRoundedRect( const Rectf &rect, float radius, const Paint &paint ) = 0;

    //! Stroke a rounded rectangle
    virtual void strokeRoundedRect( const Rectf &rect, float radius, const Paint &paint ) = 0;

    //! Fill a circle
    virtual void fillCircle( const vec2 &center, float radius, const Paint &paint ) = 0;

    //! Stroke a circle
    virtual void strokeCircle( const vec2 &center, float radius, const Paint &paint ) = 0;

    //! Fill an ellipse
    virtual void fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint ) = 0;

    //! Stroke an ellipse
    virtual void strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint ) = 0;

    //! Draw a line (stroked only)
    virtual void drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint ) = 0;

    // === Path Drawing (uncached) ===
    //! Fill a Path2d
    virtual void fillPath( const Path2d &path, const Paint &paint, FillRule rule = FillRule::NonZero ) = 0;

    //! Stroke a Path2d
    virtual void strokePath( const Path2d &path, const Paint &paint ) = 0;

    //! Fill a Shape2d
    virtual void fillShape( const Shape2d &shape, const Paint &paint, FillRule rule = FillRule::NonZero ) = 0;

    //! Stroke a Shape2d
    virtual void strokeShape( const Shape2d &shape, const Paint &paint ) = 0;

    //! Stroke a PolyLine
    virtual void strokePolyLine( const PolyLine2f &polyline, const Paint &paint ) = 0;

    //! Fill a closed PolyLine
    virtual void fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule = FillRule::NonZero ) = 0;

    // === Cached Path API ===
    //! Create a cached path from a Path2d. Cached paths are more efficient for repeated drawing.
    virtual CachedPathRef createPath( const Path2d &path ) = 0;

    //! Create a cached path from a Shape2d
    virtual CachedPathRef createPath( const Shape2d &shape ) = 0;

    //! Fill a cached path
    virtual void fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule = FillRule::NonZero ) = 0;

    //! Stroke a cached path
    virtual void strokePath( const CachedPathRef &path, const Paint &paint ) = 0;

    // === Image API ===
    //! Create an image from a gl::Texture2d
    virtual ImageRef createImage( const gl::Texture2dRef &texture ) = 0;

    //! Create an image from a Surface (uploads to GPU)
    virtual ImageRef createImage( const Surface &surface ) = 0;

    //! Draw an image at a position (1:1 pixel mapping)
    virtual void drawImage( const ImageRef &image, const vec2 &position ) = 0;

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
    //! Draw a string at a position using Cinder's Font
    virtual void drawString( const std::string &text, const vec2 &position,
                             const Font &font, const Paint &paint ) = 0;

    //! Create a cached path from text (for custom effects like stroking text)
    virtual CachedPathRef createTextPath( const std::string &text, const Font &font ) = 0;

    // === Instanced Drawing API ===
    //! Draw multiple circles at different positions (same radius, same paint)
    virtual void drawCircles( std::span<const vec2> positions, float radius, const Paint &paint ) = 0;

    //! Draw multiple circles with full transforms (same radius, same paint)
    virtual void drawCircles( std::span<const mat3> transforms, float radius, const Paint &paint ) = 0;

    //! Draw multiple rectangles at different positions (same size, same paint)
    virtual void drawRects( std::span<const vec2> positions, const vec2 &size, const Paint &paint ) = 0;

    //! Draw multiple cached paths at different positions (same paint)
    virtual void drawPaths( std::span<const vec2> positions, const CachedPathRef &path, const Paint &paint ) = 0;

    //! Draw multiple cached paths with full transforms (same paint)
    virtual void drawPaths( std::span<const mat3> transforms, const CachedPathRef &path, const Paint &paint ) = 0;

    // === Clipping API ===
    // Clips are accumulated (intersected) within a save/restore block.
    // Each call to clipPath/clipRect/clipShape intersects with the current clip.
    // Call save() before clipping to preserve the previous clip state, then restore() to revert.

    //! Clip subsequent drawing to a rectangle (intersected with current clip)
    virtual void clipRect( const Rectf &rect ) = 0;

    //! Clip subsequent drawing to a path (intersected with current clip)
    virtual void clipPath( const Path2d &path, FillRule rule = FillRule::NonZero ) = 0;

    //! Clip subsequent drawing to a shape (intersected with current clip)
    virtual void clipShape( const Shape2d &shape, FillRule rule = FillRule::NonZero ) = 0;

    //! Clip subsequent drawing to a cached path (intersected with current clip)
    virtual void clipPath( const CachedPathRef &path, FillRule rule = FillRule::NonZero ) = 0;

    // === SVG Rendering ===
    //! Render an SVG document
    virtual void draw( const svg::Doc &svg ) = 0;

protected:
    Canvas() = default;
};

using CanvasRef = std::shared_ptr<Canvas>;

} } // namespace cinder::vg
