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

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/Matrix.h"
#include "cinder/Cinder.h"

#include <vector>

// Forward declarations for Rive types
namespace rive {
    class RenderPaint;
    template<typename T> class rcp;
    namespace gpu {
        class RenderContext;
    }
}

namespace cinder { namespace vg {

//! Gradient stop for multi-color gradients
struct CI_API GradientStop {
    float position;  //! Position along gradient (0.0 to 1.0)
    ColorAf color;   //! Color at this position

    GradientStop() : position( 0 ), color( ColorAf::white() ) {}
    GradientStop( float pos, const ColorAf &c ) : position( pos ), color( c ) {}
};

//! Stroke line cap style
enum class LineCap {
    Butt,   //! Flat end at exact endpoint
    Round,  //! Semicircle extending past endpoint
    Square  //! Square extending past endpoint
};

//! Stroke line join style
enum class LineJoin {
    Miter,  //! Sharp corner (limited by miter limit)
    Round,  //! Rounded corner
    Bevel   //! Flat corner
};

//! Blend modes for compositing
enum class BlendMode {
    SrcOver,    //! Default (Porter-Duff src-over)
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Multiply,
    Hue,
    Saturation,
    Color,
    Luminosity
};

//! Fill rule for determining inside/outside of paths
enum class FillRule {
    NonZero,  //! Non-zero winding rule
    EvenOdd   //! Even-odd (alternating) rule
};

//! Gradient spread mode (how gradient extends beyond defined range)
enum class GradientSpread {
    Pad,     //! Extend edge colors
    Repeat,  //! Tile the gradient
    Reflect  //! Mirror the gradient
};

//! Paint defines the visual style for fill and stroke operations.
//! Supports solid colors, gradients, textures, and Rive's feathering.
//! Mutable and trivially copyable for creating presets.
class CI_API Paint {
public:
    Paint();

    // === Solid Color ===
    //! Set solid fill/stroke color
    Paint& setColor( const ColorAf &color );
    //! Set solid fill/stroke color (converts to ColorAf)
    Paint& setColor( const Color &color );
    //! Get the current color
    const ColorAf& getColor() const { return mColor; }

    // === Linear Gradient ===
    //! Set linear gradient with multiple color stops
    Paint& setLinearGradient( const vec2 &start, const vec2 &end,
                              const std::vector<GradientStop> &stops );
    //! Set simple two-color linear gradient
    Paint& setLinearGradient( const vec2 &start, const vec2 &end,
                              const ColorAf &startColor, const ColorAf &endColor );

    // === Radial Gradient ===
    //! Set radial gradient with multiple color stops
    Paint& setRadialGradient( const vec2 &center, float radius,
                              const std::vector<GradientStop> &stops );
    //! Set simple two-color radial gradient
    Paint& setRadialGradient( const vec2 &center, float radius,
                              const ColorAf &innerColor, const ColorAf &outerColor );

    // === Gradient Properties ===
    //! Set gradient spread mode
    Paint& setGradientSpread( GradientSpread spread );
    //! Set gradient local transform matrix
    Paint& setGradientTransform( const mat3 &transform );

    // === Stroke Properties ===
    //! Set stroke width in pixels
    Paint& setStrokeWidth( float width );
    float getStrokeWidth() const { return mStrokeWidth; }

    //! Set line cap style for stroke endpoints
    Paint& setLineCap( LineCap cap );
    LineCap getLineCap() const { return mLineCap; }

    //! Set line join style for stroke corners
    Paint& setLineJoin( LineJoin join );
    LineJoin getLineJoin() const { return mLineJoin; }

    //! Set miter limit for LineJoin::Miter
    Paint& setMiterLimit( float limit );
    float getMiterLimit() const { return mMiterLimit; }

    //! Set dash pattern and offset for dashed strokes
    //! @param pattern Alternating on/off lengths (e.g., {10, 5} = 10px on, 5px off)
    //! @param offset Starting offset into the pattern
    Paint& setDash( const std::vector<float> &pattern, float offset = 0 );
    const std::vector<float>& getDashPattern() const { return mDashPattern; }
    float getDashOffset() const { return mDashOffset; }

    // === Feathering (Rive's soft edge feature) ===
    //! Set feather amount for soft edges (blur radius)
    Paint& setFeather( float amount );
    float getFeather() const { return mFeather; }

    // === Blending ===
    //! Set blend mode for compositing
    Paint& setBlendMode( BlendMode mode );
    BlendMode getBlendMode() const { return mBlendMode; }

    //! Set global opacity multiplier (0.0 to 1.0)
    Paint& setOpacity( float opacity );
    float getOpacity() const { return mOpacity; }

    // === Image Fill ===
    //! Set an image as the fill pattern (requires vg::Image from Canvas::createImage)
    Paint& setImage( const std::shared_ptr<class Image> &image );
    //! Set transform for image fill (for tiling, scaling, rotation)
    Paint& setImageTransform( const mat3 &transform );
    //! Get the image (if set)
    std::shared_ptr<class Image> getImage() const { return mImage; }
    //! Get the image transform
    const mat3& getImageTransform() const { return mImageTransform; }

    // === Type Queries ===
    enum class Type { Solid, LinearGradient, RadialGradient, Image };
    Type getType() const { return mType; }
    bool isSolid() const { return mType == Type::Solid; }
    bool isGradient() const { return mType == Type::LinearGradient || mType == Type::RadialGradient; }
    bool isImage() const { return mType == Type::Image; }

    // === Internal: Create Rive Paint ===
    //! Create a Rive RenderPaint from this Paint (used internally by Canvas)
    rive::rcp<rive::RenderPaint> createRivePaint( rive::gpu::RenderContext* ctx, bool isStroke ) const;

private:
    Type mType = Type::Solid;

    // Solid color
    ColorAf mColor = ColorAf::white();

    // Gradient data
    vec2 mGradientStart = vec2( 0 );
    vec2 mGradientEnd = vec2( 0 );
    vec2 mGradientCenter = vec2( 0 );
    float mGradientRadius = 0;
    std::vector<GradientStop> mGradientStops;
    GradientSpread mGradientSpread = GradientSpread::Pad;
    mat3 mGradientTransform = mat3();

    // Stroke properties
    float mStrokeWidth = 1.0f;
    LineCap mLineCap = LineCap::Butt;
    LineJoin mLineJoin = LineJoin::Miter;
    float mMiterLimit = 4.0f;
    std::vector<float> mDashPattern;
    float mDashOffset = 0;

    // Effects
    float mFeather = 0;
    BlendMode mBlendMode = BlendMode::SrcOver;
    float mOpacity = 1.0f;

    // Image fill
    std::shared_ptr<class Image> mImage;
    mat3 mImageTransform = mat3();
};

} } // namespace cinder::vg
