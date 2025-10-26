#include "cinder/app/App.h"
#include "cinder/Surface.h"
#include "cinder/ip/Resize.h"
#include "cinder/Filter.h"

#include "catch.hpp"

using namespace ci;
using namespace ci::app;

TEST_CASE( "Image/Resize/ChannelOrderPreservation" )
{
	// Test that resizeCopy variants preserve channel order

	SECTION( "RGB resizeCopy preserves RGB" ) {
		Surface8u src( 10, 10, false, SurfaceChannelOrder::RGB );
		// Fill with known pattern
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 255 ) );

		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::RGB );
		REQUIRE( dst.hasAlpha() == false );

		// Verify pixel data is still in RGB order
		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
		REQUIRE( pixel.g >= 99 );
		REQUIRE( pixel.g <= 101 );
	}

	SECTION( "BGR resizeCopy preserves BGR" ) {
		Surface8u src( 10, 10, false, SurfaceChannelOrder::BGR );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 255 ) );

		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::BGR );
		REQUIRE( dst.hasAlpha() == false );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
	}

	SECTION( "RGBA resizeCopy preserves RGBA" ) {
		Surface8u src( 10, 10, true, SurfaceChannelOrder::RGBA );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 128 ) );

		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::RGBA );
		REQUIRE( dst.hasAlpha() == true );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
		REQUIRE( pixel.a >= 127 );
		REQUIRE( pixel.a <= 129 );
	}

	SECTION( "BGRA resizeCopy preserves BGRA" ) {
		Surface8u src( 10, 10, true, SurfaceChannelOrder::BGRA );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 128 ) );

		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::BGRA );
		REQUIRE( dst.hasAlpha() == true );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
		REQUIRE( pixel.a >= 127 );
		REQUIRE( pixel.a <= 129 );
	}

	SECTION( "ARGB resizeCopy preserves ARGB" ) {
		Surface8u src( 10, 10, true, SurfaceChannelOrder::ARGB );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 128 ) );

		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::ARGB );
		REQUIRE( dst.hasAlpha() == true );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
		REQUIRE( pixel.a >= 127 );
		REQUIRE( pixel.a <= 129 );
	}

	SECTION( "resizeSrgbCopy preserves channel order" ) {
		Surface8u src( 10, 10, true, SurfaceChannelOrder::BGRA );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 200, 100, 50, 128 ) );

		Surface8u dst = ip::resizeSrgbCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.getChannelOrder().getCode() == SurfaceChannelOrder::BGRA );
		REQUIRE( dst.hasAlpha() == true );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r >= 199 );
		REQUIRE( pixel.r <= 201 );
	}
}

TEST_CASE( "Image/Resize/PremultipliedAlphaPreservation" )
{
	SECTION( "resizeCopy preserves premultiplied flag" ) {
		Surface8u src( 10, 10, true );
		src.setPremultiplied( true );
		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.isPremultiplied() == true );
	}

	SECTION( "resizeCopy preserves non-premultiplied flag" ) {
		Surface8u src( 10, 10, true );
		src.setPremultiplied( false );
		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 20, 20 ) );
		REQUIRE( dst.isPremultiplied() == false );
	}
}

