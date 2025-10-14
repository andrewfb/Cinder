#include "cinder/CinderImGui.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Log.h"
#include <map>
#include <string>

using namespace ci;
using namespace ci::gl;

namespace ImGui {

// Storage for shader source edits (persisted across frames)
struct ShaderEditorState {
	std::string vertexSource;
	std::string fragmentSource;
	std::string geometrySource;
	std::string compileError;
	bool hasError = false;
	bool initialized = false;
	bool autoCompile = false;
	bool sourceChanged = false;
};

static std::map<void*, ShaderEditorState> sShaderEditorStates;

// Forward declarations
static bool ShaderEditorImpl( const gl::GlslProgRef& shader, const std::vector<UniformBinding>* bindings, bool* pOpen );
static bool ShaderUniformsWithBindings( const gl::GlslProgRef& shader, const std::vector<UniformBinding>& bindings );

// Public API - simple version (no bindings)
bool ShaderEditor( const gl::GlslProgRef& shader, bool* pOpen )
{
	return ShaderEditorImpl( shader, nullptr, pOpen );
}

// Public API - with bindings
bool ShaderEditor( const gl::GlslProgRef& shader, const std::vector<UniformBinding>& bindings, bool* pOpen )
{
	return ShaderEditorImpl( shader, &bindings, pOpen );
}

// Internal implementation
static bool ShaderEditorImpl( const gl::GlslProgRef& shader, const std::vector<UniformBinding>* bindings, bool* pOpen )
{
	if( ! shader )
		return false;

	// Get or create editor state for this shader
	void* shaderId = shader.get();
	ShaderEditorState& state = sShaderEditorStates[shaderId];

	// Initialize state on first use
	if( ! state.initialized ) {
		state.vertexSource = shader->getVertexShaderSource();
		state.fragmentSource = shader->getFragmentShaderSource();
		state.geometrySource = shader->getGeometryShaderSource();
		state.initialized = true;
	}

	bool recompiled = false;

	// GLSL Code section
	if( ImGui::CollapsingHeader( "GLSL Code", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		// Auto-compile checkbox and compile button
		ImGui::Checkbox( "Auto-Compile", &state.autoCompile );
		ImGui::SameLine();

		// Compile shader (either manually or auto)
		bool shouldCompile = false;
		if( state.autoCompile ) {
			// In auto-compile mode, compile when source changes
			ImGui::BeginDisabled();
			ImGui::Button( "Compile Shader", ImVec2( -FLT_MIN, 0 ) );
			ImGui::EndDisabled();
			shouldCompile = state.sourceChanged;
			state.sourceChanged = false;
		}
		else {
			// Manual compile mode
			shouldCompile = ImGui::Button( "Compile Shader", ImVec2( -FLT_MIN, 0 ) );
		}

		if( shouldCompile ) {
			std::string error;
			if( const_cast<gl::GlslProg*>( shader.get() )->recompile( state.vertexSource, state.fragmentSource, state.geometrySource, &error ) ) {
				state.hasError = false;
				state.compileError.clear();
				recompiled = true;
				CI_LOG_I( "Shader recompiled successfully" );
			}
			else {
				state.hasError = true;
				// Strip trailing newline from error message
				state.compileError = error;
				while( ! state.compileError.empty() && ( state.compileError.back() == '\n' || state.compileError.back() == '\r' ) ) {
					state.compileError.pop_back();
				}
				CI_LOG_E( "Shader compile error: " << error );
			}
		}

		ImGui::Separator();

		// Vertex shader editor
		if( ImGui::CollapsingHeader( "Vertex Shader", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			static const size_t bufferSize = 1024 * 16;
			static char vertexBuffer[bufferSize];

			if( state.vertexSource.size() < bufferSize ) {
				strcpy_s( vertexBuffer, bufferSize, state.vertexSource.c_str() );
			}

			if( ImGui::InputTextMultiline( "##vertex", vertexBuffer, bufferSize,
				ImVec2( -FLT_MIN, 300 ), ImGuiInputTextFlags_AllowTabInput ) ) {
				state.vertexSource = vertexBuffer;
				state.sourceChanged = true;
			}
		}

		// Fragment shader editor
		if( ImGui::CollapsingHeader( "Fragment Shader", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			static const size_t bufferSize = 1024 * 16;
			static char fragmentBuffer[bufferSize];

			if( state.fragmentSource.size() < bufferSize ) {
				strcpy_s( fragmentBuffer, bufferSize, state.fragmentSource.c_str() );
			}

			if( ImGui::InputTextMultiline( "##fragment", fragmentBuffer, bufferSize,
				ImVec2( -FLT_MIN, 300 ), ImGuiInputTextFlags_AllowTabInput ) ) {
				state.fragmentSource = fragmentBuffer;
				state.sourceChanged = true;
			}
		}

#if defined( CINDER_GL_HAS_GEOM_SHADER )
		// Geometry shader editor (optional)
		if( ! state.geometrySource.empty() ) {
			if( ImGui::CollapsingHeader( "Geometry Shader" ) ) {
				static const size_t bufferSize = 1024 * 16;
				static char geometryBuffer[bufferSize];

				if( state.geometrySource.size() < bufferSize ) {
					strcpy_s( geometryBuffer, bufferSize, state.geometrySource.c_str() );
				}

				if( ImGui::InputTextMultiline( "##geometry", geometryBuffer, bufferSize,
					ImVec2( -FLT_MIN, 300 ), ImGuiInputTextFlags_AllowTabInput ) ) {
					state.geometrySource = geometryBuffer;
					state.sourceChanged = true;
				}
			}
		}
#endif

		// Status bar - always visible at bottom of GLSL Code section
		ImGui::Separator();
		if( state.hasError ) {
			// Error state - show error message
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 0.8f, 0.8f, 1.0f ) );
			ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.3f, 0.05f, 0.05f, 0.9f ) );

			// Calculate height based on error text
			int lineCount = 1;
			for( char c : state.compileError ) {
				if( c == '\n' ) lineCount++;
			}
			float height = ImGui::GetTextLineHeight() * std::min( lineCount + 1, 8 ) + 10.0f;

			ImGui::InputTextMultiline( "##status", const_cast<char*>( state.compileError.c_str() ),
				state.compileError.size() + 1, ImVec2( -FLT_MIN, height ), ImGuiInputTextFlags_ReadOnly );

			ImGui::PopStyleColor( 2 );
		}
		else {
			// Success state - show green status
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.7f, 1.0f, 0.7f, 1.0f ) );
			ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.05f, 0.2f, 0.05f, 0.7f ) );

			std::string successMsg = "Shader compiled successfully";
			float height = ImGui::GetTextLineHeight() + 10.0f;

			ImGui::InputTextMultiline( "##status", const_cast<char*>( successMsg.c_str() ),
				successMsg.size() + 1, ImVec2( -FLT_MIN, height ), ImGuiInputTextFlags_ReadOnly );

			ImGui::PopStyleColor( 2 );
		}
	}

	// Show uniform controls
	if( bindings && ImGui::CollapsingHeader( "Uniforms", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ShaderUniformsWithBindings( shader, *bindings );
	}

	return recompiled;
}

