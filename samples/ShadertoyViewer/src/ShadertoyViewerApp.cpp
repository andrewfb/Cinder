// Shadertoy Viewer + Editor for Cinder
// A Shadertoy-compatible shader viewer, editor and CLI screenshot generator
// Supports OpenGL 3.3 core profile on Windows, macOS and Linux

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Resize.h"
#include "cinder/ip/Flip.h"
#include "cinder/CinderImGui.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"
#include "ChannelManager.h"
#include <algorithm>
#include <ctime>

using namespace ci;
using namespace ci::app;
using namespace std;

class ShadertoyViewerApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void fileDrop( FileDropEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;

	// Shader management
	void		loadShaderFromFile( const fs::path& path );
	void		compileShader( const std::string& fragmentSource );
	std::string wrapShaderSource( const std::string& userFragmentCode );

	// Rendering
	void renderShaderToFbo();
	void updateUniforms();
	gl::Fbo::Format createFboFormat();

	// Timing
	void updateTimekeeping();

	// ImGui interface
	void drawGui();
	void drawShaderPane();
	void drawPlaybackPane();
	void drawChannelsPane();

	// File operations
	void openShaderDialog();

	// Example shaders
	void					 loadExampleShader( int exampleIndex );
	std::vector<std::string> getExampleShaderPaths();
	std::vector<std::string> getExampleShaderNames();

  private:
	// Shader state
	gl::GlslProgRef mShaderProg;
	gl::FboRef		mRenderFbo;
	std::string		mCurrentFragmentSource;
	std::string		mShaderEditorText;
	fs::path		mCurrentShaderPath;
	std::string		mCompileLog;
	bool			mDrawGui = true;
	bool			mShaderCompileSuccess = false;
	bool			mShaderModified = false;
	int				mErrorLine = -1; // Line number of compile error (-1 = none)

	// Playback state
	bool   mPaused = false;
	double mTime = 0.0;
	double mLastUpdateTime = 0.0;
	int    mFrame = 0;
	float  mTimeScale = 1.0f;

	// Rendering state
	ivec2 mRenderResolution = ivec2( 1280, 720 );
	bool  mResolutionChanged = false;
	bool  mSupersample = false;		// true = 2x supersampling
	bool  mSupersampleChanged = false;

	// Mouse state (Shadertoy convention: xy = current pos, zw = click pos)
	vec4 mMouseState = vec4( 0.0f );
	bool mMouseDown = false;

	// Date state (cached per frame)
	vec4 mDate = vec4( 0.0f );

	// Channel manager
	ChannelManager mChannelManager;
	TextureType	   mLastChannelTypes[4] = { TextureType::Texture2D, TextureType::Texture2D, TextureType::Texture2D, TextureType::Texture2D };

	// ImGui state
	bool		mShowShaderPane = true;
	bool		mShowPlaybackPane = true;
	bool		mShowChannelsPane = true;
	bool		mAutoCompile = false;
	std::string mShaderTextBuffer; // Dynamic buffer for shader editor (no size limit)
};

void ShadertoyViewerApp::setup()
{
	setWindowSize( 1920, 1080 );

	// Initialize ImGui for interactive mode
	ImGui::Initialize();

	// Set larger font size
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.3f;

	// Initialize timer using Cinder's built-in timer
	mLastUpdateTime = getElapsedSeconds();

	// Create initial FBO
	mRenderFbo = gl::Fbo::create( mRenderResolution.x, mRenderResolution.y, createFboFormat() );

	// Load first example shader
	auto examples = getExampleShaderPaths();
	if( ! examples.empty() ) {
		loadExampleShader( 0 );
	}
	else {
		CI_LOG_W( "No example shaders found" );
	}

	CI_LOG_I( "Shadertoy Viewer initialized" );
}

