/*
 Copyright (c) 2025, The Cinder Project (http://libcinder.org)
 All rights reserved.

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
#include <functional>
#include <vector>
#include <mutex>

namespace cinder { namespace app {

class AppBase;

//! Singleton class for registering callbacks that fire after app construction but before setup()
//! This allows header-only blocks and libraries to register initialization code via static initializers
class CI_API AppInitCallbacks {
  public:
	using InitCallback = std::function<void(AppBase*)>;

	//! Returns the singleton instance
	static AppInitCallbacks& get();

	//! Registers a callback to be executed after app construction but before setup()
	//! Typically called from static initializers
	//! \param callback Function to be called with the App instance
	//! \return The callback index, for potential future use
	size_t registerCallback( const InitCallback& callback );

	//! Executes all registered callbacks. Called internally by the app framework.
	//! \param app The constructed app instance
	void executeCallbacks( AppBase* app );

	//! Clears all registered callbacks. Useful for testing.
	void clear();

	//! Returns the number of registered callbacks
	size_t getCallbackCount() const;

  private:
	AppInitCallbacks() = default;
	~AppInitCallbacks() = default;

	// Prevent copying
	AppInitCallbacks( const AppInitCallbacks& ) = delete;
	AppInitCallbacks& operator=( const AppInitCallbacks& ) = delete;

	mutable std::mutex		mMutex;
	std::vector<InitCallback>	mCallbacks;
};

//! Helper class for registering init callbacks via static initialization
//! Usage: static AppInitRegistrar gMyInitializer( []( app::AppBase* app ){ /* init code */ } );
class CI_API AppInitRegistrar {
  public:
	AppInitRegistrar( const AppInitCallbacks::InitCallback& callback ) {
		AppInitCallbacks::get().registerCallback( callback );
	}
};

} } // namespace cinder::app
