/*
 * AppLifecycleTest - Comprehensive test of Cinder's application lifecycle signals and initialization system
 *
 * OVERVIEW:
 * This test validates the complete application lifecycle, from static initialization through frame loops
 * to cleanup. It runs for 5 frames with 2 windows, verifying that all signals fire in the correct order
 * and that app-level signals (PreDraw/PostDraw) fire once per frame, not once per window.
 *
 * WHAT THIS TEST VALIDATES:
 *
 * 1. STATIC INITIALIZATION CALLBACKS
 *    - registerAppInitCallback() allows registration before main() or App constructor
 *    - Callbacks execute before setup() is called
 *    - Multiple callbacks can be registered (test uses 2: AppInitCallbackTest + HeaderOnlyBlock)
 *    - Header-only pattern works (static instances in headers can register callbacks)
 *    - See AppInitCallback.h for the test implementation
 *
 * 2. LIFECYCLE SIGNAL ORDER
 *    Expected sequence for each frame:
 *      First frame:    PRE_SETUP → SETUP → BEGIN_FRAME → PRE_UPDATE → UPDATE → POST_UPDATE →
 *                      PRE_DRAW → DRAW → POST_DRAW → END_FRAME
 *      Subsequent:     BEGIN_FRAME → PRE_UPDATE → UPDATE → POST_UPDATE →
 *                      PRE_DRAW → DRAW → POST_DRAW → END_FRAME
 *
 * 3. APP-LEVEL SIGNALS (Critical for instrumentation/profiling)
 *    - PreDraw:  Fires ONCE per frame BEFORE any window draws (not per-window)
 *    - PostDraw: Fires ONCE per frame AFTER all windows have drawn (not per-window)
 *    This is essential for frame-level profiling and ensures tools like Tracy can accurately
 *    measure frame timing without double-counting in multi-window scenarios.
 *
 * 4. FRAME-LEVEL SIGNALS
 *    - BeginFrame: Marks the start of a new frame
 *    - EndFrame:   Marks the end of a frame (after all windows drawn and PostDraw fired)
 *
 * 5. UPDATE SIGNALS
 *    - PreUpdate:  Fires before update() override
 *    - PostUpdate: Fires after update() override
 *
 * 6. MULTI-WINDOW BEHAVIOR
 *    - Two windows are created to test that app-level signals fire correctly
 *    - Each window's draw() is called once per frame
 *    - PreDraw fires once before first window draws
 *    - PostDraw fires once after last window draws
 *    - Frame counting happens in EndFrame (after all windows drawn) to avoid race conditions
 *
 * 7. THREAD NAMING SIGNAL
 *    - setThreadName() triggers getSignalThreadName()
 *    - Useful for debugging and profiling tools
 *    - Test creates 3 named threads: MainThread, TestWorkerThread, AudioProcessingThread
 *
 * 8. SIGNAL PRIORITY ORDERING
 *    - Signals support priority-based execution order
 *    - Higher priority handlers execute first
 *    - Test verifies with HIGH(100), DEFAULT(0), LOW(-100) priority handlers
 *
 * HOW IT WORKS:
 *
 * 1. Constructor connects to all lifecycle signals and sets up state tracking
 * 2. setup() creates a second window and initializes test infrastructure
 * 3. Each frame (0-4):
 *    - update() sets mSetupComplete flag on first call, then logs frame number
 *    - draw() is called for each window (only counted if mSetupComplete && mFrameCount < mMaxFrames)
 *    - Signals fire and are recorded with timestamps
 * 4. EndFrame increments frame counter after all windows have drawn
 * 5. After frame 4 (mFrameCount reaches mMaxFrames), printSummary() is called
 * 6. Summary verifies:
 *    - All signals fired
 *    - Correct sequence for each frame
 *    - Each window drawn exactly 5 times
 *    - PreDraw/PostDraw fired exactly 5 times (once per frame, not per window)
 *    - Thread names were captured
 *    - Init callbacks executed before setup()
 * 7. App auto-quits after 2 second pause
 *
 * FRAME COUNTING METHODOLOGY:
 * Frame counting happens in EndFrame signal handler (not in draw()) to ensure both windows
 * draw during the same logical frame. This eliminates off-by-one errors where incrementing
 * in the first window's draw() would cause the second window to see an already-incremented count.
 * Additionally, mSetupComplete flag prevents counting draws that happen during window creation.
 *
 * INTERPRETING RESULTS:
 * - Green "ALL TESTS PASSED!" means the lifecycle is working correctly
 * - Red "SOME TESTS FAILED!" indicates a problem - check error messages in output
 * - Console output shows detailed sequence verification and timing information
 *
 * RELATED FILES:
 * - test/AppLifecycleTest/include/AppInitCallback.h: Header-only static init callback test
 * - include/cinder/app/AppBase.h: Signal definitions
 * - src/cinder/app/AppBase.cpp: Signal implementation
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
	POST_DRAW,
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
		case LifecycleState::POST_DRAW:   return "POST_DRAW";
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
	void verifySequence();
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
	bool mSetupComplete = false; // Only count draws after setup completes
	vector<string> mErrors;

	// Multi-window tracking
	WindowRef mSecondWindow;
	int mPreDrawCountThisFrame = 0;
	int mPostDrawCountThisFrame = 0;
	map<WindowRef, int> mDrawCallsPerWindow;
	WindowRef mActiveWindowDuringDraw;
	WindowRef mLastWindowDrawn;
	int mWindowDrawCountThisFrame = 0;

	// Signal connections
	signals::Connection mPreSetupConn;
	signals::Connection mBeginFrameConn;
	signals::Connection mEndFrameConn;
	signals::Connection mUpdateConn;
	signals::Connection mPreUpdateConn;
	signals::Connection mPostUpdateConn;
	signals::Connection mPreDrawConn;
	signals::Connection mPostDrawConn;
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

			// Reset per-frame counters
			mPreDrawCountThisFrame = 0;
			mPostDrawCountThisFrame = 0;
			mWindowDrawCountThisFrame = 0;
			mLastWindowDrawn.reset();
		}
	});

	mPreUpdateConn = getSignalPreUpdate().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// Should always come from BEGIN_FRAME
			verifyStateTransition( LifecycleState::BEGIN_FRAME, LifecycleState::PRE_UPDATE );
			recordState( LifecycleState::PRE_UPDATE, "Signal" );
		}
	});

	// NOTE: There's also a getSignalUpdate() that fires between PreUpdate and the update() override
	// We don't track this as a separate state since update() override is the main event
	mUpdateConn = getSignalUpdate().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// This fires between PRE_UPDATE signal and UPDATE override
			// We just log it but don't change state since UPDATE override is more important
			cout << "[Signal] Update signal fired (between PreUpdate signal and update() override)" << endl;
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
			mPreDrawCountThisFrame++;

			// CRITICAL: PreDraw should fire exactly ONCE per frame (app-level signal)
			if( mPreDrawCountThisFrame > 1 ) {
				string error = "PreDraw signal fired " + to_string(mPreDrawCountThisFrame) +
				               " times in frame " + to_string(mFrameCount) + " (should be exactly 1)";
				mErrors.push_back( error );
				mTestsPassed = false;
				cerr << "ERROR: " << error << endl;
			}

			verifyStateTransition( LifecycleState::POST_UPDATE, LifecycleState::PRE_DRAW );
			recordState( LifecycleState::PRE_DRAW, "Signal" );
		}
	});

	mPostDrawConn = getSignalPostDraw().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			mPostDrawCountThisFrame++;

			// CRITICAL: PostDraw should fire exactly ONCE per frame (app-level signal)
			if( mPostDrawCountThisFrame > 1 ) {
				string error = "PostDraw signal fired " + to_string(mPostDrawCountThisFrame) +
				               " times in frame " + to_string(mFrameCount) + " (should be exactly 1)";
				mErrors.push_back( error );
				mTestsPassed = false;
				cerr << "ERROR: " << error << endl;
			}

			// CRITICAL: PostDraw should fire AFTER all windows have drawn
			int expectedWindowDraws = 2; // We have 2 windows
			if( mWindowDrawCountThisFrame < expectedWindowDraws ) {
				string error = "PostDraw signal fired after only " + to_string(mWindowDrawCountThisFrame) +
				               " window draws (expected " + to_string(expectedWindowDraws) + ") in frame " +
				               to_string(mFrameCount);
				mErrors.push_back( error );
				mTestsPassed = false;
				cerr << "ERROR: " << error << endl;
			}

			verifyStateTransition( LifecycleState::DRAW, LifecycleState::POST_DRAW );
			recordState( LifecycleState::POST_DRAW, "Signal" );
		}
	});

	mEndFrameConn = getSignalEndFrame().connect( [this]() {
		if( mFrameCount < mMaxFrames ) {
			// CRITICAL: Verify PreDraw and PostDraw fired exactly once this frame
			if( mPreDrawCountThisFrame != 1 ) {
				string error = "PreDraw signal fired " + to_string(mPreDrawCountThisFrame) +
				               " times in frame " + to_string(mFrameCount) + " (should be exactly 1)";
				mErrors.push_back( error );
				mTestsPassed = false;
				cerr << "ERROR: " << error << endl;
			}
			if( mPostDrawCountThisFrame != 1 ) {
				string error = "PostDraw signal fired " + to_string(mPostDrawCountThisFrame) +
				               " times in frame " + to_string(mFrameCount) + " (should be exactly 1)";
				mErrors.push_back( error );
				mTestsPassed = false;
				cerr << "ERROR: " << error << endl;
			}

			// Should come after POST_DRAW
			verifyStateTransition( LifecycleState::POST_DRAW, LifecycleState::END_FRAME );
			recordState( LifecycleState::END_FRAME, "Signal" );

			// Increment frame count at END of frame (after all windows have drawn)
			mFrameCount++;

			// If this was the last frame, print summary and quit
			if( mFrameCount == mMaxFrames ) {
				printSummary();

				// Auto-quit after showing results
				std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
				quit();
			}
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

	// Create a second window for multi-window testing
	cout << "\n=== Creating Second Window for Multi-Window Test ===" << endl;
	mSecondWindow = createWindow( Window::Format().size( 300, 200 ).pos( 450, 100 ).title( "Second Window" ) );
	cout << "Second window created. Testing that app-level signals fire once per frame..." << endl;

	// Initialize draw call counters for both windows
	mDrawCallsPerWindow[getWindow()] = 0;
	mDrawCallsPerWindow[mSecondWindow] = 0;

	cout << "Setup complete. Press ESC to quit." << endl;
	cout << "Running for " << mMaxFrames << " frames with 2 windows..." << endl;
}

void AppLifecycleTest::update()
{
	// Mark setup as complete on first update - only count draws after this point
	if( !mSetupComplete ) {
		mSetupComplete = true;
	}

	if( mFrameCount < mMaxFrames ) {
		verifyStateTransition( LifecycleState::PRE_UPDATE, LifecycleState::UPDATE );
		recordState( LifecycleState::UPDATE, "Override" );

		cout << "\n--- Frame " << (mFrameCount + 1) << " ---" << endl;
	}
}

void AppLifecycleTest::draw()
{
	WindowRef currentWindow = getWindow();

	if( mFrameCount < mMaxFrames && mSetupComplete ) {
		// Track which window is being drawn and count draw calls per window
		// (Only count draws that happen after setup completes and within our frame limit)
		mDrawCallsPerWindow[currentWindow]++;
		mActiveWindowDuringDraw = currentWindow;
		mWindowDrawCountThisFrame++;
		mLastWindowDrawn = currentWindow;

		// Capture the display frame number (for consistent display across windows)
		int displayFrameNum = mFrameCount + 1;

		// PreDraw signal should have been called before draw() - but only once for the first window
		if( currentWindow == getWindowIndex(0) ) {
			verifyStateTransition( LifecycleState::PRE_DRAW, LifecycleState::DRAW );
			recordState( LifecycleState::DRAW, "Override" );
		}

		gl::clear( Color( 0.2f, 0.2f, 0.3f ) );

		// Draw frame counter (use captured value so both windows show same frame number)
		gl::drawString( "Frame: " + to_string( displayFrameNum ), vec2( 10, 30 ) );
		gl::drawString( "Press ESC to quit", vec2( 10, 50 ) );

		// Show which window this is
		if( currentWindow == getWindowIndex(0) ) {
			gl::drawString( "Window: Main (1 of 2)", vec2( 10, 90 ) );
		} else {
			gl::drawString( "Window: Second (2 of 2)", vec2( 10, 90 ) );
		}

		if( mTestsPassed ) {
			gl::color( 0, 1, 0 );
			gl::drawString( "All tests passing!", vec2( 10, 70 ) );
		} else {
			gl::color( 1, 0, 0 );
			gl::drawString( "Test failures detected!", vec2( 10, 70 ) );
		}
	}

	// Frame count is now incremented in EndFrame signal (after ALL windows have drawn)
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

void AppLifecycleTest::verifySequence()
{
	// Define the expected sequence for the first frame
	// This is the EXACT order events should occur
	vector<pair<LifecycleState, string>> expectedFirstFrame = {
		{ LifecycleState::PRE_SETUP,   "Signal" },
		{ LifecycleState::SETUP,       "Override" },
		{ LifecycleState::BEGIN_FRAME, "Signal" },
		{ LifecycleState::PRE_UPDATE,  "Signal" },
		{ LifecycleState::UPDATE,      "Override" },
		{ LifecycleState::POST_UPDATE, "Signal" },
		{ LifecycleState::PRE_DRAW,    "Signal" },
		{ LifecycleState::DRAW,        "Override" },
		{ LifecycleState::POST_DRAW,   "Signal" },
		{ LifecycleState::END_FRAME,   "Signal" }
	};

	// Expected sequence for subsequent frames
	vector<pair<LifecycleState, string>> expectedSubsequentFrame = {
		{ LifecycleState::BEGIN_FRAME, "Signal" },
		{ LifecycleState::PRE_UPDATE,  "Signal" },
		{ LifecycleState::UPDATE,      "Override" },
		{ LifecycleState::POST_UPDATE, "Signal" },
		{ LifecycleState::PRE_DRAW,    "Signal" },
		{ LifecycleState::DRAW,        "Override" },
		{ LifecycleState::POST_DRAW,   "Signal" },
		{ LifecycleState::END_FRAME,   "Signal" }
	};

	cout << "\n=== Sequence Verification ===" << endl;

	// Extract frames
	vector<vector<pair<LifecycleState, string>>> actualFrames;
	vector<pair<LifecycleState, string>> currentFrame;

	for( const auto& record : mStateHistory ) {
		currentFrame.push_back( { record.state, record.source } );
		if( record.state == LifecycleState::END_FRAME ) {
			actualFrames.push_back( currentFrame );
			currentFrame.clear();
		}
	}

	bool allFramesMatch = true;

	// Verify first frame
	if( !actualFrames.empty() ) {
		cout << "\n--- First Frame ---" << endl;
		cout << "Expected sequence:" << endl;
		for( size_t i = 0; i < expectedFirstFrame.size(); i++ ) {
			cout << "  " << (i+1) << ". " << stateToString( expectedFirstFrame[i].first )
			     << " [" << expectedFirstFrame[i].second << "]" << endl;
		}

		cout << "\nActual sequence:" << endl;
		const auto& actualFirstFrame = actualFrames[0];
		for( size_t i = 0; i < actualFirstFrame.size(); i++ ) {
			cout << "  " << (i+1) << ". " << stateToString( actualFirstFrame[i].first )
			     << " [" << actualFirstFrame[i].second << "]" << endl;
		}

		if( expectedFirstFrame.size() != actualFirstFrame.size() ) {
			string error = "Frame 1: Sequence length mismatch: expected " + to_string( expectedFirstFrame.size() ) +
			               " events, got " + to_string( actualFirstFrame.size() );
			mErrors.push_back( error );
			mTestsPassed = false;
			allFramesMatch = false;
			cerr << "ERROR: " << error << endl;
		} else {
			for( size_t i = 0; i < expectedFirstFrame.size(); i++ ) {
				if( expectedFirstFrame[i].first != actualFirstFrame[i].first ||
				    expectedFirstFrame[i].second != actualFirstFrame[i].second ) {
					string error = "Frame 1: Sequence mismatch at position " + to_string(i+1) +
					               ": expected " + string(stateToString( expectedFirstFrame[i].first )) +
					               " [" + expectedFirstFrame[i].second + "], got " +
					               string(stateToString( actualFirstFrame[i].first )) +
					               " [" + actualFirstFrame[i].second + "]";
					mErrors.push_back( error );
					mTestsPassed = false;
					allFramesMatch = false;
					cerr << "ERROR: " << error << endl;
				}
			}
		}
	}

	// Verify subsequent frames
	for( size_t frameIdx = 1; frameIdx < actualFrames.size() && frameIdx < (size_t)mMaxFrames; frameIdx++ ) {
		cout << "\n--- Frame " << (frameIdx + 1) << " ---" << endl;
		const auto& actualFrame = actualFrames[frameIdx];

		if( expectedSubsequentFrame.size() != actualFrame.size() ) {
			string error = "Frame " + to_string(frameIdx + 1) + ": Sequence length mismatch: expected " +
			               to_string( expectedSubsequentFrame.size() ) + " events, got " +
			               to_string( actualFrame.size() );
			mErrors.push_back( error );
			mTestsPassed = false;
			allFramesMatch = false;
			cerr << "ERROR: " << error << endl;
		} else {
			bool frameMatches = true;
			for( size_t i = 0; i < expectedSubsequentFrame.size(); i++ ) {
				if( expectedSubsequentFrame[i].first != actualFrame[i].first ||
				    expectedSubsequentFrame[i].second != actualFrame[i].second ) {
					string error = "Frame " + to_string(frameIdx + 1) + ": Sequence mismatch at position " +
					               to_string(i+1) + ": expected " +
					               string(stateToString( expectedSubsequentFrame[i].first )) +
					               " [" + expectedSubsequentFrame[i].second + "], got " +
					               string(stateToString( actualFrame[i].first )) +
					               " [" + actualFrame[i].second + "]";
					mErrors.push_back( error );
					mTestsPassed = false;
					allFramesMatch = false;
					frameMatches = false;
					cerr << "ERROR: " << error << endl;
				}
			}
			if( frameMatches ) {
				cout << "[PASS] Frame " << (frameIdx + 1) << " sequence correct" << endl;
			}
		}
	}

	cout << "\n[" << (allFramesMatch ? "PASS" : "FAIL") << "] Event sequence verification for all "
	     << actualFrames.size() << " frames" << endl;
}

void AppLifecycleTest::printSummary()
{
	cout << "\n\n========================================" << endl;
	cout << "         TEST SUMMARY REPORT" << endl;
	cout << "========================================" << endl;

	// Verify the complete sequence FIRST
	verifySequence();

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
	bool hasSetup = false;
	bool hasPreUpdate = false;
	bool hasUpdate = false;
	bool hasPostUpdate = false;
	bool hasPreDraw = false;
	bool hasDraw = false;
	bool hasPostDraw = false;

	for( const auto& record : mStateHistory ) {
		if( record.state == LifecycleState::BEGIN_FRAME && record.source == "Signal" )
			hasBeginFrame = true;
		if( record.state == LifecycleState::END_FRAME && record.source == "Signal" )
			hasEndFrame = true;
		if( record.state == LifecycleState::PRE_SETUP && record.source == "Signal" )
			hasPreSetup = true;
		if( record.state == LifecycleState::SETUP && record.source == "Override" )
			hasSetup = true;
		if( record.state == LifecycleState::PRE_UPDATE && record.source == "Signal" )
			hasPreUpdate = true;
		if( record.state == LifecycleState::UPDATE && record.source == "Override" )
			hasUpdate = true;
		if( record.state == LifecycleState::POST_UPDATE && record.source == "Signal" )
			hasPostUpdate = true;
		if( record.state == LifecycleState::PRE_DRAW && record.source == "Signal" )
			hasPreDraw = true;
		if( record.state == LifecycleState::DRAW && record.source == "Override" )
			hasDraw = true;
		if( record.state == LifecycleState::POST_DRAW && record.source == "Signal" )
			hasPostDraw = true;
	}

	cout << "[" << (hasPreSetup ? "PASS" : "FAIL") << "] PreSetup signal fired" << endl;
	cout << "[" << (hasSetup ? "PASS" : "FAIL") << "] setup() override called" << endl;
	cout << "[" << (hasBeginFrame ? "PASS" : "FAIL") << "] BeginFrame signal fired" << endl;
	cout << "[" << (hasPreUpdate ? "PASS" : "FAIL") << "] PreUpdate signal fired" << endl;
	cout << "[" << (hasUpdate ? "PASS" : "FAIL") << "] update() override called" << endl;
	cout << "[" << (hasPostUpdate ? "PASS" : "FAIL") << "] PostUpdate signal fired" << endl;
	cout << "[" << (hasPreDraw ? "PASS" : "FAIL") << "] PreDraw signal fired" << endl;
	cout << "[" << (hasDraw ? "PASS" : "FAIL") << "] draw() override called" << endl;
	cout << "[" << (hasPostDraw ? "PASS" : "FAIL") << "] PostDraw signal fired" << endl;
	cout << "[" << (hasEndFrame ? "PASS" : "FAIL") << "] EndFrame signal fired" << endl;

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

	// Verify multi-window behavior
	cout << "\nMulti-Window Verification:" << endl;
	cout << "--------------------------" << endl;

	int expectedDrawsPerWindow = mMaxFrames;
	bool multiWindowTestPassed = true;

	// Check that each window was drawn the correct number of times
	for( const auto& pair : mDrawCallsPerWindow ) {
		WindowRef window = pair.first;
		int drawCount = pair.second;
		bool isCorrect = (drawCount == expectedDrawsPerWindow);

		string windowName = (window == getWindowIndex(0)) ? "Main window" : "Second window";
		cout << "[" << (isCorrect ? "PASS" : "FAIL") << "] " << windowName << " drawn "
		     << drawCount << " times (expected " << expectedDrawsPerWindow << ")" << endl;

		if( !isCorrect ) {
			multiWindowTestPassed = false;
		}
	}

	// The critical test: PreDraw and PostDraw should have fired exactly once per frame
	// NOT once per window (which would be mMaxFrames * 2 for 2 windows)
	// Check if we recorded any PreDraw/PostDraw errors
	bool preDrawTestPassed = true;
	bool postDrawTestPassed = true;
	for( const auto& error : mErrors ) {
		if( error.find("PreDraw signal fired") != string::npos ) {
			preDrawTestPassed = false;
		}
		if( error.find("PostDraw signal fired") != string::npos ) {
			postDrawTestPassed = false;
		}
	}
	cout << "[" << (preDrawTestPassed ? "PASS" : "FAIL") << "] PreDraw signal fired exactly once per frame" << endl;
	cout << "[" << (postDrawTestPassed ? "PASS" : "FAIL") << "] PostDraw signal fired exactly once per frame" << endl;
	cout << "[" << (postDrawTestPassed ? "PASS" : "FAIL") << "] PostDraw signal fired after all windows drawn" << endl;

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
	bool allTestsPassed = mTestsPassed &&
	                      hasPreSetup && hasSetup &&
	                      hasBeginFrame &&
	                      hasPreUpdate && hasUpdate && hasPostUpdate &&
	                      hasPreDraw && hasDraw && hasPostDraw &&
	                      hasEndFrame &&
	                      initCallbacksPassed &&
	                      hasThreadNames &&
	                      mThreadNames.size() >= 3 &&
	                      multiWindowTestPassed;

	if( allTestsPassed ) {
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
#if defined( CINDER_MSW )
	settings->setConsoleWindowEnabled( true );
#endif
}

CINDER_APP( AppLifecycleTest, RendererGl, prepareSettings )
