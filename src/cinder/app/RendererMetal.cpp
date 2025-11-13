/*
 Copyright (c) 2025, The Cinder Project, All rights reserved.

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

#include "cinder/app/RendererMetal.h"
#include <glfw/glfw3.h>

#if defined( CINDER_MAC )

//#include "cinder/app/glfw/RendererImplGlfw.h"
#include "cinder/app/glfw/RendererImplGlfwMetal.h"

namespace cinder { namespace app {

RendererMetal::RendererMetal( const RendererMetal::Options &options )
	: Renderer(), mImpl( nullptr ), mOptions( options )
{}

RendererMetal::RendererMetal( const RendererMetal &renderer )
	: Renderer( renderer ), mImpl( nullptr ), mOptions( renderer.mOptions )
{}

RendererMetal::~RendererMetal()
{
	delete mImpl;
}

void RendererMetal::setup( void* nativeWindow, RendererRef sharedRenderer )
{
	if( ! mImpl ) {
		mImpl = new RendererImplGlfwMetal( this );
	}

	if( ! mImpl->initialize( (GLFWwindow*)nativeWindow, sharedRenderer ) ) {
		throw ExcRendererAllocation( "RendererMetalGlfw initialization failed." );
	}
}

RendererImplGlfw* RendererMetal::getGlfwRendererImpl()
{
	return mImpl;
}

void RendererMetal::startDraw()
{
	if( mStartDrawFn ) {
		mStartDrawFn( this );
	}
	else {
		// Begin a new Metal frame
		if( mImpl )
            mImpl->beginFrame();
	}
}

void RendererMetal::makeCurrentContext( bool force )
{
	mImpl->makeCurrentContext( force );
}

void RendererMetal::swapBuffers()
{
	mImpl->swapBuffers();
}

void RendererMetal::finishDraw()
{
	if( mFinishDrawFn ) {
		mFinishDrawFn( this );
	}
	else {
		// End the Metal frame and present
		if( mImpl )
            mImpl->endFrame();
	}
}

void RendererMetal::defaultResize()
{
	mImpl->defaultResize();
}

void* RendererMetal::getMetalDevice() const
{
	if( mImpl )
        return mImpl->getMetalDevice();
	return nullptr;
}

void* RendererMetal::getMetalCommandQueue() const
{
	if( mImpl )
		return mImpl->getMetalCommandQueue();

	return nullptr;
}

void* RendererMetal::getMetalLayer() const
{
    if( mImpl )
        return mImpl->getMetalLayer();

    return nullptr;
}

Surface8u RendererMetal::copyWindowSurface( const Area &area, int32_t windowHeightPixels )
{
	// TODO: Implement Metal surface capture
	// For now, return an empty surface
	Surface8u s( area.getWidth(), area.getHeight(), false );
	return s;
}

} } // namespace cinder::app

#endif // CINDER_MAC
