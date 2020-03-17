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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/text/Face.h"

typedef struct FT_FaceRec_		*FT_Face;
typedef struct FT_SizeRec_		*FT_Size;

namespace cinder {
	template<typename T> 		class ChannelT;
	typedef ChannelT<uint8_t>	Channel8u;
	class Shape2d;
} // cinder forward declrations

namespace cinder { namespace text {

class Manager;

class Font {
  public:
	~Font();

	Face*		getFace() { return mFace; }

	//! Text ascender in pixels
	float		getAscender() const;
	//! Text descender in pixels
	float		getDescender() const;
	//! Text height in pixels
	float		getHeight() const;
  	
	//! Returns a font-relative index for UTF-32 codepoint \a utf32Char. Returns \c 0 if the font cannot represent \a utf32Char. Passes through to Face::getCharIndex()
	uint32_t		getCharIndex( uint32_t utf32Char ) const { return mFace->getCharIndex( utf32Char ); }
  	
	//! Returns a Channel8u containing the rasterized glyph \a glyphIndex. Note that this index is not a Unicode codepoint, and can be obtained with \a getCharIndex().
	cinder::Channel8u		getGlyphBitmap( uint32_t glyphIndex ) const;

	//! Returns a Shape2d containing the outline for glyph \a glyphIndex. Note that this index is not a Unicode codepoint, and can be obtained with \a getCharIndex().
	cinder::Shape2d			getGlyphShape( uint32_t glyphIndex ) const;
  	
  private:
	Font( Face *face, float size );

	//! Returns size as 26.6 fixed point
  	int32_t		getDiscreteSize() const { return static_cast<int32_t>( mSize * 64 ); }
	
	Face		*mFace;
	FT_Size		mFtSize;
	float		mSize;
	
	friend Manager;
};

} } // namespace cinder::text