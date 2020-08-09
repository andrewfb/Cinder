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

AttrString& AttrString::operator<<( Tracking tracking )
{
	setCurrentTracking( tracking );
	return *this;
}

AttrStringIter AttrString::iterate() const
{
	return AttrStringIter( this );
}

void AttrString::append( const string &utf8Str )
{
	mString.append( ci::toUtf32( utf8Str ) );
	if( mCurrentTrackingActive )
		mTrackings.setEndLimit( mString.size() );
}

void AttrString::append( const char *utf8Str )
{
	mString.append( ci::toUtf32( utf8Str ) );
	if( mCurrentTrackingActive )
		mTrackings.setEndLimit( mString.size() );
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

void AttrString::setCurrentTracking( Tracking tracking )
{
	if( ! tracking.isDefault() ) {
		mTrackings.set( mString.size(), mString.size(), tracking ); 
		mCurrentTrackingActive = true;
	}
	else
		mCurrentTrackingActive = false;
}

void AttrString::setTracking( size_t start, size_t end, Tracking tracking )
{
	if( tracking.isDefault() )
		mTrackings.clearInterval( start, end );
	else
		mTrackings.set( start, end, tracking ); 
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

void AttrString::printDebug()
{
	auto runIt = iterate();
	while( runIt.nextRun() ) {
		float tracking = runIt.getRunTracking();
		std::cout << "{ '" << runIt.getRunUtf8() << "' Font:" << *runIt.getRunFont() << " Tracking: " << tracking << std::endl;
	}
	std::cout << std::endl; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

AttrStringIter::AttrStringIter( const AttrString *attrStr )
	: mAttrStr( attrStr ), mStrStartOffset( 0 ), mStrLength( attrStr->size() ), mFirstRun( true ), mFontsDone( false )
{
}

std::string AttrStringIter::getRunUtf8() const
{
	return ci::toUtf8( &mAttrStr->mString[mStrStartOffset], ( mStrEndOffset - mStrStartOffset ) * 4 );
}

void AttrStringIter::firstRun()
{
	mStrEndOffset = mStrLength;

	// font
	size_t fontEnd;
	if( mAttrStr->mFonts.empty() ) { // no fonts means default
		mFont = nullptr;
		fontEnd = mStrLength;
		mFontsDone = true;
	}
	else {
		mFontIter = mAttrStr->mFonts.begin();
		if( mFontIter->first > 0 ) { // first font span starts after start of string
			mFont = nullptr;
			fontEnd = mFontIter->first;
		}
		else {
			mFont = mFontIter->second.value;
			fontEnd = mFontIter->second.limit;
		}
	}	
	mStrEndOffset = std::min( mStrEndOffset, fontEnd );
	
	// tracking
	size_t trackingEnd;
	if( mAttrStr->mTrackings.empty() ) { // no trackings means default
		mTracking = AttrString::Tracking();
		trackingEnd = mStrLength;
		mTrackingsDone = true;
	}
	else {
		mTrackingIter = mAttrStr->mTrackings.begin();
		if( mTrackingIter->first > 0 ) { // first tracking span starts after start of string
			mTracking = AttrString::Tracking();
			trackingEnd = mTrackingIter->first;
		}
		else {
			mTracking = mTrackingIter->second.value;
			trackingEnd = mTrackingIter->second.limit;
		}
	}	
	mStrEndOffset = std::min( mStrEndOffset, trackingEnd );
}

// move all iterators forward so that their starts >= mStrStartOffset
void AttrStringIter::advance()
{
	size_t newStrEnd = mStrLength;
	mStrStartOffset = mStrEndOffset;

	// font
	if( ! mFontsDone ) {
		size_t fontSpanStart, fontSpanEnd;
		const Font *fontSpanFont = nullptr;
		
		if( mStrStartOffset >= mFontIter->second.limit ) // time to increment
			++mFontIter;

		if( mFontIter == mAttrStr->mFonts.end() ) { // hit the end of the fonts interval map; see if we have an active font on the AttrString
			if( mAttrStr->mCurrentFontStart >= 0 ) {
				fontSpanStart = mAttrStr->mCurrentFontStart;
				fontSpanEnd = mStrLength;
				fontSpanFont = mAttrStr->mCurrentFont;
			}
			else {
				mFontsDone = true;
				fontSpanStart = mStrLength; // will fail range test
			}
		}
		else {
			fontSpanStart = mFontIter->first;
			fontSpanEnd = mFontIter->second.limit;
			fontSpanFont = mFontIter->second.value;
		}

		if( mStrStartOffset < fontSpanStart ) { // still before this span
			mFont = nullptr;
		}
		else {
			CI_ASSERT( mStrStartOffset <= fontSpanEnd );
			mFont = fontSpanFont;
			newStrEnd = std::min( newStrEnd, fontSpanEnd );
		}
	}

	if( ! mTrackingsDone ) {
		size_t trackingSpanStart, trackingSpanEnd;
		AttrString::Tracking trackingSpanTracking;
		
		if( mStrStartOffset >= mTrackingIter->second.limit ) // time to increment
			++mTrackingIter;

		if( mTrackingIter == mAttrStr->mTrackings.end() ) { // hit the end of the tracking interval map; see if we have an active tracking on the AttrString
			mTrackingsDone = true;
			trackingSpanStart = mStrLength; // will fail range test
		}
		else {
			trackingSpanStart = mTrackingIter->first;
			trackingSpanEnd = mTrackingIter->second.limit;
			trackingSpanTracking = mTrackingIter->second.value;
		}

		if( mStrStartOffset < trackingSpanStart ) { // still before this span
			mTracking = AttrString::Tracking();
		}
		else {
			CI_ASSERT( mStrStartOffset <= trackingSpanEnd );
			mTracking = trackingSpanTracking;
			newStrEnd = std::min( newStrEnd, trackingSpanEnd );
		}
	}
	
	mStrEndOffset = newStrEnd;
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
