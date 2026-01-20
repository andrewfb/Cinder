/*
 Copyright (c) 2025, The Cinder Project
 */

#include "cinder/vg/CanvasD3d12.h"
#include "cinder/Log.h"

// Rive includes
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/d3d12/render_context_d3d12_impl.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include "rive/math/raw_path.hpp"
#include "rive/renderer/draw.hpp"
#include "rive_render_path.hpp"
#include "rive_render_paint.hpp"

// D3D12 helper header
#include "cinder/vg/rive/d3dx12.h"

using namespace rive;
using namespace rive::gpu;
using Microsoft::WRL::ComPtr;

namespace cinder { namespace vg {

// ------------------------------------------------------------------------------------------------
// Internal implementation structs (pimpl)
// ------------------------------------------------------------------------------------------------

struct CachedPathD3d12Impl {
    rcp<RenderPath> rivePath;
};

struct ImageD3d12Impl {
    rcp<RiveRenderImage> riveImage;
};

// ------------------------------------------------------------------------------------------------
// Helper functions (D3D12-specific)
// ------------------------------------------------------------------------------------------------

// Helper to convert Cinder mat3 to Rive Mat2D
static Mat2D toRiveMat( const mat3 &m )
{
    // Cinder mat3 is column-major, Rive Mat2D is [xx, xy, yx, yy, tx, ty]
    return Mat2D( m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1] );
}

// Helper to convert Cinder FillRule to Rive FillRule
static rive::FillRule toRiveFillRule( FillRule rule )
{
    switch( rule ) {
        case FillRule::EvenOdd: return rive::FillRule::evenOdd;
        case FillRule::NonZero:
        default:                return rive::FillRule::nonZero;
    }
}

// ------------------------------------------------------------------------------------------------
// CachedPathD3d12 implementation
// ------------------------------------------------------------------------------------------------

CachedPathD3d12::CachedPathD3d12() : mImpl( std::make_unique<CachedPathD3d12Impl>() ) {}

CachedPathD3d12::~CachedPathD3d12() = default;

// ------------------------------------------------------------------------------------------------
// ImageD3d12 implementation
// ------------------------------------------------------------------------------------------------

ImageD3d12::ImageD3d12() : mImpl( std::make_unique<ImageD3d12Impl>() ) {}

ImageD3d12::~ImageD3d12() = default;

// ------------------------------------------------------------------------------------------------
// DisplayListD3d12 implementation
// ------------------------------------------------------------------------------------------------

DisplayListD3d12::DisplayListD3d12( CanvasD3d12* canvas )
    : mOwnerCanvas( canvas )
{
}

DisplayListD3d12::~DisplayListD3d12() = default;

void DisplayListD3d12::beginRecording( Canvas* canvas )
{
    if( mRecording ) {
        CI_LOG_W( "DisplayListD3d12::beginRecording called while already recording" );
        return;
    }
    mRecordingCanvas = canvas;
    mRecording = true;
    mValid = false;
    mCommands.clear();
    mBounds = Rectf();

    // Set recording pointer on the canvas
    if( auto* d3d12Canvas = dynamic_cast<CanvasD3d12*>( canvas ) ) {
        d3d12Canvas->mRecordingDisplayList = this;
    }
}

void DisplayListD3d12::endRecording()
{
    if( ! mRecording ) {
        CI_LOG_W( "DisplayListD3d12::endRecording called without beginRecording" );
        return;
    }

    // Clear recording pointer on the canvas
    if( auto* d3d12Canvas = dynamic_cast<CanvasD3d12*>( mRecordingCanvas ) ) {
        d3d12Canvas->mRecordingDisplayList = nullptr;
    }

    mRecording = false;
    mRecordingCanvas = nullptr;
    mValid = !mCommands.empty();
}

void DisplayListD3d12::replay( Canvas* canvas )
{
    if( ! mValid || mCommands.empty() )
        return;

    auto* d3d12Canvas = dynamic_cast<CanvasD3d12*>( canvas );
    if( ! d3d12Canvas ) {
        CI_LOG_W( "DisplayListD3d12::replay requires a CanvasD3d12" );
        return;
    }

    // Get the current view transform (e.g., from CanvasUi) to compose with recorded transforms
    mat3 viewTransform = d3d12Canvas->getTransform();

    for( const auto& cmd : mCommands ) {
        switch( cmd.type ) {
            case CommandType::FillPath:
                if( cmd.cachedPath ) {
                    d3d12Canvas->save();
                    d3d12Canvas->setTransform( viewTransform * cmd.transform );
                    d3d12Canvas->fillPath( cmd.cachedPath, cmd.paint, cmd.fillRule );
                    d3d12Canvas->restore();
                }
                break;
            case CommandType::StrokePath:
                if( cmd.cachedPath ) {
                    d3d12Canvas->save();
                    d3d12Canvas->setTransform( viewTransform * cmd.transform );
                    d3d12Canvas->strokePath( cmd.cachedPath, cmd.paint );
                    d3d12Canvas->restore();
                }
                break;
            case CommandType::DrawImage:
                if( cmd.image ) {
                    d3d12Canvas->save();
                    d3d12Canvas->setTransform( viewTransform * cmd.transform );
                    if( cmd.srcRect.getWidth() > 0 )
                        d3d12Canvas->drawImage( cmd.image, cmd.srcRect, cmd.destRect );
                    else
                        d3d12Canvas->drawImage( cmd.image, cmd.destRect );
                    d3d12Canvas->restore();
                }
                break;
            case CommandType::Save:
                d3d12Canvas->save();
                break;
            case CommandType::Restore:
                d3d12Canvas->restore();
                break;
            case CommandType::SetTransform:
                d3d12Canvas->setTransform( viewTransform * cmd.transform );
                break;
            case CommandType::Clip:
                if( cmd.cachedPath ) {
                    d3d12Canvas->save();
                    d3d12Canvas->setTransform( viewTransform * cmd.transform );
                    d3d12Canvas->clipPath( cmd.cachedPath, cmd.fillRule );
                    d3d12Canvas->restore();
                }
                break;
        }
    }
}

void DisplayListD3d12::clear()
{
    mCommands.clear();
    mValid = false;
    mBounds = Rectf();
}

void DisplayListD3d12::recordFillPath( const CachedPathRef& path, const Paint& paint, FillRule rule )
{
    if( ! mRecording || ! path )
        return;

    Command cmd;
    cmd.type = CommandType::FillPath;
    cmd.cachedPath = path;
    cmd.paint = paint;
    cmd.fillRule = rule;
    cmd.transform = mRecordingCanvas ? mRecordingCanvas->getTransform() : mat3();
    mCommands.push_back( cmd );

    if( path->getBounds().getWidth() > 0 )
        mBounds.include( path->getBounds() );
}

void DisplayListD3d12::recordStrokePath( const CachedPathRef& path, const Paint& paint )
{
    if( ! mRecording || ! path )
        return;

    Command cmd;
    cmd.type = CommandType::StrokePath;
    cmd.cachedPath = path;
    cmd.paint = paint;
    cmd.transform = mRecordingCanvas ? mRecordingCanvas->getTransform() : mat3();
    mCommands.push_back( cmd );

    if( path->getBounds().getWidth() > 0 )
        mBounds.include( path->getBounds() );
}

void DisplayListD3d12::recordDrawImage( const ImageRef& image, const Rectf& destRect )
{
    if( ! mRecording || ! image )
        return;

    Command cmd;
    cmd.type = CommandType::DrawImage;
    cmd.image = image;
    cmd.destRect = destRect;
    cmd.transform = mRecordingCanvas ? mRecordingCanvas->getTransform() : mat3();
    mCommands.push_back( cmd );

    mBounds.include( destRect );
}

void DisplayListD3d12::recordDrawImage( const ImageRef& image, const Rectf& srcRect, const Rectf& destRect )
{
    if( ! mRecording || ! image )
        return;

    Command cmd;
    cmd.type = CommandType::DrawImage;
    cmd.image = image;
    cmd.srcRect = srcRect;
    cmd.destRect = destRect;
    cmd.transform = mRecordingCanvas ? mRecordingCanvas->getTransform() : mat3();
    mCommands.push_back( cmd );

    mBounds.include( destRect );
}

void DisplayListD3d12::recordSave()
{
    if( ! mRecording )
        return;

    Command cmd;
    cmd.type = CommandType::Save;
    mCommands.push_back( cmd );
}

void DisplayListD3d12::recordRestore()
{
    if( ! mRecording )
        return;

    Command cmd;
    cmd.type = CommandType::Restore;
    mCommands.push_back( cmd );
}

void DisplayListD3d12::recordSetTransform( const mat3& transform )
{
    if( ! mRecording )
        return;

    Command cmd;
    cmd.type = CommandType::SetTransform;
    cmd.transform = transform;
    mCommands.push_back( cmd );
}

void DisplayListD3d12::recordClip( const CachedPathRef& path, FillRule rule )
{
    if( ! mRecording || ! path )
        return;

    Command cmd;
    cmd.type = CommandType::Clip;
    cmd.cachedPath = path;
    cmd.fillRule = rule;
    cmd.transform = mRecordingCanvas ? mRecordingCanvas->getTransform() : mat3();
    mCommands.push_back( cmd );
}

// ------------------------------------------------------------------------------------------------
// CanvasD3d12 implementation
// ------------------------------------------------------------------------------------------------

CanvasD3d12Ref CanvasD3d12::create( app::RendererD3d12Ref renderer )
{
    CI_ASSERT_MSG( renderer, "CanvasD3d12::create requires a valid RendererD3d12" );
    return CanvasD3d12Ref( new CanvasD3d12( renderer ) );
}

CanvasD3d12::CanvasD3d12( app::RendererD3d12Ref renderer )
    : mRenderer( renderer )
{
    initializeD3d12();
}

CanvasD3d12::~CanvasD3d12()
{
    if( mInFrame ) {
        CI_LOG_W( "CanvasD3d12 destroyed while still in frame" );
    }

    // Wait for GPU to complete all work before destroying resources
    if( mRenderer ) {
        mRenderer->waitForGpu();
    }

    // Clear render targets before context destruction
    mRenderTargets.clear();

    mRiveRenderer = nullptr;  // Clear base class pointer
    mOwnedRiveRenderer.reset();
    mRiveContext.reset();
}

void CanvasD3d12::releaseRenderTargets()
{
    // Track if we were in a Rive frame - affects what cleanup we can do
    bool wasInRiveFrame = mInFrame;

    // If we're in a frame, abort it (don't close command list - caller owns it)
    if( mInFrame ) {
        mRiveRenderer = nullptr;
        mOwnedRiveRenderer.reset();
        mCommandList = nullptr;
        mInFrame = false;
    }

    // Wait for GPU to finish using render targets
    if( mRenderer )
        mRenderer->waitForGpu();

    // Release all cached render targets (releases ComPtr refs to back buffers)
    UINT bufferCount = mRenderer ? mRenderer->getBufferCount() : 0;
    for( UINT i = 0; i < mRenderTargets.size(); i++ ) {
        if( mRenderTargets[i] )
            mRenderTargets[i]->releaseTexturesImmediately();
        mRenderTargets[i] = nullptr;
    }
    mRenderTargets.clear();
    mRenderTargets.resize( bufferCount );

    // Release Rive resources (only if we weren't mid-frame)
    if( mRiveContext && !wasInRiveFrame )
        mRiveContext->releaseResources();

    // Force-purge Rive's resource purgatory to release deferred D3D12Texture deletions
    if( mRiveContextD3d12 ) {
        auto manager = mRiveContextD3d12->manager();
        if( manager ) {
            uint64_t currentFrame = manager->currentFrameNumber();
            uint64_t bc = mRenderer ? mRenderer->getBufferCount() : 3;
            uint64_t newFrame = currentFrame + bc + 2;
            uint64_t newSafeFrame = currentFrame;
            manager->advanceFrameNumber( newFrame, newSafeFrame );
            mFrameNumber = newSafeFrame + bc + 2;
        }
    }

    mLastFrameSize = ivec2( -1, -1 );
}

void CanvasD3d12::initializeD3d12()
{
    auto device = mRenderer->getDevice();
    CI_ASSERT_MSG( device, "RendererD3d12 device is null" );

    UINT bufferCount = mRenderer->getBufferCount();

    // Create temporary command allocator and list for Rive initialization
    ComPtr<ID3D12CommandAllocator> initAllocator;
    ComPtr<ID3D12GraphicsCommandList> initCommandList;

    HRESULT hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS( &initAllocator ) );
    if( FAILED( hr ) )
        throw vg::Exc( "Failed to create D3D12 command allocator for initialization" );

    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        initAllocator.Get(),
        nullptr,
        IID_PPV_ARGS( &initCommandList ) );
    if( FAILED( hr ) )
        throw vg::Exc( "Failed to create D3D12 command list for initialization" );

    // Create Rive context - this records initialization commands
    D3DContextOptions riveOptions;

    mRiveContext = RenderContextD3D12Impl::MakeContext(
        ComPtr<ID3D12Device>( device ),
        initCommandList.Get(),
        riveOptions );

    if( ! mRiveContext )
        throw vg::Exc( "Failed to create Rive D3D12 RenderContext" );

    // Store the D3D12-specific pointer for D3D12-specific calls
    mRiveContextD3d12 = mRiveContext->static_impl_cast<RenderContextD3D12Impl>();

    // Execute initialization commands
    initCommandList->Close();
    ID3D12CommandList* lists[] = { initCommandList.Get() };
    mRenderer->getCommandQueue()->ExecuteCommandLists( 1, lists );

    // Wait for initialization to complete
    {
        ComPtr<ID3D12Fence> initFence;
        HANDLE initEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
        device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &initFence ) );
        mRenderer->getCommandQueue()->Signal( initFence.Get(), 1 );
        if( initFence->GetCompletedValue() < 1 ) {
            initFence->SetEventOnCompletion( 1, initEvent );
            WaitForSingleObject( initEvent, INFINITE );
        }
        CloseHandle( initEvent );
    }

    // Pre-allocate render target vector (will lazily create targets)
    mRenderTargets.resize( bufferCount );
}

