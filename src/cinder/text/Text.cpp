/*
 Copyright (c) 2020, The Cinder Project: http://libcinder.org
 All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

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

#include "cinder/Cinder.h"
#include "cinder/Filesystem.h"
#include "cinder/text/Text.h"
#include "cinder/text/AttrString.h"
#include "cinder/Channel.h"
#include "cinder/ip/Fill.h"
#include <memory>

#include <freetype/ft2build.h>
#include <freetype/freetype.h>

using namespace std;

namespace cinder { namespace text {

Manager* Manager::get()
{
	static Manager instance;
	return &instance;
}

Manager::Manager()
{
	mLibraryPtr = make_unique<FT_Library>();
	FT_Init_FreeType( mLibraryPtr.get() );
}

Manager::~Manager()
{
	mFonts.clear();
	mFaces.clear();
	FT_Done_FreeType( *mLibraryPtr );
}

Face* Manager::loadFace( const ci::fs::path &path, int faceIndex )
{
	FT_Face ftFace;
	FT_Error error = FT_New_Face( *mLibraryPtr, path.c_str(), faceIndex, &ftFace );
	if( error )
	 	throw FreeTypeExc( error );
	
	Face *result;
	{
		lock_guard<mutex> lock( mFaceMutex );
		mFaces.emplace_back( new Face( ftFace ) );
		result = mFaces.back().get(); 
	}
	
	return result;
}

Font* Manager::findFont( const Face *face, float size ) const
{
	int32_t discreteSize = static_cast<int32_t>( size * 64 ); // 26.6 fixed point
	for( const auto &font : mFonts )
		if( font->getFace() == face && font->getDiscreteSize() == discreteSize )
		   	return font.get();

	return nullptr;
}

Font* Manager::loadFont( Face *face, float size )
{
	Font *result;
	result = findFont( face, size );
	if( ! result ) { // couldn't find a match - create a new Font
		lock_guard<mutex> lock( mFaceMutex );
		mFonts.emplace_back( new Font( face, size ) );
		result = mFonts.back().get(); 
	}	
	
	return result;
}

Channel8u renderString( const Font *font, const char *utf8String )
{ 
	return font->renderString( utf8String );
}

void measureString( const AttrString& attrString, float *resultWidth, float *resultHeight, float *resultBaseline )
{
	float measuredWidth = 0, measuredHeight = 0, measuredBaseline = 0;
	auto runIt = attrString.iterate();
	while( runIt.nextRun() ) {
		float glyphsWidth;
		runIt.getRunFont()->shapeString( runIt.getRunStrPtr(), runIt.getRunLength(), nullptr, nullptr, &glyphsWidth );
		measuredWidth += glyphsWidth;
		measuredHeight = std::max<float>( measuredHeight, runIt.getRunFont()->getHeight() );
		measuredBaseline = std::max<float>( measuredBaseline, runIt.getRunFont()->getAscender() );
	}
	
	if( resultWidth )
	 	*resultWidth = measuredWidth;
	if( resultHeight )
		*resultHeight = measuredHeight;
	if( resultBaseline )
		*resultBaseline = measuredBaseline;
}

Channel8u renderString( const AttrString &attrString )
{
	std::vector<uint32_t> glyphIndices;
	std::vector<float> glyphPositions;

	float resultWidthF, resultHeightF, baseline;
	measureString( attrString, &resultWidthF, &resultHeightF, &baseline );
	int resultWidth = (int)ceil( resultWidthF );
	int resultHeight = (int)ceil( resultHeightF );
	
	Channel8u result( resultWidth, resultHeight );
	ip::fill( &result, (uint8_t)0 );
	
	auto runIt = attrString.iterate();
	float penX = 0, runWidth;
	while( runIt.nextRun() ) {
		const Font *font = runIt.getRunFont(); 
		font->lock();
		font->shapeString( runIt.getRunStrPtr(), runIt.getRunLength(), &glyphIndices, &glyphPositions, &runWidth );
		for( size_t i = 0; i < glyphIndices.size(); ++i ) {
			if( FT_Error err = FT_Load_Glyph( font->getFace()->getFtFace(), glyphIndices[i], FT_LOAD_DEFAULT ) )
				throw text::FreeTypeExc( err );
			if( FT_Error err = FT_Render_Glyph( font->getFace()->getFtFace()->glyph, FT_RENDER_MODE_NORMAL ) )
				throw text::FreeTypeExc( err );

			const FT_Bitmap &ftBitmap = font->getFace()->getFtFace()->glyph->bitmap;
			if( font->getFace()->getFtFace()->glyph->format == FT_GLYPH_FORMAT_BITMAP ) {
				Channel8u glyph( ftBitmap.width, ftBitmap.rows );
				glyph.copyFrom( ci::Channel8u( ftBitmap.width, ftBitmap.rows, ftBitmap.pitch, 1, ftBitmap.buffer ), glyph.getBounds() );
				auto offsetLeft = font->getFace()->getFtFace()->glyph->bitmap_left;
				auto offsetTop = font->getFace()->getFtFace()->glyph->bitmap_top;
				result.copyFrom( glyph, glyph.getBounds(), ivec2( (int32_t)glyphPositions[i] + penX, baseline - offsetTop ) - ivec2( -offsetLeft, 0 ) );
			}
		}
		font->unlock();
		penX += runWidth;
	}
	return result;
}

} } // namespace cinder::text
