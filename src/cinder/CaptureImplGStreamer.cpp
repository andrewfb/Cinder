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

#include "cinder/CaptureImplGStreamer.h"

#if defined( CINDER_LINUX )

#include "cinder/Capture.h"
#include "cinder/Log.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iterator>

using namespace std;

namespace cinder {

namespace {

static const SurfaceChannelOrder kCaptureChannelOrder = SurfaceChannelOrder::RGB;

static string deriveUniqueId( GstDevice *device )
{
	string identifier;
	if( ! device )
		return identifier;

	GstStructure *props = gst_device_get_properties( device );
	if( props ) {
		const gchar *path = gst_structure_get_string( props, "device.path" );
		if( path )
			identifier = path;
		else {
			const gchar *node = gst_structure_get_string( props, "device.node" );
			if( node )
				identifier = node;
		}
		gst_structure_free( props );
	}

	if( identifier.empty() ) {
		identifier = gst_device_get_display_name( device );
	}

	return identifier;
}

static bool pathExists( const string &path )
{
	if( path.empty() )
		return true;

	std::error_code ec;
	return std::filesystem::exists( path, ec );
}

} // namespace

// CaptureImplGStreamer::SurfaceCache

class CaptureImplGStreamer::SurfaceCache {
  public:
	SurfaceCache( int32_t width, int32_t height, SurfaceChannelOrder sco, int numSurfaces )
		: mWidth( width ), mHeight( height ), mSCO( sco )
	{
		mSurfaces.resize( numSurfaces );
		allocateSurfaces();
	}

	Surface8uRef getNewSurface()
	{
		auto it = find_if( mSurfaces.begin(), mSurfaces.end(), []( const Surface8uRef &s ) { return s && s.use_count() == 1; } );
		if( it == mSurfaces.end() ) {
			mSurfaces.push_back( Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO ) );
			it = prev( mSurfaces.end() );
		}
		return *it;
	}

	void resize( int32_t width, int32_t height )
	{
		if( ( width == mWidth ) && ( height == mHeight ) )
			return;
		mWidth = width;
		mHeight = height;
		allocateSurfaces();
	}

  private:
	void allocateSurfaces()
	{
		for( auto &surface : mSurfaces ) {
			surface = Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO );
		}
	}

	vector<Surface8uRef> mSurfaces;
	int32_t			mWidth = 0;
	int32_t			mHeight = 0;
	SurfaceChannelOrder	mSCO;
};

// static
void CaptureImplGStreamer::ensureGStreamerInitialized()
{
	static std::once_flag flag;
	std::call_once( flag, []() {
		if( ! gst_is_initialized() ) {
			GError *err = nullptr;
			if( ! gst_init_check( nullptr, nullptr, &err ) ) {
				std::string message = "GStreamer initialization failed";
				if( err && err->message )
					message += ": " + std::string( err->message );
				if( err )
					g_error_free( err );
				CI_LOG_E( message );
			}
		}
	} );
}

CaptureImplGStreamer::Device::Device( GstDevice *device, const std::string &name, const std::string &uniqueId )
	: mUniqueId( uniqueId ), mDevice( device )
{
	mName = name;
	if( mDevice )
		gst_object_ref( mDevice );
}

CaptureImplGStreamer::Device::~Device()
{
	if( mDevice ) {
		gst_object_unref( mDevice );
		mDevice = nullptr;
	}
}

bool CaptureImplGStreamer::Device::checkAvailable() const
{
	if( ! mDevice )
		return false;

	GstElement *probe = gst_device_create_element( mDevice, nullptr );
	if( ! probe )
		return false;
	gst_object_unref( probe );
	return true;
}

bool CaptureImplGStreamer::Device::isConnected() const
{
	if( ! mDevice )
		return false;

	string path;
	GstStructure *props = gst_device_get_properties( mDevice );
	if( props ) {
		const gchar *devicePath = gst_structure_get_string( props, "device.path" );
		if( devicePath )
			path = devicePath;
		gst_structure_free( props );
	}

	if( path.empty() )
		return true;

	return pathExists( path );
}

