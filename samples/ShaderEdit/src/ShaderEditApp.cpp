#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderImGui.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ShaderEditApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown( KeyEvent event ) override;

  private:
	void createPhongShader();

	gl::GlslProgRef		mShader;
	gl::BatchRef		mGeo;
	CameraPersp			mCamera;
	CameraUi			mCamUi;

	// Uniform values - bound to ShaderEditor
	Colorf				mAmbientColor = Colorf( 0.2f, 0.2f, 0.2f );
	Colorf				mDiffuseColor = Colorf( 0.8f, 0.3f, 0.2f );
	Colorf				mSpecularColor = Colorf( 1.0f, 1.0f, 1.0f );
	vec3				mPointLightPosition = vec3( 5.0f, 5.0f, 5.0f );
	vec3				mDirectionalLightDir = vec3( -0.2f, -1.0f, -0.3f );
	Colorf				mDirectionalLightColor = Colorf( 0.3f, 0.3f, 0.4f );
	float				mShininess = 32.0f;

	float				mRotation = 0.0f;
};

void ShaderEditApp::setup()
{
	CI_LOG_I( "ShaderEdit Sample - Live Shader Editing with Bindings" );

	// Setup camera
	mCamera.lookAt( vec3( 3, 3, 3 ), vec3( 0 ) );
	mCamera.setPerspective( 45.0f, getWindowAspectRatio(), 0.1f, 100.0f );
	mCamUi = CameraUi( &mCamera, getWindow() );

	// Initialize ImGui
	ImGui::Initialize();

	// Create Phong shader
	createPhongShader();

	// Create a teapot to render
	auto teapot = geom::Teapot().subdivisions( 32 );
	mGeo = gl::Batch::create( teapot, mShader );

	gl::enableDepthRead();
	gl::enableDepthWrite();
}

void ShaderEditApp::createPhongShader()
{
	try {
		mShader = gl::GlslProg::create( loadAsset( "phong.vert" ), loadAsset( "phong.frag" ) );
		mShader->setLabel( "Phong Shader" );
		CI_LOG_I( "Phong shader created successfully" );
	}
	catch( const gl::GlslProgCompileExc &exc ) {
		CI_LOG_E( "Shader compile error: " << exc.what() );
	}
}

void ShaderEditApp::update()
{
	// Rotate the teapot
	mRotation += 0.01f;
}

void ShaderEditApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.1f ) );
	gl::setMatrices( mCamera );

	if( mShader && mGeo ) {
		// Draw rotating teapot
		// Uniforms are automatically set by ShaderEditor from our bound variables
		gl::ScopedGlslProg shaderScope( mShader );
		gl::ScopedModelMatrix modelScope;
		gl::rotate( mRotation, vec3( 0, 1, 0 ) );
		mGeo->draw();
	}

	// Custom window containing FPS and ShaderEditor
	ImGui::Begin( "Shader Editor Panel" );

	// FPS at the top
	ImGui::Text( "FPS: %.1f", getAverageFps() );
	ImGui::Separator();

	// ShaderEditor embedded in our window!
	ImGui::ShaderEditor( mShader, {
		{ "uAmbientColor", &mAmbientColor },                       // Colorf* → auto COLOR
		{ "uDiffuseColor", &mDiffuseColor },                       // Colorf* → auto COLOR
		{ "uSpecularColor", &mSpecularColor },                     // Colorf* → auto COLOR
		{ "uPointLightPosition", &mPointLightPosition },           // vec3* → auto VECTOR
		{ "uDirectionalLightDir", &mDirectionalLightDir, ImGui::Vec3Semantic::DIRECTION }, // vec3* with DIRECTION override
		{ "uDirectionalLightColor", &mDirectionalLightColor },     // Colorf* → auto COLOR
		{ "uShininess", &mShininess, 1.0f, 128.0f }                // float* with range
	});

	ImGui::End();
}

void ShaderEditApp::keyDown( KeyEvent event )
{
	// No need for key handling - window can be closed normally
}

CINDER_APP( ShaderEditApp, RendererGl, []( App::Settings *settings ) {
	settings->setWindowSize( 1400, 900 );
	settings->setTitle( "ShaderEdit - Live Shader Editing with Bindings" );
	settings->setHighDensityDisplayEnabled( false );
} )
