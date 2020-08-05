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
#include "cinder/IntervalMap.h"


namespace cinder { namespace text {

class AttrStringIter;

class AttrString
{
  public:
	template<typename T>
	struct Span {
	   	Span( size_t start, size_t end, T value )
			   	: start( start ), end( end ), value( value ) {}
	   	Span( size_t start )
			   	: start( start ), end( start ) {}
	   	Span() {}
	
	   	bool    isOpen() const { return start == end; }
	
	   	size_t start, end;  // [start,end) range
	   	
	   	T value;
	   	
	   	bool operator<(const Span &span) const { return start < span.start; }
	   	bool operator==(const Span &span) const { return start == span.start && end == span.end && value == span.value; }
	   	bool operator!=(const Span &span) const { return start != span.start || end != span.end || value != span.value; }
	};
	typedef Span<const Font*>		FontSpan;

	friend AttrStringIter;

	struct Tracking {
		Tracking( float tracking ) : mTracking( tracking ) {}
		
		float	mTracking;
	};

	AttrString();
	AttrString( const std::string &utf8Str );

	AttrString& operator<<( const std::string &utf8Str );
	AttrString& operator<<( const char *utf8Str );
	AttrString& operator<<( const Font *font );

	void 	setFont( size_t start, size_t end, const Font *font );
	void 	setFont( const Font *font );
	void 	setColorA( size_t start, size_t end, ColorA s );

	void	append( const std::string &utf8Str );
	void	append( const char *utf8Str );
	void	append( Tracking tracking ) { appendTracking( tracking.mTracking ); }
	void	appendTracking( float tracking );
//	void	append( const ColorA8u &color );

	size_t	size() const { return mString.size(); }
	bool	empty() const { return mString.empty(); }

	AttrStringIter		iterate() const;

	const std::u32string&		getStringUtf32() const { return mString; }

	std::vector<FontSpan>		getFontSpans() const;
	void						setFontSpans( const std::vector<FontSpan> &spans );
  protected:
	mutable IntervalMap<const Font*> 		mFonts;
	mutable IntervalMap<ColorA> 			mColorAs;
//	std::vector<std::pair<size_t,ColorA8u>>	mColors;
	
	const Font*			mCurrentFont;
	mutable ssize_t		mCurrentFontStart = -1;
	ColorA				mCurrentColor;
	mutable ssize_t		mCurrentColorStart = -1;
	
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
	const Font*		getRunFont() { return mFont; }
//  	bool			getRunTrackingIsConstant() const { return mRunTrackingIsConstant; }
//  	void			getRunTrackingValue() const { return mRunTrackingValue; }
  	
  private:
  	AttrStringIter( const AttrString *attrStr );
	void			firstRun();
	void			advance();

	const AttrString	*mAttrStr;

	bool				mFirstRun;
	ssize_t				mStrStartOffset, mStrEndOffset;
	const size_t		mStrLength;

	IntervalMap<const Font*>::ConstMapIter		mFontIter;
	const Font*									mFont;
};
	
} } // namespace cinder::text