void ShadertoyViewerApp::update()
{
	updateTimekeeping();

	// Recreate FBO if resolution or supersampling changed
	if( mResolutionChanged || mSupersampleChanged ) {
		// FBO is rendered at supersample resolution (2x if enabled)
		ivec2 fboSize = mRenderResolution * ( mSupersample ? 2 : 1 );
		mRenderFbo = gl::Fbo::create( fboSize.x, fboSize.y, createFboFormat() );
		mResolutionChanged = false;
		mSupersampleChanged = false;
	}

	// Auto-recompile if any channel texture type changed
	bool channelTypeChanged = false;
	for( int i = 0; i < 4; ++i ) {
		TextureType currentType = mChannelManager.getChannel( i ).textureType;
		if( currentType != mLastChannelTypes[i] ) {
			channelTypeChanged = true;
			mLastChannelTypes[i] = currentType;
		}
	}
	if( channelTypeChanged && !mCurrentFragmentSource.empty() ) {
		CI_LOG_I( "Channel texture type changed - auto-recompiling shader" );
		compileShader( mCurrentFragmentSource );
	}
}

void ShadertoyViewerApp::updateTimekeeping()
{
	// Calculate timing delta using Cinder's built-in timer (double precision)
	double currentTime = getElapsedSeconds();
	double timeDelta = currentTime - mLastUpdateTime;
	mLastUpdateTime = currentTime;

	// Update time and frame counter
	if( ! mPaused ) {
		mTime += timeDelta * mTimeScale;
		mFrame++;

		// Advance iChannelTime ONLY for time-dependent sources (videos, buffers)
		// Static images/cubemaps stay at 0.0 for Shadertoy parity
		for( int i = 0; i < 4; ++i ) {
			if( mChannelManager.getChannel( i ).timeDependent ) {
				mChannelManager.getChannel( i ).time += (float)( timeDelta * mTimeScale );
			}
		}
	}

	// Update iDate once per frame (used by both shader uniform and GUI)
	auto	t = std::time( nullptr );
	std::tm tm;
#if defined( CINDER_MSW )
	localtime_s( &tm, &t );
#else
	tm = *std::localtime( &t );
#endif
	float seconds = tm.tm_hour * 3600.0f + tm.tm_min * 60.0f + tm.tm_sec;
	mDate = vec4( (float)( tm.tm_year + 1900 ), (float)( tm.tm_mon + 1 ), (float)tm.tm_mday, seconds );
}

gl::Fbo::Format ShadertoyViewerApp::createFboFormat()
{
	gl::Fbo::Format format;
	format.setSamples( 0 );
	format.disableDepth(); // No depth testing needed for fullscreen quad

	gl::Texture2d::Format texFormat;
	texFormat.setInternalFormat( GL_RGBA8 );
	texFormat.setMinFilter( GL_LINEAR );
	texFormat.setMagFilter( GL_LINEAR );
	texFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

	format.setColorTextureFormat( texFormat );
	return format;
}

void ShadertoyViewerApp::draw()
{
	// Render shader to FBO
	renderShaderToFbo();

	// Clear main window
	gl::clear( Color( 0.1f, 0.1f, 0.1f ) );

	// Draw FBO to screen (upper-left corner, aspect-ratio preserving)
	if( mRenderFbo ) {
		gl::ScopedBlendAlpha blend;
		gl::ScopedColor		 color( 1.0f, 1.0f, 1.0f );

		// Draw at display resolution (downsamples if supersampled)
		Rectf destRect( 0, 0, (float)mRenderResolution.x, (float)mRenderResolution.y );
		gl::draw( mRenderFbo->getColorTexture(), destRect );
	}

	if( mDrawGui )
		drawGui();
}

