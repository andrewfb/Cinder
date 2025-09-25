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
	#include "cinder/Filesystem.h"
	#include <iostream>
	#include <iterator>

using namespace std;

namespace cinder {

namespace {

static const SurfaceChannelOrder kCaptureChannelOrder = SurfaceChannelOrder::RGB;

// Custom deleters as inline functions
inline void gstElementDeleter( GstElement* elem ) { if( elem ) gst_object_unref( GST_OBJECT( elem ) ); }
inline void gstCapsDeleter( GstCaps* caps ) { if( caps ) gst_caps_unref( caps ); }
inline void gstSampleDeleter( GstSample* sample ) { if( sample ) gst_sample_unref( sample ); }
inline void gstMessageDeleter( GstMessage* msg ) { if( msg ) gst_message_unref( msg ); }
inline void gstBusDeleter( GstBus* bus ) { if( bus ) gst_object_unref( GST_OBJECT( bus ) ); }

// Type aliases for smart pointers using function pointer deleters
using GstElementPtr = std::unique_ptr<GstElement, decltype(&gstElementDeleter)>;
using GstCapsPtr = std::unique_ptr<GstCaps, decltype(&gstCapsDeleter)>;
using GstSamplePtr = std::unique_ptr<GstSample, decltype(&gstSampleDeleter)>;
using GstMessagePtr = std::unique_ptr<GstMessage, decltype(&gstMessageDeleter)>;
using GstBusPtr = std::unique_ptr<GstBus, decltype(&gstBusDeleter)>;

static string deriveUniqueId( GstDevice* device )
{
	string identifier;
	if( ! device )
		return identifier;

	GstStructure* props = gst_device_get_properties( device );
	if( props ) {
		const gchar* path = gst_structure_get_string( props, "device.path" );
		if( path )
			identifier = path;
		else {
			const gchar* node = gst_structure_get_string( props, "device.node" );
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

static bool pathExists( const string& path )
{
	if( path.empty() )
		return true;

	return ci::fs::exists( path );
}

} // namespace

// CaptureImplGStreamer::SurfaceCache

class CaptureImplGStreamer::SurfaceCache {
  public:
	SurfaceCache( int32_t width, int32_t height, SurfaceChannelOrder sco, int numSurfaces )
		: mWidth( width )
		, mHeight( height )
		, mSCO( sco )
	{
		mSurfaces.resize( numSurfaces );
		allocateSurfaces();
	}

	Surface8uRef getNewSurface()
	{
		auto it = find_if( mSurfaces.begin(), mSurfaces.end(), []( const Surface8uRef& s ) { return s && s.use_count() == 1; } );
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
		for( auto& surface : mSurfaces ) {
			surface = Surface8u::create( mWidth, mHeight, mSCO.hasAlpha(), mSCO );
		}
	}

	vector<Surface8uRef> mSurfaces;
	int32_t				 mWidth = 0;
	int32_t				 mHeight = 0;
	SurfaceChannelOrder	 mSCO;
};

// static
void CaptureImplGStreamer::ensureGStreamerInitialized()
{
	static std::once_flag flag;
	std::call_once( flag, []() {
		if( ! gst_is_initialized() ) {
			GError* err = nullptr;
			if( ! gst_init_check( nullptr, nullptr, &err ) ) {
				std::string message = "GStreamer initialization failed";
				if( err && err->message )
					message += ": " + std::string( err->message );
				if( err )
					g_error_free( err );
				throw CaptureExcInitFail( message );
			}
		}
	} );
}

CaptureImplGStreamer::Device::Device( GstDevice* device, const std::string& name, const std::string& uniqueId )
	: mUniqueId( uniqueId )
	, mDevice( device )
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

	GstElement* probe = gst_device_create_element( mDevice, nullptr );
	if( ! probe )
		return false;
	gst_object_unref( probe );
	return true;
}

bool CaptureImplGStreamer::Device::isConnected() const
{
	if( ! mDevice )
		return false;

	string		  path;
	GstStructure* props = gst_device_get_properties( mDevice );
	if( props ) {
		const gchar* devicePath = gst_structure_get_string( props, "device.path" );
		if( devicePath )
			path = devicePath;
		gst_structure_free( props );
	}

	if( path.empty() )
		return true;

	return pathExists( path );
}

CaptureImplGStreamer::CaptureImplGStreamer( int32_t width, int32_t height, const Capture::DeviceRef device )
	: mSurfaceCache()
	, mCurrentFrame()
	, mHasNewFrame( false )
	, mPipeline( nullptr )
	, mSource( nullptr )
	, mVideoConvert( nullptr )
	, mCapsFilter( nullptr )
	, mAppSink( nullptr )
	, mBus( nullptr )
	, mRunBusWatch( false )
	, mRequestedWidth( width )
	, mRequestedHeight( height )
	, mBestWidth( width )
	, mBestHeight( height )
	, mWidth( width )
	, mHeight( height )
	, mIsCapturing( false )
	, mDevice( device )
{
	ensureGStreamerInitialized();

	// Find the best matching resolution for the requested size
	Capture::DeviceRef selectedDevice = device;
	if( ! selectedDevice ) {
		const auto& devices = getDevices();
		if( ! devices.empty() )
			selectedDevice = devices.front();
	}

	mBestWidth = width;
	mBestHeight = height;

	if( ! initializePipeline( mBestWidth, mBestHeight ) )
		throw CaptureExcInitFail( "Failed to initialize capture pipeline" );
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
		throw CaptureExcInitFail( "Failed to initialize capture pipeline" );

	GstStateChangeReturn result = gst_element_set_state( mPipeline, GST_STATE_PLAYING );
	if( result == GST_STATE_CHANGE_FAILURE ) {
		CI_LOG_E( "Failed to set pipeline to PLAYING state" );

		// Get error details
		GstBus*		bus = gst_element_get_bus( mPipeline );
		GstMessage* msg = gst_bus_pop_filtered( bus, GST_MESSAGE_ERROR );
		if( msg ) {
			GError* err = nullptr;
			gchar*	dbg = nullptr;
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

		throw CaptureExcInitFail( "Failed to start GStreamer pipeline" );
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
	bool			  hasNewFrame = mHasNewFrame;
	mHasNewFrame = false;
	return hasNewFrame;
}

Surface8uRef CaptureImplGStreamer::getSurface() const
{
	lock_guard<mutex> lock( mMutex );
	return mCurrentFrame;
}

namespace {

enum class PipelineType {
	RAW_DIRECT,      // v4l2src -> videoconvert -> appsink (for video/x-raw)
	JPEG_DECODE,     // v4l2src -> jpegdec -> videoconvert -> appsink (for image/jpeg)
	H264_DECODE,     // v4l2src -> h264parse -> avdec_h264 -> videoconvert -> appsink (for video/x-h264)
	HEVC_DECODE,     // v4l2src -> h265parse -> avdec_h265 -> videoconvert -> appsink (for video/x-h265)
	DECODEBIN_FALLBACK // v4l2src -> decodebin -> videoconvert -> appsink (for unknown compressed formats)
};

struct DeviceFormatInfo {
	PipelineType pipelineType;
	std::string mediaType;
	GstStructure* bestFormat;
};

DeviceFormatInfo analyzeDeviceFormat( GstDevice* device, int32_t targetWidth, int32_t targetHeight ) {
	DeviceFormatInfo info = { PipelineType::DECODEBIN_FALLBACK, "", nullptr };

	if( ! device ) {
		return info;
	}

	GstCapsPtr deviceCaps( gst_device_get_caps( device ), gstCapsDeleter );
	if( ! deviceCaps ) {
		return info;
	}

	// Look for the best matching format, prioritizing efficiency
	int numStructures = gst_caps_get_size( deviceCaps.get() );
	int32_t bestScore = -1;
	GstStructure* bestStructure = nullptr;
	PipelineType bestPipelineType = PipelineType::DECODEBIN_FALLBACK;
	std::string bestMediaType;

	for( int i = 0; i < numStructures; ++i ) {
		GstStructure* structure = gst_caps_get_structure( deviceCaps.get(), i );
		if( ! structure ) continue;

		const gchar* mediaTypeName = gst_structure_get_name( structure );
		if( ! mediaTypeName ) continue;

		std::string mediaType( mediaTypeName );

		int width, height;
		if( ! gst_structure_get_int( structure, "width", &width ) ||
			! gst_structure_get_int( structure, "height", &height ) ) {
			continue;
		}

		// Calculate score for this format (same as existing resolution matching)
		double targetAspect = (double)targetWidth / targetHeight;
		double currentAspect = (double)width / height;
		bool aspectMatch = std::abs( currentAspect - targetAspect ) < 0.05;

		int32_t score = 0;
		if( width == targetWidth && height == targetHeight ) {
			score = 1000000;
		}
		else if( aspectMatch && width <= targetWidth && height <= targetHeight ) {
			score = 500000 + width + height;
		}
		else if( aspectMatch && width >= targetWidth && height >= targetHeight ) {
			score = 300000 - ( width - targetWidth ) - ( height - targetHeight );
		}
		else if( width <= targetWidth && height <= targetHeight ) {
			score = 200000 + width + height;
		}
		else {
			int32_t excess = std::max( 0, width - targetWidth ) + std::max( 0, height - targetHeight );
			score = 100000 - excess;
		}

		// Apply pipeline efficiency bonus (prefer simpler pipelines)
		PipelineType pipelineType = PipelineType::DECODEBIN_FALLBACK;
		int32_t efficiencyBonus = 0;

		if( mediaType == "video/x-raw" ) {
			pipelineType = PipelineType::RAW_DIRECT;
			efficiencyBonus = 50000; // Highest efficiency - no decoding needed
		}
		else if( mediaType == "image/jpeg" ) {
			pipelineType = PipelineType::JPEG_DECODE;
			efficiencyBonus = 30000; // Good efficiency - hardware JPEG decode
		}
		else if( mediaType == "video/x-h264" ) {
			pipelineType = PipelineType::H264_DECODE;
			efficiencyBonus = 20000; // Moderate efficiency - H.264 decode
		}
		else if( mediaType == "video/x-h265" || mediaType == "video/x-hevc" ) {
			pipelineType = PipelineType::HEVC_DECODE;
			efficiencyBonus = 15000; // HEVC decode - slightly more complex than H.264
		}
		else {
			pipelineType = PipelineType::DECODEBIN_FALLBACK;
			efficiencyBonus = 0; // Lowest efficiency - generic decoding
		}

		score += efficiencyBonus;

		if( score > bestScore ) {
			bestScore = score;
			if( bestStructure && bestStructure != structure ) {
				// Don't free - structures are owned by the caps
			}
			bestStructure = structure;
			bestPipelineType = pipelineType;
			bestMediaType = mediaType;
		}
	}

	if( bestStructure ) {
		info.pipelineType = bestPipelineType;
		info.mediaType = bestMediaType;
		info.bestFormat = gst_structure_copy( bestStructure );
	}

	// deviceCaps automatically cleaned up by unique_ptr
	return info;
}

} // anonymous namespace

bool CaptureImplGStreamer::initializePipeline( int32_t width, int32_t height )
{
	if( mPipeline )
		return true;

	// Analyze device capabilities to choose optimal pipeline
	DeviceFormatInfo formatInfo = { PipelineType::DECODEBIN_FALLBACK, "", nullptr };
	if( mDevice ) {
		auto gstDevice = std::dynamic_pointer_cast<CaptureImplGStreamer::Device>( mDevice );
		if( gstDevice && gstDevice->getGstDevice() ) {
			formatInfo = analyzeDeviceFormat( gstDevice->getGstDevice(), width, height );
		}
	}

	CI_LOG_I( "Using pipeline type: " << (int)formatInfo.pipelineType << " for media type: " << formatInfo.mediaType );

	// Create v4l2src
	GstElementPtr source( gst_element_factory_make( "v4l2src", "camera-source" ), gstElementDeleter );
	if( ! source ) {
		throw CaptureExcInitFail( "Failed to create GStreamer source element" );
	}

	// Set device path
	if( mDevice ) {
		auto gstDevice = std::dynamic_pointer_cast<CaptureImplGStreamer::Device>( mDevice );
		if( gstDevice && gstDevice->getGstDevice() ) {
			GstStructure* props = gst_device_get_properties( gstDevice->getGstDevice() );
			if( props ) {
				// Try different property names for device path
				const gchar* device_path = gst_structure_get_string( props, "api.v4l2.path" );
				if( ! device_path ) {
					device_path = gst_structure_get_string( props, "device.path" );
				}
				if( device_path ) {
					g_object_set( source.get(), "device", device_path, nullptr );
				}
				else {
					throw CaptureExcInitFail( "Cannot determine device path for selected camera" );
				}
				gst_structure_free( props );
			}
		}
		else {
			throw CaptureExcInitFail( "No camera device available or selected" );
		}
	}

	// Set source format if we found a specific one
	GstElementPtr sourceCapsFilter( nullptr, gstElementDeleter );
	if( formatInfo.bestFormat ) {
		sourceCapsFilter.reset( gst_element_factory_make( "capsfilter", "source-capsfilter" ) );
		if( sourceCapsFilter ) {
			GstCapsPtr sourceCaps( gst_caps_new_empty(), gstCapsDeleter );
			gst_caps_append_structure( sourceCaps.get(), formatInfo.bestFormat ); // Takes ownership
			g_object_set( sourceCapsFilter.get(), "caps", sourceCaps.get(), nullptr );
		}
		formatInfo.bestFormat = nullptr; // Ownership transferred
	}

	// Create common elements
	GstElementPtr videoConvert( gst_element_factory_make( "videoconvert", "videoconvert" ), gstElementDeleter );
	GstElementPtr capsFilter( gst_element_factory_make( "capsfilter", "capsfilter" ), gstElementDeleter );
	GstElementPtr appSink( gst_element_factory_make( "appsink", "appsink" ), gstElementDeleter );

	if( ! videoConvert || ! capsFilter || ! appSink ) {
		throw CaptureExcInitFail( "Failed to create common GStreamer elements" );
	}

	// Create pipeline-specific decoder elements
	GstElementPtr decoder( nullptr, gstElementDeleter );
	GstElementPtr parser( nullptr, gstElementDeleter );

	switch( formatInfo.pipelineType ) {
		case PipelineType::RAW_DIRECT:
			// No decoder needed for raw video
			break;

		case PipelineType::JPEG_DECODE:
			decoder.reset( gst_element_factory_make( "jpegdec", "jpegdec" ) );
			if( ! decoder ) {
				throw CaptureExcInitFail( "Failed to create JPEG decoder" );
			}
			break;

		case PipelineType::H264_DECODE:
			parser.reset( gst_element_factory_make( "h264parse", "h264parse" ) );
			decoder.reset( gst_element_factory_make( "avdec_h264", "avdec_h264" ) );
			if( ! parser || ! decoder ) {
				throw CaptureExcInitFail( "Failed to create H.264 decoder elements" );
			}
			break;

		case PipelineType::HEVC_DECODE:
			parser.reset( gst_element_factory_make( "h265parse", "h265parse" ) );
			decoder.reset( gst_element_factory_make( "avdec_h265", "avdec_h265" ) );
			if( ! parser || ! decoder ) {
				throw CaptureExcInitFail( "Failed to create HEVC/H.265 decoder elements" );
			}
			break;

		case PipelineType::DECODEBIN_FALLBACK:
		default:
			decoder.reset( gst_element_factory_make( "decodebin", "decodebin" ) );
			if( ! decoder ) {
				throw CaptureExcInitFail( "Failed to create decodebin fallback" );
			}
			break;
	}

	// Set RGB format but let resolution negotiate automatically
	GstCapsPtr caps( gst_caps_new_simple( "video/x-raw", "format", G_TYPE_STRING, "RGB", nullptr ), gstCapsDeleter );
	g_object_set( capsFilter.get(), "caps", caps.get(), nullptr );

	g_object_set( appSink.get(), "emit-signals", FALSE, "sync", FALSE, "max-buffers", 1, "drop", TRUE, nullptr );

	static GstAppSinkCallbacks sCallbacks = { nullptr, nullptr, &CaptureImplGStreamer::onNewSample };
	gst_app_sink_set_callbacks( GST_APP_SINK( appSink.get() ), &sCallbacks, this, nullptr );

	// Create pipeline
	GstElementPtr pipeline( gst_pipeline_new( "cinder-capture" ), gstElementDeleter );
	if( ! pipeline ) {
		throw CaptureExcInitFail( "Failed to create GStreamer pipeline" );
	}

	// Build and link pipeline based on type
	switch( formatInfo.pipelineType ) {
		case PipelineType::RAW_DIRECT: {
			// Pipeline: v4l2src [-> capsfilter] -> videoconvert -> capsfilter -> appsink
			if( sourceCapsFilter ) {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), sourceCapsFilter.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), sourceCapsFilter.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link RAW pipeline with source caps filter" );
				}
			}
			else {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link RAW pipeline" );
				}
			}
			break;
		}

		case PipelineType::JPEG_DECODE: {
			// Pipeline: v4l2src [-> capsfilter] -> jpegdec -> videoconvert -> capsfilter -> appsink
			if( sourceCapsFilter ) {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), sourceCapsFilter.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), sourceCapsFilter.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link JPEG pipeline with source caps filter" );
				}
			}
			else {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link JPEG pipeline" );
				}
			}
			break;
		}

		case PipelineType::H264_DECODE: {
			// Pipeline: v4l2src [-> capsfilter] -> h264parse -> avdec_h264 -> videoconvert -> capsfilter -> appsink
			if( sourceCapsFilter ) {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), sourceCapsFilter.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), sourceCapsFilter.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link H.264 pipeline with source caps filter" );
				}
			}
			else {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link H.264 pipeline" );
				}
			}
			break;
		}

		case PipelineType::HEVC_DECODE: {
			// Pipeline: v4l2src [-> capsfilter] -> h265parse -> avdec_h265 -> videoconvert -> capsfilter -> appsink
			if( sourceCapsFilter ) {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), sourceCapsFilter.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), sourceCapsFilter.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link HEVC pipeline with source caps filter" );
				}
			}
			else {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), parser.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link HEVC pipeline" );
				}
			}
			break;
		}

		case PipelineType::DECODEBIN_FALLBACK:
		default: {
			// Pipeline: v4l2src [-> capsfilter] -> decodebin -> videoconvert -> capsfilter -> appsink
			if( sourceCapsFilter ) {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), sourceCapsFilter.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link_many( source.get(), sourceCapsFilter.get(), decoder.get(), nullptr ) ) {
					throw CaptureExcInitFail( "Failed to link source through caps filter to decodebin" );
				}
			}
			else {
				gst_bin_add_many( GST_BIN( pipeline.get() ), source.get(), decoder.get(), videoConvert.get(), capsFilter.get(), appSink.get(), nullptr );
				if( ! gst_element_link( source.get(), decoder.get() ) ) {
					throw CaptureExcInitFail( "Failed to link source to decodebin" );
				}
			}

			// Link videoconvert to capsfilter to appsink (separate for decodebin's dynamic pads)
			if( ! gst_element_link_many( videoConvert.get(), capsFilter.get(), appSink.get(), nullptr ) ) {
				throw CaptureExcInitFail( "Failed to link decodebin video processing elements" );
			}

			// Store for decodebin pad-added callback
			mVideoConvert = videoConvert.get();

			// Connect decodebin's dynamic pad
			g_signal_connect( decoder.get(), "pad-added", G_CALLBACK( onDecoderPadAdded ), videoConvert.get() );
			break;
		}
	}

	// Transfer ownership to member variables (release from unique_ptr)
	// Note: All elements added to the pipeline are now owned by it
	mPipeline = pipeline.release();
	mSource = source.release();
	mVideoConvert = videoConvert.release();
	mCapsFilter = capsFilter.release();
	mAppSink = appSink.release();
	mBus = gst_element_get_bus( mPipeline );

	// Release ownership of elements that were added to pipeline but aren't stored as members
	sourceCapsFilter.release();  // Owned by pipeline now
	decoder.release();           // Owned by pipeline now
	parser.release();            // Owned by pipeline now

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
			GstMessage* message = gst_bus_timed_pop( mBus, GST_MSECOND * 100 );
			if( ! message )
				continue;

			switch( GST_MESSAGE_TYPE( message ) ) {
				case GST_MESSAGE_ERROR:
					{
						GError* err = nullptr;
						gchar*	dbg = nullptr;
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
void CaptureImplGStreamer::onDecoderPadAdded( GstElement* decoder, GstPad* pad, gpointer userData )
{

	GstElement* videoConvert = static_cast<GstElement*>( userData );
	GstPad*		sinkPad = gst_element_get_static_pad( videoConvert, "sink" );

	if( gst_pad_is_linked( sinkPad ) ) {
		gst_object_unref( sinkPad );
		return;
	}

	// Check if this is a video pad
	GstCaps*	  caps = gst_pad_query_caps( pad, nullptr );
	GstStructure* str = gst_caps_get_structure( caps, 0 );
	const gchar*  name = gst_structure_get_name( str );

	if( g_str_has_prefix( name, "video/" ) ) {
		GstPadLinkReturn ret = gst_pad_link( pad, sinkPad );
		if( GST_PAD_LINK_FAILED( ret ) ) {
			CI_LOG_E( "Failed to link decoder to videoconvert" );
		}
		else {
		}
	}

	gst_caps_unref( caps );
	gst_object_unref( sinkPad );
}

GstFlowReturn CaptureImplGStreamer::onNewSample( GstAppSink* sink, gpointer userData )
{
	auto* impl = static_cast<CaptureImplGStreamer*>( userData );
	return impl->handleSample( gst_app_sink_pull_sample( sink ) );
}

GstFlowReturn CaptureImplGStreamer::handleSample( GstSample* sample )
{
	if( ! sample ) {
		return GST_FLOW_OK;
	}

	// Take ownership of the sample
	GstSamplePtr samplePtr( sample, gstSampleDeleter );

	GstCaps*   caps = gst_sample_get_caps( samplePtr.get() );
	GstBuffer* buffer = gst_sample_get_buffer( samplePtr.get() );
	if( ! caps || ! buffer ) {
		return GST_FLOW_OK;
	}

	GstVideoInfo info;
	if( ! gst_video_info_from_caps( &info, caps ) ) {
		return GST_FLOW_OK;
	}

	GstMapInfo mapInfo;
	if( ! gst_buffer_map( buffer, &mapInfo, GST_MAP_READ ) ) {
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

		Surface8uRef   surface = mSurfaceCache->getNewSurface();
		uint8_t*	   dst = surface->getData();
		int32_t		   dstStride = surface->getRowBytes();
		const uint8_t* src = mapInfo.data;

		for( int32_t row = 0; row < height; ++row ) {
			memcpy( dst + row * dstStride, src + row * stride, std::min<int32_t>( dstStride, stride ) );
		}

		mCurrentFrame = surface;
		mHasNewFrame = true;
		mWidth = width;
		mHeight = height;
	}

	gst_buffer_unmap( buffer, &mapInfo );
	// samplePtr automatically cleaned up

	return GST_FLOW_OK;
}

// static
const vector<Capture::DeviceRef>& CaptureImplGStreamer::getDevices( bool forceRefresh )
{
	ensureGStreamerInitialized();

	static vector<Capture::DeviceRef> sDevices;
	static mutex					  sDeviceMutex;
	static bool						  sEnumerated = false;

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

	GstDeviceMonitor* monitor = gst_device_monitor_new();
	gst_device_monitor_add_filter( monitor, "Video/Source", nullptr );
	gst_device_monitor_start( monitor );

	GList* devices = gst_device_monitor_get_devices( monitor );
	for( GList* it = devices; it != nullptr; it = it->next ) {
		GstDevice* gstDevice = GST_DEVICE( it->data );
		if( ! gstDevice )
			continue;

		string name = gst_device_get_display_name( gstDevice );
		string uniqueId = deriveUniqueId( gstDevice );
		auto   device = make_shared<CaptureImplGStreamer::Device>( gstDevice, name, uniqueId );
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
