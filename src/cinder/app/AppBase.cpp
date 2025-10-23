/*
 Copyright (c) 2012, The Cinder Project, All rights reserved.
 Copyright (c) Microsoft Open Technologies, Inc. All rights reserved.

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
#if defined( CINDER_UWP )
	#define ASIO_WINDOWS_RUNTIME 1
#endif

#if defined( __ANDROID__ ) && defined( __clang__ )
 	#if defined( __GNUC__ )
 		#define SAVED__GNUC__ __GNUC__
 		#undef __GNUC__
 	#endif

 	#if defined( __GNUC_MINOR__ )
 		#define SAVED__GNUC_MINOR__ __GNUC_MINOR__
 		#undef __GNUC_MINOR__
 	#endif

 	#define __GNUC__ 		4
 	#define __GNUC_MINOR__	9
	#include "asio/asio.hpp"

 	#if defined( SAVED__GNUC__ )
 		#undef  __GNUC__
 		#define __GNUC__ SAVED__GNUC__
 		#undef  SAVED__GNUC__
 	#else
		#undef __GNUC__
 	#endif 

 	#if defined( SAVED__GNUC_MINOR__ )
 		#undef  __GNUC_MINOR__
 		#define __GNUC_MINOR__ SAVED__GNUC_MINOR__
 		#undef  SAVED__GNUC_MINOR__
 	#else
		#undef __GNUC_MINOR__
 	#endif
#else
  #if defined( linux ) || defined( __linux ) || defined( __linux__ )
    #define CINDER_ASIO_CLANG_BUILTIN_OFFSETOF
  #endif
 	#if defined( __ANDROID__ )
 		#include "cinder/android/libc_helper.h"
 	#endif
	#include "asio/asio.hpp"
#endif

#include "cinder/app/AppBase.h"
#include "cinder/app/Renderer.h"
#include "cinder/Camera.h"
#include "cinder/System.h"
#include "cinder/Utilities.h"
#include "cinder/Timeline.h"
#include "cinder/Thread.h"
#include "cinder/Log.h"

#include <mutex>

using namespace std;


#if defined( CINDER_ANDROID )
#include "cinder/android/AndroidDevLog.h"
using namespace ci::android;
#endif

namespace cinder { namespace app {

// Forward declare to make it a friend
class InstrumentationScopeStack;

// Internal helper class that aggregates scopes from multiple instrumenters
// Destructor iterates in reverse to ensure LIFO semantics
class InstrumentationScopeStack {
	friend class AppBase;
  public:
	enum class Phase {
		Setup,
		Frame,
		Update,
		Draw,
		PostDraw,
		Cleanup
	};

	InstrumentationScopeStack( AppBase* app, Phase phase )
	{
		if( ! app )
			return;

		// Copy the registry snapshot while holding the lock to avoid deadlock
		std::vector<std::shared_ptr<AppInstrumentation>> instrumenters;
		{
			std::lock_guard<std::mutex> lock( app->mInstrumentationMutex );
			instrumenters = app->mInstrumenters;
		}

		// Reserve space to avoid reallocations
		mScopes.reserve( instrumenters.size() );

		// Call the appropriate begin* method on each instrumenter (without holding lock)
		for( const auto& instrumenter : instrumenters ) {
			AppInstrumentation::ScopePtr scope;

			switch( phase ) {
				case Phase::Setup:
					scope = instrumenter->beginSetup( *app );
					break;
				case Phase::Frame:
					scope = instrumenter->beginFrame( *app );
					break;
				case Phase::Update:
					scope = instrumenter->beginUpdate( *app );
					break;
				case Phase::Draw:
					scope = instrumenter->beginDraw( *app );
					break;
				case Phase::PostDraw:
					scope = instrumenter->beginPostDraw( *app );
					break;
				case Phase::Cleanup:
					scope = instrumenter->beginCleanup( *app );
					break;
			}

			// Store scope (even if nullptr) to maintain ordering
			if( scope ) {
				mScopes.push_back( std::move( scope ) );
			}
		}
	}

	~InstrumentationScopeStack()
	{
		// Scopes are destroyed in reverse order (LIFO) automatically
		// as the vector destructs from back to front
		mScopes.clear();
	}

	// Non-copyable, non-movable
	InstrumentationScopeStack( const InstrumentationScopeStack& ) = delete;
	InstrumentationScopeStack& operator=( const InstrumentationScopeStack& ) = delete;

  private:
	std::vector<AppInstrumentation::ScopePtr> mScopes;
};

AppBase*					AppBase::sInstance = nullptr;			// Static instance of App, effectively a singleton
AppBase::Settings*			AppBase::sSettingsFromMain = nullptr;
std::vector<std::function<void(AppBase*)>>*	AppBase::sInitCallbacks = nullptr;
std::mutex*								AppBase::sInitCallbacksMutex = nullptr;
static std::thread::id		sPrimaryThreadId = std::this_thread::get_id();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AppBase::Settings

#if defined( CINDER_ANDROID )
AppBase::Settings::Settings()
	: mShouldQuit( false ), mQuitOnLastWindowClose( true ), mPowerManagementEnabled( false ),
	  mFrameRate( 60 ), mFrameRateEnabled( true ), mHighDensityDisplayEnabled( false ), mMultiTouchEnabled( true )
#else
AppBase::Settings::Settings()
	: mShouldQuit( false ), mQuitOnLastWindowClose( true ), mPowerManagementEnabled( false ),
	  mFrameRate( 60 ), mFrameRateEnabled( true ), mHighDensityDisplayEnabled( false ), mMultiTouchEnabled( false )
#endif		
{
}

void AppBase::Settings::init( const RendererRef &defaultRenderer, const char *title )
{
	mDefaultRenderer = defaultRenderer;
	if( title )
		mTitle = title;
}

void AppBase::Settings::disableFrameRate()
{
	mFrameRateEnabled = false;
}

void AppBase::Settings::setFrameRate( float frameRate )
{
	mFrameRate = frameRate;
}

void AppBase::Settings::prepareWindow( const Window::Format &format )
{
	mWindowFormats.push_back( format );
}

void AppBase::Settings::setShouldQuit( bool shouldQuit )
{
	mShouldQuit = shouldQuit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AppBase

AppBase::AppBase()
	: mFrameCount( 0 ), mAverageFps( 0 ), mFpsSampleInterval( 1 ), mTimer( true ), mTimeline( Timeline::create() ),
		mFpsLastSampleFrame( 0 ), mFpsLastSampleTime( 0 ), mLaunchCalled( false ), mQuitRequested( false )
{
	sInstance = this;

	mDefaultRenderer = sSettingsFromMain->getDefaultRenderer();
	mMultiTouchEnabled = sSettingsFromMain->isMultiTouchEnabled();
	mHighDensityDisplayEnabled = sSettingsFromMain->isHighDensityDisplayEnabled();

	mIo = shared_ptr<asio::io_context>( new asio::io_context() );
	mIoWork = shared_ptr<asio::io_context::work>( new asio::io_context::work( *mIo ) );

	// due to an issue with boost::filesystem's static initialization on Windows, 
	// it's necessary to create a fs::path here in case of secondary threads doing the same thing simultaneously
#if defined( CINDER_MSW )
	fs::path dummyPath( "dummy" );
#endif
}

AppBase::~AppBase()
{
	mIo->stop();
}

AppBase* AppBase::get() 
{ 
	return sInstance; 
}

// These are called by application instantiation main functions
// static
void AppBase::prepareLaunch()
{	
	Platform::get()->prepareLaunch();
}

// static
void AppBase::initialize( Settings *settings, const RendererRef &defaultRenderer, const char *title )
{
	settings->init( defaultRenderer, title );

	sSettingsFromMain = settings;
}

void AppBase::onTerminate()
{
	if( auto excptr = std::current_exception() ) {
		try {
			std::rethrow_exception( excptr );
		}
		catch( const std::exception & exc ) {
			CI_LOG_F( "Terminating due to uncaught exception, type: " << System::demangleTypeName( typeid( exc ).name() ) << ", what: " << exc.what() );
		}
		catch( ... ) {
			CI_LOG_F( "Terminating due to unknown uncaught exception" );
		}
	}
	std::exit( EXIT_FAILURE );
}

void AppBase::executeLaunch()
{
	std::set_terminate( onTerminate );

	// a quit() was called from the app constructor; don't launch
	if( mQuitRequested )
		return;

	mLaunchCalled = true;
	launch();
}

// static
void AppBase::cleanupLaunch()
{
	Platform::get()->cleanupLaunch();

#if defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	// This will delete Platform::sInstance if it's not null. 
	// Afterwards Platform::sInstance will be set to null.
	Platform::set( nullptr );
#endif 	
}

void AppBase::registerAppInitCallback( const std::function<void(AppBase*)> &callback )
{
	// Thread-safe initialization using static local variables (guaranteed by C++11)
	static std::mutex sLocalMutex;
	static std::vector<std::function<void(AppBase*)>> sLocalCallbacks;

	// Initialize the static pointers to point to the static locals
	static std::once_flag sInitFlag;
	std::call_once( sInitFlag, []() {
		sInitCallbacksMutex = &sLocalMutex;
		sInitCallbacks = &sLocalCallbacks;
	});

	std::lock_guard<std::mutex> lock( sLocalMutex );
	sLocalCallbacks.push_back( callback );
}

void AppBase::executeInitCallbacks()
{
	// Copy callbacks to avoid holding lock while executing user code
	std::vector<std::function<void(AppBase*)>> callbacksCopy;

	if( sInitCallbacks && sInitCallbacksMutex ) {
		{
			std::lock_guard<std::mutex> lock( *sInitCallbacksMutex );
			callbacksCopy = *sInitCallbacks;
			// Clear callbacks after copying to free memory
			sInitCallbacks->clear();
		}

		// Now execute callbacks without holding the lock
		for( const auto& callback : callbacksCopy ) {
			callback( this );
		}
	}
}

void AppBase::addInstrumentation( std::shared_ptr<AppInstrumentation> instrumenter )
{
	if( ! instrumenter )
		return;

	std::lock_guard<std::mutex> lock( mInstrumentationMutex );

	mInstrumenters.push_back( instrumenter );
	mHasInstrumentation.store( true, std::memory_order_relaxed );
}

std::shared_ptr<InstrumentationScopeStack> AppBase::makeInstrumentationScope( InstrumentationPhase phase )
{
	// Short-circuit if no instrumenters are registered
	if( ! mHasInstrumentation.load( std::memory_order_relaxed ) )
		return nullptr;

	// Convert AppBase::InstrumentationPhase to InstrumentationScopeStack::Phase
	InstrumentationScopeStack::Phase stackPhase;

	switch( phase ) {
		case InstrumentationPhase::Setup:
			stackPhase = InstrumentationScopeStack::Phase::Setup;
			break;
		case InstrumentationPhase::Frame:
			stackPhase = InstrumentationScopeStack::Phase::Frame;
			break;
		case InstrumentationPhase::Update:
			stackPhase = InstrumentationScopeStack::Phase::Update;
			break;
		case InstrumentationPhase::Draw:
			stackPhase = InstrumentationScopeStack::Phase::Draw;
			break;
		case InstrumentationPhase::PostDraw:
			stackPhase = InstrumentationScopeStack::Phase::PostDraw;
			break;
		case InstrumentationPhase::Cleanup:
			stackPhase = InstrumentationScopeStack::Phase::Cleanup;
			break;
		default:
			return nullptr;
	}

	return std::make_shared<InstrumentationScopeStack>( this, stackPhase );
}

void AppBase::privateBeginFrame__()
{
	// Emit beginFrame signal at the start of each frame
	mSignalBeginFrame.emit();
}

void AppBase::privateEndFrame__()
{
	// Emit endFrame signal at the end of each frame
	mSignalEndFrame.emit();
}

void AppBase::privateSetup__()
{
	mTimeline->stepTo( static_cast<float>( getElapsedSeconds() ) );

	// Execute init callbacks before setup
	executeInitCallbacks();

	// Emit preSetup signal
	mSignalPreSetup.emit();

	setup();
}

void AppBase::privateUpdate__()
{
	mFrameCount++;

	// service asio::io_context
	mIo->poll();

	if( getNumWindows() > 0 ) {
		WindowRef mainWin = getWindowIndex( 0 );
		if( mainWin )
			mainWin->getRenderer()->makeCurrentContext();
	}

	mSignalUpdate.emit();

	// Emit preUpdate signal
	mSignalPreUpdate.emit();

	update();

	// Emit postUpdate signal
	mSignalPostUpdate.emit();

	mTimeline->stepTo( static_cast<float>( getElapsedSeconds() ) );

	double now = mTimer.getSeconds();
	if( now > mFpsLastSampleTime + mFpsSampleInterval ) {
		//calculate average Fps over sample interval
		uint32_t framesPassed = mFrameCount - mFpsLastSampleFrame;
		mAverageFps = (float)(framesPassed / (now - mFpsLastSampleTime));

		mFpsLastSampleTime = now;
		mFpsLastSampleFrame = mFrameCount;
	}
}

void AppBase::emitCleanup()
{
	mSignalCleanup.emit();
	cleanup();
}

void AppBase::emitWillResignActive()
{
	mSignalWillResignActive.emit();
}

void AppBase::emitDidBecomeActive()
{
	mSignalDidBecomeActive.emit();
}

void AppBase::emitDisplayConnected( const DisplayRef &display )
{
	mSignalDisplayConnected.emit( display );
}

void AppBase::emitDisplayDisconnected( const DisplayRef &display )
{
	mSignalDisplayDisconnected.emit( display );
}

void AppBase::emitDisplayChanged( const DisplayRef &display )
{
	mSignalDisplayChanged.emit( display );
}

fs::path AppBase::getOpenFilePath( const fs::path &initialPath, const vector<string> &extensions )
{
	return Platform::get()->getOpenFilePath( initialPath, extensions );
}

fs::path AppBase::getFolderPath( const fs::path &initialPath )
{
	return Platform::get()->getFolderPath( initialPath );
}

fs::path AppBase::getSaveFilePath( const fs::path &initialPath, const vector<string> &extensions )
{
	return Platform::get()->getSaveFilePath( initialPath, extensions );
}

std::ostream& AppBase::console()
{
	return Platform::get()->console();
}

bool AppBase::isMainThread()
{
	return std::this_thread::get_id() == sPrimaryThreadId;
}

void AppBase::dispatchAsync( const std::function<void()> &fn )
{
	io_context().post( fn );
}

Surface	AppBase::copyWindowSurface()
{
	return getWindow()->getRenderer()->copyWindowSurface(
			getWindow()->toPixels( getWindow()->getBounds() ), getWindow()->toPixels( getWindow()->getHeight() ) );
}

Surface	AppBase::copyWindowSurface( const Area &area )
{
	Area clippedArea = area.getClipBy( getWindow()->toPixels( getWindow()->getBounds() ) );
	return getWindow()->getRenderer()->copyWindowSurface( clippedArea, getWindow()->toPixels( getWindow()->getHeight() ) );
}

void AppBase::restoreWindowContext()
{
	getWindow()->getRenderer()->makeCurrentContext();
}

} } // namespace cinder::app