TEST_CASE( "Image/Resize/DifferentChannelOrders" )
{
	// Create a simple test pattern: red square in top-left
	auto createTestPattern = []( const SurfaceChannelOrder &order, bool hasAlpha ) {
		Surface8u surf( 4, 4, hasAlpha, order );
		surf.setPixel( ivec2( 0, 0 ), ColorA8u( 255, 0, 0, 255 ) );
		surf.setPixel( ivec2( 1, 0 ), ColorA8u( 0, 255, 0, 255 ) );
		surf.setPixel( ivec2( 0, 1 ), ColorA8u( 0, 0, 255, 255 ) );
		surf.setPixel( ivec2( 1, 1 ), ColorA8u( 255, 255, 0, 255 ) );
		return surf;
	};

	SECTION( "RGB to BGR conversion during resize" ) {
		Surface8u src = createTestPattern( SurfaceChannelOrder::RGB, false );
		Surface8u dst( 8, 8, false, SurfaceChannelOrder::BGR );  // Actually resize (4x4 -> 8x8)

		ip::resize( src, &dst );

		// Verify red pixel is still red (channels swapped correctly) after interpolation
		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r == 255 );
		REQUIRE( pixel.g == 0 );
		REQUIRE( pixel.b == 0 );
	}

	SECTION( "RGBA to BGRA conversion during resize" ) {
		Surface8u src = createTestPattern( SurfaceChannelOrder::RGBA, true );
		Surface8u dst( 8, 8, true, SurfaceChannelOrder::BGRA );  // Actually resize (4x4 -> 8x8)

		ip::resize( src, &dst );

		// Verify colors and alpha are correct after interpolation
		ColorA8u red = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( red.r == 255 );
		REQUIRE( red.g == 0 );
		REQUIRE( red.b == 0 );
		REQUIRE( red.a == 255 );

		ColorA8u green = dst.getPixel( ivec2( 2, 0 ) );  // Scaled position (interpolated with black neighbors)
		REQUIRE( green.r < 100 );  // Should be mostly green, not red
		REQUIRE( green.g > 100 );  // Interpolated, so not as strong
		REQUIRE( green.b < 100 );
	}

	SECTION( "RGBA to ARGB conversion during resize" ) {
		Surface8u src = createTestPattern( SurfaceChannelOrder::RGBA, true );
		Surface8u dst( 8, 8, true, SurfaceChannelOrder::ARGB );  // Actually resize (4x4 -> 8x8)

		ip::resize( src, &dst );

		ColorA8u pixel = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( pixel.r == 255 );
		REQUIRE( pixel.a == 255 );
	}
}

TEST_CASE( "Image/Resize/PaddedFormats" )
{
	// Test that RGBX/BGRX/XRGB/XBGR formats work correctly (4 bytes/pixel with padding)

	SECTION( "RGBX format doesn't corrupt colors" ) {
		Surface8u src( 4, 4, false, SurfaceChannelOrder::RGBX );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 255, 0, 0, 255 ) );
		src.setPixel( ivec2( 1, 0 ), ColorA8u( 0, 255, 0, 255 ) );
		src.setPixel( ivec2( 0, 1 ), ColorA8u( 0, 0, 255, 255 ) );

		Surface8u dst( 8, 8, false, SurfaceChannelOrder::RGBX );
		ip::resize( src, &dst );

		// Verify pixels are intact (not reading into padding)
		ColorA8u red = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( red.r == 255 );
		REQUIRE( red.g == 0 );
		REQUIRE( red.b == 0 );

		ColorA8u green = dst.getPixel( ivec2( 2, 0 ) );  // Scaled position (interpolated with black neighbors)
		REQUIRE( green.r < 100 );  // Should be mostly green
		REQUIRE( green.g > 100 );  // Interpolated, so not as strong
		REQUIRE( green.b < 100 );
	}

	SECTION( "BGRX format doesn't corrupt colors" ) {
		Surface8u src( 4, 4, false, SurfaceChannelOrder::BGRX );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 255, 0, 0, 255 ) );
		src.setPixel( ivec2( 1, 0 ), ColorA8u( 0, 255, 0, 255 ) );

		Surface8u dst( 8, 8, false, SurfaceChannelOrder::BGRX );
		ip::resize( src, &dst );

		ColorA8u red = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( red.r == 255 );
		REQUIRE( red.g == 0 );
		REQUIRE( red.b == 0 );
	}

	SECTION( "XRGB format doesn't corrupt colors" ) {
		Surface8u src( 4, 4, false, SurfaceChannelOrder::XRGB );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 255, 0, 0, 255 ) );
		src.setPixel( ivec2( 1, 0 ), ColorA8u( 0, 255, 0, 255 ) );

		Surface8u dst( 8, 8, false, SurfaceChannelOrder::XRGB );
		ip::resize( src, &dst );

		ColorA8u red = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( red.r == 255 );
		REQUIRE( red.g == 0 );
		REQUIRE( red.b == 0 );
	}

	SECTION( "XBGR format doesn't corrupt colors" ) {
		Surface8u src( 4, 4, false, SurfaceChannelOrder::XBGR );
		src.setPixel( ivec2( 0, 0 ), ColorA8u( 255, 0, 0, 255 ) );
		src.setPixel( ivec2( 1, 0 ), ColorA8u( 0, 255, 0, 255 ) );

		Surface8u dst( 8, 8, false, SurfaceChannelOrder::XBGR );
		ip::resize( src, &dst );

		ColorA8u red = dst.getPixel( ivec2( 0, 0 ) );
		REQUIRE( red.r == 255 );
		REQUIRE( red.g == 0 );
		REQUIRE( red.b == 0 );
	}
}

