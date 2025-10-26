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

#include "cinder/Surface.h"
#include "cinder/ip/Resize.h"
#include "cinder/Filter.h"
#include "cinder/Rect.h"
#include "cinder/ChanTraits.h"

// Enable SIMD optimizations by default
#if !defined(STBIR_NO_SIMD)
	#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
		#define STBIR_SSE2
	#endif
	#if defined(__AVX__)
		#define STBIR_AVX
	#endif
	#if defined(__AVX2__)
		#define STBIR_AVX2
	#endif
	#if defined(__ARM_NEON) || defined(__ARM_NEON__)
		#define STBIR_NEON
	#endif
#endif

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "cinder/stb_image_resize2.h"

#include <vector>
#include <algorithm>

namespace cinder { namespace ip {

// Custom filter callbacks for filters not built into STB
static float stbir_sinc_blackman_kernel( float x, float scale, void * user_data )
{
	float support = *static_cast<float*>( user_data );
	float v = ( x == 0.0f ) ? 1.0f : sinf( 3.14159265358979323846f * x ) / ( 3.14159265358979323846f * x );
	// blackman window
	x /= support;
	return v * ( 0.42f + 0.50f * cosf( 3.14159265358979323846f * x ) + 0.08f * cosf( 6.2831853071795862f * x ) );
}

static float stbir_sinc_blackman_support( float scale, void * user_data )
{
	return *static_cast<float*>( user_data );
}

static float stbir_quadratic_kernel( float x, float scale, void * user_data )
{
	float t;
	if ( x < -1.5f ) return 0.0f;
	else if ( x < -0.5f ) { t = x + 1.5f; return 0.5f * t * t; }
	else if ( x < 0.5f ) return 0.75f - x * x;
	else if ( x < 1.5f ) { t = x - 1.5f; return 0.5f * t * t; }
	return 0.0f;
}

static float stbir_quadratic_support( float scale, void * user_data )
{
	return *static_cast<float*>( user_data );
}

static float stbir_gaussian_kernel( float x, float scale, void * user_data )
{
	return expf( -2.0f * x * x ) * sqrtf( 2.0f / 3.14159265358979323846f );
}

static float stbir_gaussian_support( float scale, void * user_data )
{
	return *static_cast<float*>( user_data );
}

static float stbir_bessel_blackman_kernel( float x, float scale, void * user_data )
{
	float support = *static_cast<float*>( user_data );
#if defined( CINDER_MSW )
	float v = ( x == 0.0f ) ? ( 3.14159265358979323846f / 4.0f ) : static_cast<float>( _j1( 3.14159265358979323846f * x ) ) / ( 2.0f * x );
#else
	float v = ( x == 0.0f ) ? ( 3.14159265358979323846f / 4.0f ) : static_cast<float>( j1( 3.14159265358979323846f * x ) ) / ( 2.0f * x );
#endif
	// blackman window
	x /= support;
	return v * ( 0.42f + 0.50f * cosf( 3.14159265358979323846f * x ) + 0.08f * cosf( 6.2831853071795862f * x ) );
}

static float stbir_bessel_blackman_support( float scale, void * user_data )
{
	return *static_cast<float*>( user_data );
}

// Map Cinder filters to STB filters
static stbir_filter mapFilterToStbir( const FilterBase &filter )
{
	const std::type_info &type = typeid( filter );

	if( type == typeid( FilterBox ) )
		return STBIR_FILTER_BOX;
	else if( type == typeid( FilterTriangle ) )
		return STBIR_FILTER_TRIANGLE;
	else if( type == typeid( FilterCubic ) )
		return STBIR_FILTER_CUBICBSPLINE;
	else if( type == typeid( FilterCatmullRom ) )
		return STBIR_FILTER_CATMULLROM;
	else if( type == typeid( FilterMitchell ) )
		return STBIR_FILTER_MITCHELL;
	else if( type == typeid( FilterQuadratic ) || type == typeid( FilterSincBlackman ) || type == typeid( FilterGaussian ) || type == typeid( FilterBesselBlackman ) )
		return STBIR_FILTER_OTHER; // Use custom filter callbacks
	else
		return STBIR_FILTER_DEFAULT;
}

// Check if filter needs custom callbacks
static bool needsCustomFilter( const FilterBase &filter )
{
	const std::type_info &type = typeid( filter );
	return ( type == typeid( FilterQuadratic ) || type == typeid( FilterSincBlackman ) || type == typeid( FilterGaussian ) || type == typeid( FilterBesselBlackman ) );
}

// Determine pixel layout based on channel order and alpha presence
static stbir_pixel_layout getPixelLayout( const SurfaceChannelOrder &channelOrder, bool hasAlpha, bool isPremultiplied )
{
	int code = channelOrder.getCode();

	// Handle padded 4-channel formats (RGBX, etc.) - these have no real alpha, just padding
	if( code == SurfaceChannelOrder::RGBX || code == SurfaceChannelOrder::BGRX ||
	    code == SurfaceChannelOrder::XRGB || code == SurfaceChannelOrder::XBGR ) {
		return STBIR_4CHANNEL; // 4 bytes per pixel, no alpha weighting
	}

	if( !hasAlpha ) {
		// 3-channel formats
		if( code == SurfaceChannelOrder::RGB )
			return STBIR_RGB;
		else if( code == SurfaceChannelOrder::BGR )
			return STBIR_BGR;
		else
			return STBIR_RGB; // Default for unspecified
	}
	else {
		// Has real alpha channel - check if premultiplied
		if( isPremultiplied ) {
			if( code == SurfaceChannelOrder::RGBA )
				return STBIR_RGBA_PM;
			else if( code == SurfaceChannelOrder::BGRA )
				return STBIR_BGRA_PM;
			else if( code == SurfaceChannelOrder::ARGB )
				return STBIR_ARGB_PM;
			else if( code == SurfaceChannelOrder::ABGR )
				return STBIR_ABGR_PM;
			else
				return STBIR_RGBA_PM; // Default
		}
		else {
			if( code == SurfaceChannelOrder::RGBA )
				return STBIR_RGBA;
			else if( code == SurfaceChannelOrder::BGRA )
				return STBIR_BGRA;
			else if( code == SurfaceChannelOrder::ARGB )
				return STBIR_ARGB;
			else if( code == SurfaceChannelOrder::ABGR )
				return STBIR_ABGR;
			else
				return STBIR_RGBA; // Default
		}
	}
}

// Resize implementation for Surface types
template<typename T>
void resizeSurface( const SurfaceT<T> &srcSurface, const Area &srcArea, SurfaceT<T> *dstSurface, const Area &dstArea, const FilterBase &filter, stbir_datatype dataType )
{
	Rectf clippedSrcRect;
	Area clippedDstArea;
	getClippedScaledRects( srcSurface.getBounds(), Rectf( srcArea ), dstSurface->getBounds(), dstArea, &clippedSrcRect, &clippedDstArea );

	if( ( clippedSrcRect.getWidth() <= 0 ) || ( clippedDstArea.getWidth() <= 0 )
		|| ( clippedSrcRect.getHeight() <= 0 ) || ( clippedDstArea.getHeight() <= 0 ) )
		return;

	int32_t srcWidth = (int32_t)clippedSrcRect.getWidth();
	int32_t srcHeight = (int32_t)clippedSrcRect.getHeight();
	int32_t srcOffsetX = static_cast<int32_t>( floor( clippedSrcRect.getX1() ) );
	int32_t srcOffsetY = static_cast<int32_t>( floor( clippedSrcRect.getY1() ) );
	int32_t dstWidth = (int32_t)clippedDstArea.getWidth();
	int32_t dstHeight = (int32_t)clippedDstArea.getHeight();
	int32_t dstOffsetX = clippedDstArea.getX1();
	int32_t dstOffsetY = clippedDstArea.getY1();

	// If source and destination have different channel orders, we need to handle conversion
	// STB doesn't automatically convert between different pixel layouts, so we resize to an
	// intermediate surface with source's channel order, then copy to destination
	if( srcSurface.getChannelOrder().getCode() != dstSurface->getChannelOrder().getCode() ) {
		SurfaceT<T> intermediate( dstWidth, dstHeight, srcSurface.hasAlpha(), srcSurface.getChannelOrder() );
		intermediate.setPremultiplied( srcSurface.isPremultiplied() );
		resizeSurface( srcSurface, srcArea, &intermediate, intermediate.getBounds(), filter, dataType );
		dstSurface->copyFrom( intermediate, intermediate.getBounds(), ivec2( dstOffsetX, dstOffsetY ) );
		return;
	}

	// Get pixel layout
	stbir_pixel_layout pixelLayout = getPixelLayout( srcSurface.getChannelOrder(), srcSurface.hasAlpha(), srcSurface.isPremultiplied() );

	// Get filter
	stbir_filter stbirFilter = mapFilterToStbir( filter );

	// Get source and destination pointers
	const T *srcData = reinterpret_cast<const T*>( srcSurface.getData( ivec2( srcOffsetX, srcOffsetY ) ) );
	T *dstData = reinterpret_cast<T*>( dstSurface->getData( ivec2( dstOffsetX, dstOffsetY ) ) );

	// Get strides
	int srcStride = static_cast<int>( srcSurface.getRowBytes() );
	int dstStride = static_cast<int>( dstSurface->getRowBytes() );

	// Check if we need custom filter
	if( needsCustomFilter( filter ) ) {
		// Use extended API for custom filters
		STBIR_RESIZE resize;
		stbir_resize_init( &resize, srcData, srcWidth, srcHeight, srcStride,
		                   dstData, dstWidth, dstHeight, dstStride,
		                   pixelLayout, dataType );

		const std::type_info &type = typeid( filter );
		float support = filter.getSupport();

		if( type == typeid( FilterQuadratic ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_quadratic_kernel, stbir_quadratic_support,
			                             stbir_quadratic_kernel, stbir_quadratic_support );
		}
		else if( type == typeid( FilterSincBlackman ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_sinc_blackman_kernel, stbir_sinc_blackman_support,
			                             stbir_sinc_blackman_kernel, stbir_sinc_blackman_support );
		}
		else if( type == typeid( FilterGaussian ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_gaussian_kernel, stbir_gaussian_support,
			                             stbir_gaussian_kernel, stbir_gaussian_support );
		}
		else if( type == typeid( FilterBesselBlackman ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_bessel_blackman_kernel, stbir_bessel_blackman_support,
			                             stbir_bessel_blackman_kernel, stbir_bessel_blackman_support );
		}

		resize.user_data = &support;
		stbir_set_edgemodes( &resize, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP );
		stbir_resize_extended( &resize );
	}
	else {
		// Use medium API for built-in filters
		stbir_resize( srcData, srcWidth, srcHeight, srcStride,
		              dstData, dstWidth, dstHeight, dstStride,
		              pixelLayout, dataType, STBIR_EDGE_CLAMP, stbirFilter );
	}
}

// Resize implementation for Channel types
template<typename T>
void resizeChannel( const ChannelT<T> &srcChannel, const Area &srcArea, ChannelT<T> *dstChannel, const Area &dstArea, const FilterBase &filter, stbir_datatype dataType )
{
	Rectf clippedSrcRect;
	Area clippedDstArea;
	getClippedScaledRects( srcChannel.getBounds(), Rectf( srcArea ), dstChannel->getBounds(), dstArea, &clippedSrcRect, &clippedDstArea );

	if( ( clippedSrcRect.getWidth() <= 0 ) || ( clippedDstArea.getWidth() <= 0 )
		|| ( clippedSrcRect.getHeight() <= 0 ) || ( clippedDstArea.getHeight() <= 0 ) )
		return;

	int32_t srcWidth = (int32_t)clippedSrcRect.getWidth();
	int32_t srcHeight = (int32_t)clippedSrcRect.getHeight();
	int32_t srcOffsetX = static_cast<int32_t>( floor( clippedSrcRect.getX1() ) );
	int32_t srcOffsetY = static_cast<int32_t>( floor( clippedSrcRect.getY1() ) );
	int32_t dstWidth = (int32_t)clippedDstArea.getWidth();
	int32_t dstHeight = (int32_t)clippedDstArea.getHeight();
	int32_t dstOffsetX = clippedDstArea.getX1();
	int32_t dstOffsetY = clippedDstArea.getY1();

	// Single channel
	stbir_pixel_layout pixelLayout = STBIR_1CHANNEL;

	// Get filter
	stbir_filter stbirFilter = mapFilterToStbir( filter );

	// Get source and destination pointers
	const T *srcData = srcChannel.getData( ivec2( srcOffsetX, srcOffsetY ) );
	T *dstData = dstChannel->getData( ivec2( dstOffsetX, dstOffsetY ) );

	// Get strides
	int srcStride = static_cast<int>( srcChannel.getRowBytes() );
	int dstStride = static_cast<int>( dstChannel->getRowBytes() );

	// Check if we need custom filter
	if( needsCustomFilter( filter ) ) {
		// Use extended API for custom filters
		STBIR_RESIZE resize;
		stbir_resize_init( &resize, srcData, srcWidth, srcHeight, srcStride,
		                   dstData, dstWidth, dstHeight, dstStride,
		                   pixelLayout, dataType );

		const std::type_info &type = typeid( filter );
		float support = filter.getSupport();

		if( type == typeid( FilterQuadratic ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_quadratic_kernel, stbir_quadratic_support,
			                             stbir_quadratic_kernel, stbir_quadratic_support );
		}
		else if( type == typeid( FilterSincBlackman ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_sinc_blackman_kernel, stbir_sinc_blackman_support,
			                             stbir_sinc_blackman_kernel, stbir_sinc_blackman_support );
		}
		else if( type == typeid( FilterGaussian ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_gaussian_kernel, stbir_gaussian_support,
			                             stbir_gaussian_kernel, stbir_gaussian_support );
		}
		else if( type == typeid( FilterBesselBlackman ) ) {
			stbir_set_filter_callbacks( &resize,
			                             stbir_bessel_blackman_kernel, stbir_bessel_blackman_support,
			                             stbir_bessel_blackman_kernel, stbir_bessel_blackman_support );
		}

		resize.user_data = &support;
		stbir_set_edgemodes( &resize, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP );
		stbir_resize_extended( &resize );
	}
	else {
		// Use medium API for built-in filters
		stbir_resize( srcData, srcWidth, srcHeight, srcStride,
		              dstData, dstWidth, dstHeight, dstStride,
		              pixelLayout, dataType, STBIR_EDGE_CLAMP, stbirFilter );
	}
}

// Template specializations for uint8_t
template<>
void resize( const SurfaceT<uint8_t> &srcSurface, const Area &srcArea, SurfaceT<uint8_t> *dstSurface, const Area &dstArea, const FilterBase &filter )
{
	// Default to linear color space for backward compatibility
	// Use resizeSrgb() for sRGB-aware filtering
	resizeSurface( srcSurface, srcArea, dstSurface, dstArea, filter, STBIR_TYPE_UINT8 );
}

template<>
void resize( const SurfaceT<uint8_t> &srcSurface, SurfaceT<uint8_t> *dstSurface, const FilterBase &filter )
{
	resize( srcSurface, srcSurface.getBounds(), dstSurface, dstSurface->getBounds(), filter );
}

template<>
SurfaceT<uint8_t> resizeCopy( const SurfaceT<uint8_t> &srcSurface, const Area &srcArea, const ivec2 &dstSize, const FilterBase &filter )
{
	SurfaceT<uint8_t> result( dstSize.x, dstSize.y, srcSurface.hasAlpha(), srcSurface.getChannelOrder() );
	result.setPremultiplied( srcSurface.isPremultiplied() );
	resize( srcSurface, srcArea, &result, result.getBounds(), filter );
	return result;
}

template<>
void resize( const ChannelT<uint8_t> &srcChannel, const Area &srcArea, ChannelT<uint8_t> *dstChannel, const Area &dstArea, const FilterBase &filter )
{
	resizeChannel( srcChannel, srcArea, dstChannel, dstArea, filter, STBIR_TYPE_UINT8 );
}

template<>
void resize( const ChannelT<uint8_t> &srcChannel, ChannelT<uint8_t> *dstChannel, const FilterBase &filter )
{
	resize( srcChannel, srcChannel.getBounds(), dstChannel, dstChannel->getBounds(), filter );
}

// Template specializations for float
template<>
void resize( const SurfaceT<float> &srcSurface, const Area &srcArea, SurfaceT<float> *dstSurface, const Area &dstArea, const FilterBase &filter )
{
	resizeSurface( srcSurface, srcArea, dstSurface, dstArea, filter, STBIR_TYPE_FLOAT );
}

template<>
void resize( const SurfaceT<float> &srcSurface, SurfaceT<float> *dstSurface, const FilterBase &filter )
{
	resize( srcSurface, srcSurface.getBounds(), dstSurface, dstSurface->getBounds(), filter );
}

template<>
SurfaceT<float> resizeCopy( const SurfaceT<float> &srcSurface, const Area &srcArea, const ivec2 &dstSize, const FilterBase &filter )
{
	SurfaceT<float> result( dstSize.x, dstSize.y, srcSurface.hasAlpha(), srcSurface.getChannelOrder() );
	result.setPremultiplied( srcSurface.isPremultiplied() );
	resize( srcSurface, srcArea, &result, result.getBounds(), filter );
	return result;
}

template<>
void resize( const ChannelT<float> &srcChannel, const Area &srcArea, ChannelT<float> *dstChannel, const Area &dstArea, const FilterBase &filter )
{
	resizeChannel( srcChannel, srcArea, dstChannel, dstArea, filter, STBIR_TYPE_FLOAT );
}

template<>
void resize( const ChannelT<float> &srcChannel, ChannelT<float> *dstChannel, const FilterBase &filter )
{
	resize( srcChannel, srcChannel.getBounds(), dstChannel, dstChannel->getBounds(), filter );
}

// Template specializations for uint16_t
template<>
void resize( const SurfaceT<uint16_t> &srcSurface, const Area &srcArea, SurfaceT<uint16_t> *dstSurface, const Area &dstArea, const FilterBase &filter )
{
	resizeSurface( srcSurface, srcArea, dstSurface, dstArea, filter, STBIR_TYPE_UINT16 );
}

template<>
void resize( const SurfaceT<uint16_t> &srcSurface, SurfaceT<uint16_t> *dstSurface, const FilterBase &filter )
{
	resize( srcSurface, srcSurface.getBounds(), dstSurface, dstSurface->getBounds(), filter );
}

template<>
SurfaceT<uint16_t> resizeCopy( const SurfaceT<uint16_t> &srcSurface, const Area &srcArea, const ivec2 &dstSize, const FilterBase &filter )
{
	SurfaceT<uint16_t> result( dstSize.x, dstSize.y, srcSurface.hasAlpha(), srcSurface.getChannelOrder() );
	result.setPremultiplied( srcSurface.isPremultiplied() );
	resize( srcSurface, srcArea, &result, result.getBounds(), filter );
	return result;
}

template<>
void resize( const ChannelT<uint16_t> &srcChannel, const Area &srcArea, ChannelT<uint16_t> *dstChannel, const Area &dstArea, const FilterBase &filter )
{
	resizeChannel( srcChannel, srcArea, dstChannel, dstArea, filter, STBIR_TYPE_UINT16 );
}

template<>
void resize( const ChannelT<uint16_t> &srcChannel, ChannelT<uint16_t> *dstChannel, const FilterBase &filter )
{
	resize( srcChannel, srcChannel.getBounds(), dstChannel, dstChannel->getBounds(), filter );
}

// sRGB-aware resize functions for uint8_t surfaces
void resizeSrgb( const Surface8u &srcSurface, const Area &srcArea, Surface8u *dstSurface, const Area &dstArea, const FilterBase &filter )
{
	resizeSurface( srcSurface, srcArea, dstSurface, dstArea, filter, STBIR_TYPE_UINT8_SRGB );
}

void resizeSrgb( const Surface8u &srcSurface, Surface8u *dstSurface, const FilterBase &filter )
{
	resizeSrgb( srcSurface, srcSurface.getBounds(), dstSurface, dstSurface->getBounds(), filter );
}

Surface8u resizeSrgbCopy( const Surface8u &srcSurface, const Area &srcArea, const ivec2 &dstSize, const FilterBase &filter )
{
	Surface8u result( dstSize.x, dstSize.y, srcSurface.hasAlpha(), srcSurface.getChannelOrder() );
	result.setPremultiplied( srcSurface.isPremultiplied() );
	resizeSrgb( srcSurface, srcArea, &result, result.getBounds(), filter );
	return result;
}

} } // namespace cinder::ip
