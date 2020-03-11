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
#include "cinder/Filesystem.h"
#include "cinder/text/Face.h"
#include "cinder/text/Font.h"
#include "cinder/Exception.h"

#include <vector>

typedef struct FT_LibraryRec_  	*FT_Library;


namespace cinder { namespace text {

class Manager {
  public:
	static Manager*		get();
	
	Face*				loadFace( const ci::fs::path &path, int faceIndex = 0 );
	Font*				loadFont( Face *face, float size );
	
	//! returns \c nullptr if Font has not been loaded
	Font*				findFont( const Face *face, float size ) const;
	
  private:
  	Manager();
  	~Manager();
  	
  	std::mutex							mFaceMutex;
  	std::vector<std::unique_ptr<Face>>	mFaces;
  	std::vector<std::unique_ptr<Font>>	mFonts;
	std::unique_ptr<FT_Library>			mLibraryPtr;
};

inline Face*		loadFace( const ci::fs::path &path, int faceIndex = 0 ) { return Manager::get()->loadFace( path, faceIndex ); }
inline Font*		loadFont( Face *face, float size ) { return Manager::get()->loadFont( face, size ); } 

class FreetypeExc : public Exception {
};

} } // namespace cinder::text
