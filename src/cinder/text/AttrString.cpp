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
	os << ci::toUtf8( a.getStringUtf32 );
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

AttrString& AttrString::operator<<( Font *font )
{
	append( font );
	return *this;
}

AttrStringIter AttrString::iterate() const
{
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

void AttrString::append( Font *font )
{
	if( ! mFonts.empty() ) {
		CI_ASSERT( mFonts.back().first.second == -1 );
		mFonts.back().first.second = mString.size();
		mFonts.emplace_back( make_pair( mString.size(), -1 ), font );    
	}
	else { // first font needs to start at index 0 regardless
		mFonts.emplace_back( make_pair( 0, -1 ), font );
	}
}

void AttrString::appendTracking( float tracking )
{
	if( ! mTracking.empty() ) {
		CI_ASSERT( mTracking.back().first == -1 );
		mTracking.back().first = mString.size();    
	}
	mTracking.emplace_back( -1, tracking );	
}

/*void AttrString::append( const ColorA8u &color )
{
	if( ! mColors.empty() ) {
		CI_ASSERT( mColors.back().first == -1 );
		mColors.back().first = mString.size();    
	}
	mColors.emplace_back( -1, color );	
}*/

AttrStringIter::AttrStringIter( const AttrString *attrStr )
	: mAttrStr( attrStr ), mStrStartOffset( 0 ), mStrLength( attrStr->size() ), mFirstRun( true )
{
	mFont = mAttrStr->mFonts.empty() ? nullptr : mAttrStr->mFonts.front().second;

	mStrEndOffset = mStrLength;
	if( mAttrStr->mFonts.size() > 1 )
		mStrEndOffset = std::min<size_t>( mAttrStr->mFonts.front().first.second, mStrEndOffset );
}

void AttrStringIter::updateOffset()
{
}

std::string AttrStringIter::getRunUtf8() const
{
	return ci::toUtf8( &mAttrStr->mString[mStrStartOffset], ( mStrEndOffset - mStrStartOffset ) * 4 );
}

bool AttrStringIter::nextRun()
{
	if( mFirstRun ) {
		mFirstRun = false;
		return ! mAttrStr->empty();
	}
	
	mStrStartOffset	= mStrEndOffset;
	if( mStrStartOffset == mStrLength )
		return false;
	
	// find new end based on fonts
	// find start that is > current end
	size_t newEndOffset = mStrLength;
	for( size_t i = 0; i < mAttrStr->mFonts.size(); ++i ) {
		if( mAttrStr->mFonts[i].first.second == -1 ) { // unterminated, last font span 
			mFont = mAttrStr->mFonts[i].second; 
			newEndOffset = std::min<size_t>( newEndOffset, mStrLength );
			break;
		}
		else if( sum >= mStrEndOffset ) {
			mFont = mAttrStr->mFonts[i].second;
			newEndOffset = std::min<size_t>( newEndOffset, mAttrStr->mFonts[i].first );
			break;
		} 
	}

	mStrEndOffset = newEndOffset;
	
	return true;
}

/*template<typename S, typename V> vector<S> addSpan(vector<S>& input, size_t start, size_t end, V val) {
	vector<S> newVec;
	bool added = false;
	for (auto const &span : input) {
		if (start > span.end || end < span.start) { // no conflict with new span
			newVec.push_back(span);
		} else if (end >= span.end && start <= span.start) { // erased/replaced by new span
			// do nothing
		} else if (start > span.start && end < span.end) { // new span inside old span
			newVec.push_back({span.start,start-1,span.value});
			newVec.push_back({start,end,val});
			newVec.push_back({end+1,span.end,span.value});
			added = true;
		} else if (end < span.end) {
			if (!added) {
				newVec.push_back({start,end,val});
				added = true;
			}
			newVec.push_back({end+1,span.end,span.value});
		} else if (start > span.start) {
			newVec.push_back({span.start,start-1,span.value});
			if (!added) {
				newVec.push_back({start,end,val});
				added = true;
			}
		} else {
			assert(false); // keep this in here for debugging purposes
		}
		if (!added && start > span.end) {
			newVec.push_back({start,end,val});
			added = true;
		}
	}
	if (!added) newVec.push_back({start,end,val});
	return newVec;
}
	
template<typename S> vector<S> removeSpan(vector<S>&input, size_t start, size_t end) {
	vector<S> newVec;
	for (auto const &span : input) {
		if (start > span.end || end < span.start) {
			newVec.push_back(span);
		} else if (start <= span.start && end >= span.end) {
			// do nothing
		} else if (start > span.start && end < span.end) {
			newVec.push_back({span.start,start-1,span.value});
			newVec.push_back({end+1,span.end,span.value});
		} else if (end < span.end) {
			newVec.push_back({end+1,span.end,span.value});
		} else if (start > span.start) {
			newVec.push_back({span.start,start-1,span.value});
		} else {
			assert(false); // keep this in here for debugging purposes
		}
	}
	return newVec;
}
	
void AttrString::addFont(size_t start, size_t end, Font f) {
	mFonts = addSpan(mFonts, start, end, f);
}
	
void AttrString::removeFont(size_t start, size_t end) {
	mFonts = removeSpan(mFonts, start, end);
}
	
void AttrString::addColorA(size_t start, size_t end, ColorA c) {
	mColorAs = addSpan(mColorAs, start, end, c);
}

void AttrString::removeColorA(size_t start, size_t end) {
	mColorAs = removeSpan(mColorAs, start, end);
}

AttrString& operator << (AttrString &a, const std::string& s) {
	return a;
}

template <typename S, typename V>
V findSpan( size_t &nextIndex, const vector<S> &spans, size_t charIndex, size_t &spanIndex, V defaultVal )
{
	if( spans.size() > 0 && spanIndex < spans.size() ) {
		if( charIndex > spans[spanIndex].end )
			spanIndex++;
		if( spanIndex < spans.size() ) {
			auto const &span = spans[spanIndex];
			if( charIndex < span.start && span.start - 1 < nextIndex )
				nextIndex = span.start - 1;
			else if( span.end < nextIndex && span.end < nextIndex ) // TODO: the 2nd check is redundant. Did you mean something else?
				nextIndex = span.end;
			if( charIndex >= span.start )
				return span.value;
		}
	}
	return defaultVal;
}

bool AttrStringIter::next() {
	// at initial state, length is zero: first iteration doesn't need to move charIndex
	if (mStarted) {
		charIndex = charIndex + length;
	} else {
		mStarted = true;
	}
	
	if (mStr.mString.size() == 0 || charIndex > mStr.mString.size() - 1) return false;
	
	// the candidate for the next interval. start at the end.
	size_t nextIndex = mStr.mString.size()-1;
	
	colorA = findSpan(nextIndex,mStr.mColorAs,charIndex,colorAIndex,ColorA(1.,1.,1.,1.));
	font = findSpan(nextIndex,mStr.mFonts,charIndex,fontIndex,static_cast<const Font>(DefaultFont()));
	
	length = nextIndex - charIndex + 1;
	return true;
}
*/

} } // namespace cinder::text
