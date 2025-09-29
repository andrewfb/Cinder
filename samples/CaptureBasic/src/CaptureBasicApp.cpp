#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/CinderImGui.h"
#include "cinder/Utilities.h"

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

	CaptureRef						mCapture;
	gl::TextureRef					mTexture;
	std::vector<Capture::DeviceRef> mDevices;
	int								mSelectedDeviceIndex;
	bool							mShowUI;

	// Mode selection
	std::vector<Capture::Mode> mCurrentModes;
	int						   mSelectedModeIndex;
};

void CaptureBasicApp::setup()
{
	console() << "=== CaptureBasic starting ===" << endl;

	// Initialize ImGui
	ImGui::Initialize();

	// Get devices
	mDevices = Capture::getDevices();
	console() << "Found " << mDevices.size() << " capture devices" << endl;

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
	static int	frameCount = 0;
	static bool firstFrame = true;

#if defined( USE_HW_TEXTURE )
	if( mCapture && mCapture->checkNewFrame() ) {
		mTexture = mCapture->getTexture();
		frameCount++;
		if( firstFrame ) {
			console() << "First frame received!" << endl;
			firstFrame = false;
		}
	}
#else
	if( mCapture && mCapture->checkNewFrame() ) {
		auto surface = mCapture->getSurface();
		if( surface ) {
			frameCount++;
			if( firstFrame || frameCount % 100 == 0 ) {
				console() << "Frame " << frameCount << " received - Surface size: " << surface->getWidth() << "x" << surface->getHeight() << endl;
				firstFrame = false;
			}

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
		}
		else {
			// Show message when no devices available
			ImGui::TextColored( ImVec4( 1.0f, 0.6f, 0.0f, 1.0f ), "No Capture Devices Found" );
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
						if( mCurrentModes[i].getWidth() == currentWidth && mCurrentModes[i].getHeight() == currentHeight ) {
							modeDescription = "Auto (" + toString( mCurrentModes[i] ) + ")";
							foundMatch = true;
							break;
						}
					}
					if( ! foundMatch ) {
						modeDescription = "Auto (" + std::to_string( currentWidth ) + "x" + std::to_string( currentHeight ) + ")";
					}
				}
				else {
					modeDescription = "Auto";
				}
			}
			else if( mSelectedModeIndex >= 0 && mSelectedModeIndex < (int)mCurrentModes.size() ) {
				modeDescription = toString( mCurrentModes[mSelectedModeIndex] );
			}
			else {
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
					bool   isSelected = ( mSelectedModeIndex == i );
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
				ImGui::TextColored( ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ), "Status: Connected" );
			}
			else {
				ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "Status: Disconnected" );
			}

			// If we have a selected mode, show its details
			if( mSelectedModeIndex >= 0 && mSelectedModeIndex < (int)mCurrentModes.size() ) {
				const auto& mode = mCurrentModes[mSelectedModeIndex];


				// Format frame rate - show whole numbers without decimals, keep decimals for non-whole numbers
				float fps = mode.getFrameRateFloat();
				if( fps == floor( fps ) ) {
					ImGui::Text( "Frame Rate: %.0f FPS", fps );
				}
				else {
					ImGui::Text( "Frame Rate: %.2f FPS", fps );
				}

				ImGui::Text( "Codec: %s", mode.getCodecString().c_str() );
				ImGui::Text( "Pixel Format: %s", mode.getPixelFormatString().c_str() );
				ImGui::Text( "Aspect Ratio: %.3f", mode.getAspectRatio() );
				ImGui::Text( "Compressed: %s", mode.isCompressed() ? "Yes" : "No" );
				ImGui::Text( "Color Model: %s", mode.isRGBFormat() ? "RGB" : ( mode.isYUVFormat() ? "YUV" : "Unknown" ) );
				if( ! mode.getDescription().empty() ) {
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
		mShowUI = ! mShowUI;
	}
}