CaptureImplGStreamer::CaptureImplGStreamer( int32_t width, int32_t height, const Capture::DeviceRef device )
	: mSurfaceCache(), mCurrentFrame(), mHasNewFrame( false ), mPipeline( nullptr ), mSource( nullptr ), mVideoConvert( nullptr ), mCapsFilter( nullptr ), mAppSink( nullptr ), mBus( nullptr ), mRunBusWatch( false ), mRequestedWidth( width ), mRequestedHeight( height ), mBestWidth( width ), mBestHeight( height ), mNativeWidth( width ), mNativeHeight( height ), mIsStereoCrop( false ), mWidth( width ), mHeight( height ), mIsCapturing( false ), mDevice( device )
{
	ensureGStreamerInitialized();

	// Find the best matching resolution for the requested size
	Capture::DeviceRef selectedDevice = device;
	if( ! selectedDevice ) {
		const auto &devices = getDevices();
		if( ! devices.empty() )
			selectedDevice = devices.front();
	}

	if( selectedDevice ) {
		auto gstDevice = std::dynamic_pointer_cast<CaptureImplGStreamer::Device>( selectedDevice );
		if( gstDevice && gstDevice->getGstDevice() ) {
			ivec2 bestRes = findBestResolution( gstDevice->getGstDevice(), width, height );
			mBestWidth = bestRes.x;
			mBestHeight = bestRes.y;
		}
	}

	if( ! initializePipeline( mBestWidth, mBestHeight ) )
		throw CaptureExcInitFail();
}

CaptureImplGStreamer::~CaptureImplGStreamer()
{
	stop();
	cleanupPipeline();
}

void CaptureImplGStreamer::start()
{
	if( mIsCapturing )
		return;


	if( ! mPipeline && ! initializePipeline( mRequestedWidth, mRequestedHeight ) )
		throw CaptureExcInitFail();

	GstStateChangeReturn result = gst_element_set_state( mPipeline, GST_STATE_PLAYING );
	if( result == GST_STATE_CHANGE_FAILURE ) {
		CI_LOG_E( "Failed to set pipeline to PLAYING state" );

		// Get error details
		GstBus *bus = gst_element_get_bus( mPipeline );
		GstMessage *msg = gst_bus_pop_filtered( bus, GST_MESSAGE_ERROR );
		if( msg ) {
			GError *err = nullptr;
			gchar *dbg = nullptr;
			gst_message_parse_error( msg, &err, &dbg );
			if( err ) {
				CI_LOG_E( "Pipeline error: " << err->message );
				g_error_free( err );
			}
			if( dbg ) {
				CI_LOG_E( "Debug info: " << dbg );
				g_free( dbg );
			}
			gst_message_unref( msg );
		}
		gst_object_unref( bus );

		throw CaptureExcInitFail();
	}

	mIsCapturing = true;
	{
		lock_guard<mutex> lock( mMutex );
		mHasNewFrame = false;
	}
	startBusWatch();
}

void CaptureImplGStreamer::stop()
{
	if( ! mIsCapturing )
		return;

	if( mPipeline )
		gst_element_set_state( mPipeline, GST_STATE_NULL );

	mIsCapturing = false;
	stopBusWatch();
}

bool CaptureImplGStreamer::isCapturing()
{
	return mIsCapturing;
}

bool CaptureImplGStreamer::checkNewFrame() const
{
	lock_guard<mutex> lock( mMutex );
	bool hasNewFrame = mHasNewFrame;
	mHasNewFrame = false;
	return hasNewFrame;
}

Surface8uRef CaptureImplGStreamer::getSurface() const
{
	lock_guard<mutex> lock( mMutex );
	return mCurrentFrame;
}