void CanvasD3d12::begin( const ivec2 &size )
{
    CI_ASSERT_MSG( false, "begin(size) is deprecated. Use begin(size, commandList) instead." );
}

void CanvasD3d12::begin( const ivec2 &size, ID3D12GraphicsCommandList* commandList )
{
    CI_ASSERT_MSG( ! mInFrame, "begin() called while already in frame - did you forget to call end()?" );
    CI_ASSERT_MSG( commandList, "begin() requires a valid command list" );

    // During resize/minimize, size can be 0 - skip frame gracefully
    if( size.x <= 0 || size.y <= 0 ) {
        mFrameSize = ivec2( 0 );
        mInFrame = false;
        return;
    }

    mFrameSize = size;
    mInFrame = true;
    mCommandList = commandList;

    UINT frameIndex = mRenderer->getCurrentBackBufferIndex();

    // Wait for this frame's GPU work to complete before caller reuses their allocator
    mRenderer->waitForFrame( frameIndex );

    // Get back buffer first - it should always be valid after proper initialization
    auto backBuffer = mRenderer->getCurrentBackBuffer();
    if( ! backBuffer ) {
        CI_LOG_W( "CanvasD3d12::begin() skipping frame: back buffer is null (resize in progress)" );
        mInFrame = false;
        mCommandList = nullptr;
        return;
    }

    // Use actual back buffer size as the authoritative render size
    D3D12_RESOURCE_DESC backDesc = backBuffer->GetDesc();
    ivec2 backBufferSize( static_cast<int>( backDesc.Width ), static_cast<int>( backDesc.Height ) );

    // Detect resize: either explicit (mLastFrameSize set to -1 by releaseRenderTargets)
    // or implicit (back buffer size changed from previous frame)
    bool resizeFromRelease = ( mLastFrameSize.x == -1 );
    bool resizeFromSizeChange = ( mLastFrameSize != backBufferSize && mLastFrameSize.x > 0 );
    if( resizeFromRelease || resizeFromSizeChange ) {
        // Invalidate ALL render targets (all back buffers are recreated)
        UINT bufferCount = mRenderer->getBufferCount();
        for( UINT i = 0; i < bufferCount; i++ ) {
            mRenderTargets[i] = nullptr;
        }
    }

    // Note: Don't manually transition buffer states here - Rive handles all
    // state transitions internally via setTargetTexture() and flush()

    // Get or create cached render target for this frame
    if( ! mRenderTargets[frameIndex] ||
        mRenderTargets[frameIndex]->width() != backDesc.Width ||
        mRenderTargets[frameIndex]->height() != backDesc.Height ) {
        mRenderTargets[frameIndex] = mRiveContextD3d12->makeRenderTarget( backBufferSize.x, backBufferSize.y );
    }
    mRenderTargets[frameIndex]->setTargetTexture( ComPtr<ID3D12Resource>( backBuffer ) );

    // Track size for resize detection
    mLastFrameSize = backBufferSize;
    mFrameSize     = backBufferSize;

    // Begin Rive frame
    RenderContext::FrameDescriptor frameDesc;
    frameDesc.renderTargetWidth = backBufferSize.x;
    frameDesc.renderTargetHeight = backBufferSize.y;
    frameDesc.loadAction = gpu::LoadAction::clear;
    frameDesc.clearColor = colorARGB(
        static_cast<int>( mClearColor.a * 255 ),
        static_cast<int>( mClearColor.r * 255 ),
        static_cast<int>( mClearColor.g * 255 ),
        static_cast<int>( mClearColor.b * 255 )
    );
    frameDesc.clockwiseFillOverride = true;  // Required for feathering support

    mRiveContext->beginFrame( frameDesc );

    // Create RiveRenderer for this frame and set base class pointer
    mOwnedRiveRenderer = std::make_unique<RiveRenderer>( mRiveContext.get() );
    mRiveRenderer = mOwnedRiveRenderer.get();
	CI_ASSERT( mRiveRenderer );
}

