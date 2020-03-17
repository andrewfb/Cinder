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
#include "cinder/text/Text.h"
#include "cinder/text/Font.h"
#include "cinder/text/Face.h"
#include "cinder/Channel.h"
#include "cinder/Shape2d.h"

#include <string>

#include <freetype/ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <freetype/ftsizes.h>

#include <hb.h>
#include <hb-ft.h>

using namespace std;
using namespace cinder;

namespace cinder { namespace text {

Font::Font( Face *face, float size )
	: mFace( face ), mSize( size )
{
	FT_New_Size( face->getFtFace(), &mFtSize );
	
	face->lock();
	FT_Activate_Size( mFtSize );
	FT_Set_Char_Size( face->getFtFace(), (FT_F26Dot6)(0), (FT_F26Dot6)(size * 64), 72, 72 );	
	
	mHbFont = hb_font_create( face->getHbFace() );
	hb_font_set_scale( mHbFont,
		(int)(((uint64_t)face->getFtFace()->size->metrics.x_scale * (uint64_t)face->getFtFace()->units_per_EM + (1u << 15)) >> 16),
		(int)(((uint64_t)face->getFtFace()->size->metrics.y_scale * (uint64_t)face->getFtFace()->units_per_EM + (1u << 15)) >> 16));
	hb_ft_font_set_funcs( mHbFont );
	
	face->unlock();
}

float Font::getAscender() const
{
	return mFtSize->metrics.ascender / 64.0f;
}

float Font::getDescender() const
{
	return mFtSize->metrics.descender / 64.0f;
}

float Font::getHeight() const
{
	return mFtSize->metrics.height / 64.0f;
}

inline ci::Channel8u wrapBitmap( const FT_Bitmap &bitmap )
{
	return ci::Channel8u( bitmap.width, bitmap.rows, bitmap.pitch, 1, bitmap.buffer ); 
}

cinder::Channel8u Font::getGlyphBitmap( uint32_t glyphIndex ) const
{
	mFace->lock();
	FT_Activate_Size( mFtSize );
	if( FT_Error err = FT_Load_Glyph( mFace->getFtFace(), glyphIndex, FT_LOAD_NO_SCALE | FT_LOAD_COLOR | FT_LOAD_DEFAULT | FT_LOAD_RENDER ) )
		throw text::FreeTypeExc( err );
	if( FT_Error err = FT_Render_Glyph( mFace->getFtFace()->glyph, FT_RENDER_MODE_NORMAL ) )
		throw text::FreeTypeExc( err );

    const FT_Bitmap &ftBitmap = mFace->getFtFace()->glyph->bitmap; 
	//Channel8u result( ftBitmap.width - mFace->getFtFace()->glyph->bitmap_left, ftBitmap.rows - mFace->getFtFace()->glyph->bitmap_top );  
	//result.copyFrom( wrapBitmap( ftBitmap ), result.getBounds(), ivec2( mFace->getFtFace()->glyph->bitmap_left, mFace->getFtFace()->glyph->bitmap_top ) );
	Channel8u result( ftBitmap.width, ftBitmap.rows );
	result.copyFrom( ci::Channel8u( ftBitmap.width, ftBitmap.rows, ftBitmap.pitch, 1, ftBitmap.buffer ), result.getBounds() ); 
	mFace->unlock();
	
	return result;
}

static int ftShape2dMoveTo(const FT_Vector *to, void *user)
{
	Shape2d *shape = reinterpret_cast<Shape2d*>(user);
	shape->moveTo((float)to->x / 4096.f, (float)to->y / 4096.f);
	return 0;
}

static int ftShape2dLineTo(const FT_Vector *to, void *user)
{
	Shape2d *shape = reinterpret_cast<Shape2d*>(user);
	shape->lineTo((float)to->x / 4096.f, (float)to->y / 4096.f);
	return 0;
}

static int ftShape2dConicTo(const FT_Vector *control, const FT_Vector *to, void *user)
{
	Shape2d *shape = reinterpret_cast<Shape2d*>(user);
	shape->quadTo((float)control->x / 4096.f, (float)control->y / 4096.f, (float)to->x / 4096.f, (float)to->y / 4096.f);
	return 0;
}

static int ftShape2dCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user)
{
	Shape2d *shape = reinterpret_cast<Shape2d*>(user);
	shape->curveTo((float)control1->x / 4096.f, (float)control1->y / 4096.f, (float)control2->x / 4096.f, (float)control2->y / 4096.f, (float)to->x / 4096.f, (float)to->y / 4096.f);
	return 0;
}

cinder::Shape2d	Font::getGlyphShape( uint32_t glyphIndex ) const
{
	mFace->lock();
	FT_Activate_Size( mFtSize );

	FT_Load_Glyph( mFace->getFtFace(), glyphIndex, FT_LOAD_DEFAULT );
	FT_Outline outline = mFace->getFtFace()->glyph->outline;
	FT_Outline_Funcs funcs;
	funcs.move_to = ftShape2dMoveTo;
	funcs.line_to = ftShape2dLineTo;
	funcs.conic_to = ftShape2dConicTo;
	funcs.cubic_to = ftShape2dCubicTo;
	funcs.shift = 6;
	funcs.delta = 0;

	Shape2d result;
	FT_Outline_Decompose( &outline, &funcs, &result );
	if( result.getNumContours() )
		result.close();
	result.scale(vec2(1, -1));

	mFace->unlock();
	
	return result;
}

float Font::calcStringWidth( const char *utf8String ) const
{
	hb_buffer_t *buf = hb_buffer_create();

	// Set buffer to LTR direction, common script and default language
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
	hb_buffer_set_language(buf, hb_language_get_default());

	// Add text and layout it
	hb_buffer_add_utf8( buf, utf8String, -1, 0, -1 );

	mFace->lock();
	FT_Activate_Size( mFtSize );
	hb_shape( mHbFont, buf, nullptr, 0 );
	mFace->unlock();

	// Get buffer data
	unsigned int        glyph_count = hb_buffer_get_length( buf );
	hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, NULL);

