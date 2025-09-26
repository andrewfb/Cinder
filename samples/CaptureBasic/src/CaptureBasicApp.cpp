#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/CinderImGui.h"
#include "cinder/Utilities.h"
#include <cmath>

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
	void setupCaptureWithMode( Capture::DeviceRef device, const Capture::Mode& mode );
	void updateModes();

	CaptureRef								mCapture;
	gl::TextureRef							mTexture;
	std::vector<Capture::DeviceRef>			mDevices;
	int										mSelectedDeviceIndex;
	bool									mShowUI;

	// Mode selection
	std::vector<Capture::Mode>				mCurrentModes;
	int										mSelectedModeIndex;
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

		// Device selection with refresh button
		if( ! mDevices.empty() ) {
			// Ensure selected index is valid
			if( mSelectedDeviceIndex >= (int)mDevices.size() ) {
				mSelectedDeviceIndex = 0;
			}

			// Device dropdown and refresh button on same line
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
		}

		// Add refresh button on next line, flush right
		if( ! mDevices.empty() ) {
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

		// Mode selection dropdown
		if( ! mCurrentModes.empty() ) {
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
							mSelectedModeIndex = i;
							setupCaptureWithMode( mDevices[mSelectedDeviceIndex], mCurrentModes[i] );
						}
					}
					if( isSelected ) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		// Current mode details
		if( mCapture ) {
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

				// Show frame rate range if available
				if( mode.hasFrameRateRange() ) {
					float minFps = mode.getMinFrameRateFloat();
					float maxFps = mode.getMaxFrameRateFloat();
					if( minFps != maxFps ) {
						ImGui::Text( "Frame Rate Range: %.1f - %.1f FPS", minFps, maxFps );
					}
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

		ImGui::End();
	}

}

void CaptureBasicApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_SPACE ) {
		mShowUI = !mShowUI;
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

void CaptureBasicApp::setupCaptureWithMode( Capture::DeviceRef device, const Capture::Mode& mode )
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
	}
	catch( ci::Exception &exc ) {
		CI_LOG_EXCEPTION( "Failed to setup capture with mode: " << toString( mode ) << " on device: " << device->getName(), exc );
		// Fall back to regular capture
		setupCapture( device );
	}
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

void prepareSettings( CaptureBasicApp::Settings* settings )
{
#if defined( CINDER_ANDROID )
	settings->setKeepScreenOn( true );
#endif
}

CINDER_APP( CaptureBasicApp, RendererGl, prepareSettings )
