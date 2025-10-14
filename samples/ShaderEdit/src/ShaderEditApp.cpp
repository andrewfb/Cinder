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

  private:
	void createPhongShader();

	gl::GlslProgRef		mShader;
	gl::BatchRef		mGeo;
	CameraPersp			mCamera;
	CameraUi			mCamUi;

	float				mRotation = 0.0f;
};

void ShaderEditApp::setup()
{
	CI_LOG_I( "ShaderEdit Sample - Live Shader Editing with Uniform Backing Store" );

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

		// Set default uniform values
		mShader->uniform( "uAmbientColor", vec3( 0.05f, 0.05f, 0.15f ) );       // Dark blue ambient
		mShader->uniform( "uDiffuseColor", vec3( 0.8f, 0.3f, 0.2f ) );          // Orange-red diffuse
		mShader->uniform( "uSpecularColor", vec3( 1.0f, 1.0f, 1.0f ) );         // White specular
		mShader->uniform( "uPointLightPosition", vec3( 5.0f, 5.0f, 5.0f ) );    // Point light position
		mShader->uniform( "uDirectionalLightDir", vec3( -0.2f, -1.0f, -0.3f ) );// Directional light
		mShader->uniform( "uDirectionalLightColor", vec3( 0.3f, 0.3f, 0.4f ) ); // Bluish directional
		mShader->uniform( "uShininess", 32.0f );                                 // Moderate shininess

		CI_LOG_I( "Phong shader created successfully with default uniforms" );
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
		// Uniforms are stored in shader's backing store and automatically preserved across recompilation
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

	// ShaderEditor with metadata hints for specific uniforms
	// All uniforms are auto-discovered; we just provide hints for better widgets
	ImGui::ShaderEditor( mShader, {
		{ "uDirectionalLightDir", ImGui::UniformSemantic::DIRECTION },
		{ "uAmbientColor", ImGui::UniformSemantic::COLOR },
		{ "uDiffuseColor", ImGui::UniformSemantic::COLOR },
		{ "uSpecularColor", ImGui::UniformSemantic::COLOR },
		{ "uDirectionalLightColor", ImGui::UniformSemantic::COLOR },
		{ "uShininess", 1.0f, 128.0f }
	});

	// Alternatively, just call with empty hints to get default widgets for all uniforms
	//	ImGui::ShaderEditor( mShader, {} );

	ImGui::End();
}

CINDER_APP( ShaderEditApp, RendererGl, []( App::Settings *settings ) {
	settings->setWindowSize( 1400, 900 );
	settings->setTitle( "ShaderEdit - Live Shader Editing" );
	settings->setHighDensityDisplayEnabled( false );
} )
