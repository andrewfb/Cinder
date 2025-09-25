/*
 Copyright (c) 2024
 All rights reserved.

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

#include "cinder/Cinder.h"

#if defined( CINDER_LINUX )
	#include "cinder/Capture.h"
	#include "cinder/Surface.h"

	#include <gst/gst.h>
	#include <gst/app/gstappsink.h>
	#include <gst/video/video.h>

	#include <atomic>
	#include <memory>
	#include <mutex>
	#include <thread>
	#include <vector>

namespace cinder {

/**
 * @brief GStreamer-based video capture implementation for Linux
 *
 * Automatically constructs optimal GStreamer pipelines based on camera capabilities.
 * Cameras provide either uncompressed data (YUV/RGB) or compressed data (JPEG/H.264/HEVC).
 *
 * **Uncompressed path:** v4l2src → videoconvert → appsink
 * **Compressed path:** v4l2src → [decoder] → videoconvert → appsink
 *
 * The decoder element depends on format: jpegdec, avdec_h264, avdec_h265, or decodebin fallback.
 */
class CaptureImplGStreamer {
  public:
	class Device;

	/**
	 * @brief Create a GStreamer-based video capture
	 * @param width Desired capture width (algorithm will find best match)
	 * @param height Desired capture height (algorithm will find best match)
	 * @param device Camera device to use (nullptr for default)
	 *
	 * The constructor analyzes the specified device's capabilities and constructs
	 * the optimal GStreamer pipeline. The actual resolution may differ from the
	 * requested size based on camera capabilities and the selection algorithm.
	 *
	 * @throws CaptureExcInitFail if GStreamer initialization fails
	 * @throws CaptureExcInitFail if device path cannot be determined
	 * @throws CaptureExcInitFail if pipeline construction fails
	 */
	CaptureImplGStreamer( int32_t width, int32_t height, const Capture::DeviceRef device );
	~CaptureImplGStreamer();

	/**
	 * @brief Start video capture
	 *
	 * Transitions the GStreamer pipeline to PLAYING state and begins
	 * background processing. Video frames will be available via checkNewFrame()
	 * and getSurface() after this call succeeds.
	 *
	 * @throws CaptureExcInitFail if pipeline fails to start
	 */
	void start();

	/**
	 * @brief Stop video capture
	 *
	 * Transitions the pipeline to NULL state and stops all processing.
	 * Safe to call multiple times.
	 */
	void stop();

	/**
	 * @brief Check if capture is currently active
	 * @return true if pipeline is in PLAYING state
	 */
	bool isCapturing();

	/**
	 * @brief Check for new video frame availability
	 * @return true if a new frame is available since last call
	 *
	 * This method is thread-safe and resets the new frame flag when called.
	 * Use this to avoid processing the same frame multiple times.
	 */
	bool checkNewFrame() const;

	/**
	 * @brief Get current capture dimensions
	 * @return Actual capture width (may differ from requested)
	 *
	 * The actual width is determined by the camera's capabilities and
	 * the resolution selection algorithm.
	 */
	int32_t getWidth() const { return mWidth; }

	/**
	 * @brief Get current capture dimensions
	 * @return Actual capture height (may differ from requested)
	 */
	int32_t getHeight() const { return mHeight; }

	/**
	 * @brief Get the most recent video frame as RGB surface
	 * @return Surface containing RGB pixel data, or nullptr if no frame available
	 *
	 * The returned surface is cached and reused for performance. The surface
	 * format is always RGB regardless of the camera's native format.
	 */
	Surface8uRef getSurface() const;

	/**
	 * @brief Get the device associated with this capture
	 * @return Device reference used for this capture
	 */
	const Capture::DeviceRef getDevice() const { return mDevice; }

	/**
	 * @brief Enumerate all available video capture devices
	 * @param forceRefresh Force re-enumeration of devices
	 * @return Vector of available capture devices
	 *
	 * This method caches results for performance. Set forceRefresh=true
	 * to detect newly connected or disconnected devices.
	 */
	static const std::vector<Capture::DeviceRef>& getDevices( bool forceRefresh = false );

	/**
	 * @brief Device wrapper for GStreamer video devices
	 *
	 * Represents a physical video capture device with GStreamer integration.
	 * Each device maintains its own GstDevice handle and capability information.
	 */
	class Device : public Capture::Device {
	  public:
		Device( GstDevice* device, const std::string& name, const std::string& uniqueId );
		~Device() override;

		/**
		 * @brief Check if device can be opened for capture
		 * @return true if device is available for use
		 *
		 * Tests device availability by attempting to create a GStreamer element.
		 * This is more reliable than checking file system presence.
		 */
		bool				checkAvailable() const override;

		/**
		 * @brief Check if device is physically connected
		 * @return true if device node exists in file system
		 *
		 * @note File existence doesn't guarantee the device is functional
		 */
		bool				isConnected() const override;

