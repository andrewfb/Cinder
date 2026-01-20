/*
 Copyright (c) 2025, The Cinder Project

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
#include "cinder/app/RendererD3d12.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <vector>

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
        class RenderContextD3D12Impl;
        class RenderTargetD3D12;
    }
}

// Forward declarations for Rive reference counted pointers
namespace rive {
    template<typename T> class rcp;
}

namespace cinder { namespace vg {

// Forward declare CanvasD3d12 and CanvasD3d12Ref for use in class definition
class CanvasD3d12;
using CanvasD3d12Ref = std::shared_ptr<CanvasD3d12>;

// ------------------------------------------------------------------------------------------------
// CachedPathD3d12 - D3D12 implementation of CachedPath
// ------------------------------------------------------------------------------------------------

// Forward declare internal struct defined in cpp file
struct CachedPathD3d12Impl;

class CI_API CachedPathD3d12 : public CachedPath {
public:
    CachedPathD3d12();
    ~CachedPathD3d12() override;

private:
    friend class CanvasD3d12;

    std::unique_ptr<CachedPathD3d12Impl> mImpl;
};

// ------------------------------------------------------------------------------------------------
// ImageD3d12 - D3D12 implementation of Image
// ------------------------------------------------------------------------------------------------

// Forward declare internal struct defined in cpp file
struct ImageD3d12Impl;

class CI_API ImageD3d12 : public Image {
public:
    ImageD3d12();
    ~ImageD3d12() override;

    //! Get the underlying D3D12 resource
    Microsoft::WRL::ComPtr<ID3D12Resource> getResource() const { return mResource; }

private:
    friend class CanvasD3d12;

    Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    std::unique_ptr<ImageD3d12Impl> mImpl;
};

// ------------------------------------------------------------------------------------------------
// DisplayListD3d12 - D3D12 implementation of DisplayList
// ------------------------------------------------------------------------------------------------

class CanvasD3d12;  // Forward declaration

//! D3D12 implementation of DisplayList that records drawing commands.
class CI_API DisplayListD3d12 : public DisplayList {
public:
    DisplayListD3d12( CanvasD3d12* canvas );
    ~DisplayListD3d12() override;

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

    // Recording methods (called by CanvasD3d12 during recording)
    void recordFillPath( const CachedPathRef& path, const Paint& paint, FillRule rule );
    void recordStrokePath( const CachedPathRef& path, const Paint& paint );
    void recordDrawImage( const ImageRef& image, const Rectf& destRect );
    void recordDrawImage( const ImageRef& image, const Rectf& srcRect, const Rectf& destRect );
    void recordSave();
    void recordRestore();
    void recordSetTransform( const mat3& transform );
    void recordClip( const CachedPathRef& path, FillRule rule );

private:
    friend class CanvasD3d12;

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

    CanvasD3d12* mOwnerCanvas = nullptr;
    Canvas* mRecordingCanvas = nullptr;
    bool mRecording = false;
    bool mValid = false;
    std::vector<Command> mCommands;
    Rectf mBounds;
};

using DisplayListD3d12Ref = std::shared_ptr<DisplayListD3d12>;

// ------------------------------------------------------------------------------------------------
// CanvasD3d12 - D3D12 implementation of Canvas using Rive's PLS renderer
// ------------------------------------------------------------------------------------------------

//! D3D12 implementation of Canvas using Rive's PLS renderer.
//!
//! Renders vector graphics to the D3D12 swap chain back buffer. Requires a
//! RendererD3d12 instance to access the D3D12 device, command queue, and back buffers.
//!
//! Example:
//!     auto renderer = std::dynamic_pointer_cast<RendererD3d12>( getRenderer() );
//!     auto canvas = CanvasD3d12::create( renderer );
//!
//!     // In draw():
//!     canvas->begin( getWindowSize() );
//!     canvas->fillCircle( ... );
//!     canvas->end();
//!
class CI_API CanvasD3d12 : public Canvas {
public:
    //! Create a canvas for window rendering with the given D3D12 renderer
    static CanvasD3d12Ref create( app::RendererD3d12Ref renderer );

    ~CanvasD3d12() override;

    // Non-copyable
    CanvasD3d12( const CanvasD3d12& ) = delete;
    CanvasD3d12& operator=( const CanvasD3d12& ) = delete;

    // === Frame Management ===
    //! Begin rendering at the given size with the provided command list.
    //! The command list must be in the recording state (Reset() already called).
    //! After end(), the command list remains open for additional rendering (e.g., ImGui).
    //! The caller is responsible for closing and executing the command list.
    void begin( const ivec2 &size, ID3D12GraphicsCommandList* commandList );

    //! Legacy begin() without command list - will assert. Use begin(size, commandList) instead.
    void begin( const ivec2 &size ) override;

    //! End canvas rendering. Flushes Rive but leaves command list open.
    //! After this call, the render target is in RENDER_TARGET state and ready for
    //! additional rendering. Caller must close and execute the command list.
    void end() override;
    bool inFrame() const override { return Canvas::mInFrame; }

    //! Set clear color for the frame (default is transparent black)
    //! Call before begin() to take effect on next frame
    void setClearColor( const ColorAf &color ) { mClearColor = color; }

    //! Get the current clear color
    ColorAf getClearColor() const { return mClearColor; }

    // === Cached Path API ===
    using Canvas::createPath;  // Bring createPath(Path2d) into scope
    CachedPathRef createPath( const Shape2d &shape ) override;
    void fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const CachedPathRef &path, const Paint &paint ) override;

    // === Uncached Path API (override for DisplayList recording) ===
    void fillPath( const Path2d &path, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokePath( const Path2d &path, const Paint &paint ) override;
    void fillShape( const Shape2d &shape, const Paint &paint, FillRule rule = FillRule::NonZero ) override;
    void strokeShape( const Shape2d &shape, const Paint &paint ) override;

    // === Image API ===
    //! Create an image from a D3D12 resource (texture)
    ImageRef createImage( const Microsoft::WRL::ComPtr<ID3D12Resource> &resource );

    // Note: gl::Texture2dRef not available in D3D12 - use createImage(ID3D12Resource) instead
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

    // === DisplayList API ===
    DisplayListRef createDisplayList() override;

    // === Advanced ===
    //! Get the underlying Rive RenderContext (for advanced usage)
    rive::gpu::RenderContext* getRiveContext() const { return mRiveContext.get(); }

    //! Get the D3D12 renderer
    app::RendererD3d12Ref getRenderer() const { return mRenderer; }

    //! Get the current frame's command list (valid after begin())
    //! Useful for interleaving custom D3D12 rendering (e.g., ImGui) with canvas commands
    ID3D12GraphicsCommandList* getCommandList() const { return mCommandList; }

    //! Release cached render targets that hold references to back buffers.
    //! Call this in your app's resize() handler so the renderer can successfully
    //! ResizeBuffers() on the next frame (the framework triggers RendererD3d12::defaultResize()
    //! before App::resize(), so swapchain resizing is deferred until startDraw()).
    void releaseRenderTargets();

private:
    // Private constructor - use create() factory method
    CanvasD3d12( app::RendererD3d12Ref renderer );

    // D3D12-specific initialization
    void initializeD3d12();

    // Cached path drawing helper
    void drawCachedPathInternal( const CachedPathD3d12* cachedPath, const Paint &paint,
                                  bool fill, bool stroke, FillRule rule = FillRule::NonZero );

    // Renderer reference
    app::RendererD3d12Ref mRenderer;

    // D3D12-specific Rive context access
    rive::gpu::RenderContextD3D12Impl* mRiveContextD3d12 = nullptr;  // Raw pointer for D3D12-specific calls

    // Command list provided by caller (not owned)
    ID3D12GraphicsCommandList* mCommandList = nullptr;

    // Cached render targets (one per swap chain buffer)
    std::vector<rive::rcp<rive::gpu::RenderTargetD3D12>> mRenderTargets;

    // Frame size (for end() to know dimensions)
    ivec2 mFrameSize;

    // Last frame size (for resize detection)
    ivec2 mLastFrameSize;

    // Frame counter for Rive resource management
    // Rive requires monotonically increasing frame numbers for resource lifecycle
    uint64_t mFrameNumber = 2;  // Start at 2 since Rive init uses frame 1

    // DisplayList recording - when non-null, draw calls are recorded instead of rendered
    friend class DisplayListD3d12;
    DisplayListD3d12* mRecordingDisplayList = nullptr;

    // Clear color for frame (used with Rive's clear loadAction)
    ColorAf mClearColor = ColorAf( 0.2f, 0.2f, 0.25f, 1.0f );
};

} } // namespace cinder::vg