	unsigned int string_width_in_pixels = 0;
	for (int i = 0; i < glyph_count; ++i) {
		string_width_in_pixels += glyph_pos[i].x_advance / 64.0;
	}
	
	hb_buffer_destroy( buf );
	
	return string_width_in_pixels;
}

void Font::shapeString( const char *utf8String, std::vector<uint32_t> *outGlyphIndices, std::vector<float> *outGlyphPositions ) const
{
	hb_buffer_t *buf = hb_buffer_create();

	// Set buffer to LTR direction, common script and default language
	hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
	hb_buffer_set_script( buf, HB_SCRIPT_COMMON );
	hb_buffer_set_language( buf, hb_language_get_default() );

	// Add text and layout it
	hb_buffer_add_utf8( buf, utf8String, -1, 0, -1 );
	
	shapeBuffer( buf, outGlyphIndices, outGlyphPositions );
	
	hb_buffer_destroy( buf );
}  

void Font::shapeString( const uint32_t *utf32String, std::vector<uint32_t> *outGlyphIndices, std::vector<float> *outGlyphPositions ) const
{
	hb_buffer_t *buf = hb_buffer_create();

	// Set buffer to LTR direction, common script and default language
	hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
	hb_buffer_set_script( buf, HB_SCRIPT_COMMON );
	hb_buffer_set_language( buf, hb_language_get_default() );

	// Add text and layout it
	hb_buffer_add_utf32( buf, utf32String, -1, 0, -1 );
	
	shapeBuffer( buf, outGlyphIndices, outGlyphPositions );
	
	hb_buffer_destroy( buf );
}  

void Font::shapeBuffer( hb_buffer_t *buf, std::vector<uint32_t> *outGlyphIndices, std::vector<float> *outGlyphPositions ) const
{
	mFace->lock();
	FT_Activate_Size( mFtSize );
	hb_shape( mHbFont, buf, nullptr, 0 );
	mFace->unlock();

	// Get buffer data
	unsigned int        glyphCount = hb_buffer_get_length( buf );
	
	if( outGlyphIndices ) {
		const hb_glyph_info_t *glyph_infos = hb_buffer_get_glyph_infos( buf, nullptr );
		outGlyphIndices->resize( glyphCount );
		for( auto g = 0; g < glyphCount; ++g )
			(*outGlyphIndices)[g] = glyph_infos[g].codepoint;
	} 

	if( outGlyphIndices ) {
		const hb_glyph_position_t *glyph_positions = hb_buffer_get_glyph_positions( buf, nullptr );
		outGlyphPositions->resize( glyphCount );
		double offset = 0;
		for( auto g = 0; g < glyphCount; ++g ) {
			(*outGlyphPositions)[g] = (float)offset;
			offset += glyph_positions[g].x_advance / 64.0;
		}
	}
}

Font::~Font()
{
	hb_font_destroy( mHbFont );
	FT_Done_Size( mFtSize );
}

} } // namespace cinder::text
