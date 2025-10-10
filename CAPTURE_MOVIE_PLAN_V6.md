# Capture & Movie Implementation Plan
**Zero-Copy Video with GPU YUV→RGB Conversion**

---

## Overview

This document provides the implementation roadmap for modernizing Cinder's video capture and movie playback systems. The goal is zero-copy GPU workflows with transparent YUV→RGB conversion on Windows, macOS, and Linux.

Starting point: `blocks/AX-VideoCapture` and `blocks/AX-MediaPlayer` contain working prototypes for Windows Media Foundation capture and playback.

---

## Architecture Summary

### Core Design Principles

1. **Platform APIs deliver YUV, we convert to RGB transparently**
   - Media Foundation outputs NV12/YUY2 D3D textures
   - AVFoundation can output BGRA directly (requests RGB from platform)
   - GStreamer uses `videoconvert` element before `glupload`

2. **Different pipeline configuration for CPU vs GL paths**
   - CPU path: System memory buffers, copy to `Surface8u`
   - GL path: GPU textures with platform-specific interop

3. **Thread safety via lock-free queues**
   - Platform callbacks run on non-GL threads
   - Producer pushes `FrameEnvelope` to ring buffer
   - Consumer (GL thread) pops, converts, and tracks with GL fences

4. **Resource lifetime managed by GPU fences**
   - Hold strong references to platform resources (IMFSample, CVPixelBuffer, GstSample)
   - Insert GL fence after rendering
   - Release resources when fence signals complete

### Component Layers

```
User API:
  ci::Capture / ci::gl::CaptureGl
  ci::Movie / ci::gl::MovieGl

Platform Implementations:
  CaptureImplMediaFoundation / MovieImplMediaFoundation  (Windows)
  CaptureImplAvFoundation / MovieImplAvFoundation        (macOS)
  CaptureImplGStreamer / MovieImplGStreamer              (Linux)
  CaptureImplDirectShow                                  (Windows fallback)

GPU Interop (Windows only - others use platform mechanisms):
  msw::D3dBackplane - Shared D3D device + YUV→RGB compute shaders + WGL interop
```

### Three Frame Delivery Models

**Model A: Foreign YUV → Our RGB (Most Platforms)**
- Platform creates YUV GPU resources
- We convert YUV→RGB on GPU
- Wrap result as GL texture
- Example: Media Foundation NV12 → D3D compute → WGL interop

**Model B: Request RGB from Platform (macOS)**
- Platform does YUV→RGB internally
- We wrap platform's RGB buffer as GL texture
- Example: AVFoundation BGRA → CVOpenGLTextureCache

**Model C: CPU Fallback (DirectShow, software paths)**
- Platform delivers system memory
- We upload to GL via PBO
- Example: DirectShow ISampleGrabber → memcpy → PBO upload

---

## Implementation Checkpoints

Each checkpoint is testable independently. Later checkpoints may temporarily "undo" earlier simplifications.

---

### Checkpoint 1: Integrate AX-VideoCapture (CPU Path)

**Goal**: Move working MF capture prototype into Cinder proper, CPU-only path (no GL yet).

**Steps**:

1. Copy `blocks/AX-VideoCapture/src/MFCameraCapture.{h,cpp}` to `src/cinder/msw/`
2. Rename to `CaptureImplMediaFoundation`
3. Adapt to match existing `CaptureImpl` interface:
   ```cpp
   class CaptureImplMediaFoundation {
   public:
       CaptureImplMediaFoundation(int32_t width, int32_t height, const Capture::DeviceRef& device);
       void start();
       void stop();
       bool checkNewFrame();
       Surface8uRef getSurface() const;
       int32_t getWidth() const;
       int32_t getHeight() const;
       // ...
   };
   ```
4. Device enumeration via `IMFMediaSource` and `IMFActivate`
5. Format enumeration via `IMFMediaType` queries
6. Request RGB32 (BGRA) from Media Foundation (no YUV yet)
7. Use `IMFSourceReader` with async callback on MF thread
8. Callback copies sample to `Surface8u` (CPU path)
9. Add to `Capture.cpp` platform selection:
   ```cpp
   #if defined(CINDER_MSW)
       #if defined(CINDER_MSW_USE_MEDIA_FOUNDATION)  // New build flag
           typedef cinder::CaptureImplMediaFoundation CapturePlatformImpl;
       #else
           typedef cinder::CaptureImplDirectShow CapturePlatformImpl;  // Existing
       #endif
   #endif
   ```

**Test**: `samples/CaptureBasic` should work with MF backend, displaying camera feed.

**Verify**:
- Device enumeration lists cameras
- 1920×1080 @ 30fps captures successfully
- Image displays correctly
- No memory leaks (run for 5 minutes, check Task Manager)

---

### Checkpoint 2: D3D Device with LUID Matching