// Uniform controls using bindings
static bool ShaderUniformsWithBindings( const gl::GlslProgRef& shader, const std::vector<UniformBinding>& bindings )
{
	if( ! shader || bindings.empty() )
		return false;

	bool modified = false;

	for( const auto& binding : bindings ) {
		// Find the uniform in the shader
		GLint location = shader->getUniformLocation( binding.name );
		if( location == -1 ) {
			ImGui::TextDisabled( "%s (not found)", binding.name.c_str() );
			continue;
		}

		ImGui::PushID( location );

		// Handle different uniform types
		switch( binding.type ) {
			case GL_FLOAT: {
				float* pVal = static_cast<float*>( binding.ptr );
				if( binding.hasRange ) {
					if( ImGui::SliderFloat( binding.name.c_str(), pVal, binding.rangeMin, binding.rangeMax ) ) {
						modified = true;
					}
				}
				else {
					if( ImGui::DragFloat( binding.name.c_str(), pVal, 0.01f ) ) {
						modified = true;
					}
				}
				shader->uniform( location, *pVal );
				break;
			}

			case GL_FLOAT_VEC2: {
				vec2* pVal = static_cast<vec2*>( binding.ptr );
				if( ImGui::DragFloat2( binding.name.c_str(), &pVal->x, 0.01f ) ) {
					modified = true;
				}
				shader->uniform( location, *pVal );
				break;
			}

			case GL_FLOAT_VEC3: {
				vec3* pVal = static_cast<vec3*>( binding.ptr );
				if( binding.vec3Semantic == Vec3Semantic::COLOR ) {
					// Treat as RGB color
					if( ImGui::ColorEdit3( binding.name.c_str(), &pVal->x ) ) {
						modified = true;
					}
				}
				else {
					// Treat as generic vector
					if( ImGui::DragFloat3( binding.name.c_str(), &pVal->x, 0.1f ) ) {
						modified = true;
					}
				}
				shader->uniform( location, *pVal );
				break;
			}

			case GL_FLOAT_VEC4: {
				vec4* pVal = static_cast<vec4*>( binding.ptr );
				if( ImGui::DragFloat4( binding.name.c_str(), &pVal->x, 0.01f ) ) {
					modified = true;
				}
				shader->uniform( location, *pVal );
				break;
			}

			case GL_INT: {
				int* pVal = static_cast<int*>( binding.ptr );
				if( ImGui::DragInt( binding.name.c_str(), pVal ) ) {
					modified = true;
				}
				shader->uniform( location, *pVal );
				break;
			}

			case GL_BOOL: {
				bool* pVal = static_cast<bool*>( binding.ptr );
				if( ImGui::Checkbox( binding.name.c_str(), pVal ) ) {
					modified = true;
				}
				shader->uniform( location, *pVal );
				break;
			}

			default:
				ImGui::Text( "%s: 0x%04X (unsupported)", binding.name.c_str(), binding.type );
				break;
		}

		ImGui::PopID();
	}

	return modified;
}

} // namespace ImGui