void ShadertoyViewerApp::renderShaderToFbo()
{
	if( ! mShaderProg || ! mRenderFbo )
		return;

	// Bind FBO and render
	gl::ScopedFramebuffer fboBind( mRenderFbo );
	gl::ScopedViewport	  viewport( ivec2( 0 ), mRenderFbo->getSize() );
	gl::ScopedMatrices	  matrices;
	gl::setMatricesWindow( mRenderFbo->getSize() );

	// No clear needed - Shadertoy shaders write every pixel

	// Update uniforms
	updateUniforms();

	// Draw fullscreen quad
	gl::ScopedGlslProg shader( mShaderProg );
	gl::drawSolidRect( Rectf( 0, 0, (float)mRenderFbo->getWidth(), (float)mRenderFbo->getHeight() ) );

	// Unbind channel textures to avoid conflicts when drawing FBO to screen
	// Only unbind the specific texture type that was bound for each channel
	for( int i = 0; i < 4; ++i ) {
		const auto& channel = mChannelManager.getChannel( i );
		switch( channel.textureType ) {
			case TextureType::Texture2D:
				if( channel.texture )
					gl::context()->bindTexture( GL_TEXTURE_2D, 0, i );
				break;
			case TextureType::TextureCube:
				if( channel.textureCube )
					gl::context()->bindTexture( GL_TEXTURE_CUBE_MAP, 0, i );
				break;
			case TextureType::Texture3D:
				if( channel.texture3d )
					gl::context()->bindTexture( GL_TEXTURE_3D, 0, i );
				break;
		}
	}
}

void ShadertoyViewerApp::updateUniforms()
{
	if( ! mShaderProg )
		return;

	// Bind channel textures first (before setting uniforms)
	for( int i = 0; i < 4; ++i ) {
		auto& channel = mChannelManager.getChannel( i );

		// Handle different texture types
		switch( channel.textureType ) {
			case TextureType::Texture2D:
				if( channel.texture ) {
					auto& tex = channel.texture;

					// Only update filter/wrap modes if they changed (avoid redundant GL calls)
					if( channel.filterLinear != channel.lastFilterLinear ) {
						tex->setMinFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						tex->setMagFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						channel.lastFilterLinear = channel.filterLinear;
					}

					if( channel.wrapRepeat != channel.lastWrapRepeat ) {
						tex->setWrap( channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE,
									  channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE );
						channel.lastWrapRepeat = channel.wrapRepeat;
					}

					// Bind texture to unit i
					tex->bind( (uint8_t)i );
				}
				break;

			case TextureType::TextureCube:
				if( channel.textureCube ) {
					auto& tex = channel.textureCube;

					// Only update filter/wrap modes if they changed
					if( channel.filterLinear != channel.lastFilterLinear ) {
						tex->setMinFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						tex->setMagFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						channel.lastFilterLinear = channel.filterLinear;
					}

					if( channel.wrapRepeat != channel.lastWrapRepeat ) {
						tex->setWrap( channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE,
									  channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE );
						channel.lastWrapRepeat = channel.wrapRepeat;
					}

					// Bind cubemap to unit i
					tex->bind( (uint8_t)i );
				}
				break;

			case TextureType::Texture3D:
				if( channel.texture3d ) {
					auto& tex = channel.texture3d;

					// Only update filter/wrap modes if they changed
					if( channel.filterLinear != channel.lastFilterLinear ) {
						tex->setMinFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						tex->setMagFilter( channel.filterLinear ? GL_LINEAR : GL_NEAREST );
						channel.lastFilterLinear = channel.filterLinear;
					}

					if( channel.wrapRepeat != channel.lastWrapRepeat ) {
						tex->setWrap( channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE,
									  channel.wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE );
						channel.lastWrapRepeat = channel.wrapRepeat;
					}

					// Bind 3D texture to unit i
					tex->bind( (uint8_t)i );
				}
				break;
		}
	}

	// Now set all uniforms (shader is bound in renderShaderToFbo via ScopedGlslProg)

	// iResolution: viewport resolution in pixels (actual FBO size, accounts for supersampling)
	ivec2 actualResolution = mRenderResolution * ( mSupersample ? 2 : 1 );
	mShaderProg->uniform( "iResolution", vec3( (float)actualResolution.x, (float)actualResolution.y, 1.0f ) );

	// iTime: shader playback time in seconds
	mShaderProg->uniform( "iTime", (float)mTime );

	// iTimeDelta: time since last frame
	float timeDelta = (float)( getElapsedSeconds() - mLastUpdateTime );
	mShaderProg->uniform( "iTimeDelta", timeDelta );

	// iFrame: frame counter
	mShaderProg->uniform( "iFrame", mFrame );

	// iFrameRate: frames per second
	mShaderProg->uniform( "iFrameRate", getAverageFps() );

	// iMouse: xy = current position, zw = click position (scaled for supersampling)
	float supersampleScale = mSupersample ? 2.0f : 1.0f;
	vec4 scaledMouseState = mMouseState * supersampleScale;
	mShaderProg->uniform( "iMouse", scaledMouseState );

	// iDate: year, month, day, time in seconds (computed once per frame in update())
	mShaderProg->uniform( "iDate", mDate );

	// iChannel texture unit uniforms
	for( int i = 0; i < 4; ++i ) {
		std::string channelName = "iChannel" + std::to_string( i );
		mShaderProg->uniform( channelName, i );
	}

	// iChannelResolution: resolution of each channel
	vec3 resolutions[4];
	for( int i = 0; i < 4; ++i ) {
		resolutions[i] = mChannelManager.getChannel( i ).resolution;
	}
	mShaderProg->uniform( "iChannelResolution", resolutions, 4 );

	// iChannelTime: playback time for each channel
	float times[4];
	for( int i = 0; i < 4; ++i ) {
		times[i] = mChannelManager.getChannel( i ).time;
	}
	mShaderProg->uniform( "iChannelTime", times, 4 );
}