TEST_CASE( "Image/Resize/SrgbVsStandard" )
{
	// sRGB and standard resize should produce different results

	SECTION( "sRGB resize differs from standard for gamma correction" ) {
		Surface8u src( 4, 4, false );

		// Create gradient pattern
		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				uint8_t val = (uint8_t)(iter.x() * 64);
				iter.r() = val;
				iter.g() = val;
				iter.b() = val;
			}
		}

		Surface8u dstStandard( 2, 2, false );
		Surface8u dstSrgb( 2, 2, false );

		ip::resize( src, &dstStandard );
		ip::resizeSrgb( src, &dstSrgb );

		// Results should be different (sRGB does gamma correction)
		// Compare pixel at (1,0) which should be mid-tone
		ColorA8u pixelStd = dstStandard.getPixel( ivec2( 1, 0 ) );
		ColorA8u pixelSrgb = dstSrgb.getPixel( ivec2( 1, 0 ) );

		// sRGB should generally be brighter for mid-tones
		bool different = ( pixelStd.r != pixelSrgb.r ) ||
		                 ( pixelStd.g != pixelSrgb.g ) ||
		                 ( pixelStd.b != pixelSrgb.b );
		REQUIRE( different );
	}
}

TEST_CASE( "Image/Resize/BasicFunctionality" )
{
	SECTION( "Upsampling produces correct size" ) {
		Surface8u src( 10, 10, false );
		Surface8u dst( 20, 20, false );
		ip::resize( src, &dst );
		REQUIRE( dst.getWidth() == 20 );
		REQUIRE( dst.getHeight() == 20 );
	}

	SECTION( "Downsampling produces correct size" ) {
		Surface8u src( 20, 20, false );
		Surface8u dst( 10, 10, false );
		ip::resize( src, &dst );
		REQUIRE( dst.getWidth() == 10 );
		REQUIRE( dst.getHeight() == 10 );
	}

	SECTION( "resizeCopy returns correct size" ) {
		Surface8u src( 10, 10, false );
		Surface8u dst = ip::resizeCopy( src, src.getBounds(), ivec2( 15, 15 ) );
		REQUIRE( dst.getWidth() == 15 );
		REQUIRE( dst.getHeight() == 15 );
	}
}

TEST_CASE( "Image/Resize/AreaResize" )
{
	SECTION( "Partial area resize" ) {
		Surface8u src( 20, 20, false );

		// Fill with pattern
		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				iter.r() = (iter.x() < 10) ? 255 : 0;
				iter.g() = (iter.y() < 10) ? 255 : 0;
				iter.b() = 128;
			}
		}

		// Resize just top-left quadrant
		Area srcArea( 0, 0, 10, 10 );
		Surface8u dst( 5, 5, false );

		ip::resize( src, srcArea, &dst, dst.getBounds() );

		REQUIRE( dst.getWidth() == 5 );
		REQUIRE( dst.getHeight() == 5 );

		// Should mostly be red and green (from top-left quadrant)
		auto dstIter = dst.getIter();
		REQUIRE( dstIter.r() > 200 ); // Should have red
		REQUIRE( dstIter.g() > 200 ); // Should have green
	}
}

