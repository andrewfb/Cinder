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

#include <memory>
#include <vector>

namespace cinder { namespace vg {

//! Exception type for vector graphics errors
class CI_API VgExc : public Exception {
public:
    VgExc( const std::string &description ) : Exception( description ) {}
};

//! Options for canvas creation
struct CI_API CanvasOptions {
    bool disablePixelLocalStorage = false;  //!< Disable PLS (required for macOS GL 4.1)
    bool disableFragmentShaderInterlock = false;  //!< Disable FSI (required for macOS GL 4.1)
};

//! Canvas provides the main drawing interface for vector graphics.
//! This is an abstract base class - use CanvasGl for OpenGL rendering.
class CI_API Canvas {
public:
    virtual ~Canvas() = default;

    // === Frame Management ===
    //! Begin a new frame for rendering at the given size
    virtual void beginFrame( const ivec2 &size ) = 0;

    //! Begin a new frame for rendering with explicit width/height
    void beginFrame( int width, int height ) { beginFrame( ivec2( width, height ) ); }

    //! End the current frame and flush to the GPU
    virtual void endFrame() = 0;

    //! Check if currently in a frame
    virtual bool inFrame() const = 0;

    // === Transform Stack ===
    //! Push current transform state onto the stack
    virtual void save() = 0;

    //! Pop transform state from the stack
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

    // === Cinder Path/Shape Integration ===
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

protected:
    Canvas() = default;
};

using CanvasRef = std::shared_ptr<Canvas>;

} } // namespace cinder::vg
