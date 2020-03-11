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
#include "cinder/text/Font.h"
#include "cinder/text/Face.h"

#include <string>

#include <freetype/ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftsizes.h>

using namespace std;

namespace cinder { namespace text {

Font::Font( Face *face, float size )
	: mFace( face ), mSize( size )
{
	FT_New_Size( face->getFtFace(), &mFtSize );
	
	face->lock();
	FT_Activate_Size( mFtSize );
	FT_Set_Char_Size( face->getFtFace(), (FT_F26Dot6)(0), (FT_F26Dot6)(size * 64), 72, 72 );
	
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

Font::~Font()
{
	FT_Done_Size( mFtSize );
}

} } // namespace cinder::text
