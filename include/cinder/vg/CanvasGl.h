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
    class RenderPaint;
    class RenderImage;
    class RenderBuffer;
    namespace gpu {
        class RenderContext;
        class RenderContextGLImpl;
    }
}

// Forward declarations for Rive reference counted pointers
namespace rive {
    template<typename T> class rcp;
}

namespace cinder { namespace vg {

// Forward declare CanvasGl and CanvasGlRef for use in class definition
class CanvasGl;
using CanvasGlRef = std::shared_ptr<CanvasGl>;

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
// DisplayListGl - GL implementation of DisplayList
// ------------------------------------------------------------------------------------------------

class CanvasGl;  // Forward declaration

//! GL implementation of DisplayList that records drawing commands.
class CI_API DisplayListGl : public DisplayList {
public:
    DisplayListGl( CanvasGl* canvas );
    ~DisplayListGl() override;

    //! Begin recording draw commands
    void beginRecording( Canvas* canvas ) override;

    //! End recording and freeze captured paths
    void endRecording() override;

    //! Check if currently recording
    bool isRecording() const override { return mRecording; }

    //! Replay the recorded commands
    void replay( Canvas* canvas ) override;

    //! Check if display list is valid
    bool isValid() const override { return mValid && !mCommands.empty(); }

    //! Clear the display list
    void clear() override;

    //! Get command count
    size_t getCommandCount() const override { return mCommands.size(); }

    //! Get content bounds
    Rectf getBounds() const override { return mBounds; }

private:
    friend class CanvasGl;

    // Command types
    enum class CommandType {
        FillPath,
        StrokePath,
        DrawImage,
        Save,
        Restore,
        SetTransform,
        Clip
    };

    // A recorded draw command
    struct Command {
        CommandType type;
        CachedPathRef cachedPath;
        ImageRef image;
        Paint paint;
        mat3 transform;
        Rectf destRect;
        Rectf srcRect;
        FillRule fillRule = FillRule::NonZero;
    };

    // Record methods for building the display list
    void recordFillPath( const CachedPathRef& path, const Paint& paint, FillRule rule = FillRule::NonZero );
    void recordStrokePath( const CachedPathRef& path, const Paint& paint );
    void recordDrawImage( const ImageRef& image, const Rectf& destRect );
    void recordDrawImage( const ImageRef& image, const Rectf& srcRect, const Rectf& destRect );
    void recordSave();
    void recordRestore();
    void recordSetTransform( const mat3& transform );
    void recordClip( const CachedPathRef& path, FillRule rule = FillRule::NonZero );

    CanvasGl* mOwnerCanvas = nullptr;
    Canvas* mRecordingCanvas = nullptr;
    bool mRecording = false;
    bool mValid = false;
    std::vector<Command> mCommands;
    Rectf mBounds;
};

using DisplayListGlRef = std::shared_ptr<DisplayListGl>;

// ------------------------------------------------------------------------------------------------
// CanvasGl - OpenGL implementation of Canvas
// ------------------------------------------------------------------------------------------------

//! OpenGL implementation of Canvas using Rive's PLS renderer.
//!
//! Two rendering modes:
//! - Window mode: Renders directly to the window. Auto-detects MSAA and chooses
//!   the appropriate rendering path (atomic or MSAA mode).
//! - Offscreen mode: Renders to an internal FBO using atomic mode with analytical AA.
//!   Retrieve the result via getTexture().
//!
//! Example (window rendering):
//!     auto canvas = CanvasGl::create();
//!     canvas->begin( getWindowSize() );
//!     canvas->fillCircle( ... );
//!     canvas->end();
//!
//! Example (offscreen rendering):
//!     auto canvas = CanvasGl::create( 1920, 1080 );
//!     canvas->begin();
//!     canvas->fillCircle( ... );
//!     canvas->end();
//!     gl::draw( canvas->getTexture() );
//!
class CI_API CanvasGl : public Canvas {
public:
    //! Create a canvas for window rendering (auto-detects MSAA)
    static CanvasGlRef create();

    //! Create a canvas for offscreen rendering with analytical AA
    static CanvasGlRef create( int width, int height );

    ~CanvasGl() override;

    // Non-copyable
    CanvasGl( const CanvasGl& ) = delete;
    CanvasGl& operator=( const CanvasGl& ) = delete;