**Goal**: Create shared D3D11 device that matches GL adapter (critical for Optimus/hybrid laptops).

**Steps**:

1. Create `include/cinder/msw/D3dBackplane.h`
2. Implement singleton pattern:
   ```cpp
   class D3dBackplane {
   public:
       static D3dBackplane& instance();
       bool initialize();
       ID3D11Device1* getDevice();
       ID3D11DeviceContext1* getImmediateContext();
       IMFDXGIDeviceManager* getDXGIDeviceManager(UINT* resetToken);
   private:
       D3dBackplane();
       ComPtr<ID3D11Device1> mDevice;
       ComPtr<ID3D11DeviceContext1> mContext;
       ComPtr<IMFDXGIDeviceManager> mDXGIDeviceManager;
       UINT mResetToken;
   };
   ```
3. Get GL adapter LUID:
   - Try `WGL_AMD_gpu_association` extension first
   - Fall back to `DXGI_ADAPTER_FLAG_SOFTWARE` exclusion
4. Enumerate DXGI adapters, match LUID
5. Create device with flags: `D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT`
6. Create `IMFDXGIDeviceManager` and register device
7. Update `CaptureImplMediaFoundation` to use shared device

**Test**: Create simple app that initializes D3dBackplane and prints adapter info.

**Verify**:
- Correct GPU selected on hybrid systems
- MFCreateDXGIDeviceManager succeeds
- Device has VIDEO_SUPPORT capability

---

### Checkpoint 3: WGL Interop (RGB Only)

**Goal**: Wrap D3D RGB texture as GL texture via `WGL_NV_DX_interop2`.

**Steps**:

1. Add to `D3dBackplane`:
   ```cpp
   bool initializeWglInterop(HGLRC mainGlContext, HDC mainDC);
   HANDLE getWglDevice();

   // Wrap D3D texture as GL texture (no conversion yet, RGB only)
   gl::TextureRef wrapD3DTexture(ID3D11Texture2D* d3dTexture);

   // Register/unregister WGL handles
   HANDLE registerTexture(ID3D11Texture2D* d3dTexture, GLuint glTexId);
   void unregisterTexture(HANDLE wglHandle);

   // Lock/unlock for GL access
   bool lockTexture(HANDLE wglHandle);
   bool unlockTexture(HANDLE wglHandle);
   ```
2. Load `WGL_NV_DX_interop2` extension functions
3. Call `wglDXOpenDeviceNV(mDevice.Get())`
4. Implement `wrapD3DTexture`:
   - Pre-allocate GL texture (glGenTextures, glBindTexture, glTexStorage2D)
   - Call `wglDXRegisterObjectNV` with D3D texture and GL texture ID
   - Store mapping in `std::map<ID3D11Texture2D*, HANDLE>`
   - Wrap in `gl::Texture::create()` with custom deleter
5. Deleter calls `wglDXUnregisterObjectNV` when texture destroyed

**Test**: Create D3D texture with test pattern (checkerboard), wrap as GL texture, render.

**Verify**:
- Checkerboard pattern displays correctly
- Works on NVIDIA, AMD, Intel GPUs
- No GL errors (`glGetError()` returns `GL_NO_ERROR`)
- Texture destruction doesn't leak (`wglDXUnregisterObjectNV` called)

---

### Checkpoint 4: MF D3D Texture Output

**Goal**: Get Media Foundation to output D3D textures instead of system memory.

**Steps**:

1. Update `CaptureImplMediaFoundation::start()`:
   ```cpp
   ComPtr<IMFAttributes> attrs;
   MFCreateAttributes(&attrs, 2);

   // Enable D3D manager
   UINT resetToken;
   auto dxgiMgr = D3dBackplane::instance().getDXGIDeviceManager(&resetToken);
   attrs->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiMgr);

   // Set async callback
   attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, mCallback.Get());

   MFCreateSourceReaderFromMediaSource(mediaSource, attrs.Get(), &mSourceReader);
   ```
2. Still request RGB32 (no YUV yet): `MFVideoFormat_RGB32`
3. In callback, extract D3D texture:
   ```cpp
   void onSample(IMFSample* sample) {
       ComPtr<IMFMediaBuffer> buffer;
       sample->GetBufferByIndex(0, &buffer);

       ComPtr<IMFDXGIBuffer> dxgiBuffer;
       if (SUCCEEDED(buffer.As(&dxgiBuffer))) {
           ComPtr<ID3D11Texture2D> d3dTexture;
           dxgiBuffer->GetResource(IID_PPV_ARGS(&d3dTexture));

           // For testing: copy to staging buffer, verify data
           // (Later: wrap as GL texture)
       }
   }
   ```
4. For verification, copy D3D texture to staging buffer and readback to CPU

**Test**: Verify D3D texture contains correct image data via staging buffer readback.

