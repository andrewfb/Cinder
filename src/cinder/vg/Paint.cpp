/*
 Copyright (c) 2024, The Cinder Project
 */

#include "cinder/vg/Paint.h"

// Rive includes
#include "rive/renderer/render_context.hpp"
#include "rive/shapes/paint/color.hpp"
// For gradients
#include "gradient.hpp"

using namespace rive;
using namespace rive::gpu;

namespace cinder { namespace vg {

// Helper to convert Cinder ColorAf to Rive ColorInt (ARGB)
static ColorInt toRiveColor( const ColorAf &c, float opacity )
{
    float a = c.a * opacity;
    return colorARGB(
        static_cast<int>( a * 255 ),
        static_cast<int>( c.r * 255 ),
        static_cast<int>( c.g * 255 ),
        static_cast<int>( c.b * 255 )
    );
}

// Convert LineCap to Rive StrokeCap
static StrokeCap toRiveStrokeCap( LineCap cap )
{
    switch( cap ) {
        case LineCap::Butt:   return StrokeCap::butt;
        case LineCap::Round:  return StrokeCap::round;
        case LineCap::Square: return StrokeCap::square;
    }
    return StrokeCap::butt;
}

// Convert LineJoin to Rive StrokeJoin
static StrokeJoin toRiveStrokeJoin( LineJoin join )
{
    switch( join ) {
        case LineJoin::Miter: return StrokeJoin::miter;
        case LineJoin::Round: return StrokeJoin::round;
        case LineJoin::Bevel: return StrokeJoin::bevel;
    }
    return StrokeJoin::miter;
}

// Convert BlendMode to Rive BlendMode
static rive::BlendMode toRiveBlendMode( BlendMode mode )
{
    switch( mode ) {
        case BlendMode::SrcOver:     return rive::BlendMode::srcOver;
        case BlendMode::Screen:      return rive::BlendMode::screen;
        case BlendMode::Overlay:     return rive::BlendMode::overlay;
        case BlendMode::Darken:      return rive::BlendMode::darken;
        case BlendMode::Lighten:     return rive::BlendMode::lighten;
        case BlendMode::ColorDodge:  return rive::BlendMode::colorDodge;
        case BlendMode::ColorBurn:   return rive::BlendMode::colorBurn;
        case BlendMode::HardLight:   return rive::BlendMode::hardLight;
        case BlendMode::SoftLight:   return rive::BlendMode::softLight;
        case BlendMode::Difference:  return rive::BlendMode::difference;
        case BlendMode::Exclusion:   return rive::BlendMode::exclusion;
        case BlendMode::Multiply:    return rive::BlendMode::multiply;
        case BlendMode::Hue:         return rive::BlendMode::hue;
        case BlendMode::Saturation:  return rive::BlendMode::saturation;
        case BlendMode::Color:       return rive::BlendMode::color;
        case BlendMode::Luminosity:  return rive::BlendMode::luminosity;
    }
    return rive::BlendMode::srcOver;
}

// ------------------------------------------------------------------------------------------------
// Paint implementation
// ------------------------------------------------------------------------------------------------

Paint::Paint()
{
}

Paint& Paint::setColor( const ColorAf &color )
{
    mType = Type::Solid;
    mColor = color;
    return *this;
}

Paint& Paint::setColor( const Color &color )
{
    return setColor( ColorAf( color, 1.0f ) );
}

Paint& Paint::setLinearGradient( const vec2 &start, const vec2 &end,
                                  const std::vector<GradientStop> &stops )
{
    mType = Type::LinearGradient;
    mGradientStart = start;
    mGradientEnd = end;
    mGradientStops = stops;
    return *this;
}

Paint& Paint::setLinearGradient( const vec2 &start, const vec2 &end,
                                  const ColorAf &startColor, const ColorAf &endColor )
{
    return setLinearGradient( start, end, {
        GradientStop( 0.0f, startColor ),
        GradientStop( 1.0f, endColor )
    });
}

Paint& Paint::setRadialGradient( const vec2 &center, float radius,
                                  const std::vector<GradientStop> &stops )
{
    mType = Type::RadialGradient;
    mGradientCenter = center;
    mGradientRadius = radius;
    mGradientStops = stops;
    return *this;
}

Paint& Paint::setRadialGradient( const vec2 &center, float radius,
                                  const ColorAf &innerColor, const ColorAf &outerColor )
{
    return setRadialGradient( center, radius, {
        GradientStop( 0.0f, innerColor ),
        GradientStop( 1.0f, outerColor )
    });
}

Paint& Paint::setGradientSpread( GradientSpread spread )
{
    mGradientSpread = spread;
    return *this;
}

Paint& Paint::setGradientTransform( const mat3 &transform )
{
    mGradientTransform = transform;
    return *this;
}

Paint& Paint::setStrokeWidth( float width )
{
    mStrokeWidth = width;
    return *this;
}

Paint& Paint::setLineCap( LineCap cap )
{
    mLineCap = cap;
    return *this;
}

Paint& Paint::setLineJoin( LineJoin join )
{
    mLineJoin = join;
    return *this;
}

Paint& Paint::setMiterLimit( float limit )
{
    mMiterLimit = limit;
    return *this;
}

Paint& Paint::setDash( const std::vector<float> &pattern, float offset )
{
    mDashPattern = pattern;
    mDashOffset = offset;
    return *this;
}

Paint& Paint::setFeather( float amount )
{
    mFeather = amount;
    return *this;
}

Paint& Paint::setBlendMode( BlendMode mode )
{
    mBlendMode = mode;
    return *this;
}

Paint& Paint::setOpacity( float opacity )
{
    mOpacity = glm::clamp( opacity, 0.0f, 1.0f );
    return *this;
}

rcp<RenderPaint> Paint::createRivePaint( RenderContext* ctx, bool isStroke ) const
{
    if( ! ctx ) return nullptr;

    auto paint = ctx->makeRenderPaint();
    if( ! paint ) return nullptr;

    // Set fill/stroke style
    paint->style( isStroke ? RenderPaintStyle::stroke : RenderPaintStyle::fill );

    // Set blend mode
    paint->blendMode( toRiveBlendMode( mBlendMode ) );

    // Handle different paint types
    switch( mType ) {
        case Type::Solid:
            paint->color( toRiveColor( mColor, mOpacity ) );
            break;

        case Type::LinearGradient: {
            // Build gradient colors and stops
            std::vector<ColorInt> colors;
            std::vector<float> stops;
            colors.reserve( mGradientStops.size() );
            stops.reserve( mGradientStops.size() );

            for( const auto& stop : mGradientStops ) {
                colors.push_back( toRiveColor( stop.color, mOpacity ) );
                stops.push_back( stop.position );
            }

            // Rive needs at least 2 stops
            if( colors.size() < 2 ) {
                ColorInt c = colors.empty() ? toRiveColor( ColorAf::white(), mOpacity ) : colors[0];
                colors = { c, c };
                stops = { 0.0f, 1.0f };
            }

            // Create gradient shader and set it
            auto gradient = Gradient::MakeLinear(
                mGradientStart.x, mGradientStart.y,
                mGradientEnd.x, mGradientEnd.y,
                colors.data(), stops.data(), colors.size()
            );
            paint->shader( std::move( gradient ) );
            break;
        }

        case Type::RadialGradient: {
            std::vector<ColorInt> colors;
            std::vector<float> stops;
            colors.reserve( mGradientStops.size() );
            stops.reserve( mGradientStops.size() );

            for( const auto& stop : mGradientStops ) {
                colors.push_back( toRiveColor( stop.color, mOpacity ) );
                stops.push_back( stop.position );
            }

            if( colors.size() < 2 ) {
                ColorInt c = colors.empty() ? toRiveColor( ColorAf::white(), mOpacity ) : colors[0];
                colors = { c, c };
                stops = { 0.0f, 1.0f };
            }

            // Create gradient shader and set it
            auto gradient = Gradient::MakeRadial(
                mGradientCenter.x, mGradientCenter.y, mGradientRadius,
                colors.data(), stops.data(), colors.size()
            );
            paint->shader( std::move( gradient ) );
            break;
        }

        case Type::Texture:
            // TODO: Implement texture support in Phase 5
            paint->color( toRiveColor( mColor, mOpacity ) );
            break;
    }

    // Apply stroke properties if stroking
    if( isStroke ) {
        paint->thickness( mStrokeWidth );
        paint->cap( toRiveStrokeCap( mLineCap ) );
        paint->join( toRiveStrokeJoin( mLineJoin ) );
        // TODO: Dash patterns would need path dashing implementation
    }

    // Apply feathering if set
    if( mFeather > 0 ) {
        paint->feather( mFeather );
    }

    return paint;
}

} } // namespace cinder::vg
