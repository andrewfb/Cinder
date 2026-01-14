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

struct FrozenPathD3d12Impl {
    CachedPathRef cachedPath;  // Just store a reference to the cached path
    bool isStroke = false;     // Was this frozen for fill or stroke?
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
// FrozenPathD3d12 implementation
// ------------------------------------------------------------------------------------------------

FrozenPathD3d12::FrozenPathD3d12() : mImpl( std::make_unique<FrozenPathD3d12Impl>() ) {}

FrozenPathD3d12::~FrozenPathD3d12() = default;

bool FrozenPathD3d12::isValid() const
{
    // Stub implementation - valid if we have a cached path
    return mImpl && mImpl->cachedPath;
}

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
}

void DisplayListD3d12::endRecording()
{
    if( ! mRecording ) {
        CI_LOG_W( "DisplayListD3d12::endRecording called without beginRecording" );
        return;
    }
    mRecording = false;
    mRecordingCanvas = nullptr;
    mValid = !mCommands.empty();
}

void DisplayListD3d12::replay( Canvas* canvas )
{
    if( ! mValid || mCommands.empty() ) {
        return;
    }

    // TODO: Implement replay with frozen paths
    CI_LOG_W( "DisplayListD3d12::replay not yet implemented" );
}

void DisplayListD3d12::clear()
{
    mCommands.clear();
    mValid = false;
    mBounds = Rectf();
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

void CanvasD3d12::initializeD3d12()
{
    CI_LOG_I( "Creating Rive D3D12 context" );

    auto device = mRenderer->getDevice();
    CI_ASSERT_MSG( device, "RendererD3d12 device is null" );

    UINT bufferCount = mRenderer->getBufferCount();

    // Create per-frame command allocators
    for( UINT i = 0; i < bufferCount; i++ ) {
        HRESULT hr = device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS( &mCommandAllocators[i] ) );
        if( FAILED( hr ) ) {
            throw vg::Exc( "Failed to create D3D12 command allocator" );
        }
    }

    // Create command list (starts in recording state with first allocator)
    HRESULT hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mCommandAllocators[0].Get(),
        nullptr,  // No initial pipeline state
        IID_PPV_ARGS( &mCommandList ) );
    if( FAILED( hr ) ) {
        throw vg::Exc( "Failed to create D3D12 command list" );
    }

    // Create Rive context - this records initialization commands to mCommandList
    D3DContextOptions riveOptions;
    // Use default options for now

    mRiveContext = RenderContextD3D12Impl::MakeContext(
        ComPtr<ID3D12Device>( device ),
        mCommandList.Get(),
        riveOptions );

    if( ! mRiveContext ) {
        throw vg::Exc( "Failed to create Rive D3D12 RenderContext" );
    }

    // Store the D3D12-specific pointer for D3D12-specific calls
    mRiveContextD3d12 = mRiveContext->static_impl_cast<RenderContextD3D12Impl>();

    // CRITICAL: Execute initialization commands
    // Rive records static buffer uploads during MakeContext - we must execute them
    mCommandList->Close();
    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mRenderer->getCommandQueue()->ExecuteCommandLists( 1, lists );

    // Wait for initialization to complete using a dedicated fence
    // We can't use waitForGpu() here because it returns early if mFenceCounter == 0
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

    CI_LOG_I( "CanvasD3d12 created successfully with " << bufferCount << " back buffers" );
}

