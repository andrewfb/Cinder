#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Ssbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Particle type holds information for rendering and simulation.
// Used to buffer initial simulation values.
// Using std140 in ssbo requires we have members on 4 byte alignment. 
#pragma pack( push, 1 )
//__declspec( align( 4 ) )
struct Particle
{
	vec3	pos;
	float   pad1;
	vec3	ppos;
	float   pad2;
	vec3	home;
	float	damping;
};
#pragma pack( pop )

// Home many particles to create. (600k default)
const int NUM_PARTICLES = static_cast<int>( 600e3 );

/**
	Simple particle simulation with Verlet integration and mouse interaction.
	A sphere of particles is deformed by mouse interaction.
	
	This sample is the same as ParticleSphereGPU.  The only difference is
	that it uses a compute shader instead of transform feedback.
 */
class ParticleSphereCSApp : public App {
  public:
	void	setup() override;
	void	mouseDown( MouseEvent event ) override;
	void	mouseDrag( MouseEvent event ) override;
	void	mouseUp( MouseEvent event ) override;
	void	update() override;
	void	draw() override;

  private:
	enum { WORK_GROUP_SIZE = 128, };
	gl::GlslProgRef		mRenderProg;
	gl::GlslProgRef		mUpdateProg;

	// Buffers holding raw particle data on GPU.
	gl::SsboRef		mParticleBuffer;
	gl::VboRef		mColorsVbo;
	gl::VaoRef		mAttributes;

	// Mouse state suitable for passing as uniforms to update program
	bool			mMouseDown = false;
	float			mMouseForce = 0.0f;
	vec3			mMousePos = vec3( 0 );
};

void ParticleSphereCSApp::setup()
{
	// Create initial particle layout.
	vector<Particle> particles;
	vector<vec3> colors;
	particles.assign( NUM_PARTICLES, Particle() );
	const float azimuth = 256.0f * static_cast<float>( M_PI ) / particles.size();
	const float inclination = static_cast<float>( M_PI ) / particles.size();
	const float radius = 180.0f;
	vec3 center = vec3( getWindowCenter() + vec2( 0.0f, 40.0f ), 0.0f );
	for( unsigned int i = 0; i < particles.size(); ++i )
	{	// assign starting values to particles.
		float x = radius * math<float>::sin( inclination * i ) * math<float>::cos( azimuth * i );
		float y = radius * math<float>::cos( inclination * i );
		float z = radius * math<float>::sin( inclination * i ) * math<float>::sin( azimuth * i );

		auto &p = particles.at( i );
		p.pos = center + vec3( x, y, z );
		p.home = p.pos;
		p.ppos = p.home + Rand::randVec3() * 10.0f; // random initial velocity
		p.damping = Rand::randFloat( 0.965f, 0.985f );
		Color c( CM_HSV, lmap<float>( static_cast<float>(i), 0.0f, static_cast<float>( particles.size() ), 0.0f, 0.66f ), 1.0f, 1.0f );
		colors.push_back( (vec3)c );
	}

	ivec3 count = gl::getMaxComputeWorkGroupCount();
	CI_ASSERT( count.x >= ( NUM_PARTICLES / WORK_GROUP_SIZE ) );

	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer = gl::Ssbo::create( particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW );
	
	// Create a default color shader.
#if CINDER_GL_ES_VERSION >= CINDER_GL_ES_VERSION_3_1 
	mRenderProg = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "particleRender_es31.vert" ) )
			.fragment( loadAsset( "particleRender_es31.frag" ) )
			.attribLocation( "particleColor", 0 ) );
#else
	mRenderProg = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "particleRender.vert" ) )
			.fragment( loadAsset( "particleRender.frag" ) )
			.attribLocation( "particleColor", 0 ) );
#endif
	
	mColorsVbo = gl::Vbo::create<vec3>( GL_ARRAY_BUFFER, colors, GL_STATIC_DRAW );
    mAttributes = gl::Vao::create();
	gl::ScopedVao vao( mAttributes );
	gl::ScopedBuffer scopedIds( mColorsVbo );
	gl::enableVertexAttribArray( 0 );
	gl::vertexAttribPointer( 0, 3,  GL_FLOAT, GL_FALSE, 0, nullptr );
	
#if CINDER_GL_ES_VERSION >= CINDER_GL_ES_VERSION_3_1 
	mUpdateProg = gl::GlslProg::create( gl::GlslProg::Format().compute( loadAsset( "particleUpdate_es31.comp" ) ) );
#else
	mUpdateProg = gl::GlslProg::create( gl::GlslProg::Format().compute( loadAsset( "particleUpdate.comp" ) ) );
#endif
}

void ParticleSphereCSApp::mouseDown( MouseEvent event )
{
	mMouseDown = true;
	mMouseForce = 500.0f;
	mMousePos = vec3( event.getX(), event.getY(), 0.0f );
}

void ParticleSphereCSApp::mouseDrag( MouseEvent event )
{
	mMousePos = vec3( event.getX(), event.getY(), 0.0f );
}

void ParticleSphereCSApp::mouseUp( MouseEvent event )
{
	mMouseForce = 0.0f;
	mMouseDown = false;
}

void ParticleSphereCSApp::update()
{
	// Update particles on the GPU
	gl::ScopedGlslProg prog( mUpdateProg );
	mParticleBuffer->bindBase( 0 ); // corresponds to 'binding = 0' in particleUpdate.comp shader
	
	mUpdateProg->uniform( "uMouseForce", mMouseForce );
	mUpdateProg->uniform( "uMousePos", mMousePos );

	gl::dispatchCompute( NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1 );
	gl::memoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );

	// Update mouse force.
	if( mMouseDown )
		mMouseForce = 150.0f;
}

void ParticleSphereCSApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::setMatricesWindowPersp( getWindowSize() );
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::ScopedGlslProg render( mRenderProg );
	gl::ScopedBuffer scopedParticleSsbo( mParticleBuffer );
	gl::ScopedVao vao( mAttributes );
	
	gl::context()->setDefaultShaderVars();

	gl::drawArrays( GL_POINTS, 0, NUM_PARTICLES );
	
	gl::setMatricesWindow( app::getWindowSize() );
	gl::drawString( toString( static_cast<int>( getAverageFps() ) ) + " fps", vec2( 32.0f, 52.0f ) );
}

CINDER_APP( ParticleSphereCSApp, RendererGl, []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
	settings->setMultiTouchEnabled( false );
} )
