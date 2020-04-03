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

#include <string>
#include <vector>
#include <utility>

#include "cinder/Color.h"
#include "cinder/text/Font.h"


namespace cinder { namespace text {

class AttrStringIter;

class AttrString
{
  public:
	friend AttrStringIter;

	AttrString();
	AttrString( const std::string &utf8Str );

	void	append( const std::string &utf8Str );
	void	append( Font *font );

	size_t	size() const { return mString.size(); }
	bool	empty() const { return mString.empty(); }

	AttrStringIter		iterate() const;

	const std::u32string&		getStringUtf32() const { return mString; }

	//  inclusive of the end index.
/*	void addFont( size_t start, size_t end, Font s );
	void removeFont( size_t start, size_t end );
	void addColorA( size_t start, size_t end, ColorA s );
	void removeColorA( size_t start, size_t end );
	size_t size() const { return mString.size(); }                                    // Returns the size of the string in codepoints of 4 bytes each.
	bool   empty() const { return mString.empty(); }

	// a pair of <startIndex, endIndex, value> describing intervals where an attribute holds.
	// not necessarily contiguous - blank indicates default.
	// can be empty (the default).
	// Invariants:
	// intervals do not overlap.
	// intervals in ascending order.
	std::vector<FontSpan> mFonts;
	std::vector<ColorASpan> mColorAs;
	//std::vector<FloatSpan> mLineHeights;
	// Tracking should be treated as a special case.
	// Because tracking is a between-glyphs setting, we should subtract one from the given range.
	// extra tracking from index 1 to 3 should be a b  c  d e and not a b  c  d  e.
	//std::vector<FloatSpan> mTrackings;
	//std::vector<FloatSpan> mSdfWeights;*/
	
	// pair<end of span, span value>
	std::vector<std::pair<size_t,Font*>>	mFonts;
	
	std::u32string 		mString;
};
	
std::ostream& operator<<( std::ostream& os, const AttrString& dt );

class AttrStringIter {
	friend AttrString;

  public:
	bool	nextRun();
	
	ssize_t			getRunLength() const { return mStrEndOffset - mStrStartOffset; }
	const char32_t*	getRunStrPtr() { return &mAttrStr->mString[mStrStartOffset]; }
	std::string		getRunUtf8() const;
	Font*			getRunFont() { return mFont; }
  	
  private:
  	AttrStringIter( const AttrString *attrStr );
  	  	
	const AttrString	*mAttrStr;
	bool				mFirstRun;
	ssize_t				mStrStartOffset, mStrEndOffset;
	size_t				mStrLength;
	Font 				*mFont;
};

/*class AttrStringIter {
public:
	AttrStringIter(const AttrString& s) : mStr(s) {}

	bool next();
	
	Font font = DefaultFont();
	ColorA colorA = ColorA();
	size_t charIndex = 0;
	size_t length = 0;
	
private:
	const AttrString &mStr;
	bool mStarted = false;
	
	size_t fontIndex = 0; // the current index into fontspans. we might be before the beginning of this index.
	size_t colorAIndex = 0;
};*/
	
} } // namespace cinder::text