void CanvasD3d12::end()
{
    // If begin() skipped the frame (resize/minimize), just return
    if( ! mInFrame ) {
        return;
    }

    // Clear base class pointer and destroy the renderer before flush
    mRiveRenderer = nullptr;
    mOwnedRiveRenderer.reset();

    UINT frameIndex = mRenderer->getCurrentBackBufferIndex();

    // Set up command lists for Rive flush
    RenderContextD3D12Impl::CommandLists cmdLists;
    cmdLists.copyComandList = nullptr;
    cmdLists.directComandList = mCommandList;

    // Flush Rive rendering to cached render target
    UINT bufferCount = mRenderer->getBufferCount();
    uint64_t safeFrame = (mFrameNumber > bufferCount) ? (mFrameNumber - bufferCount - 1) : 0;

    RenderContext::FlushResources flushRes;
    flushRes.renderTarget = mRenderTargets[frameIndex].get();
    flushRes.externalCommandBuffer = &cmdLists;
    flushRes.currentFrameNumber = mFrameNumber;
    flushRes.safeFrameNumber = safeFrame;
    mRiveContext->flush( flushRes );

    ++mFrameNumber;

    // After Rive flush, render target is in COMMON state.
    // Transition to RENDER_TARGET so caller can do additional rendering (e.g., ImGui).
    auto backBuffer = mRenderer->getCurrentBackBuffer();
    if( backBuffer && mCommandList ) {
        CD3DX12_RESOURCE_BARRIER toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_RENDER_TARGET );
        mCommandList->ResourceBarrier( 1, &toRenderTarget );

        // Set render target and viewport/scissor for caller's convenience
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRenderer->getRtvHandle( frameIndex );
        mCommandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)mFrameSize.x, (float)mFrameSize.y, 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, (LONG)mFrameSize.x, (LONG)mFrameSize.y };
        mCommandList->RSSetViewports( 1, &viewport );
        mCommandList->RSSetScissorRects( 1, &scissor );
    }

    // Command list remains open - caller is responsible for:
    // 1. Additional rendering (e.g., ImGui)
    // 2. Transitioning render target to PRESENT state
    // 3. Closing and executing the command list
    // 4. Signaling the frame fence via mRenderer->signalFrameFence()

    mInFrame = false;
    mCommandList = nullptr;
}