**Verify**:
- `IMFSample` contains `IMFDXGIBuffer` (not system memory)
- `GetResource` succeeds and returns `ID3D11Texture2D`
- Staging buffer readback shows correct image
- D3D debug layer reports no errors

---

### Checkpoint 5: GL Path with RGB (No YUV Yet)

**Goal**: Full zero-copy path with RGB textures (temporary - YUV comes next).

**Steps**:

1. Create `include/cinder/gl/CaptureGl.h`:
   ```cpp
   namespace cinder { namespace gl {

   class CaptureGl {
   public:
       static CaptureGlRef create(int32_t width, int32_t height, const Capture::DeviceRef& device);

       void start();
       void stop();
       bool checkNewFrame();
       gl::TextureRef getTexture() const;
       // ... mirror Capture API

   private:
       std::shared_ptr<CaptureImplMediaFoundation> mImpl;
       gl::TextureRef mCurrentTexture;
   };

   }} // namespace cinder::gl
   ```
2. Update `CaptureImplMediaFoundation` to support GL consumers:
   - Callback creates `FrameEnvelope` with `ComPtr<IMFSample>`
   - Push to `moodycamel::ReaderWriterQueue<FrameEnvelope>`
3. In `CaptureGl::checkNewFrame()` (on GL thread):
   ```cpp
   bool checkNewFrame() {
       FrameEnvelope envelope;
       if (!mImpl->dequeueFrame(envelope)) {
           return false;
       }

       // Extract D3D texture from IMFSample
       ComPtr<ID3D11Texture2D> d3dTex = extractTexture(envelope.platformResource);

       // Wrap as GL texture via WGL interop
       gl::TextureRef glTex = D3dBackplane::instance().wrapD3DTexture(d3dTex.Get());

       // Insert fence to track GPU completion
       GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
       envelope.fence = fence;
       envelope.glTexture = glTex;

       // Store in pending queue
       mPendingFrames.push_back(std::move(envelope));

       // Return latest texture
       mCurrentTexture = mPendingFrames.back().glTexture;
       return true;
   }
   ```
4. Implement `retirePendingFrames()` called before `checkNewFrame()`:
   ```cpp
   void retirePendingFrames() {
       while (!mPendingFrames.empty()) {
           auto& front = mPendingFrames.front();

           GLenum result = glClientWaitSync(front.fence, 0, 0);
           if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
               glDeleteSync(front.fence);
               mPendingFrames.pop_front();
           } else {
               break;  // Oldest frame not done yet
           }
       }
   }
   ```

**Test**: `samples/CaptureGl` displays camera via GL texture.