		/**
		 * @brief Get unique device identifier
		 * @return Device path (e.g., "/dev/video0") or other unique identifier
		 */
		Capture::DeviceIdentifier getUniqueId() const override { return mUniqueId; }

		/**
		 * @brief Get underlying GStreamer device handle
		 * @return GstDevice pointer for advanced usage
		 */
		GstDevice* getGstDevice() const { return mDevice; }

	  private:
		std::string mUniqueId;
		GstDevice*	mDevice;
	};

  private:
	class SurfaceCache;

	/**
	 * @brief Initialize GStreamer pipeline based on device capabilities
	 * @param width Target width for pipeline
	 * @param height Target height for pipeline
	 * @return true if pipeline was created successfully
	 *
	 * This is where the magic happens - the method:
	 * 1. Analyzes device capabilities using analyzeDeviceFormat()
	 * 2. Selects optimal pipeline type based on format and efficiency
	 * 3. Creates appropriate GStreamer elements
	 * 4. Links elements into working pipeline
	 * 5. Sets up callbacks for frame delivery
	 */
	bool initializePipeline( int32_t width, int32_t height );

	/**
	 * @brief Clean up GStreamer pipeline resources
	 *
	 * Safely destroys all GStreamer objects and resets state.
	 * Called from destructor and when pipeline recreation is needed.
	 */
	void cleanupPipeline();

	/**
	 * @brief Start background bus monitoring thread
	 *
	 * GStreamer uses a "bus" system for error reporting and state changes.
	 * This method starts a background thread that monitors for pipeline
	 * errors and handles them appropriately.
	 */
	void startBusWatch();

	/**
	 * @brief Stop background bus monitoring thread
	 */
	void stopBusWatch();

	/**
	 * @brief GStreamer callback for new frame arrival
	 * @param sink AppSink element that received the frame
	 * @param userData Pointer to CaptureImplGStreamer instance
	 * @return GStreamer flow status
	 *
	 * This static callback is called by GStreamer when a new frame is available.
	 * It extracts the frame data and forwards to handleSample().
	 */
	static GstFlowReturn onNewSample( GstAppSink* sink, gpointer userData );

	/**
	 * @brief GStreamer callback for dynamic pad connections (decodebin only)
	 * @param decoder Decodebin element
	 * @param pad Newly created output pad
	 * @param userData Pointer to videoconvert element
	 *
	 * Decodebin creates output pads dynamically after analyzing the input stream.
	 * This callback connects those pads to the rest of the pipeline.
	 * Only used for DECODEBIN_FALLBACK pipeline type.
	 */
	static void			onDecoderPadAdded( GstElement* decoder, GstPad* pad, gpointer userData );

	/**
	 * @brief Process incoming video frame
	 * @param sample GStreamer sample containing frame data
	 * @return GStreamer flow status
	 *
	 * Converts GStreamer video frame to Cinder Surface format:
	 * 1. Extracts video metadata (width, height, stride)
	 * 2. Maps frame memory for reading
	 * 3. Copies pixel data to cached surface
	 * 4. Signals new frame availability
	 */
	GstFlowReturn		handleSample( GstSample* sample );

	/**
	 * @brief Ensure GStreamer is initialized
	 *
	 * Thread-safe initialization of GStreamer library.
	 * Called once per process, subsequent calls are no-ops.
	 */
	static void			ensureGStreamerInitialized();


	// Thread synchronization
	mutable std::mutex			  mMutex;
	std::unique_ptr<SurfaceCache> mSurfaceCache;
	mutable Surface8uRef		  mCurrentFrame;
	mutable bool				  mHasNewFrame;

	// GStreamer pipeline elements
	GstElement* mPipeline;      ///< Main pipeline container
	GstElement* mSource;        ///< v4l2src element
	GstElement* mVideoConvert;  ///< videoconvert element
	GstElement* mCapsFilter;    ///< Output format filter
	GstElement* mAppSink;       ///< Application sink for frame delivery
	GstBus*		mBus;           ///< Pipeline message bus

	// Background processing
	std::thread		  mBusWatchThread;  ///< Bus monitoring thread
	std::atomic<bool> mRunBusWatch;     ///< Bus monitoring control flag

	// Resolution tracking
	int32_t				 mRequestedWidth;   ///< Originally requested width
	int32_t				 mRequestedHeight;  ///< Originally requested height
	int32_t				 mBestWidth;        ///< Best available width from device
	int32_t				 mBestHeight;       ///< Best available height from device
	std::atomic<int32_t> mWidth;            ///< Current actual width
	std::atomic<int32_t> mHeight;           ///< Current actual height
	std::atomic<bool>	 mIsCapturing;      ///< Capture state flag

	Capture::DeviceRef mDevice;  ///< Associated device
};

} // namespace cinder

#endif // defined( CINDER_LINUX )