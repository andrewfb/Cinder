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

#pragma once

#include <memory>

namespace cinder { namespace app {

class AppBase;

//! \brief Base class for application instrumentation (profiling, tracing, etc.)
//!
//! Instrumenters can hook into key points in the app lifecycle to measure performance,
//! log events, or integrate with external profilers. Multiple instrumenters can be
//! registered simultaneously via AppBase::addInstrumentation().
//!
//! Scopes are RAII objects that automatically begin/end profiling regions. The returned
//! unique_ptr manages the scope's lifetime - destruction triggers the end event.
class AppInstrumentation {
  public:
	virtual ~AppInstrumentation() = default;

	//! \brief RAII scope object for instrumentation regions
	//!
	//! The scope is active from construction until destruction. Instrumenters should
	//! call their profiler's BeginEvent in the Scope constructor and EndEvent in the
	//! destructor to ensure proper pairing even with early returns or exceptions.
	struct Scope {
		virtual ~Scope() = default;
	};

	using ScopePtr = std::unique_ptr<Scope>;

	//! Called before setup. Return a Scope that will be destroyed after setup completes.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginSetup( AppBase &app ) { return nullptr; }

	//! Called at the beginning of each frame. Return a Scope that will be destroyed at frame end.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginFrame( AppBase &app ) { return nullptr; }

	//! Called before the update phase. Return a Scope that will be destroyed after update completes.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginUpdate( AppBase &app ) { return nullptr; }

	//! Called before the draw phase. Return a Scope that will be destroyed after draw completes.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginDraw( AppBase &app ) { return nullptr; }

	//! Called before the post-draw phase. Return a Scope that will be destroyed after post-draw completes.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginPostDraw( AppBase &app ) { return nullptr; }

	//! Called before cleanup. Return a Scope that will be destroyed after cleanup completes.
	//! \param app Reference to the application
	//! \return Scope object or nullptr if no instrumentation needed
	virtual ScopePtr beginCleanup( AppBase &app ) { return nullptr; }
};

}} // namespace cinder::app