bool CaptureImplGStreamer::initializePipeline( int32_t width, int32_t height )
{
	if( mPipeline )
		return true;


	// For now, always use v4l2src directly to avoid pipewire issues
	GstElement *source = gst_element_factory_make( "v4l2src", "camera-source" );
	if( source ) {
		// Try to set the correct device path using the selected device
		if( mDevice ) {
			auto gstDevice = std::dynamic_pointer_cast<CaptureImplGStreamer::Device>( mDevice );
			if( gstDevice && gstDevice->getGstDevice() ) {
				GstStructure *props = gst_device_get_properties( gstDevice->getGstDevice() );
				if( props ) {
					// Try different property names for device path
					const gchar *device_path = gst_structure_get_string( props, "api.v4l2.path" );
					if( ! device_path ) {
						device_path = gst_structure_get_string( props, "device.path" );
					}
					if( device_path ) {
							g_object_set( source, "device", device_path, nullptr );
					} else {
						// Try default device
						g_object_set( source, "device", "/dev/video0", nullptr );
					}
					gst_structure_free( props );
				}
			} else {
				// Use default device
				g_object_set( source, "device", "/dev/video0", nullptr );
			}
		}
	}

	// Add a caps filter right after source to constrain the input resolution
	GstElement *sourceCapsFilter = gst_element_factory_make( "capsfilter", "source-capsfilter" );
	if( sourceCapsFilter ) {
		// Create caps that match the best resolution we found, but in the camera's native format
		GstCaps *sourceCaps = nullptr;

		// Try to find a format that matches our target resolution
		const auto &devices = getDevices();
		if( ! devices.empty() ) {
			auto selectedDevice = devices.front();
			auto gstDevice = std::dynamic_pointer_cast<CaptureImplGStreamer::Device>( selectedDevice );
			if( gstDevice && gstDevice->getGstDevice() ) {
				GstCaps *deviceCaps = gst_device_get_caps( gstDevice->getGstDevice() );
				if( deviceCaps ) {
					// Look for caps that match our target resolution
					int numStructures = gst_caps_get_size( deviceCaps );
					for( int i = 0; i < numStructures; ++i ) {
						GstStructure *structure = gst_caps_get_structure( deviceCaps, i );
						if( structure ) {
							int capWidth, capHeight;
							if( gst_structure_get_int( structure, "width", &capWidth ) &&
								gst_structure_get_int( structure, "height", &capHeight ) &&
								capWidth == width && capHeight == height ) {

								sourceCaps = gst_caps_new_empty();
								GstStructure *newStructure = gst_structure_copy( structure );
								gst_caps_append_structure( sourceCaps, newStructure );
									break;
							}
						}
					}
					gst_caps_unref( deviceCaps );
				}
			}
		}

		if( sourceCaps ) {
			g_object_set( sourceCapsFilter, "caps", sourceCaps, nullptr );
			gst_caps_unref( sourceCaps );
		} else {
				gst_object_unref( sourceCapsFilter );
			sourceCapsFilter = nullptr;
		}
	}

	if( ! source ) {
		CI_LOG_E( "Failed to create GStreamer source element" );
		return false;
	}

	// Add decoder and processing elements
	GstElement *decoder = gst_element_factory_make( "decodebin", "decoder" );
	GstElement *videoConvert = gst_element_factory_make( "videoconvert", "videoconvert" );
	GstElement *capsFilter = gst_element_factory_make( "capsfilter", "capsfilter" );
	GstElement *appSink = gst_element_factory_make( "appsink", "appsink" );

	if( ! decoder || ! videoConvert || ! capsFilter || ! appSink ) {
		if( source )
			gst_object_unref( source );
		if( decoder )
			gst_object_unref( decoder );
		if( videoConvert )
			gst_object_unref( videoConvert );
		if( capsFilter )
			gst_object_unref( capsFilter );
		if( appSink )
			gst_object_unref( appSink );
		CI_LOG_E( "Failed to create GStreamer elements" );
		return false;
	}

	// Set RGB format but let resolution negotiate automatically
	GstCaps *caps = gst_caps_new_simple( "video/x-raw",
		"format", G_TYPE_STRING, "RGB",
		nullptr );
	g_object_set( capsFilter, "caps", caps, nullptr );
	gst_caps_unref( caps );

	g_object_set( appSink,
		"emit-signals", FALSE,
		"sync", FALSE,
		"max-buffers", 1,
		"drop", TRUE,
		nullptr );

	static GstAppSinkCallbacks sCallbacks = { nullptr, nullptr, &CaptureImplGStreamer::onNewSample };
	gst_app_sink_set_callbacks( GST_APP_SINK( appSink ), &sCallbacks, this, nullptr );

	GstElement *pipeline = gst_pipeline_new( "cinder-capture" );
	if( ! pipeline ) {
		gst_object_unref( source );
		gst_object_unref( videoConvert );
		gst_object_unref( capsFilter );
		gst_object_unref( appSink );
		CI_LOG_E( "Failed to create GStreamer pipeline" );
		return false;
	}

	// Add all elements to pipeline
	if( sourceCapsFilter ) {
		gst_bin_add_many( GST_BIN( pipeline ), source, sourceCapsFilter, decoder, videoConvert, capsFilter, appSink, nullptr );

		// Link source through caps filter to decoder
		if( ! gst_element_link_many( source, sourceCapsFilter, decoder, nullptr ) ) {
			CI_LOG_E( "Failed to link source through caps filter to decoder" );
			gst_object_unref( pipeline );
			mPipeline = nullptr;
			return false;
		}
	} else {
		gst_bin_add_many( GST_BIN( pipeline ), source, decoder, videoConvert, capsFilter, appSink, nullptr );

		// Link source to decoder directly
		if( ! gst_element_link( source, decoder ) ) {
			CI_LOG_E( "Failed to link source to decoder" );
			gst_object_unref( pipeline );
			mPipeline = nullptr;
			return false;
		}
	}

	// Link videoconvert to capsfilter to appsink
	if( ! gst_element_link_many( videoConvert, capsFilter, appSink, nullptr ) ) {
		CI_LOG_E( "Failed to link video processing elements" );
		gst_object_unref( pipeline );
		mPipeline = nullptr;
		return false;
	}

	// Store for later use in decodebin pad-added callback
	mVideoConvert = videoConvert;

	// Connect decodebin's dynamic pad
	g_signal_connect( decoder, "pad-added", G_CALLBACK( onDecoderPadAdded ), videoConvert );

	mPipeline = pipeline;
	mSource = source;
	mCapsFilter = capsFilter;
	mAppSink = appSink;
	mBus = gst_element_get_bus( mPipeline );

	return true;
}