void CanvasD3d12::begin( const ivec2 &size )
{
    CI_ASSERT_MSG( ! mInFrame, "begin() called while already in frame - did you forget to call end()?" );
    CI_ASSERT_MSG( size.x > 0 && size.y > 0, "begin() size must be positive" );

    mFrameSize = size;
    mInFrame = true;

    UINT frameIndex = mRenderer->getCurrentBackBufferIndex();

    // Wait for this frame's GPU work to complete before reusing its allocator
    // Note: After resize, fence values are preserved (set to mFenceCounter), so this still works
    mRenderer->waitForFrame( frameIndex );

    // Get back buffer first - it should always be valid after proper initialization
    // If this fails, the renderer is in a bad state (resize failed or not initialized)
    auto backBuffer = mRenderer->getCurrentBackBuffer();
    if( ! backBuffer ) {
        // Can happen briefly during ResizeBuffers when the app's draw() is invoked while
        // swapchain buffers are being torn down/recreated. Skip the frame gracefully.
        CI_LOG_W( "CanvasD3d12::begin() skipping frame: back buffer is null (resize in progress)" );
        mInFrame = false;
        return;
    }

    // Reset command allocator and command list for this frame
    mCommandAllocators[frameIndex]->Reset();
    mCommandList->Reset( mCommandAllocators[frameIndex].Get(), nullptr );

    // Use actual back buffer size as the authoritative render size
    D3D12_RESOURCE_DESC backDesc = backBuffer->GetDesc();
    ivec2 backBufferSize( static_cast<int>( backDesc.Width ), static_cast<int>( backDesc.Height ) );

    // Detect resize by comparing to last known back buffer size
    bool resizeDetected = ( mLastFrameSize != backBufferSize && mLastFrameSize.x > 0 && mLastFrameSize.y > 0 );
    if( resizeDetected ) {
        // Invalidate ALL render targets (all back buffers are recreated)
        UINT bufferCount = mRenderer->getBufferCount();
        for( UINT i = 0; i < bufferCount; i++ ) {
            mRenderTargets[i] = nullptr;
            mBackBufferInCommonState[i] = true;  // All new buffers are in COMMON state
        }
    }

    // Handle new buffers from ResizeBuffers - they start in COMMON state
    // Rive's setTargetTexture() assumes PRESENT state, so transition COMMON→PRESENT
    // DON'T transition to RENDER_TARGET here - Rive will do that internally
    if( mBackBufferInCommonState[frameIndex] ) {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_PRESENT );
        mCommandList->ResourceBarrier( 1, &barrier );
        mBackBufferInCommonState[frameIndex] = false;
    }
    // Note: Do NOT transition PRESENT→RENDER_TARGET here - Rive handles this internally

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
    CI_ASSERT_MSG( mInFrame, "end() called without begin()" );

    // Clear base class pointer and destroy the renderer before flush
    mRiveRenderer = nullptr;
    mOwnedRiveRenderer.reset();

    UINT frameIndex = mRenderer->getCurrentBackBufferIndex();

    // Set up command lists for Rive flush
    // Rive's D3D12 implementation expects a CommandLists struct via externalCommandBuffer
    RenderContextD3D12Impl::CommandLists cmdLists;
    cmdLists.copyComandList = nullptr;  // Use directComandList for copies too
    cmdLists.directComandList = mCommandList.Get();

    // Flush Rive rendering to cached render target
    // Use proper frame numbers for resource lifecycle management
    // safeFrameNumber = frame that's definitely completed on GPU
    // With triple buffering: frame N is being recorded, frame N-1 may be executing,
    // frame N-2 may still be in flight, frame N-3 is definitely done.
    // Use bufferCount as the lag to ensure safety across different buffer configurations.
    UINT bufferCount = mRenderer->getBufferCount();
    uint64_t safeFrame = (mFrameNumber > bufferCount) ? (mFrameNumber - bufferCount - 1) : 0;

    RenderContext::FlushResources flushRes;
    flushRes.renderTarget = mRenderTargets[frameIndex].get();
    flushRes.externalCommandBuffer = &cmdLists;
    flushRes.currentFrameNumber = mFrameNumber;
    flushRes.safeFrameNumber = safeFrame;
    mRiveContext->flush( flushRes );

    // Increment frame counter for next frame
    ++mFrameNumber;

    // Invoke post-flush callback for additional D3D12 rendering (e.g., ImGui)
    // After Rive flush, render target is in COMMON state
    if( mPostFlushCallback ) {
        // Get current back buffer
        auto backBuffer = mRenderer->getCurrentBackBuffer();
        if( backBuffer ) {
            // Transition COMMON → RENDER_TARGET for ImGui rendering
            CD3DX12_RESOURCE_BARRIER toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_RENDER_TARGET );
            mCommandList->ResourceBarrier( 1, &toRenderTarget );

            // Set render target for ImGui
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRenderer->getRtvHandle( frameIndex );
            mCommandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

            // Set viewport and scissor
            D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)mFrameSize.x, (float)mFrameSize.y, 0.0f, 1.0f };
            D3D12_RECT scissor = { 0, 0, (LONG)mFrameSize.x, (LONG)mFrameSize.y };
            mCommandList->RSSetViewports( 1, &viewport );
            mCommandList->RSSetScissorRects( 1, &scissor );

            // Invoke callback (e.g., ImGui rendering)
            mPostFlushCallback( mCommandList.Get() );

            // Transition RENDER_TARGET → PRESENT for swap chain
            CD3DX12_RESOURCE_BARRIER toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
                backBuffer,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT );
            mCommandList->ResourceBarrier( 1, &toPresent );
        }
    }
    // Note: If no callback, Rive's flush() transitions the render target to COMMON state.
    // D3D12_RESOURCE_STATE_COMMON is implicitly promotable to PRESENT for swap chain buffers.

    // Close and execute command list
    mCommandList->Close();
    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mRenderer->getCommandQueue()->ExecuteCommandLists( 1, lists );

    // Signal frame fence for synchronization
    mRenderer->signalFrameFence();

    mInFrame = false;
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
    if( ! path || ! mRiveRenderer ) return;

    auto d3d12Path = std::dynamic_pointer_cast<CachedPathD3d12>( path );
    if( ! d3d12Path || ! d3d12Path->mImpl->rivePath ) {
        // Fall back to base class implementation using source shape
        Canvas::fillPath( path, paint, rule );
        return;
    }

    drawCachedPathInternal( d3d12Path.get(), paint, true, false, rule );
}

