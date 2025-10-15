#include "cinder/CinderImGui.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Log.h"
#include <map>
#include <set>
#include <string>

using namespace ci;
using namespace ci::gl;

namespace ImGui {

// Storage for shader source edits (persisted across frames)
struct ShaderEditorState {
	std::string vertexSource;
	std::string fragmentSource;
	std::string geometrySource;
	std::string computeSource;
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
static bool RenderUniformFromCache( const gl::GlslProgRef& shader, const gl::GlslProg::Uniform& uniform, UniformSemantic semantic = UniformSemantic::DEFAULT, bool hasRange = false, float rangeMin = 0.0f, float rangeMax = 0.0f );

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
		state.computeSource = shader->getComputeShaderSource();
		state.initialized = true;
	}

	// Push unique ID for this shader editor to avoid conflicts when multiple editors are open
	ImGui::PushID( shaderId );

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
			bool success = false;

			// Check if this is a compute shader
			if( ! state.computeSource.empty() ) {
				// Compute shader: use compileShader + rebuild
				GLuint compHandle = gl::GlslProg::compileShader( GL_COMPUTE_SHADER, state.computeSource, &error );
				if( compHandle ) {
					success = const_cast<gl::GlslProg*>( shader.get() )->rebuild( 0, 0, 0, compHandle, &error );
					glDeleteShader( compHandle );
				}
			}
			else {
				// Regular vertex/fragment/geometry shader: use recompile
				success = const_cast<gl::GlslProg*>( shader.get() )->recompile( state.vertexSource, state.fragmentSource, state.geometrySource, &error );
			}

