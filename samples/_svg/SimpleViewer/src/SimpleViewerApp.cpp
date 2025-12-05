/*
 SVG SimpleViewer - View SVG files with multiple renderers

 Supports three rendering backends:
 - Cairo: Software rendering via Cairo (accurate)
 - OpenGL: Hardware-accelerated via gl::draw(svg::Doc)
 - vg::Canvas (Rive): Hardware-accelerated via Rive PLS renderer

 Drag & drop SVG files to view them.
 Use mouse wheel to zoom, drag to pan.
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/svg/Svg.h"
#include "cinder/svg/SvgGl.h"
#include "cinder/cairo/Cairo.h"
#include "cinder/CinderImGui.h"
#include "cinder/vg/CanvasGl.h"
#include "cinder/CanvasUi.h"

using namespace ci;
using namespace ci::app;
using namespace std;

enum class RendererType {
	Cairo,
	OpenGL,
	VgCanvas
};

class SimpleViewerApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void fileDrop( FileDropEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	void load( fs::path path );

	RendererType			mRenderer = RendererType::Cairo;
	svg::DocRef				mDoc;
	gl::TextureRef			mCairoTex;    // Cached Cairo render
	vg::CanvasGlRef			mCanvas;       // vg::Canvas for Rive rendering
	CanvasUi				mCanvasUi;     // Pan/zoom UI
	fs::path				mCurrentFile;

	// Info
	float					mLastRenderTimeMs = 0.0f;
};

void SimpleViewerApp::setup()
{
	// Initialize ImGui
	ImGui::Initialize( ImGui::Options().window( getWindow() ).enableKeyboard( true ) );
	ImGui::GetStyle().ScaleAllSizes( getWindowContentScale() );
	ImGui::GetStyle().FontScaleMain = getWindowContentScale();

	// Create vg::Canvas for Rive rendering
	vg::CanvasOptions options;
	mCanvas = vg::createCanvasGl( options );

	// Connect CanvasUi to window for pan/zoom
	mCanvasUi.connect( getWindow() );
}

void SimpleViewerApp::update()
{
}

gl::TextureRef renderCairoToTexture( svg::DocRef doc )
{
	int width = doc->getWidth() ? (int)doc->getWidth() : 640;
	int height = doc->getHeight() ? (int)doc->getHeight() : 480;

	cairo::SurfaceImage srf( width, height, true );
	cairo::Context ctx( srf );
	ctx.render( *doc );
	ctx.flush();

	return gl::Texture::create( srf.getSurface() );
}

void SimpleViewerApp::load( fs::path path )
{
	try {
		auto startTime = std::chrono::high_resolution_clock::now();

		if( path.extension() != ".svgz" )
			mDoc = svg::Doc::create( path );
		else // compressed
			mDoc = svg::Doc::createFromSvgz( loadFile( path ) );

		mCurrentFile = path;

		// Pre-render Cairo texture
		mCairoTex = renderCairoToTexture( mDoc );

		// Set content bounds and fit to window
		int docWidth = mDoc->getWidth() ? (int)mDoc->getWidth() : 640;
		int docHeight = mDoc->getHeight() ? (int)mDoc->getHeight() : 480;
		mCanvasUi.setContentBounds( Rectf( 0, 0, (float)docWidth, (float)docHeight ) );
		mCanvasUi.fitAll();

		auto endTime = std::chrono::high_resolution_clock::now();
		mLastRenderTimeMs = std::chrono::duration<float, std::milli>( endTime - startTime ).count();
	}
	catch( ci::Exception &exc ) {
		console() << "exception caught parsing svg doc, what: " << exc.what() << endl;
	}
}

void SimpleViewerApp::fileDrop( FileDropEvent event )
{
	load( event.getFile( 0 ) );
}

void SimpleViewerApp::mouseDown( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse ) {
		mCanvasUi.disable();
	} else {
		mCanvasUi.enable();
	}
}

void SimpleViewerApp::mouseDrag( MouseEvent event )
{
	// CanvasUi handles drag via signals when connected
}

void SimpleViewerApp::mouseUp( MouseEvent event )
{
	mCanvasUi.enable();
}

void SimpleViewerApp::mouseWheel( MouseEvent event )
{
	if( ImGui::GetIO().WantCaptureMouse ) {
		mCanvasUi.disable();
	} else {
		mCanvasUi.enable();
	}
}

void SimpleViewerApp::draw()
{
	gl::clear( Color( 0.2f, 0.2f, 0.2f ) );
	gl::enableAlphaBlending();

	if( mDoc ) {
		gl::color( Color::white() );

		auto startTime = std::chrono::high_resolution_clock::now();

		// Get transform from CanvasUi
		mat3 transform = mCanvasUi.getTransform2d();

		switch( mRenderer ) {
			case RendererType::Cairo:
				if( mCairoTex ) {
					// Apply transform via gl matrix stack
					gl::ScopedModelMatrix scopedMat;
					gl::multModelMatrix( mCanvasUi.getModelMatrix() );
					gl::draw( mCairoTex );
				}
				break;

			case RendererType::OpenGL:
				{
					// Apply transform via gl matrix stack
					gl::ScopedModelMatrix scopedMat;
					gl::multModelMatrix( mCanvasUi.getModelMatrix() );
					gl::draw( *mDoc );
				}
				break;

			case RendererType::VgCanvas:
				if( mCanvas ) {
					mCanvas->begin( toPixels( getWindowSize() ) );
					mCanvas->setTransform( transform );
					mCanvas->draw( *mDoc );
					mCanvas->end();
				}
				break;
		}

		auto endTime = std::chrono::high_resolution_clock::now();
		float frameTimeMs = std::chrono::duration<float, std::milli>( endTime - startTime ).count();

		// Smooth the timing
		mLastRenderTimeMs = mLastRenderTimeMs * 0.9f + frameTimeMs * 0.1f;
	}
	else {
		gl::color( Color::white() );
		gl::drawStringCentered( "Drag & Drop an SVG file", getWindowCenter() );
	}

	// ImGui UI
	ImGui::Begin( "SVG Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize );

	ImGui::Text( "Renderer:" );

	bool cairoSelected = (mRenderer == RendererType::Cairo);
	bool openglSelected = (mRenderer == RendererType::OpenGL);
	bool vgSelected = (mRenderer == RendererType::VgCanvas);

	if( ImGui::RadioButton( "Cairo", cairoSelected ) ) {
		mRenderer = RendererType::Cairo;
	}
	ImGui::SameLine();
	if( ImGui::RadioButton( "OpenGL", openglSelected ) ) {
		mRenderer = RendererType::OpenGL;
	}
	ImGui::SameLine();
	if( ImGui::RadioButton( "vg::Canvas (Rive)", vgSelected ) ) {
		mRenderer = RendererType::VgCanvas;
	}

	ImGui::Separator();

	if( mDoc ) {
		ImGui::Text( "File: %s", mCurrentFile.filename().string().c_str() );

		int docWidth = mDoc->getWidth() ? (int)mDoc->getWidth() : 0;
		int docHeight = mDoc->getHeight() ? (int)mDoc->getHeight() : 0;
		if( docWidth > 0 && docHeight > 0 ) {
			ImGui::Text( "Size: %d x %d", docWidth, docHeight );
		} else {
			ImGui::Text( "Size: (unspecified)" );
		}

		ImGui::Text( "Render time: %.2f ms", mLastRenderTimeMs );

		ImGui::Separator();
		ImGui::Text( "Zoom: %.1f%%", mCanvasUi.getZoom() * 100.0f );

		if( ImGui::Button( "Reset View" ) ) {
			mCanvasUi.viewOnetoOne();
		}
		ImGui::SameLine();
		if( ImGui::Button( "Fit" ) ) {
			mCanvasUi.fitAll();
		}
	}
	else {
		ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.5f, 1.0f ), "Drop an SVG file to view" );
	}

	ImGui::Separator();
	ImGui::TextDisabled( "Scroll to zoom, drag to pan" );

	ImGui::End();
}


CINDER_APP( SimpleViewerApp, RendererGl( RendererGl::Options().msaa( 4 ) ),
	[]( App::Settings* settings ) {
		settings->setTitle( "SVG SimpleViewer" );
		settings->setWindowSize( 800, 600 );
	}
)