std::string ShadertoyViewerApp::wrapShaderSource( const std::string& userFragmentCode )
{
	// Generate a wrapper that provides Shadertoy-compatible environment
	std::stringstream ss;

	ss << "#version 330 core\n";
	ss << "\n";
	ss << "// Shadertoy uniforms\n";
	ss << "uniform vec3      iResolution;\n";
	ss << "uniform float     iTime;\n";
	ss << "uniform float     iTimeDelta;\n";
	ss << "uniform int       iFrame;\n";
	ss << "uniform float     iFrameRate;\n";
	ss << "uniform vec4      iMouse;\n";
	ss << "uniform vec4      iDate;\n";

	// Declare correct sampler types based on loaded textures
	for( int i = 0; i < 4; ++i ) {
		const auto& channel = mChannelManager.getChannel( i );
		std::string channelName = "iChannel" + std::to_string( i );

		switch( channel.textureType ) {
			case TextureType::Texture2D:
				ss << "uniform sampler2D " << channelName << ";\n";
				break;
			case TextureType::TextureCube:
				ss << "uniform samplerCube " << channelName << ";\n";
				break;
			case TextureType::Texture3D:
				ss << "uniform sampler3D " << channelName << ";\n";
				break;
		}
	}

	ss << "uniform vec3      iChannelResolution[4];\n";
	ss << "uniform float     iChannelTime[4];\n";
	ss << "\n";
	ss << "out vec4 fragColor;\n";
	ss << "\n";
	ss << "#line 1\n"; // Reset line numbering for user code
	ss << userFragmentCode;
	ss << "\n";
	ss << "// Main entry point\n";
	ss << "void main()\n";
	ss << "{\n";
	ss << "	vec2 fragCoord = gl_FragCoord.xy;\n";
	ss << "	mainImage( fragColor, fragCoord );\n";
	ss << "}\n";

	return ss.str();
}


