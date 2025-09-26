/*
 Copyright (c) 2010, The Barbarian Group
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

#include "cinder/CaptureImplDirectShow.h"
#include "cinder/msw/DirectShowCapture.h"
#include <dshow.h>
#include <dvdmedia.h>  // For VIDEO_STREAM_CONFIG_CAPS
#include <set>
#include <tuple>

// DirectShow GUID constants that may be missing from some headers
#ifndef MEDIASUBTYPE_I420
static const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
#endif

using namespace std;

namespace cinder {

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SurfaceCache

class SurfaceCache {
  public:
	SurfaceCache( int32_t width, int32_t height, SurfaceChannelOrder sco, int numSurfaces )
		: mSurfaces( numSurfaces, nullptr ), mWidth( width ), mHeight( height ), mSCO( sco )
	{
		for( auto& surf : mSurfaces ) {
			surf = Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO );
		}
	}

	Surface8uRef getNewSurface()
	{
		// try to find a surface that isn't used by anyone else
		// TODO: this is racy, but the worst that can happen is that we over-allocate Surfaces in the cache
		auto it = std::find_if( mSurfaces.begin(), mSurfaces.end(), [](const Surface8uRef& s) { return s.use_count() == 1; } );

		// if no free surface is found, create a new one and add it to the cache set
		if( it == mSurfaces.end() ) {
			mSurfaces.push_back( Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO ) );
			it = mSurfaces.end() - 1;
		}

		return *it;
	}

  private:
	std::vector<Surface8uRef>			mSurfaces;
	int32_t								mWidth, mHeight;
	SurfaceChannelOrder					mSCO;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DirectShowCapture

namespace impl {
	static DirectShowCapture& setupDirectShowCapture()
	{
		static DirectShowCapture inst;
		return inst;
	}
}

static DirectShowCapture& getDirectShowCapture()
{
	static DirectShowCapture& instance = impl::setupDirectShowCapture();
	return instance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CaptureImplDirectShow

bool CaptureImplDirectShow::Device::checkAvailable() const
{
	DirectShowCapture tempCapture;
	return ( mUniqueId >=0 ) && ( mUniqueId < (int)CaptureImplDirectShow::getDevices().size() ) && tempCapture.isDeviceConnected( mUniqueId );
}

bool CaptureImplDirectShow::Device::isConnected() const
{
	DirectShowCapture tempCapture;
	return tempCapture.isDeviceConnected( mUniqueId );
}

const vector<Capture::DeviceRef>& CaptureImplDirectShow::getDevices( bool forceRefresh )
{
	static bool firstCall = true;
	static std::vector<Capture::DeviceRef>	devices;

	if( firstCall || forceRefresh ) {
		auto deviceNames = DirectShowCapture::getDeviceNames();
		devices.resize( deviceNames.size() );
		for ( int i = 0; i < (int)deviceNames.size(); ++i ) {
			devices[i] = std::make_shared<CaptureImplDirectShow::Device>( deviceNames[i], i );
		}

		firstCall = false;
	}
	return devices;
}

CaptureImplDirectShow::CaptureImplDirectShow( int32_t width, int32_t height, const Capture::DeviceRef device )
	: mWidth( width ), mHeight( height ), mCurrentFrame( Surface8u::create( width, height, false, SurfaceChannelOrder::BGR ) ), mDeviceID( 0 )
{
	mDevice = device;
	if( mDevice ) {
		mDeviceID = device->getUniqueId();
	}
	mDirectShowCapture = std::make_unique<DirectShowCapture>();
	if( ! mDirectShowCapture->setupDevice( mDeviceID, mWidth, mHeight ) )
		throw CaptureExcInitFail( "Failed to setup DirectShow video input device" );
	if( ! mDirectShowCapture->start() )
		throw CaptureExcInitFail( "Failed to start DirectShow video capture" );
	mWidth = mDirectShowCapture->getWidth();
	mHeight = mDirectShowCapture->getHeight();
	mIsCapturing = true;
	mSurfaceCache.reset( new SurfaceCache( mWidth, mHeight, SurfaceChannelOrder::BGR, 4 ) );
}

static GUID pixelFormatToGuid( Capture::Mode::PixelFormat pixelFormat )
{
	switch( pixelFormat ) {
		case Capture::Mode::PixelFormat::RGB24:
			return MEDIASUBTYPE_RGB24;
		case Capture::Mode::PixelFormat::BGR24:
			return MEDIASUBTYPE_RGB24;
		case Capture::Mode::PixelFormat::ARGB32:
			return MEDIASUBTYPE_RGB32;
		case Capture::Mode::PixelFormat::BGRA32:
			return MEDIASUBTYPE_RGB32;
		case Capture::Mode::PixelFormat::YUV420P:
			return MEDIASUBTYPE_IYUV;
		case Capture::Mode::PixelFormat::NV12:
			return MEDIASUBTYPE_NV12;
		case Capture::Mode::PixelFormat::YUY2:
			return MEDIASUBTYPE_YUY2;
		case Capture::Mode::PixelFormat::UYVY:
			return MEDIASUBTYPE_UYVY;
		case Capture::Mode::PixelFormat::I420:
			return MEDIASUBTYPE_I420;
		case Capture::Mode::PixelFormat::YV12:
			return MEDIASUBTYPE_YV12;
		default:
			return MEDIASUBTYPE_RGB24;
	}
}

CaptureImplDirectShow::CaptureImplDirectShow( const Capture::DeviceRef& device, const Capture::Mode& mode )
	: mWidth( mode.getWidth() ), mHeight( mode.getHeight() ), mCurrentFrame( Surface8u::create( mode.getWidth(), mode.getHeight(), false, SurfaceChannelOrder::BGR ) ), mDeviceID( 0 )
{
	mDevice = device;
	if( mDevice ) {
		mDeviceID = device->getUniqueId();
	}

	mDirectShowCapture = std::make_unique<DirectShowCapture>();
	if( ! mDirectShowCapture->setupDevice( mDeviceID, mode ) )
		throw CaptureExcInitFail( "Failed to setup DirectShow video input device with specified mode" );
	if( ! mDirectShowCapture->start() )
		throw CaptureExcInitFail( "Failed to start DirectShow video capture with specified mode" );

	mWidth = mDirectShowCapture->getWidth();
	mHeight = mDirectShowCapture->getHeight();
	mIsCapturing = true;
	mSurfaceCache.reset( new SurfaceCache( mWidth, mHeight, SurfaceChannelOrder::BGR, 4 ) );
}

CaptureImplDirectShow::~CaptureImplDirectShow()
{
	if( mDirectShowCapture ) {
		mDirectShowCapture->stop();
	}
}

void CaptureImplDirectShow::start()
{
	if( mIsCapturing ) return;

	if( ! mDirectShowCapture || ! mDirectShowCapture->start() )
		throw CaptureExcInitFail( "Failed to start DirectShow video capture" );
	mIsCapturing = true;
}

void CaptureImplDirectShow::stop()
{
	if( ! mIsCapturing ) return;

	if( mDirectShowCapture ) {
		mDirectShowCapture->stop();
	}
	mIsCapturing = false;
}

bool CaptureImplDirectShow::isCapturing()
{
	return mIsCapturing;
}

bool CaptureImplDirectShow::checkNewFrame() const
{
	return mDirectShowCapture ? mDirectShowCapture->isFrameNew() : false;
}

Surface8uRef CaptureImplDirectShow::getSurface() const
{
	if( mDirectShowCapture && mDirectShowCapture->isFrameNew() ) {
		mCurrentFrame = mSurfaceCache->getNewSurface();
		mDirectShowCapture->getPixels( mCurrentFrame->getData(), false, true );
	}

	return mCurrentFrame;
}

std::vector<Capture::Mode> CaptureImplDirectShow::Device::getModes() const
{
	// Use a temporary DirectShowCapture instance just for mode enumeration
	DirectShowCapture tempCapture;
	return tempCapture.getDeviceModes(mUniqueId);
}

} //namespace