			if( success ) {
				state.hasError = false;
				state.compileError.clear();
				recompiled = true;
				// Don't log success - user can see it in the UI
			}
			else {
				state.hasError = true;
				// Strip trailing newline from error message
				state.compileError = error;
				while( ! state.compileError.empty() && ( state.compileError.back() == '\n' || state.compileError.back() == '\r' ) ) {
					state.compileError.pop_back();
				}
				// Don't log error - user can see it in the UI
			}
		}

		ImGui::Separator();

		// Compute shader editor (if present)
		if( ! state.computeSource.empty() ) {
			if( ImGui::CollapsingHeader( "Compute Shader", ImGuiTreeNodeFlags_DefaultOpen ) ) {
				static const size_t bufferSize = 1024 * 16;
				static char computeBuffer[bufferSize];

				if( state.computeSource.size() < bufferSize ) {
					strcpy_s( computeBuffer, bufferSize, state.computeSource.c_str() );
				}

				if( ImGui::InputTextMultiline( "##compute", computeBuffer, bufferSize,
					ImVec2( -FLT_MIN, 300 ), ImGuiInputTextFlags_AllowTabInput ) ) {
					state.computeSource = computeBuffer;
					state.sourceChanged = true;
				}
			}
		}

		// Vertex shader editor
		if( ! state.vertexSource.empty() && ImGui::CollapsingHeader( "Vertex Shader", ImGuiTreeNodeFlags_DefaultOpen ) ) {
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
		if( ! state.fragmentSource.empty() && ImGui::CollapsingHeader( "Fragment Shader", ImGuiTreeNodeFlags_DefaultOpen ) ) {
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

	// Show uniform controls - always show header, will auto-discover uniforms if no bindings provided
	if( ImGui::CollapsingHeader( "Uniforms", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		if( bindings && ! bindings->empty() ) {
			// Have explicit bindings - use them (additive mode: explicit bindings + auto-discovered)
			ShaderUniformsWithBindings( shader, *bindings );
		}
		else {
			// No bindings - auto-discover all uniforms
			ShaderUniformsWithBindings( shader, {} );
		}
	}

	ImGui::PopID();

	return recompiled;
}

// Helper function to render a uniform widget using backing store
static bool RenderUniformFromCache( const gl::GlslProgRef& shader, const gl::GlslProg::Uniform& uniform, UniformSemantic semantic, bool hasRange, float rangeMin, float rangeMax )
{
	bool modified = false;
	GLint location = uniform.getLocation();

	ImGui::PushID( location );

	switch( uniform.getType() ) {
		case GL_FLOAT: {
			float val = 0.0f;
			shader->getUniformValue( uniform.getName(), &val );
			if( hasRange ) {
				if( ImGui::SliderFloat( uniform.getName().c_str(), &val, rangeMin, rangeMax ) ) {
					shader->uniform( location, val );
					modified = true;
				}
			}
			else {
				if( ImGui::DragFloat( uniform.getName().c_str(), &val, 0.01f ) ) {
					shader->uniform( location, val );
					modified = true;
				}
			}
			break;
		}

		case GL_FLOAT_VEC2: {
			vec2 val( 0.0f );
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragFloat2( uniform.getName().c_str(), &val.x, 0.01f ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_FLOAT_VEC3: {
			vec3 val( 0.0f );
			shader->getUniformValue( uniform.getName(), &val );
			if( semantic == UniformSemantic::COLOR ) {
				if( ImGui::ColorEdit3( uniform.getName().c_str(), &val.x ) ) {
					shader->uniform( location, val );
					modified = true;
				}
			}
			else {
				if( ImGui::DragFloat3( uniform.getName().c_str(), &val.x, 0.1f ) ) {
					shader->uniform( location, val );
					modified = true;
				}
			}
			break;
		}

		case GL_FLOAT_VEC4: {
			vec4 val( 0.0f );
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragFloat4( uniform.getName().c_str(), &val.x, 0.01f ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_INT: {
			int val = 0;
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragInt( uniform.getName().c_str(), &val ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_BOOL: {
			bool val = false;
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::Checkbox( uniform.getName().c_str(), &val ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_INT_VEC2: {
			ivec2 val( 0 );
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragInt2( uniform.getName().c_str(), &val.x ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_INT_VEC3: {
			ivec3 val( 0 );
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragInt3( uniform.getName().c_str(), &val.x ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_INT_VEC4: {
			ivec4 val( 0 );
			shader->getUniformValue( uniform.getName(), &val );
			if( ImGui::DragInt4( uniform.getName().c_str(), &val.x ) ) {
				shader->uniform( location, val );
				modified = true;
			}
			break;
		}

		case GL_FLOAT_MAT4: {
			ImGui::TextDisabled( "%s: mat4 (not editable)", uniform.getName().c_str() );
			break;
		}

		case GL_FLOAT_MAT3: {
			ImGui::TextDisabled( "%s: mat3 (not editable)", uniform.getName().c_str() );
			break;
		}

		case GL_FLOAT_MAT2: {
			ImGui::TextDisabled( "%s: mat2 (not editable)", uniform.getName().c_str() );
			break;
		}

		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
			ImGui::TextDisabled( "%s: sampler (not editable)", uniform.getName().c_str() );
			break;

		default:
			ImGui::TextDisabled( "%s: 0x%04X (unsupported)", uniform.getName().c_str(), uniform.getType() );
			break;
	}

	ImGui::PopID();
	return modified;
}

// Uniform controls using bindings (additive mode with metadata hints)
static bool ShaderUniformsWithBindings( const gl::GlslProgRef& shader, const std::vector<UniformBinding>& bindings )
{
	if( ! shader )
		return false;

	bool modified = false;

	// Build map of uniform names to their metadata hints
	std::map<std::string, UniformBinding> bindingMap;
	for( const auto& binding : bindings ) {
		bindingMap[binding.name] = binding;
	}

	// Render all uniforms, using metadata hints where available
	const auto& activeUniforms = shader->getActiveUniforms();

	// Separate into hinted and unhinted for better organization
	std::vector<const gl::GlslProg::Uniform*> hintedUniforms;
	std::vector<const gl::GlslProg::Uniform*> unhintedUniforms;

	for( const auto& uniform : activeUniforms ) {
		if( bindingMap.find( uniform.getName() ) != bindingMap.end() ) {
			hintedUniforms.push_back( &uniform );
		}
		else {
			unhintedUniforms.push_back( &uniform );
		}
	}

	// First, render uniforms with explicit metadata hints
	for( const auto* uniform : hintedUniforms ) {
		const auto& binding = bindingMap[uniform->getName()];
		if( RenderUniformFromCache( shader, *uniform, binding.semantic, binding.hasRange, binding.rangeMin, binding.rangeMax ) ) {
			modified = true;
		}
	}

	// Then, render remaining uniforms (excluding Cinder Context uniforms)
	if( ! unhintedUniforms.empty() && ! hintedUniforms.empty() ) {
		ImGui::Separator();
		ImGui::Text( "Other uniforms:" );
	}

	for( const auto* uniform : unhintedUniforms ) {
		if( RenderUniformFromCache( shader, *uniform, UniformSemantic::DEFAULT, false, 0.0f, 0.0f ) ) {
			modified = true;
		}
	}

	return modified;
}

} // namespace ImGui