void CaptureBasicApp::setupCapture( Capture::DeviceRef device )
{
	console() << "\n=== Setting up capture with device: " << device->getName() << " (auto mode) ===" << endl;
	try {
		// Stop and fully release old capture
		if( mCapture ) {
			console() << "Stopping previous capture" << endl;
			mCapture->stop();
		}

		// Reset everything to ensure clean state
		mCapture = nullptr;
		mTexture = nullptr;

		// Use a working mode instead of the first one (which might be problematic)
		if( ! mCurrentModes.empty() ) {
			// Try to find 2560x720 mode that we know works, otherwise use first mode
			int modeIndex = 0;
			for( size_t i = 0; i < mCurrentModes.size(); i++ ) {
				if( mCurrentModes[i].getWidth() == 2560 && mCurrentModes[i].getHeight() == 720 ) {
					modeIndex = i;
					break;
				}
			}
			console() << "Creating capture with mode [" << modeIndex << "]: " << toString( mCurrentModes[modeIndex] ) << endl;
			mCapture = Capture::create( device, mCurrentModes[modeIndex] );
		}
		else {
			console() << "No modes available, trying default 640x480" << endl;
			mCapture = Capture::create( 640, 480, device );
		}
		mCapture->start();
		console() << "Capture started successfully - actual size: " << mCapture->getWidth() << "x" << mCapture->getHeight() << endl;
	}
	catch( ci::Exception& exc ) {
		CI_LOG_EXCEPTION( "Failed to setup capture with device: " << device->getName(), exc );
		console() << "ERROR: Failed to setup capture!" << endl;
		// Leave mCapture as nullptr if setup fails
		mCapture = nullptr;
		mTexture = nullptr;
	}
}

void CaptureBasicApp::printDevices()
{
	for( const auto& device : Capture::getDevices() ) {
		console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
				  << ( device->isFrontFacing() ? "Front" : "Rear" ) << "-facing"
#endif
				  << endl;
	}
}

bool CaptureBasicApp::setupCaptureWithMode( Capture::DeviceRef device, const Capture::Mode& mode )
{
	console() << "\n=== Setting up capture with specific mode ===" << endl;
	console() << "Device: " << device->getName() << endl;
	console() << "Mode: " << toString( mode ) << endl;

	try {
		// Stop and fully release old capture
		if( mCapture ) {
			console() << "Stopping previous capture" << endl;
			mCapture->stop();
		}

		// Reset everything to ensure clean state
		mCapture = nullptr;
		mTexture = nullptr;

		// Create capture with specific mode
		console() << "Creating capture with mode..." << endl;
		mCapture = Capture::create( device, mode );
		console() << "Starting capture..." << endl;
		mCapture->start();

		console() << "Capture started successfully!" << endl;
		console() << "Actual size: " << mCapture->getWidth() << "x" << mCapture->getHeight() << endl;
		CI_LOG_I( "Created capture with mode: " << toString( mode ) );
		return true;
	}
	catch( ci::Exception& exc ) {
		CI_LOG_EXCEPTION( "Failed to setup capture with mode: " << toString( mode ) << " on device: " << device->getName(), exc );
		console() << "ERROR: Failed to setup capture with mode! Falling back to auto mode." << endl;
		// Fall back to regular capture
		setupCapture( device );
		return false;
	}
}


void CaptureBasicApp::updateModes()
{
	mCurrentModes.clear();
	mSelectedModeIndex = -1; // Reset to auto mode

	if( mSelectedDeviceIndex >= 0 && mSelectedDeviceIndex < (int)mDevices.size() ) {
		console() << "\n=== Getting modes for device: " << mDevices[mSelectedDeviceIndex]->getName() << " ===" << endl;
		try {
			mCurrentModes = mDevices[mSelectedDeviceIndex]->getModes();
			console() << "Found " << mCurrentModes.size() << " modes:" << endl;
			for( size_t i = 0; i < mCurrentModes.size(); i++ ) {
				console() << "  [" << i << "] " << toString( mCurrentModes[i] ) << endl;
			}
		}
		catch( ci::Exception& exc ) {
			CI_LOG_EXCEPTION( "Failed to get modes for device: " << mDevices[mSelectedDeviceIndex]->getName(), exc );
			console() << "ERROR: Failed to get modes!" << endl;
		}
	}
}


void prepareSettings( CaptureBasicApp::Settings* settings )
{
#if defined( CINDER_ANDROID )
	settings->setKeepScreenOn( true );
#endif

#if defined( CINDER_MSW )
	// Enable console window on Windows for debugging
	settings->setConsoleWindowEnabled( true );
#endif
}

CINDER_APP( CaptureBasicApp, RendererGl, prepareSettings )
