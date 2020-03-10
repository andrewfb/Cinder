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
#include "cinder/Filesystem.h"
#include "cinder/text/Text.h"
#include <memory>

#include <freetype/ft2build.h>
#include <freetype/freetype.h>

using namespace std;

namespace cinder { namespace text {

Manager* Manager::get()
{
	static Manager instance;
	return &instance;
}

Manager::Manager()
{
	mLibraryPtr = make_unique<FT_Library>();
	FT_Init_FreeType( mLibraryPtr.get() );
}

Manager::~Manager()
{
	FT_Done_FreeType( *mLibraryPtr );
}

Face* Manager::loadFace( const ci::fs::path &path, int faceIndex )
{
	FT_Face ftFace;
	FT_Error error = FT_New_Face( *mLibraryPtr, path.c_str(), faceIndex, &ftFace );
	if( error )
	 	throw FreetypeExc();
	
	{
		lock_guard<mutex> lock( mFaceMutex );
		mFaces.emplace_back( new Face( ftFace ) );
	}
	
	return mFaces.back().get();
}

} } // namespace cinder::text
