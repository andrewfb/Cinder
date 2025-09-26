#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/CinderImGui.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include <cmath>
#include <algorithm>

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined( CINDER_ANDROID )
    #define USE_HW_TEXTURE
#endif

class CaptureBasicApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown( KeyEvent event ) override;

  private:
	void printDevices();
	void setupCapture( Capture::DeviceRef device );
	bool setupCaptureWithMode( Capture::DeviceRef device, const Capture::Mode& mode );
	void updateModes();
	void startStressTest();
	void stopStressTest();
	void performStressSwitch();
	void scheduleNextStressSwitch();
	void diagnoseModeCapabilities(); // TEMPORARY: diagnostic function

	CaptureRef								mCapture;
	gl::TextureRef							mTexture;
	std::vector<Capture::DeviceRef>			mDevices;
	int										mSelectedDeviceIndex;
	bool									mShowUI;

	// Mode selection
	std::vector<Capture::Mode>				mCurrentModes;
	int										mSelectedModeIndex;

	// Stress test controls
	bool									mStressTestActive;
	double									mNextStressSwitchTime;
	double									mCurrentStressHoldDuration;
};

void CaptureBasicApp::setup()
{
	// Initialize ImGui
	ImGui::Initialize();

	// Get devices
	mDevices = Capture::getDevices();
	mSelectedDeviceIndex = 0;
	mSelectedModeIndex = -1; // -1 means auto mode
	mShowUI = true;
	mStressTestActive = false;
	mNextStressSwitchTime = -1.0;
	mCurrentStressHoldDuration = 0.0;

	printDevices();

	// Start with first device if available
	if( ! mDevices.empty() ) {
		updateModes();
		setupCapture( mDevices[0] );
	}
}

void CaptureBasicApp::update()
{
#if defined( USE_HW_TEXTURE )
	if( mCapture && mCapture->checkNewFrame() ) {
	    mTexture = mCapture->getTexture();
	}
#else
	if( mCapture && mCapture->checkNewFrame() ) {
		auto surface = mCapture->getSurface();
		if( surface ) {
			if( ! mTexture ) {
				// Capture images come back as top-down, and it's more efficient to keep them that way
				mTexture = gl::Texture::create( *surface, gl::Texture::Format().loadTopDown() );
			}
			else {
				mTexture->update( *surface );
			}
		}
	}
#endif

	if( mStressTestActive && ( getElapsedSeconds() >= mNextStressSwitchTime ) ) {
		performStressSwitch();
	}
}

void CaptureBasicApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.1f ) );
	gl::enableAlphaBlending();

	// Draw video texture centered and scaled to fit window
	if( mTexture ) {
		Rectf destRect = Rectf( mTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mTexture, destRect );
	}

	// Draw ImGui
	if( mShowUI ) {
		ImGui::Begin( "Video Capture Control" );

		// Device selection or "No devices" message
		if( ! mDevices.empty() ) {
			// Ensure selected index is valid
			if( mSelectedDeviceIndex >= (int)mDevices.size() ) {
				mSelectedDeviceIndex = 0;
			}

			// Device dropdown
			if( ImGui::BeginCombo( "Camera Device", mDevices[mSelectedDeviceIndex]->getName().c_str() ) ) {
				for( int i = 0; i < mDevices.size(); i++ ) {
					bool isSelected = ( mSelectedDeviceIndex == i );
					if( ImGui::Selectable( mDevices[i]->getName().c_str(), isSelected ) ) {
						if( mSelectedDeviceIndex != i ) {
							mSelectedDeviceIndex = i;
							updateModes();
							// Reset to auto mode when switching devices
							mSelectedModeIndex = -1;
							setupCapture( mDevices[i] );
						}
					}
					if( isSelected ) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		} else {
			// Show message when no devices available
			ImGui::TextColored( ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "No Capture Devices Found" );
		}

		// Add refresh button on next line, flush right (always show)
		{
			float buttonWidth = 120.0f;
			float availableWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX( ImGui::GetCursorPosX() + availableWidth - buttonWidth );
			if( ImGui::Button( "Refresh Devices", ImVec2(buttonWidth, 0) ) ) {
				CI_LOG_I( "Refreshing device list..." );

				// Stop current capture if running
				if( mCapture ) {
					mCapture->stop();
					mCapture = nullptr;
					mTexture = nullptr;
				}

				// Get the new device list - force refresh to ensure we get current state
				auto newDevices = Capture::getDevices( true );

				// Print changes for debugging
				CI_LOG_I( "Previous device count: " << mDevices.size() << ", New device count: " << newDevices.size() );

				// Update the device list
				mDevices = newDevices;

				// Print the refreshed device list
				printDevices();

				// Handle device selection after refresh
				if( mDevices.empty() ) {
					mSelectedDeviceIndex = 0;
					// Clear modes when no devices
					mCurrentModes.clear();
					mSelectedModeIndex = -1;
					CI_LOG_W( "No capture devices found after refresh" );
				} else {
					// Try to maintain current device selection if possible
					if( mSelectedDeviceIndex >= (int)mDevices.size() ) {
						mSelectedDeviceIndex = 0;
					}

					// Update modes for the selected device
					updateModes();

					// Restart capture with selected device
					setupCapture( mDevices[mSelectedDeviceIndex] );
				}
			}
		}

		// Mode selection dropdown - only show when we have devices
		if( ! mDevices.empty() && ! mCurrentModes.empty() ) {
			// Determine display text for current mode
			string modeDescription;
			if( mSelectedModeIndex == -1 ) {
				// Auto mode - try to match current resolution with available modes
				if( mCapture ) {
					int currentWidth = mCapture->getWidth();
					int currentHeight = mCapture->getHeight();

					// Find matching mode
					bool foundMatch = false;
					for( size_t i = 0; i < mCurrentModes.size(); i++ ) {
						if( mCurrentModes[i].getWidth() == currentWidth &&
							mCurrentModes[i].getHeight() == currentHeight ) {
							modeDescription = "Auto (" + toString( mCurrentModes[i] ) + ")";
							foundMatch = true;
							break;
						}
					}
					if( ! foundMatch ) {
						modeDescription = "Auto (" + std::to_string(currentWidth) + "x" + std::to_string(currentHeight) + ")";
					}
				} else {
					modeDescription = "Auto";
				}
			} else if( mSelectedModeIndex >= 0 && mSelectedModeIndex < (int)mCurrentModes.size() ) {
				modeDescription = toString( mCurrentModes[mSelectedModeIndex] );
			} else {
				modeDescription = "Invalid mode";
			}

			if( ImGui::BeginCombo( "Capture Mode", modeDescription.c_str() ) ) {
				// Auto option
				bool isAutoSelected = ( mSelectedModeIndex == -1 );
				if( ImGui::Selectable( "Auto", isAutoSelected ) ) {
					if( mSelectedModeIndex != -1 ) {
						mSelectedModeIndex = -1;
						setupCapture( mDevices[mSelectedDeviceIndex] );
					}
				}
				if( isAutoSelected ) {
					ImGui::SetItemDefaultFocus();
				}

				// Separator between auto and specific modes
				ImGui::Separator();

				// Specific mode options
				for( int i = 0; i < mCurrentModes.size(); i++ ) {
					bool isSelected = ( mSelectedModeIndex == i );
					string description = toString( mCurrentModes[i] );
					if( ImGui::Selectable( description.c_str(), isSelected ) ) {
						if( mSelectedModeIndex != i ) {
							if( setupCaptureWithMode( mDevices[mSelectedDeviceIndex], mCurrentModes[i] ) ) {
								mSelectedModeIndex = i;
							}
							else {
								mSelectedModeIndex = -1;
							}
						}
					}
					if( isSelected ) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		// Current mode details - only show when we have devices
		if( ! mDevices.empty() && mCapture ) {
			ImGui::Separator();
			ImGui::Text( "Current Mode Details:" );
			ImGui::Text( "Resolution: %d x %dpx", mCapture->getWidth(), mCapture->getHeight() );

			// Connection status
			if( mCapture->isCapturing() ) {
				ImGui::TextColored( ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Connected" );
			} else {
				ImGui::TextColored( ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Disconnected" );
			}

			// If we have a selected mode, show its details
			if( mSelectedModeIndex >= 0 && mSelectedModeIndex < (int)mCurrentModes.size() ) {
				const auto& mode = mCurrentModes[mSelectedModeIndex];


				// Format frame rate - show whole numbers without decimals, keep decimals for non-whole numbers
				float fps = mode.getFrameRateFloat();
				if( fps == floor(fps) ) {
					ImGui::Text( "Frame Rate: %.0f FPS", fps );
				} else {
					ImGui::Text( "Frame Rate: %.2f FPS", fps );
				}

				ImGui::Text( "Codec: %s", mode.getCodecString().c_str() );
				ImGui::Text( "Pixel Format: %s", mode.getPixelFormatString().c_str() );
				ImGui::Text( "Aspect Ratio: %.3f", mode.getAspectRatio() );
				ImGui::Text( "Compressed: %s", mode.isCompressed() ? "Yes" : "No" );
				ImGui::Text( "Color Model: %s", mode.isRGBFormat() ? "RGB" : (mode.isYUVFormat() ? "YUV" : "Unknown") );
				if( !mode.getDescription().empty() ) {
					ImGui::Text( "Description: %s", mode.getDescription().c_str() );
				}
			}
		}

		ImGui::Separator();
		ImGui::Text( "Stress Testing" );
		std::string stressLabel = mStressTestActive ? "Stop Stress Test" : "Start Stress Test";
		if( ImGui::Button( stressLabel.c_str() ) ) {
			if( mStressTestActive ) {
				stopStressTest();
			}
			else if( mDevices.empty() ) {
				CI_LOG_W( "Cannot start stress test, no capture devices available" );
			}
			else {
				startStressTest();
			}
		}

		if( mDevices.empty() ) {
			ImGui::TextColored( ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "No capture devices detected." );
		}
		else if( mStressTestActive ) {
			double remaining = std::max( 0.0, mNextStressSwitchTime - getElapsedSeconds() );
			const auto &device = mDevices[std::min<int>( mSelectedDeviceIndex, (int)mDevices.size() - 1 )];
			ImGui::Text( "Active device: %s", device->getName().c_str() );
			if( mSelectedModeIndex >= 0 && mSelectedModeIndex < (int)mCurrentModes.size() ) {
				ImGui::Text( "Active mode: %s", toString( mCurrentModes[mSelectedModeIndex] ).c_str() );
			}
			else {
				ImGui::Text( "Active mode: Auto" );
			}
			ImGui::Text( "Next switch in %.1f s", remaining );
			if( mCurrentStressHoldDuration > 0.0 ) {
				ImGui::Text( "Current hold target: %.1f s", mCurrentStressHoldDuration );
			}
		}
		else {
			ImGui::Text( "Stress test idle" );
		}

		ImGui::End();
	}

}

void CaptureBasicApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_SPACE ) {
		mShowUI = !mShowUI;
	}
	else if( event.getCode() == KeyEvent::KEY_d ) {
		// TEMPORARY: Press 'D' to diagnose device capabilities
		diagnoseModeCapabilities();
	}
}

void CaptureBasicApp::setupCapture( Capture::DeviceRef device )
{
	try {
		// Stop and fully release old capture
		if( mCapture ) {
			mCapture->stop();
		}

		// Reset everything to ensure clean state
		mCapture = nullptr;
		mTexture = nullptr;

		mCapture = Capture::create( 640, 480, device );
		mCapture->start();
	}
	catch( ci::Exception &exc ) {
		CI_LOG_EXCEPTION( "Failed to setup capture with device: " << device->getName(), exc );
		// Leave mCapture as nullptr if setup fails
		mCapture = nullptr;
		mTexture = nullptr;
	}
}

void CaptureBasicApp::printDevices()
{
	for( const auto &device : Capture::getDevices() ) {
		console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
		<< ( device->isFrontFacing() ? "Front" : "Rear" ) << "-facing"
#endif
		<< endl;
	}
}

bool CaptureBasicApp::setupCaptureWithMode( Capture::DeviceRef device, const Capture::Mode& mode )
{
	try {
		// Stop and fully release old capture
		if( mCapture ) {
			mCapture->stop();
		}

		// Reset everything to ensure clean state
		mCapture = nullptr;
		mTexture = nullptr;

		// Create capture with specific mode
		mCapture = Capture::create( device, mode );
		mCapture->start();

		CI_LOG_I( "Created capture with mode: " << toString( mode ) );
		return true;
	}
	catch( ci::Exception &exc ) {
		CI_LOG_EXCEPTION( "Failed to setup capture with mode: " << toString( mode ) << " on device: " << device->getName(), exc );
		// Fall back to regular capture
		setupCapture( device );
		return false;
	}
}

void CaptureBasicApp::startStressTest()
{
	if( mStressTestActive )
		return;

	if( mDevices.empty() ) {
		CI_LOG_W( "Stress test not started: no capture devices available." );
		return;
	}

	CI_LOG_I( "Starting capture stress test" );
	mStressTestActive = true;
	performStressSwitch();
}

void CaptureBasicApp::stopStressTest()
{
	if( ! mStressTestActive )
		return;

	mStressTestActive = false;
	mNextStressSwitchTime = -1.0;
	mCurrentStressHoldDuration = 0.0;
	CI_LOG_I( "Stopped capture stress test" );
}

void CaptureBasicApp::performStressSwitch()
{
	if( ! mStressTestActive )
		return;

	if( mDevices.empty() ) {
		CI_LOG_W( "Stopping stress test: no capture devices available" );
		stopStressTest();
		return;
	}

	int deviceCount = (int)mDevices.size();
	int nextDeviceIndex = ci::Rand::randInt( deviceCount );
	if( nextDeviceIndex != mSelectedDeviceIndex ) {
		mSelectedDeviceIndex = nextDeviceIndex;
		updateModes();
	}
	else if( mCurrentModes.empty() ) {
		updateModes();
	}

	if( mSelectedDeviceIndex >= deviceCount ) {
		mSelectedDeviceIndex = 0;
	}

	auto device = mDevices[mSelectedDeviceIndex];

	if( ! mCurrentModes.empty() ) {
		int modeIndex = ci::Rand::randInt( (int)mCurrentModes.size() );
		const auto &mode = mCurrentModes[modeIndex];
		bool success = setupCaptureWithMode( device, mode );
		if( success ) {
			mSelectedModeIndex = modeIndex;
			CI_LOG_I( "Stress test switched to device: " << device->getName() << ", mode: " << toString( mode ) );
		}
		else {
			mSelectedModeIndex = -1;
			CI_LOG_W( "Stress test falling back to auto mode on device: " << device->getName() );
		}
	}
	else {
		mSelectedModeIndex = -1;
		setupCapture( device );
		CI_LOG_I( "Stress test switched to device: " << device->getName() << " using auto mode" );
	}

	scheduleNextStressSwitch();
}

void CaptureBasicApp::scheduleNextStressSwitch()
{
	if( ! mStressTestActive )
		return;

	float holdSeconds = ci::Rand::randFloat( 2.0f, 6.0f );
	const float longHoldChance = 0.05f;
	bool longHold = ( ci::Rand::randFloat() < longHoldChance );
	if( longHold ) {
		holdSeconds = 10.0f * 60.0f; // 10 minutes
		CI_LOG_I( "Stress test entering extended hold" );
	}

	mCurrentStressHoldDuration = holdSeconds;
	mNextStressSwitchTime = getElapsedSeconds() + holdSeconds;
	CI_LOG_I( "Next stress test switch scheduled in " << holdSeconds << " seconds" );
}

void CaptureBasicApp::updateModes()
{
	mCurrentModes.clear();
	mSelectedModeIndex = -1; // Reset to auto mode

	if( mSelectedDeviceIndex >= 0 && mSelectedDeviceIndex < (int)mDevices.size() ) {
		try {
			mCurrentModes = mDevices[mSelectedDeviceIndex]->getModes();
		}
		catch( ci::Exception &exc ) {
			CI_LOG_EXCEPTION( "Failed to get modes for device: " << mDevices[mSelectedDeviceIndex]->getName(), exc );
		}
	}
}

// TEMPORARY: Diagnostic function to discover actual device capabilities
void CaptureBasicApp::diagnoseModeCapabilities()
{
	if( mSelectedDeviceIndex < 0 || mSelectedDeviceIndex >= (int)mDevices.size() ) {
		console() << "No valid device selected" << endl;
		return;
	}
	
	console() << "=== DIAGNOSTIC: Analyzing device capabilities ===" << endl;
	console() << "Device: " << mDevices[mSelectedDeviceIndex]->getName() << endl;
	
	// Try different resolutions that are common for cameras and see which ones work
	std::vector<std::pair<int, int>> testResolutions = {
		{160, 120}, {320, 240}, {424, 240}, {640, 360}, {640, 480},
		{672, 376}, {848, 480}, {960, 540}, {1024, 576}, {1280, 720},
		{1344, 756}, {1920, 1080}, {2208, 1242}, {2560, 1440}, {3840, 2160},
		// ZED camera specific resolutions
		{672, 376}, {1344, 756}, {2208, 1242}, {2560, 720}, {1920, 1080}
	};
	
	console() << "Testing resolutions..." << endl;
	for( const auto& res : testResolutions ) {
		try {
			// Create a temporary capture to test this resolution
			CaptureRef testCapture = Capture::create( res.first, res.second, mDevices[mSelectedDeviceIndex] );
			console() << "✓ " << res.first << "x" << res.second << " - SUPPORTED (actual: " << testCapture->getWidth() << "x" << testCapture->getHeight() << ")" << endl;
			testCapture->stop();
		}
		catch( ci::Exception &exc ) {
			// Resolution not supported - this is expected for most
		}
	}
	
	console() << "=== End diagnostic ===" << endl;
}

void prepareSettings( CaptureBasicApp::Settings* settings )
{
#if defined( CINDER_ANDROID )
	settings->setKeepScreenOn( true );
#endif
}

CINDER_APP( CaptureBasicApp, RendererGl, prepareSettings )