void CaptureImplGStreamer::cleanupPipeline()
{
	stopBusWatch();

	if( mPipeline ) {
		gst_element_set_state( mPipeline, GST_STATE_NULL );
		gst_object_unref( mPipeline );
	}

	if( mBus )
		gst_object_unref( mBus );

	mPipeline = nullptr;
	mSource = nullptr;
	mVideoConvert = nullptr;
	mCapsFilter = nullptr;
	mAppSink = nullptr;
	mBus = nullptr;
}

void CaptureImplGStreamer::startBusWatch()
{
	if( ! mBus || mRunBusWatch )
		return;

	if( mBusWatchThread.joinable() )
		mBusWatchThread.join();

	mRunBusWatch = true;
	mBusWatchThread = std::thread( [this]() {
		while( mRunBusWatch ) {
			GstMessage *message = gst_bus_timed_pop( mBus, GST_MSECOND * 100 );
			if( ! message )
				continue;

			switch( GST_MESSAGE_TYPE( message ) ) {
				case GST_MESSAGE_ERROR: {
					GError *err = nullptr;
					gchar *dbg = nullptr;
					gst_message_parse_error( message, &err, &dbg );
					if( err && err->message )
						CI_LOG_E( "Capture error: " << err->message );
					if( dbg )
						CI_LOG_E( "Debug info: " << dbg );
					if( err )
						g_error_free( err );
					if( dbg )
						g_free( dbg );
					mRunBusWatch = false;
					break;
				}
				case GST_MESSAGE_EOS:
					mRunBusWatch = false;
					break;
				default:
					break;
			}

			gst_message_unref( message );
		}
	} );
}

void CaptureImplGStreamer::stopBusWatch()
{
	mRunBusWatch = false;
	if( mBusWatchThread.joinable() )
		mBusWatchThread.join();
}

// static
void CaptureImplGStreamer::onDecoderPadAdded( GstElement *decoder, GstPad *pad, gpointer userData )
{

	GstElement *videoConvert = static_cast<GstElement*>( userData );
	GstPad *sinkPad = gst_element_get_static_pad( videoConvert, "sink" );

	if( gst_pad_is_linked( sinkPad ) ) {
		gst_object_unref( sinkPad );
		return;
	}

	// Check if this is a video pad
	GstCaps *caps = gst_pad_query_caps( pad, nullptr );
	GstStructure *str = gst_caps_get_structure( caps, 0 );
	const gchar *name = gst_structure_get_name( str );

	if( g_str_has_prefix( name, "video/" ) ) {
		GstPadLinkReturn ret = gst_pad_link( pad, sinkPad );
		if( GST_PAD_LINK_FAILED( ret ) ) {
			CI_LOG_E( "Failed to link decoder to videoconvert" );
		} else {
			}
	}

	gst_caps_unref( caps );
	gst_object_unref( sinkPad );
}

GstFlowReturn CaptureImplGStreamer::onNewSample( GstAppSink *sink, gpointer userData )
{
	auto *impl = static_cast<CaptureImplGStreamer*>( userData );
	return impl->handleSample( gst_app_sink_pull_sample( sink ) );
}

