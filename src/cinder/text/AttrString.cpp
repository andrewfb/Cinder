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

namespace {
// handle the span a newSpan intersects with on its "left" - its predecessor
template<typename T>
void insertSpanHandlePred( vector<typename AttrString::Span<T>> *spans, typename std::vector<AttrString::Span<T>>::iterator pred, const AttrString::Span<T> &newSpan, size_t strLength )
{
	size_t predEnd = pred->isOpen() ? strLength : pred->end; // implicitly terminate an open span
	if( newSpan.end <= pred->start ) { // entirely before pred
		spans->insert( pred, newSpan );
	}
	else { // intersecting first span; could be exact match, could be new entirely inside old, could be
		CI_ASSERT( newSpan.end > pred->start );
		if( newSpan.start == pred->start && newSpan.end == predEnd ) { // exactly matches pred; replace value
			pred->value = newSpan.value;
		}
		else if( newSpan.start == pred->start && newSpan.end < predEnd ) { // start matches pred, end is within
			if( pred->isOpen() )
				pred->start = pred->end = newSpan.start;
			else
				pred->start = newSpan.start;
			spans->insert( pred, newSpan );
		}
		else if( newSpan.start > pred->start ) { // new contained within pred
			if( newSpan.end < predEnd ) {
				// divide pred around new
				pred->end = newSpan.start;
				auto right = spans->insert( spans->end(), AttrString::Span<T>( newSpan.end, predEnd, pred->value ) );
				spans->insert( right, newSpan );
			}
			else { // new start is within pred, and ends match
				size_t prevPredStart = pred->start;
				if( pred->isOpen() )
					pred->start = pred->end = newSpan.end;
				else
					pred->start = newSpan.end;
				auto rep = spans->insert( pred, newSpan );
				spans->insert( rep, AttrString::Span<T>( prevPredStart, newSpan.start, pred->value ) );
			}
		}
	}
}

} // anonymous namespace

template<typename T>
void AttrString::insertSpan( vector<Span<T>> *spans, const Span<T> &newSpan, size_t strLength )
{
	CI_ASSERT( newSpan.start <= newSpan.end ); 
	
	if( spans->empty() ) {
		spans->push_back( newSpan );
		return;
	}

	typename vector<Span<T>>::iterator succ = std::lower_bound( spans->begin(), spans->end(), newSpan ); // span whose start > newSpan.start
	if( succ != spans->begin() ) // we have a predecessor; handle it
		insertSpanHandlePred( spans, succ - 1, newSpan, strLength );
		
/*	else if( successor == spans->begin() ) { // we are <= the first item, so no predecessor
	 	// +-new-+        intersects with successor
	 	//    +-suc-+
		if( newSpan.end > successor->start && newSpan.end < successor->end )
			successor->start = newSpan.end;
		// +---new---+      supersedes successor
		//   +-suc-+
		else if( newSpan.start < successor->start && newSpan.end > successor->end )
		 	*successor = newSpan;
		else // no overlap with successor
			spans->insert( spans->begin(), newSpan );
	}
	else {
		auto predecessor = successor - 1; // span whose start preceds newSpan.start
		//  +-new-+    newSpan trims off the end of predecessor
		// +-pre-+
		if( predecessor->start < newSpan.start && predecessor->end <= newSpan.end )
			predecessor->end = newSpan.start;
		// ( +-new-+ )   newSpan contained within predecessor; needs to generate 0, 1, or 2 spans
		// +---pre---+
		else if( predecessor->start <= newSpan.start && predecessor->end >= newSpan.end ) {
	//		if( 
		}
	}*/
}
	
template<typename S> vector<S>
removeSpan(vector<S>&input, size_t start, size_t end) {
/*	vector<S> newVec;
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
	return newVec;*/
}

	
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

AttrString& AttrString::operator<<( Font *font )
{
	setFont( font );
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

void AttrString::setFont( const Font *font )
{
	// terminate last Font span assuming it's open
	if( ! mFonts.empty() && mFonts.back().isOpen() )
		mFonts.back().end = mString.size();
	
	mFonts.push_back( Span<const Font*>( mString.size(), mString.size(), font ) );
}

void AttrString::setFont( size_t start, size_t end, const Font* f )
{
	insertSpan( &mFonts, Span<const Font*>( start, end, f ), mString.size() );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

AttrStringIter::AttrStringIter( const AttrString *attrStr )
	: mAttrStr( attrStr ), mStrStartOffset( 0 ), mStrLength( attrStr->size() ), mFirstRun( true )
{
	if( mAttrStr->mFonts.empty() || mAttrStr->mFonts.front().start > 0 ) {
		mFont = nullptr;
		mStrEndOffset = mStrLength;
	}
	else {
		mFont = mAttrStr->mFonts.front().value;
		mStrEndOffset = mAttrStr->mFonts.front().end;
	}
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
	else if( mStrEndOffset == mStrLength )
		return false;
	
	mStrStartOffset	= mStrEndOffset;
	if( mStrStartOffset == mStrLength )
		return false;
	
	// find new end based on fonts
	// find start that is > current end
	size_t newEndOffset;
	auto fontIt = std::lower_bound( mAttrStr->mFonts.begin(), mAttrStr->mFonts.end(), AttrString::Span<const Font*>( mStrStartOffset ) );
	if( fontIt == mAttrStr->mFonts.end() ) { // we're past the last font - just use the last font
		mFont = ( mAttrStr->mFonts.empty() ) ? nullptr : mAttrStr->mFonts.back().value;
		newEndOffset = mStrLength;
	}
	else if( fontIt->start == mStrStartOffset ) { // this run starts exactly at the font span
		mFont = fontIt->value;
		if( fontIt->isOpen() )
			newEndOffset = mStrLength;
		else
			newEndOffset = fontIt->end;
	}
	else if( fontIt == mAttrStr->mFonts.begin() ) { // first font starts after this
		mFont = nullptr;
		newEndOffset = fontIt->start;
	}
	else { // normal case; fontIt points to span just after this
		fontIt = fontIt - 1;
		mFont = fontIt->value;
		if( fontIt->isOpen() )
			newEndOffset = mStrLength;
		else
			newEndOffset = fontIt->end;
	}

	mStrEndOffset = newEndOffset;
	
	return true;
}

/*
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
