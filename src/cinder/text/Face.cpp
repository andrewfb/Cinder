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
#include "cinder/text/Face.h"

#include <string>

#include <freetype/ft2build.h>
#include <freetype/freetype.h>
#include <hb.h>
#include <hb-ft.h>

using namespace std;

namespace cinder { namespace text {

Face::Face( FT_Face face )
	: mFtFace( face )
{
	mHbFace = hb_ft_face_create_referenced( face );
	if( ! mHbFace )
		throw HarfBuzzExc(); 
}

Face::~Face()
{
	hb_face_destroy( mHbFace );
	FT_Done_Face( mFtFace );
}

int32_t Face::getNumGlyphs() const
{
	return (int32_t)mFtFace->num_glyphs;
}

uint32_t Face::getCharIndex( uint32_t utf32Char ) const
{
	return FT_Get_Char_Index( mFtFace, utf32Char );
}

std::string Face::getFamilyName() const
{
	return std::string( mFtFace->family_name );
}

std::string	Face::getStyleName() const
{
	return std::string( mFtFace->style_name );
}

bool Face::hasColor() const
{
	return (mFtFace->face_flags & FT_FACE_FLAG_COLOR) != 0;
}

vector<int32_t> Face::getFixedSizes() const
{
	vector<int32_t> result;
	for( FT_Int i = 0; i < mFtFace->num_fixed_sizes; ++i ) {
		FT_Bitmap_Size *f = &mFtFace->available_sizes[i]; 
		result.push_back( f->height );
	}
	
	return result;
}

} } // namespace cinder::text