GstFlowReturn CaptureImplGStreamer::handleSample( GstSample *sample )
{
	if( ! sample ) {
		return GST_FLOW_OK;
	}

	GstCaps *caps = gst_sample_get_caps( sample );
	GstBuffer *buffer = gst_sample_get_buffer( sample );
	if( ! caps || ! buffer ) {
		gst_sample_unref( sample );
		return GST_FLOW_OK;
	}

	GstVideoInfo info;
	if( ! gst_video_info_from_caps( &info, caps ) ) {
		gst_sample_unref( sample );
		return GST_FLOW_OK;
	}

	GstMapInfo mapInfo;
	if( ! gst_buffer_map( buffer, &mapInfo, GST_MAP_READ ) ) {
		gst_sample_unref( sample );
		return GST_FLOW_OK;
	}

	int32_t width = GST_VIDEO_INFO_WIDTH( &info );
	int32_t height = GST_VIDEO_INFO_HEIGHT( &info );
	int32_t stride = GST_VIDEO_INFO_PLANE_STRIDE( &info, 0 );

	{
		lock_guard<mutex> lock( mMutex );

		if( ! mSurfaceCache )
			mSurfaceCache = make_unique<SurfaceCache>( width, height, kCaptureChannelOrder, 4 );
		else
			mSurfaceCache->resize( width, height );

		Surface8uRef surface = mSurfaceCache->getNewSurface();
		uint8_t *dst = surface->getData();
		int32_t dstStride = surface->getRowBytes();
		const uint8_t *src = mapInfo.data;

		for( int32_t row = 0; row < height; ++row ) {
			memcpy( dst + row * dstStride, src + row * stride, std::min<int32_t>( dstStride, stride ) );
		}

		mCurrentFrame = surface;
		mHasNewFrame = true;
		mWidth = width;
		mHeight = height;
	}

	gst_buffer_unmap( buffer, &mapInfo );
	gst_sample_unref( sample );

	return GST_FLOW_OK;
}