void CanvasD3d12::strokePath( const CachedPathRef &path, const Paint &paint )
{
    if( ! path || ! mRiveRenderer ) return;

    auto d3d12Path = std::dynamic_pointer_cast<CachedPathD3d12>( path );
    if( ! d3d12Path || ! d3d12Path->mImpl->rivePath ) {
        // Fall back to base class implementation using source shape
        Canvas::strokePath( path, paint );
        return;
    }

    drawCachedPathInternal( d3d12Path.get(), paint, false, true );
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
    // TODO: Create D3D12 texture from surface and wrap in Image
    CI_LOG_W( "CanvasD3d12::createImage(Surface) not yet implemented" );
    return nullptr;
}

void CanvasD3d12::drawImage( const ImageRef &image, const Rectf &destRect )
{
    if( ! image || ! mRiveRenderer ) return;

    auto d3d12Image = std::dynamic_pointer_cast<ImageD3d12>( image );
    if( ! d3d12Image || ! d3d12Image->mImpl->riveImage ) {
        CI_LOG_W( "drawImage: Invalid D3D12 image" );
        return;
    }

    // TODO: Implement image drawing
    CI_LOG_W( "CanvasD3d12::drawImage not yet implemented" );
}

void CanvasD3d12::drawImage( const ImageRef &image, const Rectf &srcRect, const Rectf &destRect )
{
    if( ! image || ! mRiveRenderer ) return;

    // TODO: Implement image drawing with source rect
    CI_LOG_W( "CanvasD3d12::drawImage(srcRect) not yet implemented" );
}

void CanvasD3d12::drawImageMesh( const ImageRef &image,
                                  std::span<const vec2> vertices,
                                  std::span<const vec2> uvs,
                                  std::span<const uint16_t> indices,
                                  float opacity )
{
    if( ! image || ! mRiveRenderer ) return;

    // TODO: Implement mesh image drawing
    CI_LOG_W( "CanvasD3d12::drawImageMesh not yet implemented" );
}

// ------------------------------------------------------------------------------------------------
// FrozenPath API
// ------------------------------------------------------------------------------------------------

FrozenPathRef CanvasD3d12::freezePathFill( const CachedPathRef &path, const Paint &paint, FillRule rule )
{
    if( ! path ) return nullptr;

    auto frozen = std::make_shared<FrozenPathD3d12>();
    frozen->mSourcePath = path;
    frozen->mBounds = path->getBounds();
    frozen->mIsStroke = false;
    frozen->mFillRule = rule;
    frozen->mImpl->cachedPath = path;
    frozen->mImpl->isStroke = false;

    return frozen;
}

FrozenPathRef CanvasD3d12::freezePathStroke( const CachedPathRef &path, const Paint &paint )
{
    if( ! path ) return nullptr;

    auto frozen = std::make_shared<FrozenPathD3d12>();
    frozen->mSourcePath = path;
    frozen->mBounds = path->getBounds();
    frozen->mIsStroke = true;
    frozen->mStrokeWidth = paint.getStrokeWidth();
    frozen->mImpl->cachedPath = path;
    frozen->mImpl->isStroke = true;

    return frozen;
}

void CanvasD3d12::drawFrozenPath( const FrozenPathRef &frozenPath, const Paint &paint )
{
    if( ! frozenPath ) return;

    auto d3d12Frozen = std::dynamic_pointer_cast<FrozenPathD3d12>( frozenPath );
    if( ! d3d12Frozen || ! d3d12Frozen->mImpl->cachedPath ) return;

    // Just draw the cached path - D3D12/Rive handles caching internally
    if( d3d12Frozen->mImpl->isStroke ) {
        strokePath( d3d12Frozen->mImpl->cachedPath, paint );
    } else {
        fillPath( d3d12Frozen->mImpl->cachedPath, paint, d3d12Frozen->mFillRule );
    }
}

// ------------------------------------------------------------------------------------------------
// DisplayList API
// ------------------------------------------------------------------------------------------------

DisplayListRef CanvasD3d12::createDisplayList()
{
    return std::make_shared<DisplayListD3d12>( this );
}

} } // namespace cinder::vg