    // === Frame Management ===
    //! Begin rendering at the given size (window mode)
    void begin( const ivec2 &size ) override;

    //! Begin rendering (offscreen mode uses FBO dimensions)
    void begin();

    void end() override;
    bool inFrame() const override { return Canvas::mInFrame; }

    // === Rendering Mode ===
    //! Get the rendering mode
    RenderMode getRenderMode() const { return mRenderMode; }

    //! Returns true if rendering offscreen (to internal FBO)
    bool isOffscreen() const { return mRenderMode == RenderMode::Offscreen; }

    //! Get the texture (offscreen mode only). Returns nullptr in window mode.
    gl::Texture2dRef getTexture() const;

    //! Get the FBO dimensions (offscreen mode only). Returns (0,0) in window mode.
    ivec2 getFboSize() const;

    // === Transform Stack ===
    // save(), restore() - use base class implementations (delegates to implSave/implRestore)
    // translate, rotate, scale, transform, setTransform, resetTransform - use base class implementations
    // fillRect, strokeRect, fillCircle, strokeCircle, etc. - use base class implementations

    // === Path Drawing (uncached) ===
    // fillPath, strokePath, fillShape, strokeShape, strokePolyLine, fillPolyLine
    // - use base class implementations (delegates to implFillShape/implStrokeShape)

    // === Path Drawing (uncached) - override for DisplayList recording ===
    void fillPath( const Path2d &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const Path2d &path, const Paint &paint ) override;
    void fillShape( const Shape2d &shape, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokeShape( const Shape2d &shape, const Paint &paint ) override;

    // === Cached Path API ===
    using Canvas::createPath;  // Bring createPath(Path2d) into scope
    CachedPathRef createPath( const Shape2d &shape ) override;
    void fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const CachedPathRef &path, const Paint &paint ) override;

    // === Image API ===
    ImageRef createImage( const gl::Texture2dRef &texture ) override;
    ImageRef createImage( const Surface &surface ) override;
    using Canvas::drawImage;  // Bring base class drawImage(position) into scope
    void drawImage( const ImageRef &image, const Rectf &destRect ) override;
    void drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect ) override;
    void drawImageMesh( const ImageRef &image,
                        std::span<const vec2> vertices,
                        std::span<const vec2> uvs,
                        std::span<const uint16_t> indices,
                        float opacity = 1.0f ) override;

    // === Text API ===
    // drawString, createTextPath - use base class implementations

    // === Instanced Drawing API ===
    // drawCircles, drawRects, drawPaths - use base class implementations

    // === Clipping API ===
    // clipRect, clipPath, clipShape - use base class implementations (delegates to implClipPath)

    // === DisplayList API ===
    DisplayListRef createDisplayList() override;

    // === SVG Rendering ===
    using Canvas::draw;  // Bring base class draw(svg::Doc) into scope

    // === Advanced ===
    //! Get the underlying Rive RenderContext (for advanced usage)
    rive::gpu::RenderContext* getRiveContext() const { return mRiveContext.get(); }

    // clearGlyphCache() - use base class implementation

private:
    // Private constructors - use create() factory methods
    CanvasGl( RenderMode mode, int fboWidth = 0, int fboHeight = 0 );

    // GL-specific initialization and state management
    void initializeGl( bool forceMsaaMode );
    void invalidateState();
    void saveHostState();
    void restoreHostState( const ivec2 &size );

    // Cached path drawing helper
    void drawCachedPathInternal( const CachedPathGl* cachedPath, const Paint &paint,
                                  bool fill, bool stroke, FillRule rule = FillRule::NonZero );

    // GL-specific Rive context access
    rive::gpu::RenderContextGLImpl* mRiveContextGl = nullptr;  // Raw pointer for GL-specific calls

    // Rendering mode and configuration
    RenderMode mRenderMode = RenderMode::Window;
    bool mWindowHasMsaa = false;  // True if window uses MSAA (detected at create time)

    // Internal FBO (used for offscreen mode OR window mode with MSAA)
    gl::FboRef mInternalFbo;
    ivec2 mInternalFboSize;

    // Frame size (for end() to know dimensions)
    ivec2 mFrameSize;

    // DisplayList recording - when non-null, draw calls are recorded instead of rendered
    friend class DisplayListGl;
    DisplayListGl* mRecordingDisplayList = nullptr;
};

} } // namespace cinder::vg