ivec2 CaptureImplGStreamer::findBestResolution( GstDevice *device, int32_t targetWidth, int32_t targetHeight )
{
	if( ! device ) {
		return ivec2( targetWidth, targetHeight );
	}

	GstCaps *deviceCaps = gst_device_get_caps( device );
	if( ! deviceCaps ) {
		return ivec2( targetWidth, targetHeight );
	}

	struct Resolution {
		int32_t width, height;
		int32_t score;
	};

	std::vector<Resolution> availableResolutions;
	int numStructures = gst_caps_get_size( deviceCaps );

	// Parse all available resolutions from device capabilities
	for( int i = 0; i < numStructures; ++i ) {
		GstStructure *structure = gst_caps_get_structure( deviceCaps, i );
		if( ! structure )
			continue;

		// Check if this structure has width and height
		if( gst_structure_has_field( structure, "width" ) && gst_structure_has_field( structure, "height" ) ) {
			int width, height;
			if( gst_structure_get_int( structure, "width", &width ) &&
				gst_structure_get_int( structure, "height", &height ) ) {

				// Improved scoring algorithm that handles stereo cameras better
				double targetAspect = (double)targetWidth / targetHeight;
				double currentAspect = (double)width / height;
				double aspectDiff = abs(currentAspect - targetAspect);

				// Calculate pixel area ratios to prioritize reasonable scaling
				double targetArea = (double)targetWidth * targetHeight;
				double currentArea = (double)width * height;
				double areaRatio = currentArea / targetArea;
				double areaScore = 0;

				// Prefer areas close to target (0.5x-4x range is reasonable)
				if( areaRatio >= 0.5 && areaRatio <= 4.0 ) {
					// Best score when area is close to 1x
					areaScore = 100000 - std::abs(std::log(areaRatio)) * 20000;
				} else {
					// Penalize very large or very small area ratios
					areaScore = 50000 - std::abs(std::log(areaRatio)) * 30000;
				}

				// Calculate dimension matching scores
				int32_t widthDiff = abs(width - targetWidth);
				int32_t heightDiff = abs(height - targetHeight);
				double dimensionScore = 50000 - (widthDiff + heightDiff) / 4.0;

				int32_t score = 0;

				// Exact resolution match gets highest priority
				if( width == targetWidth && height == targetHeight ) {
					score = 1000000;
				}
				// Good aspect ratio match (within 10% difference) - traditional scoring
				else if( aspectDiff < 0.10 ) {
					if( width >= targetWidth && height >= targetHeight ) {
						score = 900000 - (widthDiff + heightDiff);
					} else if( width <= targetWidth && height <= targetHeight ) {
						score = 800000 + (width + height) / 100;
					} else {
						score = 700000 - abs((int32_t)widthDiff) - abs((int32_t)heightDiff);
					}
				}
				// For very different aspect ratios (stereo cameras), prioritize area and reasonable dimensions
				else {
					score = (int32_t)(areaScore + dimensionScore);
					// Bonus for reasonable dimensions even with bad aspect ratio
					if( width <= targetWidth * 3 && height <= targetHeight * 3 ) {
						score += 20000;
					}
				}

				availableResolutions.push_back({width, height, score});

				// For stereo cameras (aspect ratio > 3:1), also consider single-eye crop
				if( currentAspect > 3.0 ) {
					int32_t singleEyeWidth = width / 2;
					double singleEyeAspect = (double)singleEyeWidth / height;
					double singleEyeAspectDiff = abs(singleEyeAspect - targetAspect);

					// Calculate score for single-eye crop
					int32_t singleEyeScore = 0;
					if( singleEyeAspectDiff < 0.10 ) {
						// Good aspect ratio match for single eye
						singleEyeScore = 950000 - abs(singleEyeWidth - targetWidth) - abs(height - targetHeight);
					} else {
						// Calculate area-based score for single eye
						double singleEyeArea = (double)singleEyeWidth * height;
						double singleEyeAreaRatio = singleEyeArea / targetArea;
						double singleEyeAreaScore = 0;
						if( singleEyeAreaRatio >= 0.5 && singleEyeAreaRatio <= 4.0 ) {
							singleEyeAreaScore = 100000 - std::abs(std::log(singleEyeAreaRatio)) * 20000;
						} else {
							singleEyeAreaScore = 50000 - std::abs(std::log(singleEyeAreaRatio)) * 30000;
						}
						double singleEyeDimensionScore = 50000 - (abs(singleEyeWidth - targetWidth) + abs(height - targetHeight)) / 4.0;
						singleEyeScore = (int32_t)(singleEyeAreaScore + singleEyeDimensionScore) + 30000; // Bonus for stereo crop
					}

					availableResolutions.push_back({singleEyeWidth, height, singleEyeScore});
				}
			}
		}
	}

	gst_caps_unref( deviceCaps );

	// Find the resolution with the highest score
	if( availableResolutions.empty() ) {
		return ivec2( targetWidth, targetHeight );
	}

	auto best = std::max_element( availableResolutions.begin(), availableResolutions.end(),
		[]( const Resolution& a, const Resolution& b ) { return a.score < b.score; } );

	return ivec2( best->width, best->height );
}

// static
const vector<Capture::DeviceRef>& CaptureImplGStreamer::getDevices( bool forceRefresh )
{
	ensureGStreamerInitialized();

	static vector<Capture::DeviceRef> sDevices;
	static mutex sDeviceMutex;
	static bool sEnumerated = false;

	if( forceRefresh ) {
		lock_guard<mutex> lock( sDeviceMutex );
		sDevices.clear();
		sEnumerated = false;
	}

	if( sEnumerated )
		return sDevices;

	lock_guard<mutex> lock( sDeviceMutex );
	if( sEnumerated )
		return sDevices;

	GstDeviceMonitor *monitor = gst_device_monitor_new();
	gst_device_monitor_add_filter( monitor, "Video/Source", nullptr );
	gst_device_monitor_start( monitor );

	GList *devices = gst_device_monitor_get_devices( monitor );
	for( GList *it = devices; it != nullptr; it = it->next ) {
		GstDevice *gstDevice = GST_DEVICE( it->data );
		if( ! gstDevice )
			continue;

		string name = gst_device_get_display_name( gstDevice );
		string uniqueId = deriveUniqueId( gstDevice );
		auto device = make_shared<CaptureImplGStreamer::Device>( gstDevice, name, uniqueId );
		sDevices.push_back( device );
	}

	g_list_free_full( devices, g_object_unref );
	gst_device_monitor_stop( monitor );
	gst_object_unref( monitor );

	sEnumerated = true;
	return sDevices;
}

} // namespace cinder

#endif // defined( CINDER_LINUX )