// ------------------------------------------------------------------------------------------------
// Cached Path API
// ------------------------------------------------------------------------------------------------

CachedPathRef CanvasD3d12::createPath( const Shape2d &shape )
{
    auto cachedPath = std::make_shared<CachedPathD3d12>();
    cachedPath->mSourceShape = shape;
    cachedPath->mBounds = shape.calcBoundingBox();

    // Create Rive render path
    RawPath rawPath = toRivePath( shape );
    cachedPath->mImpl->rivePath = mRiveContext->makeRenderPath( rawPath, rive::FillRule::nonZero );

    return cachedPath;
}

void CanvasD3d12::fillPath( const CachedPathRef &path, const Paint &paint, FillRule rule )
{
    if( ! path ) return;

    // If recording, redirect to DisplayList
    if( mRecordingDisplayList ) {
        mRecordingDisplayList->recordFillPath( path, paint, rule );
        return;
    }

    if( ! mRiveRenderer ) return;

    auto d3d12Path = std::dynamic_pointer_cast<CachedPathD3d12>( path );
    if( ! d3d12Path || ! d3d12Path->mImpl->rivePath ) {
        Canvas::fillPath( path, paint, rule );
        return;
    }

    drawCachedPathInternal( d3d12Path.get(), paint, true, false, rule );
}

