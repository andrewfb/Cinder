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
  	template<typename T>
	struct Span {
		Span() {}
		Span( size_t start, size_t end, T value )
			: start( start ), end( end ), value( value ) {}
		Span( size_t start )
			: start( start ), end( start ) {}

		bool	isOpen() const { return start == end; }

		size_t start, end;  // [start,end) range
		
		T value;
		
		bool operator<(const Span &span) const { return start < span.start; }
	};


	friend AttrStringIter;

	struct Tracking {
		Tracking( float tracking ) : mTracking( tracking ) {}
		
		float	mTracking;
	};

	AttrString();
	AttrString( const std::string &utf8Str );

	AttrString& operator<<( const std::string &utf8Str );
	AttrString& operator<<( const char *utf8Str );
	AttrString& operator<<( Font *font );

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
  protected:
  	template<typename T>	void insertSpan( std::vector<Span<T>> *spans, const Span<T> &newSpan, size_t strLength );


	std::vector<Span<const Font*>> 	mFonts;
	std::vector<Span<ColorA>> 		mColorAs;
//	std::vector<std::pair<size_t,ColorA8u>>	mColors;
	
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

  	void 	updateOffset();
  	  	
	const AttrString	*mAttrStr;
	bool				mFirstRun;

	size_t 				fontIndex = 0; // the current index into fontspans. we might be before the beginning of this index.
	size_t 				colorAIndex = 0;


	ssize_t				mStrStartOffset, mStrEndOffset;
	const size_t		mStrLength;
	bool				mRunTrackingIsConstant;
	float				mRunTrackingValue;
	const Font 			*mFont;
	ColorA 				mColorA;
};
	
} } // namespace cinder::text



#if 0
class AttrStringIter {
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
};
	
} } // namespace cinder::text




#endif
