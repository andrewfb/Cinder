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

#include "cinder/vg/Canvas.h"
#include "cinder/svg/Svg.h"
#include "cinder/Log.h"

#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/math/raw_path.hpp"

using namespace rive;
using namespace rive::gpu;

namespace cinder { namespace vg {

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
// GlyphCache implementation
// ------------------------------------------------------------------------------------------------

CachedPathRef GlyphCache::getGlyph( Canvas* canvas, const Font &font, Font::Glyph glyphIndex )
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
// Canvas implementation - Transform Stack
// ------------------------------------------------------------------------------------------------

Canvas::Canvas()
    : mTransform( mat3() )
{
}

Canvas::~Canvas()
{
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Path Conversion (Cinder -> Rive)
// ------------------------------------------------------------------------------------------------

RawPath Canvas::toRivePath( const Path2d &path )
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

RawPath Canvas::toRivePath( const Shape2d &shape )
{
    RawPath rawPath;

    for( const auto& contour : shape.getContours() ) {
        RawPath contourPath = toRivePath( contour );
        rawPath.addPath( contourPath, nullptr ); // nullptr = identity transform
    }

    return rawPath;
}

RawPath Canvas::toRivePath( const PolyLine2f &polyline )
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
// Canvas implementation - Internal Drawing
// ------------------------------------------------------------------------------------------------

void Canvas::drawPathInternal( RawPath rawPath, const Paint &paint,
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

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Fill/Stroke Shape (uses Rive)
// ------------------------------------------------------------------------------------------------

void Canvas::implFillShape( const Shape2d &shape, const Paint &paint, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, true, false, rule );
}

void Canvas::implStrokeShape( const Shape2d &shape, const Paint &paint )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Drawing outside of begin()/end()" );

    RawPath rawPath = toRivePath( shape );
    drawPathInternal( std::move( rawPath ), paint, false, true );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Clipping (uses Rive)
// ------------------------------------------------------------------------------------------------

void Canvas::implClipPath( const Path2d &path, FillRule rule )
{
    CI_ASSERT_MSG( mInFrame && mRiveRenderer, "Clipping outside of begin()/end()" );

    RawPath rawPath = toRivePath( path );
    auto rivePath = mRiveContext->makeRenderPath( rawPath, toRiveFillRule( rule ) );

    // Apply transform, clip, then restore transform (but keep clip)
    // Rive clips accumulate within a save/restore block
    mRiveRenderer->transform( toRiveMat( mTransform ) );
    mRiveRenderer->clipPath( rivePath.get() );
    mRiveRenderer->transform( toRiveMat( glm::inverse( mTransform ) ) );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Transform Stack
// ------------------------------------------------------------------------------------------------

void Canvas::translate( const vec2 &offset )
{
    mat3 t( 1.0f );
    t[2][0] = offset.x;
    t[2][1] = offset.y;
    mTransform = mTransform * t;
}

void Canvas::rotate( float radians )
{
    float c = std::cos( radians );
    float s = std::sin( radians );
    mat3 r( 1.0f );
    r[0][0] = c;  r[0][1] = s;
    r[1][0] = -s; r[1][1] = c;
    mTransform = mTransform * r;
}

void Canvas::scale( const vec2 &s )
{
    mat3 sc( 1.0f );
    sc[0][0] = s.x;
    sc[1][1] = s.y;
    mTransform = mTransform * sc;
}

void Canvas::transform( const mat3 &m )
{
    mTransform = mTransform * m;
}

void Canvas::setTransform( const mat3 &m )
{
    mTransform = m;
}

void Canvas::resetTransform()
{
    mTransform = mat3();
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Save/Restore
// ------------------------------------------------------------------------------------------------

void Canvas::save()
{
    mTransformStack.push_back( mTransform );
    if( mRiveRenderer ) {
        mRiveRenderer->save();
    }
}

void Canvas::restore()
{
    if( ! mTransformStack.empty() ) {
        mTransform = mTransformStack.back();
        mTransformStack.pop_back();
    }
    if( mRiveRenderer ) {
        mRiveRenderer->restore();
    }
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Clipping
// ------------------------------------------------------------------------------------------------

void Canvas::clipRect( const Rectf &rect )
{
    Path2d path;
    path.moveTo( rect.getUpperLeft() );
    path.lineTo( rect.getUpperRight() );
    path.lineTo( rect.getLowerRight() );
    path.lineTo( rect.getLowerLeft() );
    path.close();
    implClipPath( path, FillRule::NonZero );
}

void Canvas::clipPath( const Path2d &path, FillRule rule )
{
    implClipPath( path, rule );
}

void Canvas::clipShape( const Shape2d &shape, FillRule rule )
{
    // Convert Shape2d to a single Path2d by appending all contours
    Path2d combinedPath;
    for( const auto &contour : shape.getContours() ) {
        if( combinedPath.empty() ) {
            combinedPath = contour;
        } else {
            // Append contour points
            const auto &points = contour.getPoints();
            const auto &segments = contour.getSegments();
            if( ! points.empty() ) {
                combinedPath.moveTo( points[0] );
                size_t ptIdx = 1;
                for( auto seg : segments ) {
                    switch( seg ) {
                        case Path2d::LINETO:
                            combinedPath.lineTo( points[ptIdx++] );
                            break;
                        case Path2d::QUADTO:
                            combinedPath.quadTo( points[ptIdx], points[ptIdx + 1] );
                            ptIdx += 2;
                            break;
                        case Path2d::CUBICTO:
                            combinedPath.curveTo( points[ptIdx], points[ptIdx + 1], points[ptIdx + 2] );
                            ptIdx += 3;
                            break;
                        case Path2d::CLOSE:
                            combinedPath.close();
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    implClipPath( combinedPath, rule );
}

void Canvas::clipPath( const CachedPathRef &path, FillRule rule )
{
    implClipPath( path, rule );
}

void Canvas::implClipPath( const CachedPathRef &path, FillRule rule )
{
    // Default implementation: use the source shape from cached path
    if( path ) {
        const Shape2d &shape = path->getSourceShape();
        // Convert shape to Path2d and call the Path2d version
        for( const auto &contour : shape.getContours() ) {
            implClipPath( contour, rule );
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Drawing Primitives
// ------------------------------------------------------------------------------------------------

void Canvas::fillRect( const Rectf &rect, const Paint &paint )
{
    Path2d path;
    path.moveTo( rect.getUpperLeft() );
    path.lineTo( rect.getUpperRight() );
    path.lineTo( rect.getLowerRight() );
    path.lineTo( rect.getLowerLeft() );
    path.close();
    fillPath( path, paint );
}

void Canvas::strokeRect( const Rectf &rect, const Paint &paint )
{
    Path2d path;
    path.moveTo( rect.getUpperLeft() );
    path.lineTo( rect.getUpperRight() );
    path.lineTo( rect.getLowerRight() );
    path.lineTo( rect.getLowerLeft() );
    path.close();
    strokePath( path, paint );
}

void Canvas::fillRoundedRect( const Rectf &rect, float radius, const Paint &paint )
{
    float r = std::min( radius, std::min( rect.getWidth(), rect.getHeight() ) / 2.0f );
    float x1 = rect.x1, y1 = rect.y1, x2 = rect.x2, y2 = rect.y2;

    Path2d path;
    path.moveTo( x1 + r, y1 );
    path.lineTo( x2 - r, y1 );
    path.arc( vec2( x2 - r, y1 + r ), r, -float(M_PI_2), 0.0f );
    path.lineTo( x2, y2 - r );
    path.arc( vec2( x2 - r, y2 - r ), r, 0.0f, float(M_PI_2) );
    path.lineTo( x1 + r, y2 );
    path.arc( vec2( x1 + r, y2 - r ), r, float(M_PI_2), float(M_PI) );
    path.lineTo( x1, y1 + r );
    path.arc( vec2( x1 + r, y1 + r ), r, float(M_PI), float(M_PI) + float(M_PI_2) );
    path.close();
    fillPath( path, paint );
}

void Canvas::strokeRoundedRect( const Rectf &rect, float radius, const Paint &paint )
{
    float r = std::min( radius, std::min( rect.getWidth(), rect.getHeight() ) / 2.0f );
    float x1 = rect.x1, y1 = rect.y1, x2 = rect.x2, y2 = rect.y2;

    Path2d path;
    path.moveTo( x1 + r, y1 );
    path.lineTo( x2 - r, y1 );
    path.arc( vec2( x2 - r, y1 + r ), r, -float(M_PI_2), 0.0f );
    path.lineTo( x2, y2 - r );
    path.arc( vec2( x2 - r, y2 - r ), r, 0.0f, float(M_PI_2) );
    path.lineTo( x1 + r, y2 );
    path.arc( vec2( x1 + r, y2 - r ), r, float(M_PI_2), float(M_PI) );
    path.lineTo( x1, y1 + r );
    path.arc( vec2( x1 + r, y1 + r ), r, float(M_PI), float(M_PI) + float(M_PI_2) );
    path.close();
    strokePath( path, paint );
}

void Canvas::fillCircle( const vec2 &center, float radius, const Paint &paint )
{
    Path2d path;
    path.arc( center, radius, 0, float( M_PI * 2 ), true );
    path.close();
    fillPath( path, paint );
}

void Canvas::strokeCircle( const vec2 &center, float radius, const Paint &paint )
{
    Path2d path;
    path.arc( center, radius, 0, float( M_PI * 2 ), true );
    path.close();
    strokePath( path, paint );
}

void Canvas::fillEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    // Build ellipse as transformed circle
    Path2d path;
    const int segments = 64;
    for( int i = 0; i <= segments; ++i ) {
        float t = float( i ) / float( segments ) * float( M_PI * 2 );
        vec2 p = center + vec2( std::cos( t ) * radii.x, std::sin( t ) * radii.y );
        if( i == 0 )
            path.moveTo( p );
        else
            path.lineTo( p );
    }
    path.close();
    fillPath( path, paint );
}

void Canvas::strokeEllipse( const vec2 &center, const vec2 &radii, const Paint &paint )
{
    Path2d path;
    const int segments = 64;
    for( int i = 0; i <= segments; ++i ) {
        float t = float( i ) / float( segments ) * float( M_PI * 2 );
        vec2 p = center + vec2( std::cos( t ) * radii.x, std::sin( t ) * radii.y );
        if( i == 0 )
            path.moveTo( p );
        else
            path.lineTo( p );
    }
    path.close();
    strokePath( path, paint );
}

void Canvas::drawLine( const vec2 &p0, const vec2 &p1, const Paint &paint )
{
    Path2d path;
    path.moveTo( p0 );
    path.lineTo( p1 );
    strokePath( path, paint );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Path Drawing (uncached)
// ------------------------------------------------------------------------------------------------

void Canvas::fillPath( const Path2d &path, const Paint &paint, FillRule rule )
{
    Shape2d shape;
    shape.appendContour( path );
    implFillShape( shape, paint, rule );
}

void Canvas::strokePath( const Path2d &path, const Paint &paint )
{
    Shape2d shape;
    shape.appendContour( path );
    implStrokeShape( shape, paint );
}

void Canvas::fillShape( const Shape2d &shape, const Paint &paint, FillRule rule )
{
    implFillShape( shape, paint, rule );
}

void Canvas::strokeShape( const Shape2d &shape, const Paint &paint )
{
    implStrokeShape( shape, paint );
}

void Canvas::strokePolyLine( const PolyLine2f &polyline, const Paint &paint )
{
    // Convert polyline to path
    Path2d path;
    const auto& points = polyline.getPoints();
    if( ! points.empty() ) {
        path.moveTo( points[0] );
        for( size_t i = 1; i < points.size(); ++i ) {
            path.lineTo( points[i] );
        }
        if( polyline.isClosed() ) {
            path.close();
        }
    }
    Shape2d shape;
    shape.appendContour( path );
    implStrokeShape( shape, paint );
}

void Canvas::fillPolyLine( const PolyLine2f &polyline, const Paint &paint, FillRule rule )
{
    // Convert polyline to closed path
    Path2d path;
    const auto& points = polyline.getPoints();
    if( ! points.empty() ) {
        path.moveTo( points[0] );
        for( size_t i = 1; i < points.size(); ++i ) {
            path.lineTo( points[i] );
        }
        path.close();
    }
    Shape2d shape;
    shape.appendContour( path );
    implFillShape( shape, paint, rule );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Cached Path API
// ------------------------------------------------------------------------------------------------

CachedPathRef Canvas::createPath( const Path2d &path )
{
    Shape2d shape;
    shape.appendContour( path );
    return createPath( shape );
}

void Canvas::fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule )
{
    if( ! path ) return;
    // Use the source shape stored in the cached path
    implFillShape( path->getSourceShape(), paint, rule );
}

void Canvas::strokePath( const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return;
    // Use the source shape stored in the cached path
    implStrokeShape( path->getSourceShape(), paint );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Instanced Drawing
// ------------------------------------------------------------------------------------------------

CachedPathRef Canvas::getOrCreateCirclePath( float radius )
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

CachedPathRef Canvas::getOrCreateRectPath( const vec2 &size )
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

void Canvas::drawCircles( std::span<const vec2> positions, float radius, const Paint &paint )
{
    auto circlePath = getOrCreateCirclePath( radius );
    if( ! circlePath ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( circlePath, paint );
        restore();
    }
}

void Canvas::drawCircles( std::span<const mat3> transforms, float radius, const Paint &paint )
{
    auto circlePath = getOrCreateCirclePath( radius );
    if( ! circlePath ) return;

    for( const mat3& xform : transforms ) {
        save();
        transform( xform );
        fillPath( circlePath, paint );
        restore();
    }
}

void Canvas::drawRects( std::span<const vec2> positions, const vec2 &size, const Paint &paint )
{
    auto rectPath = getOrCreateRectPath( size );
    if( ! rectPath ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( rectPath, paint );
        restore();
    }
}

void Canvas::drawPaths( std::span<const vec2> positions, const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return;

    for( const vec2& pos : positions ) {
        save();
        translate( pos );
        fillPath( path, paint );
        restore();
    }
}

void Canvas::drawPaths( std::span<const mat3> transforms, const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return;

    for( const mat3& xform : transforms ) {
        save();
        transform( xform );
        fillPath( path, paint );
        restore();
    }
}

void Canvas::drawFrozenPaths( std::span<const vec2> positions,
                               const FrozenPathRef &frozenPath,
                               const Paint &paint )
{
    if( ! frozenPath ) return;

    for( const auto& pos : positions ) {
        save();
        translate( pos );
        drawFrozenPath( frozenPath, paint );
        restore();
    }
}

void Canvas::drawFrozenPaths( std::span<const mat3> transforms,
                               const FrozenPathRef &frozenPath,
                               const Paint &paint )
{
    if( ! frozenPath ) return;

    for( const auto& xform : transforms ) {
        save();
        transform( xform );
        drawFrozenPath( frozenPath, paint );
        restore();
    }
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - Text
// ------------------------------------------------------------------------------------------------

void Canvas::drawString( const std::string &text, const vec2 &position,
                          const Font &font, const Paint &paint )
{
    if( text.empty() ) return;

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

CachedPathRef Canvas::createTextPath( const std::string &text, const Font &font )
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
// Canvas implementation - Convenience overloads
// ------------------------------------------------------------------------------------------------

void Canvas::drawImage( const ImageRef &image, const vec2 &position )
{
    if( ! image ) return;
    Rectf destRect( position, position + vec2( image->getSize() ) );
    drawImage( image, destRect );
}

// ------------------------------------------------------------------------------------------------
// Canvas implementation - SVG Rendering
// ------------------------------------------------------------------------------------------------

// Internal SVG renderer that uses vg::Canvas
class SvgRendererVg : public svg::Renderer {
public:
    SvgRendererVg( Canvas* canvas )
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

    Canvas* mCanvas;
    std::vector<svg::Paint> mFillStack;
    std::vector<svg::Paint> mStrokeStack;
    std::vector<float> mFillOpacityStack;
    std::vector<float> mStrokeOpacityStack;
    std::vector<float> mStrokeWidthStack;
    std::vector<svg::FillRule> mFillRuleStack;
    std::vector<svg::LineCap> mLineCapStack;
    std::vector<svg::LineJoin> mLineJoinStack;
};

void Canvas::draw( const svg::Doc &svg )
{
    SvgRendererVg renderer( this );
    svg.render( renderer );
}

} } // namespace cinder::vg