void CanvasD3d12::strokePath( const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return;

    // If recording, redirect to DisplayList
    if( mRecordingDisplayList ) {
        mRecordingDisplayList->recordStrokePath( path, paint );
        return;
    }

    if( ! mRiveRenderer ) return;

    auto d3d12Path = std::dynamic_pointer_cast<CachedPathD3d12>( path );
    if( ! d3d12Path || ! d3d12Path->mImpl->rivePath ) {
        Canvas::strokePath( path, paint );
        return;
    }

    drawCachedPathInternal( d3d12Path.get(), paint, false, true );
}

// ------------------------------------------------------------------------------------------------
// Uncached Path API (override for DisplayList recording)
// ------------------------------------------------------------------------------------------------

void CanvasD3d12::fillPath( const Path2d &path, const Paint &paint, FillRule rule )
{
    // If recording, create a CachedPath on-the-fly and record it
    if( mRecordingDisplayList ) {
        auto cachedPath = createPath( path );
        if( cachedPath )
            mRecordingDisplayList->recordFillPath( cachedPath, paint, rule );
        return;
    }

    // Otherwise, use the base class implementation
    Canvas::fillPath( path, paint, rule );
}

void CanvasD3d12::strokePath( const Path2d &path, const Paint &paint )
{
    // If recording, create a CachedPath on-the-fly and record it
    if( mRecordingDisplayList ) {
        auto cachedPath = createPath( path );
        if( cachedPath )
            mRecordingDisplayList->recordStrokePath( cachedPath, paint );
        return;
    }

    // Otherwise, use the base class implementation
    Canvas::strokePath( path, paint );
}

