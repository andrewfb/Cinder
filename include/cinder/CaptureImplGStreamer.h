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

class CaptureImplGStreamer {
  public:
	class Device;

	CaptureImplGStreamer( int32_t width, int32_t height, const Capture::DeviceRef device );
	~CaptureImplGStreamer();

	void start();
	void stop();

	bool isCapturing();
	bool checkNewFrame() const;

	int32_t getWidth() const { return mWidth; }
	int32_t getHeight() const { return mHeight; }

	Surface8uRef getSurface() const;

	const Capture::DeviceRef getDevice() const { return mDevice; }

	static const std::vector<Capture::DeviceRef>& getDevices( bool forceRefresh = false );

	class Device : public Capture::Device {
	  public:
		Device( GstDevice* device, const std::string& name, const std::string& uniqueId );
		~Device() override;

		bool					  checkAvailable() const override;
		bool					  isConnected() const override;
		Capture::DeviceIdentifier getUniqueId() const override { return mUniqueId; }

		GstDevice* getGstDevice() const { return mDevice; }

	  private:
		std::string mUniqueId;
		GstDevice*	mDevice;
	};

  private:
	class SurfaceCache;

	bool initializePipeline( int32_t width, int32_t height );
	void cleanupPipeline();
	void startBusWatch();
	void stopBusWatch();

	static GstFlowReturn onNewSample( GstAppSink* sink, gpointer userData );
	static void			 onDecoderPadAdded( GstElement* decoder, GstPad* pad, gpointer userData );
	GstFlowReturn		 handleSample( GstSample* sample );
	static void			 ensureGStreamerInitialized();

	ivec2 findBestResolution( GstDevice* device, int32_t targetWidth, int32_t targetHeight );

	mutable std::mutex			  mMutex;
	std::unique_ptr<SurfaceCache> mSurfaceCache;
	mutable Surface8uRef		  mCurrentFrame;
	mutable bool				  mHasNewFrame;

	GstElement* mPipeline;
	GstElement* mSource;
	GstElement* mVideoConvert;
	GstElement* mCapsFilter;
	GstElement* mAppSink;
	GstBus*		mBus;

	std::thread		  mBusWatchThread;
	std::atomic<bool> mRunBusWatch;

	int32_t				 mRequestedWidth;
	int32_t				 mRequestedHeight;
	int32_t				 mBestWidth;
	int32_t				 mBestHeight;
	std::atomic<int32_t> mWidth;
	std::atomic<int32_t> mHeight;
	std::atomic<bool>	 mIsCapturing;

	Capture::DeviceRef mDevice;
};

} // namespace cinder

#endif // defined( CINDER_LINUX )