void ShadertoyViewerApp::compileShader( const std::string& fragmentSource )
{
	mCurrentFragmentSource = fragmentSource;
	mCompileLog.clear();
	mShaderCompileSuccess = false;
	mErrorLine = -1;

	try {
		// Wrap the user's fragment shader
		std::string wrappedSource = wrapShaderSource( fragmentSource );

		// Simple passthrough vertex shader
		std::string vertShader = R"(
			#version 330 core
			uniform mat4 ciModelViewProjection;
			in vec4 ciPosition;
			void main() {
				gl_Position = ciModelViewProjection * ciPosition;
			}
		)";

		// Compile
		auto format = gl::GlslProg::Format().vertex( vertShader ).fragment( wrappedSource );

		mShaderProg = gl::GlslProg::create( format );

		mShaderCompileSuccess = true;
		mCompileLog = "Shader compiled successfully";
		CI_LOG_I( "Shader compiled successfully" );
	}
	catch( const gl::GlslProgCompileExc& exc ) {
		mCompileLog = exc.what();
		CI_LOG_E( "Shader compilation failed: " << exc.what() );

		// Try to parse error line number
		// Format is usually: "ERROR: 0:<line>: <message>"
		std::string log = exc.what();
		size_t		pos = log.find( ":0:" );
		if( pos != std::string::npos ) {
			size_t lineStart = pos + 3;
			size_t lineEnd = log.find( ":", lineStart );
			if( lineEnd != std::string::npos ) {
				try {
					// Line numbers now match user code thanks to #line directive
					mErrorLine = std::stoi( log.substr( lineStart, lineEnd - lineStart ) );
				}
				catch( ... ) {
					mErrorLine = -1;
				}
			}
		}

		// Keep the previous shader if compilation fails (no fallback needed)
	}
	catch( const std::exception& exc ) {
		mCompileLog = exc.what();
		CI_LOG_E( "Shader error: " << exc.what() );
	}
}

void ShadertoyViewerApp::fileDrop( FileDropEvent event )
{
	fs::path path = event.getFile( 0 );

	// Check if it's a shader file
	std::string ext = path.extension().string();
	if( ext == ".frag" || ext == ".glsl" || ext == ".fs" ) {
		loadShaderFromFile( path );
	}
	// Otherwise try to load as texture for channel 0
	else {
		mChannelManager.loadChannelTexture( 0, path );
	}
}

void ShadertoyViewerApp::loadShaderFromFile( const fs::path& path )
{
	try {
		std::string shaderText = loadString( loadFile( path ) );
		mCurrentShaderPath = path;
		mShaderEditorText = shaderText;
		mShaderTextBuffer = shaderText; // No size limit - dynamic string
		compileShader( shaderText );
		mShaderModified = false;
	}
	catch( const std::exception& exc ) {
		CI_LOG_E( "Failed to load shader file: " << exc.what() );
	}
}

void ShadertoyViewerApp::openShaderDialog()
{
	fs::path path = getOpenFilePath( "", { "frag", "glsl", "fs" } );
	if( ! path.empty() ) {
		loadShaderFromFile( path );
	}
}

void ShadertoyViewerApp::mouseMove( MouseEvent event )
{
	// Account for HiDPI displays by using content scale
	float contentScale = getWindowContentScale();
	float x = event.getX() * contentScale;
	float y = event.getY() * contentScale;

	// Clamp to render bounds
	x = glm::clamp( x, 0.0f, (float)mRenderResolution.x );
	y = glm::clamp( y, 0.0f, (float)mRenderResolution.y );

	mMouseState.x = x;
	mMouseState.y = (float)mRenderResolution.y - y; // Flip Y to match Shadertoy
}

void ShadertoyViewerApp::mouseDown( MouseEvent event )
{
	mMouseDown = true;

	// Account for HiDPI displays
	float contentScale = getWindowContentScale();
	float x = glm::clamp( event.getX() * contentScale, 0.0f, (float)mRenderResolution.x );
	float y = glm::clamp( event.getY() * contentScale, 0.0f, (float)mRenderResolution.y );

	mMouseState.x = x;
	mMouseState.y = (float)mRenderResolution.y - y;
	mMouseState.z = mMouseState.x;
	mMouseState.w = mMouseState.y;
}

void ShadertoyViewerApp::mouseDrag( MouseEvent event )
{
	if( mMouseDown ) {
		// Account for HiDPI displays
		float contentScale = getWindowContentScale();
		float x = glm::clamp( event.getX() * contentScale, 0.0f, (float)mRenderResolution.x );
		float y = glm::clamp( event.getY() * contentScale, 0.0f, (float)mRenderResolution.y );

		mMouseState.x = x;
		mMouseState.y = (float)mRenderResolution.y - y;
	}
}

void ShadertoyViewerApp::mouseUp( MouseEvent event )
{
	mMouseDown = false;
}