void CanvasD3d12::fillShape( const Shape2d &shape, const Paint &paint, FillRule rule )
{
    // If recording, create a CachedPath on-the-fly and record it
    if( mRecordingDisplayList ) {
        auto cachedPath = createPath( shape );
        if( cachedPath )
            mRecordingDisplayList->recordFillPath( cachedPath, paint, rule );
        return;
    }

    // Otherwise, use the base class implementation
    Canvas::fillShape( shape, paint, rule );
}

void CanvasD3d12::strokeShape( const Shape2d &shape, const Paint &paint )
{
    // If recording, create a CachedPath on-the-fly and record it
    if( mRecordingDisplayList ) {
        auto cachedPath = createPath( shape );
        if( cachedPath )
            mRecordingDisplayList->recordStrokePath( cachedPath, paint );
        return;
    }

    // Otherwise, use the base class implementation
    Canvas::strokeShape( shape, paint );
}

void CanvasD3d12::drawCachedPathInternal( const CachedPathD3d12* cachedPath, const Paint &paint,
                                            bool fill, bool stroke, FillRule rule )
{
    if( ! mRiveContext || ! mRiveRenderer || ! cachedPath || ! cachedPath->mImpl || ! cachedPath->mImpl->rivePath ) return;

    // Set the fill rule on the path before drawing
    // RiveRenderPath stores fill rule, and it's checked at draw time
    auto* rivePath = static_cast<RiveRenderPath*>( cachedPath->mImpl->rivePath.get() );

    if( fill ) {
        rivePath->fillRule( toRiveFillRule( rule ) );
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), false );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( rivePath, rivePaint.get() );
        mRiveRenderer->restore();
    }

    if( stroke ) {
        // Strokes always use nonZero fill rule
        rivePath->fillRule( rive::FillRule::nonZero );
        auto rivePaint = paint.createRivePaint( mRiveContext.get(), true );
        mRiveRenderer->save();
        mRiveRenderer->transform( toRiveMat( mTransform ) );
        mRiveRenderer->drawPath( rivePath, rivePaint.get() );
        mRiveRenderer->restore();
    }
}

