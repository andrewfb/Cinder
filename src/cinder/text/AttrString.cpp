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

#include "cinder/text/AttrString.h"
#include "cinder/Unicode.h"
#include "cinder/CinderAssert.h"
#include "cinder/Log.h"

using namespace std;

namespace cinder { namespace text {


AttrString::AttrString() {
}
	
AttrString::AttrString( const string &utf8Str )
{
	mString = ci::toUtf32( utf8Str );
}
	
ostream& operator<<( ostream& os, const AttrString& a )
{
	os << ci::toUtf8( a.getStringUtf32() );
	return os;
}

AttrString& AttrString::operator<<( const std::string &utf8Str )
{
	append( utf8Str );
	return *this;
}

AttrString& AttrString::operator<<( const char *utf8Str )
{
	append( utf8Str );
	return *this;
}

AttrString& AttrString::operator<<( const Font *font )
{
	setFont( font );
	return *this;
}

AttrStringIter AttrString::iterate() const
{
	// terminate current Font span
	if( mCurrentFontStart >= 0 )
		mFonts.set( mCurrentFontStart, mString.size(), mCurrentFont ); 	
	mCurrentFontStart = -1;
	
	return AttrStringIter( this );
}

void AttrString::append( const string &utf8Str )
{
	mString.append( ci::toUtf32( utf8Str ) );
}

void AttrString::append( const char *utf8Str )
{
	mString.append( ci::toUtf32( utf8Str ) );
}

void AttrString::setFont( const Font *font )
{
	// terminate current Font span
	if( mCurrentFontStart >= 0 )
		mFonts.set( mCurrentFontStart, mString.size(), mCurrentFont ); 
	
	mCurrentFont = font;
	mCurrentFontStart = mString.size();
}

void AttrString::setFont( size_t start, size_t end, const Font* f )
{
	mFonts.set( start, end, f );
}

std::vector<AttrString::FontSpan> AttrString::getFontSpans() const
{
	std::vector<AttrString::FontSpan> result;
	
	for( auto &spanIt : mFonts.getIntervals() )
		result.push_back( FontSpan( spanIt.first, spanIt.second.limit, spanIt.second.value ) );
	
	return result;
}

void AttrString::setFontSpans( const std::vector<FontSpan> &spans )
{
	mFonts.clear();
	for( auto &s : spans )
	 	mFonts.set( s.start, s.end, s.value );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

AttrStringIter::AttrStringIter( const AttrString *attrStr )
	: mAttrStr( attrStr ), mStrStartOffset( 0 ), mStrLength( attrStr->size() ), mFirstRun( true )
{
}

std::string AttrStringIter::getRunUtf8() const
{
	return ci::toUtf8( &mAttrStr->mString[mStrStartOffset], ( mStrEndOffset - mStrStartOffset ) * 4 );
}

void AttrStringIter::firstRun()
{
	size_t fontEnd;
	if( mAttrStr->mFonts.empty() ) {
		mFontDefault = true;
		fontEnd = mStrLength;
		mFont = nullptr;
	}
	else {
		mFontIter = mAttrStr->mFonts.begin();
		if( mFontIter->first > 0 ) {
			mFontDefault = true;
			fontEnd = mFontIter->first;
		}
		else {
			mFontDefault = false;
			fontEnd = mFontIter->second.limit;
		}
		mFont = mFontIter->second.value;
	}
	
	mStrEndOffset = fontEnd;
}

// move all iterators forward so that their starts >= mStrStartOffset
void AttrStringIter::advance()
{
	size_t newStrEnd = mStrLength;
	mStrStartOffset = mStrEndOffset;
	if( ! mAttrStr->mFonts.empty() && mFontIter != mAttrStr->mFonts.end() ) {
		if( mStrStartOffset < mFontIter->first ) { // still before this span
			CI_ASSERT( mFontDefault );
		}
		else if( mStrStartOffset < mFontIter->second.limit ) { // inside this span
//			s = mFontIter->second.limit; 
//			CI_ASSERT( ! mFontDefault );
		}
		else {
			++mFontIter;
			if( mFontIter == mAttrStr->mFonts.end() ) { // hit the end of the fonts
				mFontDefault = true;
				mFont = nullptr;
			}
			if( mStrStartOffset < mFontIter->first ) {
				mFontDefault = true;
				newStrEnd = std::min( newStrEnd, mFontIter->first );
				mFont = nullptr; 
			}
			else {
				mFontDefault = false;
				mFont = mFontIter->second.value;
				newStrEnd = std::min( newStrEnd, mFontIter->second.limit );
			}
		}
	}
	
	mStrEndOffset = newStrEnd;
	mFont = mFontIter->second.value;
}

bool AttrStringIter::nextRun()
{
	if( mFirstRun ) {
		firstRun();
		mFirstRun = false;
		return mStrEndOffset > 0;
	}

	if( mStrEndOffset == mStrLength )
		return false;
	
	advance();
	
	return true;
}

} } // namespace cinder::text
