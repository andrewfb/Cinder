/*
 VectorGraphicsDemoD3d12 - Interactive demo framework using Cinder's built-in ImGui
 D3D12 version of VectorGraphicsDemo
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererD3d12.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/CanvasUi.h"
// ImGui - uses CinderImGui for input handling, imgui_impl_dx12 for rendering
#include "cinder/CinderImGui.h"
#include "imgui/imgui_impl_dx12.h"
#include <d3d12.h>

#include "cinder/vg/CanvasD3d12.h"
#include "cinder/vg/Paint.h"

#include <set>

#include "Demo.h"

using namespace ci;
using namespace ci::app;

// ============================================================================
// Demo 1: Primitives
// ============================================================================
class PrimitivesDemo : public Demo {
public:
    std::string getName() const override { return "Primitives"; }
    std::string getDescription() const override { return "Shapes with fills, strokes, caps, joins"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 500, 420 ); }

    void setup( vg::CanvasD3d12Ref canvas ) override {
        mCanvas = canvas;
        mFont = Font( "Arial", 14 );
    }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        vg::Paint textPaint;
        textPaint.setColor( ColorAf(0.7f,0.7f,0.7f,1) );

        // Row 1: Filled shapes (smaller)
        vg::Paint fill;
        fill.setColor( mFillColor );
        canvas->fillRect( Rectf( 50, 30, 120, 100 ), fill );
        canvas->fillCircle( vec2( 185, 65 ), 35, fill );
        canvas->fillRoundedRect( Rectf( 255, 30, 325, 100 ), mCornerRadius, fill );

        // Row 2: Stroked shapes (smaller)
        vg::Paint stroke;
        stroke.setColor( mStrokeColor ).setStrokeWidth( mStrokeWidth )
              .setLineCap( mLineCap ).setLineJoin( mLineJoin );
        canvas->strokeRect( Rectf( 50, 130, 120, 200 ), stroke );
        canvas->strokeCircle( vec2( 185, 165 ), 35, stroke );
        canvas->strokeRoundedRect( Rectf( 255, 130, 325, 200 ), mCornerRadius, stroke );

        // Row 3: Cap styles comparison
        float capY = 255;
        vg::Paint capDemo;
        capDemo.setColor( ColorAf(0.5f,0.8f,0.5f,1) ).setStrokeWidth( 14 );

        capDemo.setLineCap( vg::LineCap::Butt );
        canvas->drawLine( vec2(50, capY), vec2(130, capY), capDemo );
        canvas->drawString( "Butt", vec2(70, capY + 25), mFont, textPaint );

        capDemo.setLineCap( vg::LineCap::Round );
        canvas->drawLine( vec2(180, capY), vec2(260, capY), capDemo );
        canvas->drawString( "Round", vec2(195, capY + 25), mFont, textPaint );

        capDemo.setLineCap( vg::LineCap::Square );
        canvas->drawLine( vec2(310, capY), vec2(390, capY), capDemo );
        canvas->drawString( "Square", vec2(322, capY + 25), mFont, textPaint );

        // Row 4: Join styles comparison using triangles
        float joinY = 320;
        vg::Paint joinDemo;
        joinDemo.setColor( ColorAf(0.8f,0.5f,0.8f,1) ).setStrokeWidth( 10 );

        // Triangle with Miter join
        joinDemo.setLineJoin( vg::LineJoin::Miter );
        Path2d tri1;
        tri1.moveTo( vec2(50, joinY + 40) );
        tri1.lineTo( vec2(90, joinY) );
        tri1.lineTo( vec2(130, joinY + 40) );
        tri1.close();
        canvas->strokePath( canvas->createPath( tri1 ), joinDemo );
        canvas->drawString( "Miter", vec2(68, joinY + 60), mFont, textPaint );

        // Triangle with Round join
        joinDemo.setLineJoin( vg::LineJoin::Round );
        Path2d tri2;
        tri2.moveTo( vec2(180, joinY + 40) );
        tri2.lineTo( vec2(220, joinY) );
        tri2.lineTo( vec2(260, joinY + 40) );
        tri2.close();
        canvas->strokePath( canvas->createPath( tri2 ), joinDemo );
        canvas->drawString( "Round", vec2(195, joinY + 60), mFont, textPaint );

        // Triangle with Bevel join
        joinDemo.setLineJoin( vg::LineJoin::Bevel );
        Path2d tri3;
        tri3.moveTo( vec2(310, joinY + 40) );
        tri3.lineTo( vec2(350, joinY) );
        tri3.lineTo( vec2(390, joinY + 40) );
        tri3.close();
        canvas->strokePath( canvas->createPath( tri3 ), joinDemo );
        canvas->drawString( "Bevel", vec2(328, joinY + 60), mFont, textPaint );
    }

    void drawUI() override {
        ImGui::ColorEdit4( "Fill", &mFillColor.r );
        ImGui::ColorEdit4( "Stroke", &mStrokeColor.r );
        ImGui::SliderFloat( "Width", &mStrokeWidth, 1, 20 );
        ImGui::SliderFloat( "Corner", &mCornerRadius, 0, 30 );
    }

private:
    ColorAf mFillColor{ 0.3f, 0.6f, 1.0f, 1.0f };
    ColorAf mStrokeColor{ 1.0f, 0.4f, 0.4f, 1.0f };
    float mStrokeWidth = 4, mCornerRadius = 12;
    vg::LineCap mLineCap = vg::LineCap::Round;
    vg::LineJoin mLineJoin = vg::LineJoin::Round;
    Font mFont;
};

// ============================================================================
// Demo 2: Gradients
// ============================================================================
class GradientsDemo : public Demo {
public:
    std::string getName() const override { return "Gradients"; }
    std::string getDescription() const override { return "Linear and radial gradients with controls"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 500, 400 ); }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        // Linear gradient with angle control
        float angleRad = mAngle * M_PI / 180.0f;
        vec2 center1( 125, 100 );
        vec2 dir( cos(angleRad), sin(angleRad) );
        vec2 p1 = center1 - dir * 75.0f;
        vec2 p2 = center1 + dir * 75.0f;
        vg::Paint linear;
        linear.setLinearGradient( p1, p2, mColor1, mColor2 ).setFeather( mFeather );
        canvas->fillRect( Rectf( 50, 25, 200, 175 ), linear );

        // Radial gradient with offset center
        vec2 radialCenter( 350, 100 );
        vec2 focalOffset( mFocalX, mFocalY );
        float circleRadius = 70;  // Fixed circle size
        vg::Paint radial;
        radial.setRadialGradient( radialCenter + focalOffset, mGradientRadius, mColor1, mColor2 ).setFeather( mFeather );
        canvas->fillCircle( radialCenter, circleRadius, radial );

        // Multi-stop gradient bar
        std::vector<vg::GradientStop> stops;
        for( int i = 0; i <= mStops; i++ ) {
            float t = i / (float)mStops;
            ColorAf c = mColor1.lerp( t, mColor2 );
            if( mRainbow ) {
                c = ColorAf( ci::hsvToRgb( vec3( t, mSaturation, 1.0f ) ), mAlpha );
            }
            stops.push_back( { t, c } );
        }
        vg::Paint multi;
        multi.setLinearGradient( vec2( 50, 220 ), vec2( 450, 220 ), stops ).setFeather( mFeather );
        canvas->fillRoundedRect( Rectf( 50, 200, 450, 280 ), 15, multi );

        // Gradient with alpha
        vg::Paint alphaGrad;
        ColorAf c1 = mColor1; c1.a = mAlpha;
        ColorAf c2 = mColor2; c2.a = 0.0f;
        alphaGrad.setLinearGradient( vec2(50, 310), vec2(450, 310), c1, c2 );
        canvas->fillRoundedRect( Rectf( 50, 300, 450, 370 ), 10, alphaGrad );
    }

    void drawUI() override {
        ImGui::ColorEdit3( "Color 1", &mColor1.r );
        ImGui::ColorEdit3( "Color 2", &mColor2.r );
        ImGui::Separator();
        ImGui::SliderFloat( "Angle", &mAngle, 0, 360 );
        ImGui::SliderFloat( "Grad Radius", &mGradientRadius, 10, 150 );
        ImGui::SliderFloat( "Focal X", &mFocalX, -50, 50 );
        ImGui::SliderFloat( "Focal Y", &mFocalY, -50, 50 );
        ImGui::Separator();
        ImGui::SliderInt( "Stops", &mStops, 2, 12 );
        ImGui::Checkbox( "Rainbow", &mRainbow );
        if( mRainbow ) ImGui::SliderFloat( "Saturation", &mSaturation, 0, 1 );
        ImGui::SliderFloat( "Alpha", &mAlpha, 0, 1 );
        ImGui::SliderFloat( "Feather", &mFeather, 0, 30 );
    }

private:
    ColorAf mColor1{ 1, 0.3f, 0.3f, 1 }, mColor2{ 0.3f, 0.3f, 1, 1 };
    int mStops = 4;
    float mGradientRadius = 70, mAngle = 45, mFocalX = 0, mFocalY = 0;
    float mAlpha = 1.0f, mSaturation = 0.8f, mFeather = 0;
    bool mRainbow = false;
};

// ============================================================================
// Demo 3: Transforms (Solar System)
// ============================================================================
class TransformsDemo : public Demo {
public:
    std::string getName() const override { return "Transforms"; }
    std::string getDescription() const override { return "Transform stack - Solar System"; }
    Rectf getContentBounds() const override { return Rectf( -350, -250, 350, 250 ); }

    void update( double dt ) override { mTime += dt * mSpeed; }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        vg::Paint sun;
        sun.setRadialGradient( vec2(0), 50, ColorAf(1,1,0.5f,1), ColorAf(1,0.6f,0,1) );
        canvas->fillCircle( vec2(0), 50, sun );

        for( int i = 0; i < mPlanets; i++ ) {
            float orbit = 80 + i * 45;
            float speed = 1.0f / (1 + i * 0.5f);
            float size = 12 - i;

            if( mShowOrbits ) {
                vg::Paint op; op.setColor( ColorAf(0.3f,0.3f,0.4f,0.5f) ).setStrokeWidth(1);
                canvas->strokeCircle( vec2(0), orbit, op );
            }

            canvas->save();
            canvas->rotate( mTime * speed );
            canvas->translate( vec2( orbit, 0 ) );
            ColorAf pc( 0.3f + 0.7f*(i%3==0), 0.3f + 0.7f*(i%3==1), 0.3f + 0.7f*(i%3==2), 1 );
            vg::Paint pp; pp.setColor( pc );
            canvas->fillCircle( vec2(0), size, pp );
            canvas->restore();
        }
    }

    void drawUI() override {
        ImGui::SliderFloat( "Speed", &mSpeed, 0, 3 );
        ImGui::SliderInt( "Planets", &mPlanets, 1, 8 );
        ImGui::Checkbox( "Orbits", &mShowOrbits );
    }

private:
    float mTime = 0, mSpeed = 1;
    int mPlanets = 5;
    bool mShowOrbits = true;
};

// ============================================================================
// Demo 4: Feathering
// ============================================================================
class FeatheringDemo : public Demo {
public:
    std::string getName() const override { return "Feathering"; }
    std::string getDescription() const override { return "Soft edges on shapes"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 550, 300 ); }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        // Circles with progressively more red and more feathering
        for( int i = 0; i < 5; i++ ) {
            float t = i / 4.0f;  // 0 to 1
            ColorAf color = mColor;
            color.r = glm::mix( mColor.r, 1.0f, t );  // More red as feathering increases
            color.g = glm::mix( mColor.g, 0.2f, t );
            color.b = glm::mix( mColor.b, 0.2f, t );
            vg::Paint p;
            p.setColor( color ).setFeather( mBase + i * mIncrement );
            canvas->fillCircle( vec2( 60 + i * 100, 100 ), 40, p );
        }

        vg::Paint r;
        r.setColor( ColorAf(0.3f,0.8f,0.4f,mOpacity) ).setFeather( mFeather );
        canvas->fillRect( Rectf( 50, 180, 200, 280 ), r );

        vg::Paint g;
        g.setLinearGradient( vec2(250,180), vec2(400,280), ColorAf(1,0.5f,0.2f,1), ColorAf(0.2f,0.5f,1,1) )
         .setFeather( mFeather );
        canvas->fillRoundedRect( Rectf( 250, 180, 400, 280 ), 15, g );
    }

    void drawUI() override {
        ImGui::SliderFloat( "Feather", &mFeather, 0, 100 );
        ImGui::SliderFloat( "Base", &mBase, 0, 20 );
        ImGui::SliderFloat( "Inc", &mIncrement, 1, 20 );
        ImGui::SliderFloat( "Opacity", &mOpacity, 0, 1 );
        ImGui::ColorEdit4( "Base Color", &mColor.r );
    }

private:
    float mFeather = 12, mBase = 0, mIncrement = 8, mOpacity = 1;
    ColorAf mColor{ 0.4f, 0.6f, 1, 1 };
};

// ============================================================================
// Demo 5: Instancing
// ============================================================================
class InstancingDemo : public Demo {
public:
    std::string getName() const override { return "Instancing"; }
    std::string getDescription() const override { return "Batch drawing many shapes"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 800, 600 ); }

    void setup( vg::CanvasD3d12Ref canvas ) override {
        mCanvas = canvas;
        regenerate();
        rebuildStar();
    }

    void rebuildStar() {
        if( !mCanvas ) return;
        Path2d star;
        for( int i = 0; i < 10; i++ ) {
            float a = i * M_PI / 5.0f - M_PI / 2;
            float r = (i % 2 == 0) ? 1.0f : 0.4f;
            if( i == 0 ) star.moveTo( vec2( cos(a), sin(a) ) * r * mSize );
            else star.lineTo( vec2( cos(a), sin(a) ) * r * mSize );
        }
        star.close();
        mStar = mCanvas->createPath( star );
    }

    void regenerate() {
        mPositions.clear();
        mTransforms.clear();
        mCircleColors.clear();
        Rand rnd;
        for( int i = 0; i < mCount; i++ ) {
            mPositions.push_back( vec2( rnd.nextFloat(50,350), rnd.nextFloat(50,550) ) );
            // Complementary colors using golden angle
            float hue = fmod( i * 0.618033988749895f, 1.0f );
            mCircleColors.push_back( ColorAf( ci::hsvToRgb( vec3( hue, 0.7f, 0.9f ) ), 1.0f ) );

            mat3 x(1);
            x[2][0] = rnd.nextFloat(450,750);
            x[2][1] = rnd.nextFloat(50,550);
            float s = rnd.nextFloat(0.5f,1.5f);
            x[0][0] = s; x[1][1] = s;
            mTransforms.push_back( x );
        }
    }

    void update( double dt ) override {
        if( mAnimate ) {
            mTime += dt;
            for( size_t i = 0; i < mTransforms.size(); i++ ) {
                float a = mTime * 0.5f + i * 0.1f;
                float s = 0.8f + 0.3f * sin( mTime + i * 0.2f );
                mTransforms[i][0][0] = s * cos(a);
                mTransforms[i][0][1] = s * sin(a);
                mTransforms[i][1][0] = -s * sin(a);
                mTransforms[i][1][1] = s * cos(a);
            }
        }
    }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        // Draw circles with individual colors
        for( size_t i = 0; i < mPositions.size(); i++ ) {
            vg::Paint cp; cp.setColor( ColorAf(mCircleColors[i].r, mCircleColors[i].g, mCircleColors[i].b, mOpacity) );
            canvas->fillCircle( mPositions[i], mRadius, cp );
        }

        if( mStar ) {
            vg::Paint sp; sp.setColor( ColorAf(mStarColor.r,mStarColor.g,mStarColor.b,mOpacity) );
            canvas->drawPaths( mTransforms, mStar, sp );
        }

        vg::Paint lp; lp.setColor( ColorAf(0.4f,0.4f,0.4f,1) ).setStrokeWidth(2);
        canvas->drawLine( vec2(400,20), vec2(400,580), lp );
    }

    void drawUI() override {
        if( ImGui::SliderInt( "Count", &mCount, 10, 1000 ) ) regenerate();
        ImGui::SliderFloat( "Radius", &mRadius, 5, 30 );
        if( ImGui::SliderFloat( "Star Size", &mSize, 5, 30 ) ) rebuildStar();
        ImGui::SliderFloat( "Opacity", &mOpacity, 0.1f, 1 );
        ImGui::ColorEdit3( "Stars", &mStarColor.r );
        ImGui::Checkbox( "Animate", &mAnimate );
    }

private:
    std::vector<vec2> mPositions;
    std::vector<ColorAf> mCircleColors;
    std::vector<mat3> mTransforms;
    vg::CachedPathRef mStar;
    int mCount = 100;
    float mRadius = 15, mSize = 15, mOpacity = 0.7f, mTime = 0;
    ColorAf mStarColor{1,0.8f,0.2f,1};
    bool mAnimate = true;
};

// ============================================================================
// Demo 6: Blend Modes
// ============================================================================
class BlendModesDemo : public Demo {
public:
    std::string getName() const override { return "Blend Modes"; }
    std::string getDescription() const override { return "All 15 blend modes in action"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 600, 500 ); }

    void setup( vg::CanvasD3d12Ref canvas ) override {
        mFont = Font( "Arial", 11 );
    }

    void update( double dt ) override {
        if( mAnimate ) mTime += dt * mSpeed;
    }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        const char* names[] = {
            "SrcOver", "Screen", "Overlay", "Darken", "Lighten",
            "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference",
            "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity"
        };
        vg::BlendMode modes[] = {
            vg::BlendMode::SrcOver, vg::BlendMode::Screen, vg::BlendMode::Overlay,
            vg::BlendMode::Darken, vg::BlendMode::Lighten, vg::BlendMode::ColorDodge,
            vg::BlendMode::ColorBurn, vg::BlendMode::HardLight, vg::BlendMode::SoftLight,
            vg::BlendMode::Difference, vg::BlendMode::Exclusion, vg::BlendMode::Multiply,
            vg::BlendMode::Hue, vg::BlendMode::Saturation, vg::BlendMode::Color,
            vg::BlendMode::Luminosity
        };

        vg::Paint textPaint;
        textPaint.setColor( ColorAf(0.7f, 0.7f, 0.7f, 1) );

        int cols = 4;
        float cellW = 140, cellH = 115;

        for( int i = 0; i < 16; i++ ) {
            int col = i % cols;
            int row = i / cols;
            float x = 20 + col * cellW;
            float y = 20 + row * cellH;

            // Background rectangle (base layer)
            vg::Paint bg;
            bg.setColor( mBaseColor );
            canvas->fillRect( Rectf( x, y, x + 100, y + 70 ), bg );

            // Animated circle position
            float circleX = x + 50 + sin( mTime + i * 0.5f ) * 20;
            float circleY = y + 35 + cos( mTime * 0.7f + i * 0.3f ) * 15;

            // Blended circle with radial gradient and feather
            vg::Paint fg;
            fg.setRadialGradient( vec2( circleX, circleY ), 30,
                                  mBlendColor,
                                  ColorAf( mBlendColor.r * 0.6f, mBlendColor.g * 0.6f, mBlendColor.b * 0.6f, mBlendColor.a ) )
              .setBlendMode( modes[i] )
              .setFeather( mFeather );
            canvas->fillCircle( vec2( circleX, circleY ), 30, fg );

            // Label
            canvas->drawString( names[i], vec2( x + 50 - strlen(names[i]) * 3, y + 80 ), mFont, textPaint );
        }
    }

    void drawUI() override {
        const char* names[] = {
            "SrcOver", "Screen", "Overlay", "Darken", "Lighten",
            "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference",
            "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity"
        };
        ImGui::Combo( "Mode", &mSelectedMode, names, 16 );
        ImGui::ColorEdit4( "Base", &mBaseColor.r );
        ImGui::ColorEdit4( "Blend", &mBlendColor.r );
        ImGui::SliderFloat( "Feather", &mFeather, 0, 20 );
        ImGui::Separator();
        ImGui::Checkbox( "Animate", &mAnimate );
        if( mAnimate ) ImGui::SliderFloat( "Speed", &mSpeed, 0.1f, 3.0f );
    }

private:
    Font mFont;
    ColorAf mBaseColor{ 0.2f, 0.5f, 0.9f, 1.0f };
    ColorAf mBlendColor{ 1.0f, 0.4f, 0.2f, 0.8f };
    int mSelectedMode = 0;
    float mFeather = 0;
    float mTime = 0, mSpeed = 1.0f;
    bool mAnimate = true;
};

// ============================================================================
// Demo 7: Clipping
// ============================================================================
class ClippingDemo : public Demo {
public:
    std::string getName() const override { return "Clipping"; }
    std::string getDescription() const override { return "Intersecting clips: circle AND triangle"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 500, 400 ); }

    void setup( vg::CanvasD3d12Ref canvas ) override {
        mCanvas = canvas;
        mFont = Font( "Arial", 14 );

        // Create circle path
        Path2d circle;
        circle.arc( vec2(0), mCircleRadius, 0, M_PI * 2 );
        circle.close();
        mCirclePath = canvas->createPath( circle );

        // Create triangle path
        Path2d tri;
        for( int i = 0; i < 3; i++ ) {
            float a = i * M_PI * 2 / 3 - M_PI / 2;
            vec2 pt = vec2( cos(a), sin(a) ) * mTriangleRadius;
            if( i == 0 ) tri.moveTo( pt );
            else tri.lineTo( pt );
        }
        tri.close();
        mTrianglePath = canvas->createPath( tri );
    }

    void update( double dt ) override {
        if( mAnimate ) mTime += dt * mSpeed;
    }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        vec2 center( 250, 200 );

        // Draw shape outlines for reference
        vg::Paint outlinePaint;
        outlinePaint.setColor( ColorAf(0.5f, 0.5f, 0.6f, 0.4f) ).setStrokeWidth( 2 );

        // Circle outline
        canvas->save();
        canvas->translate( center );
        canvas->strokePath( mCirclePath, outlinePaint );
        canvas->restore();

        // Triangle outline (offset by 50%)
        vec2 triOffset = vec2( cos(mTime * 0.5f), sin(mTime * 0.5f) ) * mTriangleRadius * 0.5f;
        canvas->save();
        canvas->translate( center + triOffset );
        canvas->rotate( mTime * 0.3f );
        canvas->strokePath( mTrianglePath, outlinePaint );
        canvas->restore();

        // ========================================
        // KEY DEMO: Two clipPath calls intersected
        // ========================================
        canvas->save();
        canvas->translate( center );

        // First clip: circle at center
        if( mClipCircle ) {
            canvas->clipPath( mCirclePath );
        }

        // Second clip: triangle offset 50% (clips are intersected!)
        canvas->translate( triOffset );
        canvas->rotate( mTime * 0.3f );
        if( mClipTriangle ) {
            canvas->clipPath( mTrianglePath );
        }

        // Draw a big gradient rect - only the intersection is visible
        vg::Paint gradient;
        gradient.setRadialGradient( vec2(0), 150,
                                    ColorAf( 1.0f, 0.9f, 0.2f, 1.0f ),   // yellow center
                                    ColorAf( 1.0f, 0.3f, 0.1f, 1.0f ) ); // orange edge
        canvas->fillRect( Rectf( -200, -200, 200, 200 ), gradient );

        canvas->restore();

        // Labels
        vg::Paint textPaint;
        textPaint.setColor( ColorAf(0.8f, 0.8f, 0.8f, 1) );
        canvas->drawString( "clipPath(circle)", vec2(20, 30), mFont, textPaint );
        canvas->drawString( "clipPath(triangle)", vec2(20, 50), mFont, textPaint );
        canvas->drawString( "= intersection visible", vec2(20, 70), mFont, textPaint );
    }

    void drawUI() override {
        ImGui::Checkbox( "Clip Circle", &mClipCircle );
        ImGui::Checkbox( "Clip Triangle", &mClipTriangle );
        ImGui::Separator();
        ImGui::Checkbox( "Animate", &mAnimate );
        if( mAnimate ) ImGui::SliderFloat( "Speed", &mSpeed, 0.1f, 3.0f );
    }

private:
    Font mFont;
    vg::CachedPathRef mCirclePath, mTrianglePath;
    float mCircleRadius = 120;
    float mTriangleRadius = 100;
    float mTime = 0, mSpeed = 1.0f;
    bool mAnimate = true;
    bool mClipCircle = true;
    bool mClipTriangle = true;
};

// ============================================================================
// Demo 8: Images
// ============================================================================
class ImagesDemo : public Demo {
public:
    std::string getName() const override { return "Images"; }
    std::string getDescription() const override { return "Image drawing with transforms"; }
    Rectf getContentBounds() const override { return Rectf( 0, 0, 500, 350 ); }

    void setup( vg::CanvasD3d12Ref canvas ) override {
        Surface8u surf( 64, 64, true );
        auto iter = surf.getIter();
        while( iter.line() ) {
            while( iter.pixel() ) {
                bool light = ((iter.x()/8) + (iter.y()/8)) % 2 == 0;
                iter.r() = light ? 255 : 100;
                iter.g() = light ? 200 : 150;
                iter.b() = light ? 100 : 255;
                iter.a() = 255;
            }
        }
        mImage = canvas->createImage( surf );
    }

    void update( double dt ) override { if( mAnimate ) mRot += dt * mSpeed; }

    void draw( vg::CanvasD3d12Ref canvas ) override {
        if( !mImage ) return;
        canvas->drawImage( mImage, vec2( 50, 50 ) );
        canvas->drawImage( mImage, Rectf( 50, 150, 50 + 80*mScale, 150 + 80*mScale ) );

        canvas->save();
        canvas->translate( vec2( 300, 120 ) );
        canvas->rotate( mRot );
        canvas->translate( vec2( -32, -32 ) );
        canvas->drawImage( mImage, vec2( 0, 0 ) );
        canvas->restore();

        for( int i = 0; i < 4; i++ ) {
            float s = 25 + i * 15;
            canvas->drawImage( mImage, Rectf( 50 + i*70, 270, 50 + i*70 + s, 270 + s ) );
        }
    }

    void drawUI() override {
        ImGui::SliderFloat( "Scale", &mScale, 0.5f, 3 );
        ImGui::SliderFloat( "Speed", &mSpeed, 0, 5 );
        ImGui::Checkbox( "Animate", &mAnimate );
    }

private:
    vg::ImageRef mImage;
    float mScale = 1, mRot = 0, mSpeed = 1;
    bool mAnimate = true;
};

// ============================================================================
// Main App
// ============================================================================
class VectorGraphicsDemoD3d12App : public App {
public:
    void setup() override;
    void resize() override;
    void update() override;
    void draw() override;
    void keyDown( KeyEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void mouseMove( MouseEvent event ) override;

private:
    void switchDemo( int i );
    void renderImGui( ID3D12GraphicsCommandList* cmdList );

    app::RendererD3d12Ref mRenderer;
    vg::CanvasD3d12Ref mCanvas;
    CanvasUi mCanvasUi;
    std::vector<DemoRef> mDemos;
    int mDemoIndex = 0;
    DemoRef mDemo;
    double mLastTime = 0;
    float mFps = 60;
    bool mDemoDragging = false;
};

void VectorGraphicsDemoD3d12App::setup()
{
    try {
        mRenderer = std::dynamic_pointer_cast<RendererD3d12>( getRenderer() );
        mCanvas = vg::CanvasD3d12::create( mRenderer );
    } catch( const std::exception &e ) {
        CI_LOG_E( "Canvas init failed: " << e.what() );
        return;
    }

    // Initialize ImGui - CinderImGui handles input via signals, we handle D3D12 rendering
    ImGui::Initialize();

    // Set up post-flush callback for ImGui rendering
    // This is called after Rive flush, with render target in RENDER_TARGET state
    mCanvas->setPostFlushCallback( [this]( ID3D12GraphicsCommandList* cmdList ) {
        renderImGui( cmdList );
    } );

    mDemos = {
        std::make_shared<PrimitivesDemo>(),
        std::make_shared<GradientsDemo>(),
        std::make_shared<TransformsDemo>(),
        std::make_shared<FeatheringDemo>(),
        std::make_shared<InstancingDemo>(),
        std::make_shared<BlendModesDemo>(),
        std::make_shared<ClippingDemo>(),
        std::make_shared<ImagesDemo>()
    };
    for( auto& d : mDemos ) d->setup( mCanvas );
    switchDemo( 0 );
    mLastTime = getElapsedSeconds();
}

void VectorGraphicsDemoD3d12App::resize()
{
    // Wait for GPU work before resize
    if( mRenderer )
        mRenderer->waitForGpu();

    // Let the renderer handle swap chain resize
    if( mRenderer )
        mRenderer->defaultResize();
}

void VectorGraphicsDemoD3d12App::switchDemo( int i )
{
    if( i < 0 || i >= (int)mDemos.size() ) return;
    mDemoIndex = i;
    mDemo = mDemos[i];
    mDemo->setCanvasUi( &mCanvasUi );
    mCanvasUi.setContentBounds( mDemo->getContentBounds() );
    mCanvasUi.connect( getWindow() );
    mCanvasUi.fitAll();
    mCanvasUi.setZoomLimits( 0.1f, 10.0f );
}

void VectorGraphicsDemoD3d12App::update()
{
    double t = getElapsedSeconds();
    double dt = t - mLastTime;
    mLastTime = t;
    if( dt > 0 ) mFps = mFps * 0.95f + (1.0f/dt) * 0.05f;
    if( mDemo ) mDemo->update( dt );
}

void VectorGraphicsDemoD3d12App::draw()
{
    if( !mCanvas ) return;

    // Canvas rendering
    mCanvas->begin( toPixels( getWindowSize() ) );
    mCanvas->setTransform( mCanvasUi.getTransform2d() );

    if( mDemo ) {
        mDemo->draw( mCanvas );
        vg::Paint bp; bp.setColor( ColorAf(0.3f,0.3f,0.4f,0.5f) ).setStrokeWidth(1);
        mCanvas->strokeRect( mDemo->getContentBounds(), bp );
    }

    mCanvas->end();
}

// Separate method called via post-flush callback
void VectorGraphicsDemoD3d12App::renderImGui( ID3D12GraphicsCommandList* cmdList )
{
    if( ID3D12DescriptorHeap* imguiHeap = ImGui::GetD3D12SrvHeap() ) {
        // Set ImGui descriptor heap
        ID3D12DescriptorHeap* imguiHeaps[] = { imguiHeap };
        cmdList->SetDescriptorHeaps( 1, imguiHeaps );

        ImGui_ImplDX12_NewFrame();

        // Set display size and delta time
        ivec2 winSize = toPixels( getWindowSize() );
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2( (float)winSize.x, (float)winSize.y );
        static double lastTime = 0.0;
        double currentTime = getElapsedSeconds();
        io.DeltaTime = lastTime > 0.0 ? (float)( currentTime - lastTime ) : ( 1.0f / 60.0f );
        lastTime = currentTime;

        ImGui::NewFrame();

        // Demo selector and control panel
        ImGui::Begin( "Vector Graphics D3D12 Demo" );
        ImGui::Text( "FPS: %.1f", mFps );
        ImGui::Separator();

        // Demo selector
        const char* demoNames[] = { "Primitives", "Gradients", "Transforms", "Feathering",
                                    "Instancing", "Blend Modes", "Clipping", "Images" };
        if( ImGui::BeginCombo( "Demo", demoNames[mDemoIndex] ) ) {
            for( int i = 0; i < (int)mDemos.size(); i++ ) {
                bool selected = ( i == mDemoIndex );
                if( ImGui::Selectable( demoNames[i], selected ) ) {
                    switchDemo( i );
                }
                if( selected ) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if( ImGui::Button( "Fit All" ) ) mCanvasUi.fitAll();
        ImGui::SameLine();
        ImGui::Text( "%s", mDemo ? mDemo->getDescription().c_str() : "" );

        ImGui::Separator();

        // Demo-specific UI
        if( mDemo ) mDemo->drawUI();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmdList );
    }
}

void VectorGraphicsDemoD3d12App::keyDown( KeyEvent event )
{
    if( event.getCode() == KeyEvent::KEY_ESCAPE || event.getChar() == 'q' ) quit();
    else if( event.getChar() >= '1' && event.getChar() <= '8' ) switchDemo( event.getChar() - '1' );
    else if( event.getChar() == '0' ) mCanvasUi.fitAll();
}

void VectorGraphicsDemoD3d12App::mouseDown( MouseEvent event )
{
    if( mDemo && mDemo->onMouseDown( event ) ) {
        mDemoDragging = true;
    }
}

void VectorGraphicsDemoD3d12App::mouseDrag( MouseEvent event )
{
    if( mDemoDragging && mDemo ) {
        mDemo->onMouseDrag( event );
    }
}

void VectorGraphicsDemoD3d12App::mouseUp( MouseEvent event )
{
    if( mDemo ) mDemo->onMouseUp( event );
    mDemoDragging = false;
}

void VectorGraphicsDemoD3d12App::mouseMove( MouseEvent event )
{
    if( mDemo ) mDemo->onMouseMove( event );
}

CINDER_APP( VectorGraphicsDemoD3d12App, RendererD3d12( RendererD3d12::Options().debugLayer( true ) ),
    []( App::Settings* settings ) {
        settings->setTitle( "VectorGraphics D3D12 Demo" );
        settings->setWindowSize( 1200, 800 );
    }
)
