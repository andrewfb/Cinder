#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CinderImGui.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/CameraUi.h"
#include "cinder/ImageIo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ShaderEditComputeApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;

  private:
	struct Boid {
		vec4 position;  // xyz = position, w = unused
		vec4 velocity;  // xyz = velocity, w = unused
		vec4 color;     // rgba = color
	};

	static const int NUM_BOIDS = 5000;

	gl::GlslProgRef		mComputeShader;
	gl::GlslProgRef		mRenderShader;
	gl::VboRef			mBoidVbo;
	gl::VaoRef			mBoidVao;

	CameraPersp			mCamera;
	CameraUi			mCamUi;

	bool				mTakeScreenshot = false;
	int					mFrameCount = 0;
};

void ShaderEditComputeApp::setup()
{
	CI_LOG_I( "ShaderEditCompute - Flocking Simulation" );

	// Check for screenshot command line argument
	for( const auto& arg : getCommandLineArgs() ) {
		if( arg == "--screenshot" ) {
			mTakeScreenshot = true;
			CI_LOG_I( "Screenshot mode enabled" );
		}
	}

	// Setup camera
	mCamera.lookAt( vec3( 0, 0, 8 ), vec3( 0 ) );
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 0.1f, 100.0f );
	mCamUi = CameraUi( &mCamera, getWindow() );

	// Initialize ImGui
	ImGui::Initialize();

	// Create boid data
	vector<Boid> boids( NUM_BOIDS );
	for( int i = 0; i < NUM_BOIDS; ++i ) {
		// Random position in sphere
		vec3 pos = randVec3() * Rand::randFloat( 0.5f, 2.5f );
		boids[i].position = vec4( pos, 1.0f );

		// Random initial velocity
		boids[i].velocity = vec4( randVec3() * 0.5f, 0.0f );

		// Initial color
		boids[i].color = vec4( 0.3f, 0.6f, 0.9f, 1.0f );
	}

	// Create VBO with boid data
	mBoidVbo = gl::Vbo::create( GL_ARRAY_BUFFER, boids, GL_DYNAMIC_DRAW );

	// Create VAO
	mBoidVao = gl::Vao::create();
	gl::ScopedVao vaoScope( mBoidVao );
	gl::ScopedBuffer vboScope( mBoidVbo );

	// Setup attributes
	gl::enableVertexAttribArray( 0 );  // position
	gl::vertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, sizeof(Boid), (void*)offsetof(Boid, position) );

	gl::enableVertexAttribArray( 1 );  // color
	gl::vertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof(Boid), (void*)offsetof(Boid, color) );

	// Create compute shader
	try {
		mComputeShader = gl::GlslProg::create( gl::GlslProg::Format()
			.compute( loadAsset( "flocking.comp" ) )
			.label( "Flocking Compute" ) );

		// Set default uniforms
		mComputeShader->uniform( "uSeparationRadius", 0.3f );
		mComputeShader->uniform( "uAlignmentRadius", 0.8f );
		mComputeShader->uniform( "uCohesionRadius", 1.0f );
		mComputeShader->uniform( "uSeparationStrength", 1.5f );
		mComputeShader->uniform( "uAlignmentStrength", 0.8f );
		mComputeShader->uniform( "uCohesionStrength", 0.5f );
		mComputeShader->uniform( "uMaxSpeed", 2.0f );
		mComputeShader->uniform( "uCenterAttraction", 0.5f );
		mComputeShader->uniform( "uNoiseStrength", 0.1f );
		mComputeShader->uniform( "uDeltaTime", 0.016f );

		CI_LOG_I( "Compute shader created" );
	}
	catch( const exception& exc ) {
		CI_LOG_E( "Compute shader error: " << exc.what() );
	}

	// Create render shader
	try {
		mRenderShader = gl::GlslProg::create( loadAsset( "boid.vert" ), loadAsset( "boid.frag" ) );
		mRenderShader->setLabel( "Boid Render" );

		// Set default uniforms
		mRenderShader->uniform( "uPointSize", 3.0f );
		mRenderShader->uniform( "uBrightness", 1.0f );
		mRenderShader->uniform( "uSaturation", 1.0f );

		CI_LOG_I( "Render shader created" );
	}
	catch( const exception& exc ) {
		CI_LOG_E( "Render shader error: " << exc.what() );
	}

	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enable( GL_PROGRAM_POINT_SIZE );
	gl::enable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void ShaderEditComputeApp::update()
{
	// Run compute shader
	if( mComputeShader ) {
		gl::ScopedGlslProg compute( mComputeShader );
		gl::bindBufferBase( GL_SHADER_STORAGE_BUFFER, 0, mBoidVbo );

		int numGroups = (NUM_BOIDS + 255) / 256;
		gl::dispatchCompute( numGroups, 1, 1 );
		gl::memoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
	}
}

void ShaderEditComputeApp::draw()
{
	gl::clear( Color( 0.01f, 0.01f, 0.02f ) );
	gl::setMatrices( mCamera );

	// Draw boids
	if( mRenderShader && mBoidVao ) {
		gl::ScopedGlslProg shader( mRenderShader );
		gl::ScopedVao vao( mBoidVao );

		gl::context()->setDefaultShaderVars();
		glDrawArrays( GL_POINTS, 0, NUM_BOIDS );
	}

	// ImGui shader editors
	ImGui::Begin( "Flocking Shader Editors" );

	ImGui::Text( "FPS: %.1f", getAverageFps() );
	ImGui::Text( "Boids: %d", NUM_BOIDS );
	ImGui::Separator();

	// Compute shader editor
	if( ImGui::CollapsingHeader( "Compute Shader (Flocking Behavior)", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::ShaderEditor( mComputeShader, {
			{ "uSeparationRadius", 0.1f, 1.0f },
			{ "uAlignmentRadius", 0.1f, 2.0f },
			{ "uCohesionRadius", 0.1f, 2.0f },
			{ "uSeparationStrength", 0.0f, 5.0f },
			{ "uAlignmentStrength", 0.0f, 2.0f },
			{ "uCohesionStrength", 0.0f, 2.0f },
			{ "uMaxSpeed", 0.5f, 5.0f },
			{ "uCenterAttraction", 0.0f, 2.0f },
			{ "uNoiseStrength", 0.0f, 1.0f }
		});
	}

	ImGui::Separator();

	// Render shader editor
	if( ImGui::CollapsingHeader( "Render Shaders (Visualization)", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::ShaderEditor( mRenderShader, {
			{ "uPointSize", 1.0f, 10.0f },
			{ "uBrightness", 0.0f, 3.0f },
			{ "uSaturation", 0.0f, 2.0f }
		});
	}

	ImGui::End();

	// Screenshot mode: save after 60 frames and exit
	if( mTakeScreenshot ) {
		mFrameCount++;
		if( mFrameCount == 60 ) {
			writeImage( "screenshot.png", copyWindowSurface() );
			CI_LOG_I( "Screenshot saved to screenshot.png" );
			quit();
		}
	}
}

CINDER_APP( ShaderEditComputeApp, RendererGl, []( App::Settings *settings ) {
	settings->setWindowSize( 1400, 900 );
	settings->setTitle( "ShaderEditCompute - Flocking" );
	settings->setHighDensityDisplayEnabled( false );
} )