void ShadertoyViewerApp::keyDown( KeyEvent event )
{
	// Space = pause/play
	if( event.getCode() == KeyEvent::KEY_SPACE ) {
		mPaused = ! mPaused;
	}
	// R = reset time
	else if( event.getCode() == KeyEvent::KEY_r ) {
		mTime = 0.0;
		mFrame = 0;
	}
	else if( event.getCode() == KeyEvent::KEY_g ) {
		mDrawGui = ! mDrawGui;
	}
}

void ShadertoyViewerApp::drawGui()
{
	// Individual panes
	if( mShowShaderPane )
		drawShaderPane();
	if( mShowPlaybackPane )
		drawPlaybackPane();
	if( mShowChannelsPane )
		drawChannelsPane();
}

void ShadertoyViewerApp::drawShaderPane()
{
	// Position on right side of window
	ImGui::SetNextWindowPos( ImVec2( (float)getWindowWidth() - 600, 20 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 580, 700 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "Shader", &mShowShaderPane );

	// File operations
	if( ImGui::Button( "Load Shader" ) )
		openShaderDialog();

	// Example shaders dropdown
	ImGui::SameLine();
	if( ImGui::BeginCombo( "Examples", "Load Example..." ) ) {
		auto exampleNames = getExampleShaderNames();
		for( size_t i = 0; i < exampleNames.size(); ++i ) {
			if( ImGui::Selectable( exampleNames[i].c_str() ) ) {
				loadExampleShader( (int)i );
			}
		}
		ImGui::EndCombo();
	}

	if( ! mCurrentShaderPath.empty() ) {
		ImGui::Text( "File: %s", mCurrentShaderPath.filename().string().c_str() );
	}

	ImGui::Separator();

	// Shader editor
	ImGui::Text( "Shader Code:" );

	// Calculate available height for editor (expand to fill window)
	float editorHeight = ImGui::GetContentRegionAvail().y - 80.0f; // Leave space for buttons and log

	// Use callback for dynamic resizing - no size limit
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;

	auto callback = []( ImGuiInputTextCallbackData* data ) -> int {
		if( data->EventFlag == ImGuiInputTextFlags_CallbackResize ) {
			std::string* str = (std::string*)data->UserData;
			str->resize( data->BufTextLen );
			data->Buf = (char*)str->c_str();
		}
		return 0;
	};

	// Ensure buffer has space
	if( mShaderTextBuffer.capacity() < mShaderTextBuffer.size() + 1 ) {
		mShaderTextBuffer.reserve( mShaderTextBuffer.size() + 1024 );
	}

	if( ImGui::InputTextMultiline( "##shadersource", (char*)mShaderTextBuffer.c_str(), mShaderTextBuffer.capacity(),
								   ImVec2( -1.0f, editorHeight ), flags, callback, &mShaderTextBuffer ) ) {
		mShaderTextBuffer.resize( strlen( mShaderTextBuffer.c_str() ) );
		mShaderEditorText = mShaderTextBuffer;
		mShaderModified = true;

		// Auto-compile if enabled
		if( mAutoCompile ) {
			compileShader( mShaderEditorText );
		}
	}

	// Compile button and autocompile checkbox
	if( ImGui::Button( "Compile Shader" ) ) {
		compileShader( mShaderEditorText );
	}

	ImGui::SameLine();
	ImGui::Checkbox( "Auto-compile", &mAutoCompile );

	if( mShaderModified && ! mAutoCompile ) {
		ImGui::SameLine();
		ImGui::TextColored( ImVec4( 1, 1, 0, 1 ), "Modified" );
	}

	// Compile log
	ImGui::Separator();
	ImGui::Text( "Compile Log:" );
	ImVec4 logColor = mShaderCompileSuccess ? ImVec4( 0, 1, 0, 1 ) : ImVec4( 1, 0, 0, 1 );
	ImGui::TextColored( logColor, "%s", mCompileLog.c_str() );

	ImGui::End();
}

void ShadertoyViewerApp::drawPlaybackPane()
{
	// Position on right side below Shader pane
	ImGui::SetNextWindowPos( ImVec2( (float)getWindowWidth() - 600, 740 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 580, 300 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "Playback & Uniforms", &mShowPlaybackPane );

	// Resolution controls
	ImGui::Text( "Resolution:" );
	int res[2] = { mRenderResolution.x, mRenderResolution.y };
	if( ImGui::InputInt2( "##resolution", res ) ) {
		if( res[0] > 0 && res[1] > 0 ) {
			mRenderResolution = ivec2( res[0], res[1] );
			mResolutionChanged = true;
		}
	}

	// Supersampling controls
	bool prevSupersample = mSupersample;
	ImGui::Checkbox( "Supersampling (2x)", &mSupersample );
	if( mSupersample != prevSupersample ) {
		mSupersampleChanged = true;
	}

	ImGui::Separator();

	// Playback controls
	if( mPaused ) {
		if( ImGui::Button( "Play" ) )
			mPaused = false;
	}
	else {
		if( ImGui::Button( "Pause" ) )
			mPaused = true;
	}

	ImGui::SameLine();
	if( ImGui::Button( "Reset" ) ) {
		mTime = 0.0;
		mFrame = 0;
	}

	ImGui::SliderFloat( "Time Scale", &mTimeScale, 0.0f, 2.0f );

	ImGui::Separator();

	// Stats
	ImGui::Text( "FPS: %.1f", getAverageFps() );
	ImGui::Text( "iTime: %.3f", mTime );
	ImGui::Text( "iFrame: %d", mFrame );
	ImGui::Text( "iResolution: %d x %d", mRenderResolution.x, mRenderResolution.y );

	ImGui::Separator();

	// Mouse uniforms
	ImGui::Text( "iMouse:" );
	ImGui::Text( "  xy (current): %.1f, %.1f", mMouseState.x, mMouseState.y );
	ImGui::Text( "  zw (click):   %.1f, %.1f", mMouseState.z, mMouseState.w );

	ImGui::Separator();

	// Date uniforms (computed once per frame in update())
	ImGui::Text( "iDate:" );
	ImGui::Text( "  Year:  %d", (int)mDate.x );
	ImGui::Text( "  Month: %d", (int)mDate.y );
	ImGui::Text( "  Day:   %d", (int)mDate.z );
	ImGui::Text( "  Time:  %.1f seconds", mDate.w );

	ImGui::End();
}

void ShadertoyViewerApp::drawChannelsPane()
{
	mChannelManager.drawChannelsPane( &mShowChannelsPane, this );
}

std::vector<std::string> ShadertoyViewerApp::getExampleShaderPaths()
{
	std::vector<std::string> paths;

	const fs::path dir = getAssetPath( "examples/" );
	if( dir.empty() || ! fs::exists( dir ) || ! fs::is_directory( dir ) ) {
		CI_LOG_E( "Examples directory not found: " << dir );
		return paths;
	}

	for( const auto& entry : fs::directory_iterator( dir ) ) {
		if( entry.is_regular_file() ) {
			std::string filename = entry.path().filename().string();
			// Skip hidden files, .DS_Store, and documentation files
			if( filename[0] == '.' || filename == "README.md" || entry.path().extension() == ".md" || entry.path().extension() == ".txt" ) {
				continue;
			}
			paths.push_back( entry.path().string() );
		}
	}

	std::sort( paths.begin(), paths.end() );
	return paths;
}

std::vector<std::string> ShadertoyViewerApp::getExampleShaderNames()
{
	auto			   paths = getExampleShaderPaths();
	std::vector<std::string> names;
	for( const auto& path : paths ) {
		fs::path p( path );
		std::string filename = p.stem().string(); // Get filename without extension
		names.push_back( filename );
	}
	return names;
}

void ShadertoyViewerApp::loadExampleShader( int exampleIndex )
{
	loadShaderFromFile( getExampleShaderPaths()[exampleIndex] );
}

// Entry point
CINDER_APP( ShadertoyViewerApp, RendererGl( RendererGl::Options().msaa( 0 ).version( 3, 3 ) ) )
