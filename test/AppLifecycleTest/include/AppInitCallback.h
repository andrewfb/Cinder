/*
 * AppInitCallback.h - Header-only test for app initialization callbacks
 * This demonstrates and verifies that CinderBlocks can be header-only and register
 * callbacks that fire after app construction but before setup().
 */

#pragma once

#include "cinder/app/App.h"
#include <iostream>
#include <atomic>

namespace test {

// Global counters to verify callback execution
static std::atomic<int> gInitCallbackCount{ 0 };
static std::atomic<int> gPreSetupCallbackCount{ 0 };
static std::atomic<int> gPreUpdateCallbackCount{ 0 };
static std::atomic<bool> gCallbacksExecutedBeforeSetup{ false };

// This class demonstrates the static initializer trick
// It registers itself to be called during app initialization
class AppInitCallbackTest {
public:
	AppInitCallbackTest() {
		// Register our initialization callback
		// This happens during static initialization, before main()
		ci::app::registerAppInitCallback( []( ci::app::AppBase* app ) {
			gInitCallbackCount++;
			std::cout << "[AppInitCallback] Static initializer callback #" << gInitCallbackCount.load()
			          << " executed!" << std::endl;
			std::cout << "[AppInitCallback] App instance: " << app << std::endl;

			// Mark that callbacks executed (will verify this happened before setup)
			gCallbacksExecutedBeforeSetup = true;

			// Now hook into the preSetup signal
			app->getSignalPreSetup().connect( []() {
				gPreSetupCallbackCount++;
				std::cout << "[AppInitCallback] PreSetup signal received (callback #"
				          << gPreSetupCallbackCount.load() << ")" << std::endl;
			});

			// We can also hook other signals here
			app->getSignalPreUpdate().connect( []() {
				if( gPreUpdateCallbackCount < 3 ) { // Only log first 3 to avoid spam
					gPreUpdateCallbackCount++;
					std::cout << "[AppInitCallback] PreUpdate signal #" << gPreUpdateCallbackCount.load()
					          << " (registered from init callback)" << std::endl;
				}
			});
		});
	}
};

// Static instance that registers itself
static AppInitCallbackTest sAppInitCallbackTest;

// Another example - simulating a header-only block that needs app hooks
class HeaderOnlyBlock {
public:
	HeaderOnlyBlock() {
		ci::app::registerAppInitCallback( []( ci::app::AppBase* app ) {
			gInitCallbackCount++;
			std::cout << "[HeaderOnlyBlock] Initialization callback #" << gInitCallbackCount.load()
			          << " - simulating header-only block init" << std::endl;

			// Hook into app lifecycle
			app->getSignalPreSetup().connect( []() {
				std::cout << "[HeaderOnlyBlock] Preparing block resources..." << std::endl;
			});

			app->getSignalPostUpdate().connect( []() {
				static int updateCount = 0;
				if( updateCount++ == 5 ) { // Every 5 frames
					// Simulate periodic block work
					updateCount = 0;
				}
			});

			app->getSignalCleanup().connect( []() {
				std::cout << "[HeaderOnlyBlock] Cleaning up block resources..." << std::endl;
			});
		});
	}
};

static HeaderOnlyBlock sHeaderOnlyBlock;

// Verification functions to be called from the main test
inline int getInitCallbackCount() { return gInitCallbackCount.load(); }
inline int getPreSetupCallbackCount() { return gPreSetupCallbackCount.load(); }
inline int getPreUpdateCallbackCount() { return gPreUpdateCallbackCount.load(); }
inline bool wereCallbacksExecutedBeforeSetup() { return gCallbacksExecutedBeforeSetup.load(); }

} // namespace test