TEST_CASE( "Image/Resize/DifferentFilters" )
{
	// Different filters should produce different results

	Surface8u src( 8, 8, false );

	// Checkerboard pattern
	auto iter = src.getIter();
	while( iter.line() ) {
		while( iter.pixel() ) {
			bool white = ((iter.x() / 2) + (iter.y() / 2)) % 2 == 0;
			uint8_t val = white ? 255 : 0;
			iter.r() = iter.g() = iter.b() = val;
		}
	}

	Surface8u dstBox( 4, 4, false );
	Surface8u dstTriangle( 4, 4, false );
	Surface8u dstMitchell( 4, 4, false );

	ip::resize( src, &dstBox, FilterBox() );
	ip::resize( src, &dstTriangle, FilterTriangle() );
	ip::resize( src, &dstMitchell, FilterMitchell() );

	// Filters should produce different results
	auto iterBox = dstBox.getIter();
	auto iterTri = dstTriangle.getIter();
	auto iterMit = dstMitchell.getIter();

	iterBox.pixel(); iterBox.pixel();
	iterTri.pixel(); iterTri.pixel();
	iterMit.pixel(); iterMit.pixel();

	bool boxVsTri = ( iterBox.r() != iterTri.r() );
	bool triVsMit = ( iterTri.r() != iterMit.r() );

	REQUIRE( (boxVsTri || triVsMit) ); // At least one pair should differ
}

TEST_CASE( "Image/Resize/Uint16Support" )
{
	SECTION( "uint16 surfaces resize correctly" ) {
		Surface16u src( 10, 10, false );

		// Fill with high values
		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				iter.r() = 65535;
				iter.g() = 32768;
				iter.b() = 16384;
			}
		}

		Surface16u dst( 5, 5, false );
		ip::resize( src, &dst );

		// Values should be approximately preserved
		auto dstIter = dst.getIter();
		REQUIRE( dstIter.r() > 65000 );
		REQUIRE( dstIter.g() > 32000 );
		REQUIRE( dstIter.g() < 33500 );
		REQUIRE( dstIter.b() > 16000 );
		REQUIRE( dstIter.b() < 17000 );
	}
}

TEST_CASE( "Image/Resize/SolidColorPreservation" )
{
	// Resizing a solid color should preserve that color

	SECTION( "Solid red is preserved" ) {
		Surface8u src( 10, 10, false );

		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				iter.r() = 200;
				iter.g() = 100;
				iter.b() = 50;
			}
		}

		Surface8u dst( 20, 20, false );
		ip::resize( src, &dst );

		auto dstIter = dst.getIter();
		while( dstIter.line() ) {
			while( dstIter.pixel() ) {
				// Solid colors should be preserved exactly (within rounding)
				REQUIRE( dstIter.r() >= 199 );
				REQUIRE( dstIter.r() <= 201 );
				REQUIRE( dstIter.g() >= 99 );
				REQUIRE( dstIter.g() <= 101 );
				REQUIRE( dstIter.b() >= 49 );
				REQUIRE( dstIter.b() <= 51 );
			}
		}
	}

	SECTION( "Solid white with alpha is preserved" ) {
		Surface8u src( 8, 8, true );

		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				iter.r() = 255;
				iter.g() = 255;
				iter.b() = 255;
				iter.a() = 128;
			}
		}

		Surface8u dst( 4, 4, true );
		ip::resize( src, &dst );

		auto dstIter = dst.getIter();
		// Check a center pixel
		dstIter.line(); dstIter.line();
		dstIter.pixel(); dstIter.pixel();

		REQUIRE( dstIter.r() >= 254 );
		REQUIRE( dstIter.g() >= 254 );
		REQUIRE( dstIter.b() >= 254 );
		REQUIRE( dstIter.a() >= 127 );
		REQUIRE( dstIter.a() <= 129 );
	}
}

TEST_CASE( "Image/Resize/ChannelBasicResize" )
{
	SECTION( "Single channel resize" ) {
		Channel8u src( 10, 10 );

		auto iter = src.getIter();
		while( iter.line() ) {
			while( iter.pixel() ) {
				iter.v() = (uint8_t)(iter.x() * 25);
			}
		}

		Channel8u dst( 5, 5 );
		ip::resize( src, &dst );

		REQUIRE( dst.getWidth() == 5 );
		REQUIRE( dst.getHeight() == 5 );

		// First pixel should be dark, last should be bright
		auto dstIter = dst.getIter();
		uint8_t first = dstIter.v();
		while( dstIter.line() ) {
			while( dstIter.pixel() ) {
				// Just walk to end
			}
		}
		uint8_t last = dst.getValue( ivec2( 4, 4 ) );

		REQUIRE( first < 50 );
		REQUIRE( last > 200 );
	}
}