// ------------------------------------------------------------------------------------------------
// Image API
// ------------------------------------------------------------------------------------------------

ImageRef CanvasD3d12::createImage( const Microsoft::WRL::ComPtr<ID3D12Resource> &resource )
{
    if( ! resource ) return nullptr;

    auto image = std::make_shared<ImageD3d12>();
    image->mResource = resource;

    // Get resource dimensions
    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    image->mSize = ivec2( static_cast<int>( desc.Width ), static_cast<int>( desc.Height ) );

    // TODO: Create Rive RenderImage from D3D12 resource
    // This requires uploading texture data to Rive's internal format
    CI_LOG_W( "CanvasD3d12::createImage(ID3D12Resource) not fully implemented" );

    return image;
}

ImageRef CanvasD3d12::createImage( const gl::Texture2dRef &texture )
{
    // GL textures not available in D3D12 context
    CI_LOG_E( "CanvasD3d12::createImage(gl::Texture2dRef) not supported - use createImage(ID3D12Resource) instead" );
    return nullptr;
}

ImageRef CanvasD3d12::createImage( const Surface &surface )
{
    if( ! mRiveContextD3d12 ) {
        CI_LOG_W( "Cannot create image without valid context" );
        return nullptr;
    }

    auto image = std::make_shared<ImageD3d12>();
    image->mSize = ivec2( surface.getWidth(), surface.getHeight() );

    // Convert to RGBA premultiplied
    std::vector<uint8_t> rgbaData( surface.getWidth() * surface.getHeight() * 4 );

    const uint8_t* srcData = surface.getData();
    bool srcHasAlpha = surface.hasAlpha();
    int srcPixelInc = surface.getPixelInc();
    int srcRowBytes = surface.getRowBytes();

    for( int y = 0; y < surface.getHeight(); ++y ) {
        const uint8_t* srcRow = srcData + y * srcRowBytes;
        uint8_t* dstRow = rgbaData.data() + y * surface.getWidth() * 4;

        for( int x = 0; x < surface.getWidth(); ++x ) {
            uint8_t r = srcRow[0];
            uint8_t g = srcRow[1];
            uint8_t b = srcRow[2];
            uint8_t a = srcHasAlpha ? srcRow[3] : 255;

            // Premultiply alpha
            dstRow[0] = static_cast<uint8_t>( r * a / 255 );
            dstRow[1] = static_cast<uint8_t>( g * a / 255 );
            dstRow[2] = static_cast<uint8_t>( b * a / 255 );
            dstRow[3] = a;

            srcRow += srcPixelInc;
            dstRow += 4;
        }
    }

    auto riveTexture = mRiveContextD3d12->makeImageTexture(
        surface.getWidth(), surface.getHeight(),
        1, // mip levels
        rgbaData.data() );

    image->mImpl->riveImage = make_rcp<RiveRenderImage>( std::move( riveTexture ) );

    return image;
}

void CanvasD3d12::drawImage( const ImageRef &image, const Rectf &destRect )
{
    if( ! image ) return;

    // If recording, redirect to DisplayList
    if( mRecordingDisplayList ) {
        mRecordingDisplayList->recordDrawImage( image, destRect );
        return;
    }

    if( ! mInFrame || ! mRiveRenderer ) return;

    auto d3d12Image = std::dynamic_pointer_cast<ImageD3d12>( image );
    if( ! d3d12Image || ! d3d12Image->mImpl || ! d3d12Image->mImpl->riveImage ) {
        CI_LOG_W( "drawImage: Invalid D3D12 image" );
        return;
    }

    mRiveRenderer->save();
    mRiveRenderer->transform( toRiveMat( mTransform ) );

    // Scale and position the image
    float sx = destRect.getWidth() / image->getWidth();
    float sy = destRect.getHeight() / image->getHeight();
    mRiveRenderer->transform( Mat2D( sx, 0, 0, sy, destRect.x1, destRect.y1 ) );

    mRiveRenderer->drawImage( d3d12Image->mImpl->riveImage.get(), rive::ImageSampler::LinearClamp(), rive::BlendMode::srcOver, 1.0f );
    mRiveRenderer->restore();
}

