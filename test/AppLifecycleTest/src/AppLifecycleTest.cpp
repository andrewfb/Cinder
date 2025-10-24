/*
 * AppLifecycleTest - Tests app initialization callbacks and lifecycle signals
 *
 * This test verifies:
 * 1. Static initialization callbacks via registerAppInitCallback()
 * 2. PreSetup signal fires before setup()
 * 3. PreUpdate signal fires before update()
 * 4. PostUpdate signal fires after update()
 * 5. PreDraw signal fires before draw()
 * 6. Proper ordering of all lifecycle events
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

// Include our header-only test
#include "../include/AppInitCallback.h"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace ci;
using namespace ci::app;
using namespace std;

// State machine to track lifecycle events
enum class LifecycleState {
	INIT,
	PRE_SETUP,
	SETUP,
	BEGIN_FRAME,
	PRE_UPDATE,
	UPDATE,
	POST_UPDATE,
	PRE_DRAW,
	DRAW,
	END_FRAME,
	CLEANUP
};

const char* stateToString( LifecycleState state ) {
	switch( state ) {
		case LifecycleState::INIT:        return "INIT";
		case LifecycleState::PRE_SETUP:   return "PRE_SETUP";
		case LifecycleState::SETUP:       return "SETUP";
		case LifecycleState::BEGIN_FRAME: return "BEGIN_FRAME";
		case LifecycleState::PRE_UPDATE:  return "PRE_UPDATE";
		case LifecycleState::UPDATE:      return "UPDATE";
		case LifecycleState::POST_UPDATE: return "POST_UPDATE";
		case LifecycleState::PRE_DRAW:    return "PRE_DRAW";
		case LifecycleState::DRAW:        return "DRAW";
		case LifecycleState::END_FRAME:   return "END_FRAME";
		case LifecycleState::CLEANUP:     return "CLEANUP";
	}
	return "UNKNOWN";
}

class AppLifecycleTest : public App {
public:
	AppLifecycleTest();

	void setup() override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void keyDown( KeyEvent event ) override;

private:
	void recordState( LifecycleState state, const string& source );
	void verifyStateTransition( LifecycleState from, LifecycleState to );
	void printSummary();

	struct StateRecord {
		LifecycleState state;
		string source;
		double timestamp;
	};

	vector<StateRecord> mStateHistory;
	LifecycleState mCurrentState = LifecycleState::INIT;
	int mFrameCount = 0;
	int mMaxFrames = 5; // Run for 5 frames then print summary
	bool mTestsPassed = true;
	vector<string> mErrors;

	// Signal connections
	signals::Connection mPreSetupConn;
	signals::Connection mBeginFrameConn;
	signals::Connection mEndFrameConn;
	signals::Connection mPreUpdateConn;
	signals::Connection mPostUpdateConn;
	signals::Connection mPreDrawConn;
	signals::Connection mCleanupConn;
	signals::Connection mThreadNameConn;

	// Thread name tracking
	std::vector<std::string> mThreadNames;
};

AppLifecycleTest::AppLifecycleTest()
{
	cout << "\n=== AppLifecycleTest Constructor ===" << endl;
	cout << "Connecting to app lifecycle signals..." << endl;

	// Connect to all the lifecycle signals
	mPreSetupConn = getSignalPreSetup().connect( [this]() {
		verifyStateTransition( LifecycleState::INIT, LifecycleState::PRE_SETUP );
		recordState( LifecycleState::PRE_SETUP, "Signal" );
	});

	mBeginFrameConn = getSignalBeginFrame().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// Should come from either SETUP (first frame) or END_FRAME (subsequent frames)
			if( mFrameCount == 0 ) {
				verifyStateTransition( LifecycleState::SETUP, LifecycleState::BEGIN_FRAME );
			} else {
				// After first frame, the last recorded state is END_FRAME from the previous frame
				verifyStateTransition( LifecycleState::END_FRAME, LifecycleState::BEGIN_FRAME );
			}
			recordState( LifecycleState::BEGIN_FRAME, "Signal" );
		}
	});

	mPreUpdateConn = getSignalPreUpdate().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// Should always come from BEGIN_FRAME
			verifyStateTransition( LifecycleState::BEGIN_FRAME, LifecycleState::PRE_UPDATE );
			recordState( LifecycleState::PRE_UPDATE, "Signal" );
		}
	});

	mPostUpdateConn = getSignalPostUpdate().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			verifyStateTransition( LifecycleState::UPDATE, LifecycleState::POST_UPDATE );
			recordState( LifecycleState::POST_UPDATE, "Signal" );
		}
	});

	mPreDrawConn = getSignalPreDraw().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			verifyStateTransition( LifecycleState::POST_UPDATE, LifecycleState::PRE_DRAW );
			recordState( LifecycleState::PRE_DRAW, "Signal" );
		}
	});

	mEndFrameConn = getSignalEndFrame().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// Should come after DRAW
			verifyStateTransition( LifecycleState::DRAW, LifecycleState::END_FRAME );
			recordState( LifecycleState::END_FRAME, "Signal" );
		}
	});

	mCleanupConn = getSignalCleanup().connect( [this]() {
		cout << "\n[Signal] Cleanup signal received" << endl;
		recordState( LifecycleState::CLEANUP, "Signal" );
	});

	mThreadNameConn = getSignalThreadName().connect( [this]( const std::string& name ) {
		cout << "[ThreadName] Thread named: \"" << name << "\" (thread ID: "
		     << std::this_thread::get_id() << ")" << endl;
		mThreadNames.push_back( name );
	});

	// Also test priority-based signal execution
	getSignalPreSetup().connect( 100, []() {
		cout << "[Priority Test] HIGH priority (100) PreSetup handler" << endl;
	});

	getSignalPreSetup().connect( -100, []() {
		cout << "[Priority Test] LOW priority (-100) PreSetup handler" << endl;
	});

	getSignalPreSetup().connect( 0, []() {
		cout << "[Priority Test] DEFAULT priority (0) PreSetup handler" << endl;
	});
}

void AppLifecycleTest::setup()
{
	cout << "\n=== setup() called ===" << endl;
	verifyStateTransition( LifecycleState::PRE_SETUP, LifecycleState::SETUP );
	recordState( LifecycleState::SETUP, "Override" );

	// VERIFY: App init callbacks should have been executed before setup()
	if( !test::wereCallbacksExecutedBeforeSetup() ) {
		string error = "CRITICAL: App init callbacks were NOT executed before setup()!";
		mErrors.push_back( error );
		mTestsPassed = false;
		cerr << "ERROR: " << error << endl;
	} else {
		cout << "[VERIFIED] App init callbacks executed before setup()" << endl;
	}

	// VERIFY: We should have exactly 2 init callbacks registered (AppInitCallbackTest + HeaderOnlyBlock)
	int initCount = test::getInitCallbackCount();
	if( initCount != 2 ) {
		string error = "Expected 2 init callbacks, got " + to_string( initCount );
		mErrors.push_back( error );
		mTestsPassed = false;
		cerr << "ERROR: " << error << endl;
	} else {
		cout << "[VERIFIED] Correct number of init callbacks: " << initCount << endl;
	}

	// Set up console logging
	ci::log::makeLogger<ci::log::LoggerConsole>();

	// Configure window
	getWindow()->setTitle( "App Lifecycle Test" );

	// Test thread naming
	cout << "\n=== Testing Thread Naming ===" << endl;

	// Name the main thread
	ci::setThreadName( "MainThread" );

	// Create a test worker thread
	std::thread testThread( []() {
		ci::setThreadName( "TestWorkerThread" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	});
	testThread.join();

	// Create another thread with a different name
	std::thread audioThread( []() {
		ci::setThreadName( "AudioProcessingThread" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	});
	audioThread.join();

	cout << "Setup complete. Press ESC to quit." << endl;
	cout << "Running for " << mMaxFrames << " frames..." << endl;
}

void AppLifecycleTest::update()
{
	if( mFrameCount < mMaxFrames ) {
		verifyStateTransition( LifecycleState::PRE_UPDATE, LifecycleState::UPDATE );
		recordState( LifecycleState::UPDATE, "Override" );

		cout << "\n--- Frame " << (mFrameCount + 1) << " ---" << endl;
	}
}

void AppLifecycleTest::draw()
{
	if( mFrameCount < mMaxFrames ) {
		// PreDraw signal should have been called before draw()
		verifyStateTransition( LifecycleState::PRE_DRAW, LifecycleState::DRAW );
		recordState( LifecycleState::DRAW, "Override" );

		gl::clear( Color( 0.2f, 0.2f, 0.3f ) );

		// Draw frame counter
		gl::drawString( "Frame: " + to_string( mFrameCount + 1 ), vec2( 10, 30 ) );
		gl::drawString( "Press ESC to quit", vec2( 10, 50 ) );

		if( mTestsPassed ) {
			gl::color( 0, 1, 0 );
			gl::drawString( "All tests passing!", vec2( 10, 70 ) );
		} else {
			gl::color( 1, 0, 0 );
			gl::drawString( "Test failures detected!", vec2( 10, 70 ) );
		}
	}

	mFrameCount++;

	if( mFrameCount == mMaxFrames ) {
		printSummary();

		// Auto-quit after showing results
		std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
		quit();
	}
}

void AppLifecycleTest::cleanup()
{
	cout << "\n=== cleanup() called ===" << endl;
	recordState( LifecycleState::CLEANUP, "Override" );
}

void AppLifecycleTest::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE ) {
		quit();
	}
}

void AppLifecycleTest::recordState( LifecycleState state, const string& source )
{
	mStateHistory.push_back( { state, source, getElapsedSeconds() } );
	mCurrentState = state;

	if( mFrameCount < mMaxFrames ) {
		cout << "[" << source << "] " << stateToString( state ) << " at "
		     << fixed << setprecision( 4 ) << getElapsedSeconds() << "s" << endl;
	}
}

void AppLifecycleTest::verifyStateTransition( LifecycleState from, LifecycleState to )
{
	// Now that we call verify BEFORE record, mCurrentState contains the previous state
	if( mCurrentState != from ) {
		string error = "Invalid state transition: expected " + string( stateToString( from ) ) +
		               " -> " + string( stateToString( to ) ) +
		               ", but current state is " + string( stateToString( mCurrentState ) );
		mErrors.push_back( error );
		mTestsPassed = false;
		cerr << "ERROR: " << error << endl;
	}
}

void AppLifecycleTest::printSummary()
{
	cout << "\n\n========================================" << endl;
	cout << "         TEST SUMMARY REPORT" << endl;
	cout << "========================================" << endl;

	// Print state history
	cout << "\nLifecycle Event History:" << endl;
	cout << "------------------------" << endl;
	for( const auto& record : mStateHistory ) {
		cout << setw( 15 ) << left << stateToString( record.state )
		     << " [" << setw( 8 ) << record.source << "] at "
		     << fixed << setprecision( 4 ) << record.timestamp << "s" << endl;
	}

	// Verify expected patterns
	cout << "\nVerification Results:" << endl;
	cout << "--------------------" << endl;

	bool hasBeginFrame = false;
	bool hasEndFrame = false;
	bool hasPreSetup = false;
	bool hasPreUpdate = false;
	bool hasPostUpdate = false;
	bool hasPreDraw = false;

	for( const auto& record : mStateHistory ) {
		if( record.state == LifecycleState::BEGIN_FRAME && record.source == "Signal" )
			hasBeginFrame = true;
		if( record.state == LifecycleState::END_FRAME && record.source == "Signal" )
			hasEndFrame = true;
		if( record.state == LifecycleState::PRE_SETUP && record.source == "Signal" )
			hasPreSetup = true;
		if( record.state == LifecycleState::PRE_UPDATE && record.source == "Signal" )
			hasPreUpdate = true;
		if( record.state == LifecycleState::POST_UPDATE && record.source == "Signal" )
			hasPostUpdate = true;
		if( record.state == LifecycleState::PRE_DRAW && record.source == "Signal" )
			hasPreDraw = true;
	}

	cout << "[" << (hasBeginFrame ? "PASS" : "FAIL") << "] BeginFrame signal fired" << endl;
	cout << "[" << (hasEndFrame ? "PASS" : "FAIL") << "] EndFrame signal fired" << endl;
	cout << "[" << (hasPreSetup ? "PASS" : "FAIL") << "] PreSetup signal fired" << endl;
	cout << "[" << (hasPreUpdate ? "PASS" : "FAIL") << "] PreUpdate signal fired" << endl;
	cout << "[" << (hasPostUpdate ? "PASS" : "FAIL") << "] PostUpdate signal fired" << endl;
	cout << "[" << (hasPreDraw ? "PASS" : "FAIL") << "] PreDraw signal fired" << endl;

	// Verify thread name signal
	cout << "\nThread Name Signal Verification:" << endl;
	cout << "--------------------------------" << endl;
	bool hasThreadNames = !mThreadNames.empty();
	cout << "[" << (hasThreadNames ? "PASS" : "FAIL") << "] Thread name signal fired" << endl;
	if( hasThreadNames ) {
		cout << "Captured thread names (" << mThreadNames.size() << "):" << endl;
		for( const auto& name : mThreadNames ) {
			cout << "  - " << name << endl;
		}
		// Verify we got the expected thread names
		bool hasMain = std::find( mThreadNames.begin(), mThreadNames.end(), "MainThread" ) != mThreadNames.end();
		bool hasWorker = std::find( mThreadNames.begin(), mThreadNames.end(), "TestWorkerThread" ) != mThreadNames.end();
		bool hasAudio = std::find( mThreadNames.begin(), mThreadNames.end(), "AudioProcessingThread" ) != mThreadNames.end();
		cout << "[" << (hasMain ? "PASS" : "FAIL") << "] MainThread captured" << endl;
		cout << "[" << (hasWorker ? "PASS" : "FAIL") << "] TestWorkerThread captured" << endl;
		cout << "[" << (hasAudio ? "PASS" : "FAIL") << "] AudioProcessingThread captured" << endl;
	}

	// Verify app init callbacks
	cout << "\nApp Init Callback Verification:" << endl;
	cout << "-------------------------------" << endl;
	bool initCallbacksPassed = test::wereCallbacksExecutedBeforeSetup() && (test::getInitCallbackCount() == 2);
	cout << "[" << (test::wereCallbacksExecutedBeforeSetup() ? "PASS" : "FAIL")
	     << "] Init callbacks executed before setup()" << endl;
	cout << "[" << (test::getInitCallbackCount() == 2 ? "PASS" : "FAIL")
	     << "] Expected 2 init callbacks, got " << test::getInitCallbackCount() << endl;
	cout << "[" << (test::getPreSetupCallbackCount() > 0 ? "PASS" : "FAIL")
	     << "] PreSetup callbacks from init were called: " << test::getPreSetupCallbackCount() << endl;
	cout << "[" << (test::getPreUpdateCallbackCount() > 0 ? "PASS" : "FAIL")
	     << "] PreUpdate callbacks from init were called: " << test::getPreUpdateCallbackCount() << endl;

	// Check for errors
	if( !mErrors.empty() ) {
		cout << "\nErrors Detected:" << endl;
		cout << "---------------" << endl;
		for( const auto& error : mErrors ) {
			cout << "  - " << error << endl;
		}
	}

	// Final result
	cout << "\n========================================" << endl;
	if( mTestsPassed && hasBeginFrame && hasEndFrame && hasPreSetup && hasPreUpdate &&
	    hasPostUpdate && hasPreDraw && initCallbacksPassed && hasThreadNames &&
	    mThreadNames.size() >= 3 ) {
		cout << "         ALL TESTS PASSED!" << endl;
	} else {
		cout << "         SOME TESTS FAILED!" << endl;
	}
	cout << "========================================" << endl;
}

// Make this a console app for Windows
void prepareSettings( AppLifecycleTest::Settings* settings )
{
	settings->setWindowSize( 400, 200 );
	settings->setConsoleWindowEnabled( true );
}

CINDER_APP( AppLifecycleTest, RendererGl, prepareSettings )