**Verify**:
- Zero-copy (verify with GPU profiler - no D3D→CPU→GL copy)
- CPU usage near zero
- Correct rendering
- No corruption after 5 minutes
- Memory stable (pending queue doesn't grow unbounded)

---

### Checkpoint 6: YUV→RGB Compute Shader (NV12)

**Goal**: Convert NV12 D3D texture to RGBA on GPU.

**Steps**:

1. Add to `D3dBackplane`:
   ```cpp
   struct ConversionJob {
       ComPtr<ID3D11Texture2D> yuvTexture;
       DXGI_FORMAT yuvFormat;  // e.g., DXGI_FORMAT_NV12
       Capture::Mode::ColorSpace colorspace;
       ComPtr<ID3D11Texture2D> rgbTexture;  // Output (pre-allocated)
   };

   bool convertYuvToRgb(const ConversionJob& job);
   ```
2. Write `shaders/nv12_to_rgba.hlsl`:
   ```hlsl
   Texture2D<float> yPlane : register(t0);
   Texture2D<float2> uvPlane : register(t1);
   RWTexture2D<float4> output : register(u0);

   cbuffer ColorMatrix : register(b0) {
       float4x4 yuvToRgbMatrix;
   };

   [numthreads(8, 8, 1)]
   void main(uint3 DTid : SV_DispatchThreadID) {
       uint2 pos = DTid.xy;

       float y = yPlane[pos];
       float2 uv = uvPlane[pos / 2];

       float3 yuv = float3(y, uv.x, uv.y);
       float3 rgb = mul(yuvToRgbMatrix, float4(yuv, 1.0)).rgb;

       output[pos] = float4(rgb, 1.0);
   }
   ```
3. Compile shader at build time, embed as byte array
4. Implement `convertYuvToRgb`:
   - Create SRVs for Y and UV planes
   - Create UAV for RGB output
   - Set colorspace matrix constant buffer (BT.601, BT.709, or BT.2020)
   - Dispatch compute shader: `(width/8, height/8, 1)`
   - Insert GPU fence to ensure completion
5. Update `CaptureImplMediaFoundation` to request NV12:
   ```cpp
   mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
   ```
6. Update `CaptureGl::checkNewFrame()` to perform conversion:
   ```cpp
   // Extract YUV D3D texture
   ComPtr<ID3D11Texture2D> yuvTex = extractTexture(envelope.platformResource);

   // Allocate or reuse RGB D3D texture
   ComPtr<ID3D11Texture2D> rgbTex = allocateRgbTexture(width, height);

   // Convert on GPU
   D3dBackplane::ConversionJob job;
   job.yuvTexture = yuvTex;
   job.yuvFormat = DXGI_FORMAT_NV12;
   job.colorspace = mCurrentMode.getColorSpace();
   job.rgbTexture = rgbTex;
   D3dBackplane::instance().convertYuvToRgb(job);

   // Wrap RGB texture as GL
   gl::TextureRef glTex = D3dBackplane::instance().wrapD3DTexture(rgbTex.Get());
   ```

**Test**: Capture with NV12 camera mode, verify correct colors.

**Verify**:
- Skin tones look natural (not green/purple)
- BT.709 vs BT.601 difference visible with test content
- GPU time <1ms for 1080p (use GPU profiler)
- No artifacts or banding

---

### Checkpoint 7: Additional YUV Formats

**Goal**: Support I420, YUY2, UYVY formats.

**Steps**:

1. Write shaders:
   - `i420_to_rgba.hlsl` (three planes: Y, U, V)
   - `yuy2_to_rgba.hlsl` (packed 4:2:2)
   - `uyvy_to_rgba.hlsl` (packed 4:2:2, U first)
2. Add format detection in `CaptureImplMediaFoundation`:
   ```cpp
   GUID subtype;
   mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);

   if (subtype == MFVideoFormat_NV12) {
       mYuvFormat = DXGI_FORMAT_NV12;
   } else if (subtype == MFVideoFormat_I420) {
       mYuvFormat = DXGI_FORMAT_???;  // Need custom handling
   }
   // ...
   ```
3. Update `D3dBackplane::convertYuvToRgb` to dispatch appropriate shader
4. Test with cameras/files that output each format

**Test**: Verify all YUV formats convert correctly with test patterns.

**Verify**:
- Color accuracy for each format
- No performance regression

---

### Checkpoint 8: Colorspace Detection

**Goal**: Automatically detect and apply correct colorspace matrix.

**Steps**:

1. Extend `Capture::Mode`:
   ```cpp
   enum class ColorSpace {
       BT601,   // SD video
       BT709,   // HD video (1080p)
       BT2020,  // UHD/HDR
       sRGB,
       Unknown
   };

   ColorSpace getColorSpace() const;
   ```
2. Detect from Media Foundation metadata:
   ```cpp
   ComPtr<IMFMediaType> mediaType;
   mSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &mediaType);

   UINT32 colorspace;
   if (SUCCEEDED(mediaType->GetUINT32(MF_MT_VIDEO_PRIMARIES, &colorspace))) {
       switch (colorspace) {
           case MFVideoPrimaries_BT601_625:
           case MFVideoPrimaries_BT601_525:
               return ColorSpace::BT601;
           case MFVideoPrimaries_BT709:
               return ColorSpace::BT709;
           case MFVideoPrimaries_BT2020:
               return ColorSpace::BT2020;
       }
   }

   // Fallback: infer from resolution
   if (width <= 720) return ColorSpace::BT601;  // SD
   else return ColorSpace::BT709;  // HD
   ```
3. Update compute shaders to use appropriate matrix constants

**Test**: Compare output with known-good reference images.

**Verify**:
- SD content uses BT.601
- HD content uses BT.709
- Colors match reference

---

### Checkpoint 9: Movie Playback (CPU Path)

**Goal**: Integrate AX-MediaPlayer for video file playback, CPU path first.

**Steps**:

1. Copy `blocks/AX-MediaPlayer/src/MFMediaPlayer.{h,cpp}` to `src/cinder/msw/`
2. Rename to `MovieImplMediaFoundation`
3. Implement `MovieImpl` interface (mirrors existing `Movie` class)
4. Use `IMFSourceReader` with file path
5. Playback controls: play, pause, seek, rate
6. Async callback delivers frames
7. CPU path: copy to `Surface8u`

**Test**: `samples/MovieBasic` plays MP4 files.

**Verify**:
- MP4 playback at correct framerate
- Seek works accurately
- Pause/resume stable
- No memory leaks over long playback

---

### Checkpoint 10: Movie GL Path with YUV

**Goal**: Zero-copy movie playback with YUV→RGB conversion.

**Steps**:

1. Create `include/cinder/gl/MovieGl.h`
2. Apply same pattern as `CaptureGl`:
   - Frame ring buffer
   - GL thread pops and converts
   - Fence tracking
3. Request D3D texture output from MF
4. Use `D3dBackplane::convertYuvToRgb` (same as capture)
5. WGL interop wraps result

**Test**: `samples/MovieGl` plays MP4 with zero-copy.

**Verify**:
- Zero-copy (GPU profiler confirms)
- Scrubbing/seeking responsive
- CPU usage minimal
- Correct color reproduction

---

### Checkpoint 11: Async Conversion Thread (Optional Optimization)

**Goal**: Move YUV→RGB conversion off GL thread to dedicated worker.

**Note**: This is an optimization that can be skipped initially. The compute shader is fast enough (~0.1ms) that running it on the GL thread is acceptable. Include this checkpoint if profiling shows GL thread bottleneck.

**Steps**:

1. Add to `D3dBackplane`:
   ```cpp
   bool initializeWorkerThread();
   void submitConversionJob(ConversionJob job, std::function<void(bool)> onComplete);

   private:
   std::thread mWorkerThread;
   std::atomic<bool> mRunning;
   ConcurrentCircularBuffer<ConversionJob> mJobQueue;
   ```
2. Worker thread has shared GL context via `wglShareLists()`
3. Worker processes jobs, calls completion callback
4. GL thread submits conversion job, gets notified when complete

**Test**: Verify no performance regression, thread safety.

**Verify**:
- Conversion happens on worker thread (profiler confirms)
- No race conditions (run with Thread Sanitizer)
- Latency doesn't increase

---

### Checkpoint 12: macOS AVFoundation (RGB Direct)

**Goal**: macOS capture with AVFoundation requesting RGB.

**Steps**:

1. Update existing `CaptureImplAvFoundation`:
   ```objc
   NSDictionary* settings = @{
       (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
       (id)kCVPixelBufferIOSurfacePropertiesKey: @{}  // Enable IOSurface
   };
   [mVideoOutput setVideoSettings:settings];
   ```
2. AVFoundation performs YUV→RGB internally (GPU-accelerated)
3. CPU path wraps `CVPixelBuffer` in `Surface8u` (existing code)

**Test**: `samples/CaptureBasic` on macOS.

**Verify**:
- Correct colors
- No performance issues

---

### Checkpoint 13: macOS GL Path with CVOpenGLTextureCache

**Goal**: Zero-copy macOS capture via `CVOpenGLTextureCache`.

**Steps**:

1. Create `CaptureGl` for macOS
2. Initialize texture cache:
   ```objc
   CVOpenGLTextureCacheRef mTextureCache;
   CVOpenGLTextureCacheCreate(
       kCFAllocatorDefault, nullptr,
       CGLGetCurrentContext(),
       CGLGetPixelFormat(CGLGetCurrentContext()),
       nullptr, &mTextureCache);
   ```
3. In frame callback:
   ```objc
   CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
   CVBufferRetain(pixelBuffer);

   // Push to ring buffer
   FrameEnvelope envelope;
   envelope.platformResource = pixelBuffer;
   mFrameRing.try_enqueue(std::move(envelope));
   ```
4. On GL thread in `checkNewFrame()`:
   ```objc
   FrameEnvelope envelope;
   if (!mFrameRing.try_dequeue(envelope)) return false;

   CVPixelBufferRef pixelBuffer = envelope.platformResource;

   CVOpenGLTextureRef cvTexture;
   CVOpenGLTextureCacheCreateTextureFromImage(
       kCFAllocatorDefault, mTextureCache, pixelBuffer, nullptr, &cvTexture);

   GLenum target = CVOpenGLTextureGetTarget(cvTexture);
   GLuint texId = CVOpenGLTextureGetName(cvTexture);

   envelope.glTexture = gl::Texture::create(target, texId, width, height);
   envelope.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

   mPendingFrames.push_back(std::move(envelope));
   mCurrentTexture = envelope.glTexture;
   return true;
   ```
5. Implement fence-based resource retirement (same pattern as Windows)

**Test**: `samples/CaptureGl` on macOS.

**Verify**:
- Zero-copy (Instruments.app confirms)
- No corruption
- Memory stable

---

### Checkpoint 14: macOS Movie Support

**Goal**: AVFoundation-based movie playback for macOS.

**Steps**:

1. Implement `MovieImplAvFoundation` using `AVPlayer`
2. CPU path uses `AVPlayerItemVideoOutput` → `CVPixelBuffer` → `Surface8u`
3. GL path uses `CVOpenGLTextureCache` (same as capture)
4. Handle seeking, playback rate control

**Test**: `samples/MovieBasic` and `samples/MovieGl` on macOS.

**Verify**:
- MP4/MOV playback works
- GL path is zero-copy
- Seek/scrub responsive

---

### Checkpoint 15: Linux GStreamer (CPU Path)

**Goal**: Update existing GStreamer implementation, CPU path first.

**Steps**:

1. Refactor existing `CaptureImplGStreamer`
2. Build pipeline with `videoconvert`:
   ```cpp
   std::string pipeline =
       "v4l2src ! "
       "videoconvert ! "
       "video/x-raw,format=RGBA ! "
       "appsink";
   ```
3. `appsink` callback copies to `Surface8u`

**Test**: `samples/CaptureBasic` on Linux.

**Verify**:
- Camera enumeration works
- Correct image output

---

### Checkpoint 16: Linux GL Path with glupload

**Goal**: Zero-copy Linux capture via GStreamer `glupload`.

**Steps**:

1. Create `CaptureGl` for Linux
2. Build pipeline:
   ```cpp
   std::string glPipeline =
       "v4l2src ! "
       "videoconvert ! "
       "video/x-raw,format=RGBA ! "
       "glupload ! "
       "appsink";
   ```
3. Configure `appsink` for GL memory:
   ```cpp
   GstCaps* caps = gst_caps_new_simple("video/x-raw",
       "format", G_TYPE_STRING, "RGBA",
       nullptr);
   GstCaps* glCaps = gst_caps_new_empty_simple("video/x-raw(memory:GLMemory)");
   gst_caps_append(caps, glCaps);
   g_object_set(sink, "caps", caps, nullptr);
   ```
4. In `new-sample` callback:
   ```cpp
   GstSample* sample = gst_app_sink_pull_sample(sink);
   gst_sample_ref(sample);

   FrameEnvelope envelope;
   envelope.platformResource = sample;
   mFrameRing.try_enqueue(std::move(envelope));
   ```
5. On GL thread:
   ```cpp
   FrameEnvelope envelope;
   if (!mFrameRing.try_dequeue(envelope)) return false;

   GstSample* sample = envelope.platformResource;
   GstBuffer* buffer = gst_sample_get_buffer(sample);
   GstMemory* memory = gst_buffer_peek_memory(buffer, 0);

   if (gst_is_gl_memory(memory)) {
       GstGLMemory* glMemory = (GstGLMemory*)memory;
       GLuint texId = gst_gl_memory_get_texture_id(glMemory);

       envelope.glTexture = gl::Texture::create(GL_TEXTURE_2D, texId, width, height);
       envelope.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

       mPendingFrames.push_back(std::move(envelope));
       mCurrentTexture = envelope.glTexture;
       return true;
   }
   ```

**Test**: `samples/CaptureGl` on Linux.

**Verify**:
- Zero-copy (check with GPU profiler)
- Works on Intel/AMD/NVIDIA
- No GL errors

---

### Checkpoint 17: Linux Movie Support

**Goal**: GStreamer-based movie playback for Linux.

**Steps**:

1. Update existing `MovieImplGStreamer` (if exists) or create new
2. Use `playbin` or manual pipeline with `filesrc`
3. CPU path: `appsink` → system memory
4. GL path: `glupload` → GL memory (same as capture)

**Test**: `samples/MovieBasic` and `samples/MovieGl` on Linux.

**Verify**:
- MP4/MOV playback works
- GL path is zero-copy
- Seek works

---

### Checkpoint 18: DirectShow Software Fallback

**Goal**: Keep existing DirectShow as software fallback for old cameras.

**Steps**:

1. Verify existing `CaptureImplDirectShow` still works
2. Add build option to prefer DirectShow over Media Foundation
3. Ensure graceful fallback if Media Foundation init fails

**Test**: `samples/CaptureBasic` with DirectShow backend.

**Verify**:
- Older cameras still work
- No regression from existing behavior

---

### Checkpoint 19: Spout2 Integration (Windows Only)

**Goal**: Add Spout2 sender/receiver support using `D3dBackplane`.

**Steps**:

1. Integrate Spout SDK (already in repo)
2. Add to `D3dBackplane`:
   ```cpp
   bool sendSpout(const std::string& name, ID3D11Texture2D* texture);
   bool recvSpout(const std::string& name, ID3D11Texture2D* outTexture);
   ```
3. Spout uses shared D3D textures - perfect fit for our architecture
4. Create `ci::gl::SpoutSender` and `ci::gl::SpoutReceiver` classes
5. Example usage:
   ```cpp
   // Sender (share camera)
   auto capture = gl::CaptureGl::create(...);
   auto spout = gl::SpoutSender::create("MyCam");

   void update() {
       if (capture->checkNewFrame()) {
           spout->sendTexture(capture->getTexture());
       }
   }

   // Receiver
   auto spout = gl::SpoutReceiver::create("MyCam");

   void update() {
       if (spout->checkNewFrame()) {
           mTexture = spout->getTexture();
       }
   }
   ```

**Test**: Sender/receiver sample apps.

**Verify**:
- Low latency texture sharing
- Multiple senders/receivers work
- Clean shutdown (no leaks)

---

### Checkpoint 20: Latency and Dropped Frame Telemetry

**Goal**: Expose performance metrics for debugging.

**Steps**:

1. Add to `CaptureGl` / `MovieGl`:
   ```cpp
   struct Stats {
       uint64_t framesReceived;
       uint64_t framesDropped;
       uint64_t framesRetired;
       float avgLatencyMs;      // Time from capture to GL consumption
       float avgConversionMs;   // YUV→RGB GPU time
       int queueDepth;          // Current ring buffer size
   };

   Stats getStats() const;
   ```
2. Track timestamps in `FrameEnvelope`:
   ```cpp
   std::chrono::steady_clock::time_point captureTime;
   std::chrono::steady_clock::time_point consumeTime;
   std::chrono::steady_clock::time_point conversionStartTime;
   std::chrono::steady_clock::time_point conversionEndTime;
   ```
3. Compute metrics in `checkNewFrame()` and `retirePendingFrames()`

**Test**: Display stats overlay in samples.

**Verify**:
- Metrics accurate (compare with external tools)
- Helps diagnose performance issues

---

### Checkpoint 21: Options and Configuration

**Goal**: Expose user control over backend selection and buffering.

**Steps**:

1. Add configuration:
   ```cpp
   class Capture::Options {
   public:
       enum class Backend {
           Auto,              // Platform default
           MediaFoundation,   // Windows only
           DirectShow,        // Windows only
           AVFoundation,      // macOS only
           GStreamer          // Linux only
       };

       Options& backend(Backend b);
       Options& preferHardwareAccel(bool enable);
       Options& queueDepth(int depth);  // Ring buffer size
       Options& dropPolicy(DropPolicy policy);  // FIFO, LIFO, KeepLatest
   };

   CaptureRef create(int width, int height, const DeviceRef& device, const Options& opts);
   ```
2. Implement backend selection in `Capture.cpp`
3. Respect hardware accel preference (fall back to CPU if disabled)
4. Implement drop policies in ring buffer

**Test**: Verify options work as expected.

**Verify**:
- Backend selection works
- Queue depth affects latency
- Drop policy affects behavior under load

---

### Checkpoint 22: Documentation and Samples

**Goal**: Comprehensive guides and examples.

**Steps**:

1. Update `docs/book/video-capture.md`:
   - Migration guide from old API
   - CPU vs GL path explanation
   - Performance tips
   - Platform-specific notes
2. Create samples:
   - `samples/CaptureBasic` - Simple CPU path
   - `samples/CaptureGl` - Zero-copy GL path
   - `samples/CaptureMulti` - Multiple cameras
   - `samples/MovieBasic` - File playback
   - `samples/MovieGl` - Zero-copy playback
   - `samples/SpoutDemo` - Spout sender/receiver
3. Add troubleshooting guide:
   - WGL interop issues (driver version, multi-GPU)
   - Media Foundation errors
   - GStreamer pipeline debugging
   - Performance profiling tips

**Test**: Run all samples on all platforms.

**Verify**:
- Documentation clear and accurate
- Samples compile and run
- Common issues addressed

---

## Testing Strategy

### Unit Tests

Create tests in `test/unit/video/`:
- `CaptureTest.cpp` - Device enumeration, format queries
- `FrameEnvelopeTest.cpp` - Resource lifetime, fence management
- `RingBufferTest.cpp` - Lock-free queue correctness
- `D3dBackplaneTest.cpp` - Device creation, YUV→RGB conversion
- `GlInteropTest.cpp` - WGL texture wrapping

### Integration Tests

Create tests in `test/integration/`:
- `CaptureIntegrationTest.cpp` - End-to-end capture on real hardware
- `MovieIntegrationTest.cpp` - Playback with various formats
- `ThreadSafetyTest.cpp` - Stress test with Thread Sanitizer
- `MemoryLeakTest.cpp` - Long-running capture with leak detection

### Platform-Specific Testing

**Windows**:
- Test on Intel, NVIDIA, AMD GPUs
- Test on Optimus/hybrid laptops (verify LUID matching)
- Test with USB webcams, built-in cameras, capture cards
- Test with Media Foundation and DirectShow backends

**macOS**:
- Test on Intel Macs, Apple Silicon Macs
- Test with built-in FaceTime camera, USB webcams
- Test on different macOS versions (Big Sur, Monterey, Ventura)

**Linux**:
- Test on X11 and Wayland
- Test with V4L2 cameras
- Test on Intel (VA-API), NVIDIA, AMD GPUs
- Test various GStreamer versions

### Stress Tests

- **Long-running capture**: 24 hours continuous, check memory/CPU stability
- **Mode switching**: Rapidly change resolutions, verify no leaks
- **Multi-camera**: 4+ cameras simultaneously
- **High framerate**: 1080p60, 4K30, verify frame drops acceptable
- **Seek stress**: Rapid scrubbing in movie playback

---

## Rollout Strategy

### Phase 1: Windows Foundation (Checkpoints 1-11)

Focus on getting Media Foundation working end-to-end on Windows before touching other platforms. This validates the architecture.

**Goal**: Fully working Windows implementation with CPU and GL paths, YUV conversion, Movie support.

**Deliverables**:
- `CaptureImplMediaFoundation` (CPU and GL)
- `MovieImplMediaFoundation` (CPU and GL)
- `D3dBackplane` with YUV→RGB shaders
- WGL interop working
- All Windows samples functional

### Phase 2: macOS Port (Checkpoints 12-14)

With Windows working, port architecture to macOS. Most patterns translate directly.

**Goal**: Feature parity on macOS.

**Deliverables**:
- `CaptureGl` for macOS
- `MovieGl` for macOS
- CVOpenGLTextureCache integration
- All macOS samples functional

### Phase 3: Linux Port (Checkpoints 15-17)

Complete cross-platform support.

**Goal**: Feature parity on Linux.

**Deliverables**:
- `CaptureGl` for Linux
- `MovieGl` for Linux
- GStreamer glupload integration
- All Linux samples functional

### Phase 4: Polish (Checkpoints 18-22)

Final features, optimization, documentation.

**Goal**: Production-ready release.

**Deliverables**:
- Spout2 integration
- Telemetry and stats
- Configuration options
- Comprehensive documentation
- All tests passing

---

## Success Criteria

**Performance**:
- Zero-copy GL path CPU usage <5% for 1080p30
- YUV→RGB conversion <1ms GPU time
- Latency <2 frames (capture to display)

**Reliability**:
- No memory leaks over 24-hour capture
- No crashes under stress (rapid mode switching, seek)
- Graceful degradation when hardware accel unavailable

**Compatibility**:
- Works on 95%+ of systems (recent drivers)
- Falls back to DirectShow on older Windows
- Works on hybrid GPU laptops (Optimus)

**Code Quality**:
- All unit tests passing
- No thread safety issues (Thread Sanitizer clean)
- Code reviewed and documented

**User Experience**:
- Simple API - existing code mostly works unchanged
- Clear migration guide
- Good error messages
- Comprehensive samples

---

## Open Questions

These should be resolved during implementation:

1. **WGL interop texture formats**: Does WGL interop support all DXGI formats we need? Test RGBA8, RGBA16F, etc.

2. **CVOpenGLTextureCache texture target**: Does it always use `GL_TEXTURE_RECTANGLE_ARB` or sometimes `GL_TEXTURE_2D`?

3. **GStreamer GL context sharing**: What's the exact mechanism for sharing GStreamer's GL context with Cinder's?

4. **Multi-GPU explicit control**: Should we expose adapter selection to users, or always auto-select?

5. **HDR support**: Do we need to handle HDR metadata (PQ, HLG) or just clamp to SDR?

6. **10-bit/12-bit formats**: How do we expose higher bit depths to users? Promote to 16-bit textures?

7. **Color space conversion on macOS**: When requesting BGRA from AVFoundation, does it handle BT.709→sRGB, or do we need to?

8. **GStreamer videoconvert performance**: Is it GPU-accelerated on all platforms, or CPU on some?

---

## Notes for Implementation

### For Human Developers

- Each checkpoint is independently testable - don't move forward until current one works
- Profile early and often - GPU profilers will catch unexpected copies
- Use validation layers (D3D debug layer, GL debug context) during development
- Test on low-end hardware too, not just high-end
- Document any platform quirks encountered

### For LLM Coders

- Read checkpoint description carefully before writing code
- Review existing code patterns in `blocks/AX-VideoCapture` and `blocks/AX-MediaPlayer`
- Follow Cinder coding style (check existing files)
- Add comments explaining non-obvious logic (especially thread sync, resource lifetime)
- Include error handling (check HRESULTs, verify GL operations)
- When testing fails, add diagnostic logging before asking for help

### Critical Concepts

**Thread Safety**: Platform callbacks are NEVER on the GL thread. Always use lock-free queue or mutex.

**Resource Lifetime**: Hold strong references (ComPtr, CVBufferRetain, gst_sample_ref) until GPU fence signals.

**GL Context**: WGL interop, CVOpenGLTextureCache, and GStreamer GL operations require active GL context.

**LUID Matching**: On Windows, D3D device MUST match GL adapter LUID, or interop fails silently on hybrid GPUs.

**YUV Formats**: NV12 is two-plane (Y, UV interleaved). I420 is three-plane (Y, U, V separate). Don't confuse them.

**Colorspace**: BT.601 for SD, BT.709 for HD. Wrong matrix makes skin tones look green/purple.

**Fences**: `glFenceSync` returns immediately. Must poll with `glClientWaitSync` or `glGetSynciv` to check status.