void CanvasD3d12::drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect )
{
    if( ! image ) return;

    // If recording, redirect to DisplayList
    if( mRecordingDisplayList ) {
        mRecordingDisplayList->recordDrawImage( image, srcRect, destRect );
        return;
    }

    if( ! mInFrame || ! mRiveRenderer ) return;

    auto d3d12Image = std::dynamic_pointer_cast<ImageD3d12>( image );
    if( ! d3d12Image || ! d3d12Image->mImpl || ! d3d12Image->mImpl->riveImage )
        return;

    // Create a mesh for the sub-rectangle
    float u0 = srcRect.x1 / image->getWidth();
    float v0 = srcRect.y1 / image->getHeight();
    float u1 = srcRect.x2 / image->getWidth();
    float v1 = srcRect.y2 / image->getHeight();

    std::vector<vec2> vertices = {
        vec2( destRect.x1, destRect.y1 ),
        vec2( destRect.x2, destRect.y1 ),
        vec2( destRect.x2, destRect.y2 ),
        vec2( destRect.x1, destRect.y2 )
    };

    std::vector<vec2> uvs = {
        vec2( u0, v0 ),
        vec2( u1, v0 ),
        vec2( u1, v1 ),
        vec2( u0, v1 )
    };

    std::vector<uint16_t> indices = { 0, 1, 2, 0, 2, 3 };

    drawImageMesh( image, vertices, uvs, indices, 1.0f );
}

void CanvasD3d12::drawImageMesh( const ImageRef &image,
                                  std::span<const vec2> vertices,
                                  std::span<const vec2> uvs,
                                  std::span<const uint16_t> indices,
                                  float opacity )
{
    if( ! mInFrame || ! mRiveRenderer || ! mRiveContext || ! image ) return;
    if( vertices.size() != uvs.size() ) return;

    auto d3d12Image = std::dynamic_pointer_cast<ImageD3d12>( image );
    if( ! d3d12Image || ! d3d12Image->mImpl || ! d3d12Image->mImpl->riveImage )
        return;

    // Create Rive buffers
    auto vertexBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::vertex, RenderBufferFlags::none, vertices.size() * sizeof( float ) * 2 );
    auto uvBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::vertex, RenderBufferFlags::none, uvs.size() * sizeof( float ) * 2 );
    auto indexBuffer = mRiveContext->makeRenderBuffer( RenderBufferType::index, RenderBufferFlags::none, indices.size() * sizeof( uint16_t ) );

    // Map and fill buffers
    float* vertexData = static_cast<float*>( vertexBuffer->map() );
    float* uvData = static_cast<float*>( uvBuffer->map() );
    uint16_t* indexData = static_cast<uint16_t*>( indexBuffer->map() );

    for( size_t i = 0; i < vertices.size(); ++i ) {
        vertexData[i * 2] = vertices[i].x;
        vertexData[i * 2 + 1] = vertices[i].y;
        uvData[i * 2] = uvs[i].x;
        uvData[i * 2 + 1] = uvs[i].y;
    }

    std::memcpy( indexData, indices.data(), indices.size() * sizeof( uint16_t ) );

    vertexBuffer->unmap();
    uvBuffer->unmap();
    indexBuffer->unmap();

    mRiveRenderer->save();
    mRiveRenderer->transform( toRiveMat( mTransform ) );

    mRiveRenderer->drawImageMesh(
        d3d12Image->mImpl->riveImage.get(),
        rive::ImageSampler::LinearClamp(),
        vertexBuffer,
        uvBuffer,
        indexBuffer,
        static_cast<uint32_t>( vertices.size() ),
        static_cast<uint32_t>( indices.size() ),
        rive::BlendMode::srcOver,
        opacity );

    mRiveRenderer->restore();
}

// ------------------------------------------------------------------------------------------------
// DisplayList API
// ------------------------------------------------------------------------------------------------

DisplayListRef CanvasD3d12::createDisplayList()
{
    return std::make_shared<DisplayListD3d12>( this );
}

} } // namespace cinder::vg
