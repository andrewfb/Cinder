#include "ChannelManager.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/CinderImGui.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;

ChannelManager::ChannelManager()
{
	// Initialize channels with different default patterns for variety
	// Channel 0: Checkerboard
	mChannels[0].proceduralType = ProceduralType::Checkerboard;
	mChannels[0].gridSize = 8;
	generateProceduralTexture( 0 );

	// Channel 1: Noise
	mChannels[1].proceduralType = ProceduralType::Noise;
	generateProceduralTexture( 1 );

	// Channel 2: Radial Gradient (different from checkerboard!)
	mChannels[2].proceduralType = ProceduralType::RadialGradient;
	generateProceduralTexture( 2 );

	// Channel 3: Noise
	mChannels[3].proceduralType = ProceduralType::Noise;
	generateProceduralTexture( 3 );
}

void ChannelManager::createDefaultTexture( int channelIndex )
{
	if( channelIndex < 0 || channelIndex >= 4 )
		return;

	// Create a simple checkerboard texture
	const int size = 64;
	Surface8u surface( size, size, false );

	for( int y = 0; y < size; ++y ) {
		for( int x = 0; x < size; ++x ) {
			bool check = ( ( x / 8 ) + ( y / 8 ) ) % 2 == 0;
			uint8_t value = check ? 255 : 128;
			surface.setPixel( ivec2( x, y ), Color8u( value, value, value ) );
		}
	}

	mChannels[channelIndex].texture = gl::Texture::create( surface );
	mChannels[channelIndex].resolution = vec3( size, size, 1.0f );
	mChannels[channelIndex].path = "[default checkerboard]";
}

void ChannelManager::loadChannelTexture( int channelIndex, const ci::fs::path &path )
{
	if( channelIndex < 0 || channelIndex >= 4 )
		return;

	try {
		auto img = loadImage( path );
		Surface8u surface( img );

		mChannels[channelIndex].texture = gl::Texture::create( surface );
		mChannels[channelIndex].textureCube = nullptr;
		mChannels[channelIndex].texture3d = nullptr;
		mChannels[channelIndex].textureType = TextureType::Texture2D;
		mChannels[channelIndex].resolution = vec3( surface.getWidth(), surface.getHeight(), 1.0f );
		mChannels[channelIndex].path = path;
		mChannels[channelIndex].proceduralType = ProceduralType::FileTexture;
		mChannels[channelIndex].timeDependent = false;	// Static image - don't advance time
		mChannels[channelIndex].time = 0.0f;			// Reset to 0
		CI_LOG_I( "Loaded texture for channel " << channelIndex << ": " << path );
	}
	catch( const std::exception &exc ) {
		CI_LOG_E( "Failed to load texture: " << exc.what() );
	}
}

void ChannelManager::loadChannelCubemap( int channelIndex, const ci::fs::path &path )
{
	if( channelIndex < 0 || channelIndex >= 4 )
		return;

	try {
		// Load image and automatically infer cubemap layout (horizontal cross, vertical cross, row, or column)
		auto img = loadImage( path );

		// Create cubemap - automatically infers layout based on aspect ratio
		auto cubeMap = gl::TextureCubeMap::create( img, gl::TextureCubeMap::Format().mipmap() );

		mChannels[channelIndex].texture = nullptr;
		mChannels[channelIndex].textureCube = cubeMap;
		mChannels[channelIndex].texture3d = nullptr;
		mChannels[channelIndex].textureType = TextureType::TextureCube;
		mChannels[channelIndex].resolution = vec3( (float)cubeMap->getWidth(), (float)cubeMap->getHeight(), 1.0f );
		mChannels[channelIndex].path = path;
		mChannels[channelIndex].proceduralType = ProceduralType::FileTexture;
		mChannels[channelIndex].timeDependent = false;	// Static cubemap - don't advance time
		mChannels[channelIndex].time = 0.0f;			// Reset to 0
		CI_LOG_I( "Loaded cubemap for channel " << channelIndex << ": " << path );
	}
	catch( const std::exception &exc ) {
		CI_LOG_E( "Failed to load cubemap: " << exc.what() );
	}
}

void ChannelManager::loadChannelCubemapFaces( int channelIndex, const std::vector<ci::fs::path> &faces )
{
	if( channelIndex < 0 || channelIndex >= 4 )
		return;

	if( faces.size() != 6 ) {
		CI_LOG_E( "Cubemap requires exactly 6 face images (got " << faces.size() << ")" );
		return;
	}

	try {
		// Load all 6 faces in order: +X, -X, +Y, -Y, +Z, -Z
		ImageSourceRef images[6];
		for( int i = 0; i < 6; ++i ) {
			images[i] = loadImage( faces[i] );
		}

		// Create cubemap from faces (expects images ordered { +X, -X, +Y, -Y, +Z, -Z })
		auto cubeMap = gl::TextureCubeMap::create( images, gl::TextureCubeMap::Format().mipmap() );

		mChannels[channelIndex].texture = nullptr;
		mChannels[channelIndex].textureCube = cubeMap;
		mChannels[channelIndex].texture3d = nullptr;
		mChannels[channelIndex].textureType = TextureType::TextureCube;
		mChannels[channelIndex].resolution = vec3( (float)cubeMap->getWidth(), (float)cubeMap->getHeight(), 1.0f );
		mChannels[channelIndex].path = "[Cubemap from 6 faces]";
		mChannels[channelIndex].proceduralType = ProceduralType::FileTexture;
		CI_LOG_I( "Loaded cubemap from 6 faces for channel " << channelIndex );
	}
	catch( const std::exception &exc ) {
		CI_LOG_E( "Failed to load cubemap faces: " << exc.what() );
	}
}

void ChannelManager::generateProceduralTexture( int channelIndex )
{
	if( channelIndex < 0 || channelIndex >= 4 )
		return;

	int size = mChannels[channelIndex].textureSize;
	Surface8u surface( size, size, false );
	auto &ch = mChannels[channelIndex];

	switch( ch.proceduralType ) {
		case ProceduralType::Checkerboard: {
			for( int y = 0; y < size; ++y ) {
				for( int x = 0; x < size; ++x ) {
					bool check = ( ( x / ch.gridSize ) + ( y / ch.gridSize ) ) % 2 == 0;
					Color col = check ? ch.color1 : ch.color2;
					surface.setPixel( ivec2( x, y ), Color8u( (uint8_t)( col.r * 255.0f ), (uint8_t)( col.g * 255.0f ), (uint8_t)( col.b * 255.0f ) ) );
				}
			}
			ch.path = "[Checkerboard]";
			break;
		}

		case ProceduralType::LinearGradient: {
			float angleRad = glm::radians( ch.angle );
			vec2 dir( cos( angleRad ), sin( angleRad ) );
			for( int y = 0; y < size; ++y ) {
				for( int x = 0; x < size; ++x ) {
					vec2 pos( (float)x / size - 0.5f, (float)y / size - 0.5f );
					float t = glm::dot( pos, dir ) + 0.5f;
					t = glm::clamp( t, 0.0f, 1.0f );
					float r = ch.color1.r * ( 1.0f - t ) + ch.color2.r * t;
					float g = ch.color1.g * ( 1.0f - t ) + ch.color2.g * t;
					float b = ch.color1.b * ( 1.0f - t ) + ch.color2.b * t;
					surface.setPixel( ivec2( x, y ), Color8u( (uint8_t)( r * 255.0f ), (uint8_t)( g * 255.0f ), (uint8_t)( b * 255.0f ) ) );
				}
			}
			ch.path = "[Linear Gradient]";
			break;
		}

		case ProceduralType::RadialGradient: {
			vec2 center( size / 2.0f, size / 2.0f );
			float maxDist = size * 0.5f;
			for( int y = 0; y < size; ++y ) {
				for( int x = 0; x < size; ++x ) {
					float dist = glm::distance( vec2( (float)x, (float)y ), center );
					float t = glm::clamp( dist / maxDist, 0.0f, 1.0f );
					float r = ch.color1.r * ( 1.0f - t ) + ch.color2.r * t;
					float g = ch.color1.g * ( 1.0f - t ) + ch.color2.g * t;
					float b = ch.color1.b * ( 1.0f - t ) + ch.color2.b * t;
					surface.setPixel( ivec2( x, y ), Color8u( (uint8_t)( r * 255.0f ), (uint8_t)( g * 255.0f ), (uint8_t)( b * 255.0f ) ) );
				}
			}
			ch.path = "[Radial Gradient]";
			break;
		}

		case ProceduralType::Noise: {
			// Simple random noise
			for( int y = 0; y < size; ++y ) {
				for( int x = 0; x < size; ++x ) {
					float t = (float)rand() / (float)RAND_MAX;
					float r = ch.color1.r * ( 1.0f - t ) + ch.color2.r * t;
					float g = ch.color1.g * ( 1.0f - t ) + ch.color2.g * t;
					float b = ch.color1.b * ( 1.0f - t ) + ch.color2.b * t;
					surface.setPixel( ivec2( x, y ), Color8u( (uint8_t)( r * 255.0f ), (uint8_t)( g * 255.0f ), (uint8_t)( b * 255.0f ) ) );
				}
			}
			ch.path = "[Noise]";
			break;
		}

		case ProceduralType::Clouds: {
			// Multi-octave noise for cloud-like patterns
			auto smoothNoise = [](float x, float y) -> float {
				int ix = (int)x;
				int iy = (int)y;
				float fx = x - ix;
				float fy = y - iy;

				// Hash function for pseudo-random values
				auto hash = [](int x, int y) -> float {
					int n = x + y * 57;
					n = (n << 13) ^ n;
					return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
				};

				// Bilinear interpolation
				float a = hash(ix, iy);
				float b = hash(ix + 1, iy);
				float c = hash(ix, iy + 1);
				float d = hash(ix + 1, iy + 1);

				float u = fx * fx * (3.0f - 2.0f * fx);
				float v = fy * fy * (3.0f - 2.0f * fy);

				return glm::mix(glm::mix(a, b, u), glm::mix(c, d, u), v);
			};

			for( int y = 0; y < size; ++y ) {
				for( int x = 0; x < size; ++x ) {
					float value = 0.0f;
					float amplitude = 1.0f;
					float frequency = ch.frequency / size;
					float maxValue = 0.0f;

					// Accumulate octaves
					for( int oct = 0; oct < ch.octaves; ++oct ) {
						value += smoothNoise(x * frequency, y * frequency) * amplitude;
						maxValue += amplitude;
						amplitude *= 0.5f;
						frequency *= 2.0f;
					}

					// Normalize and apply colors
					float t = (value / maxValue) * 0.5f + 0.5f;
					t = glm::clamp(t, 0.0f, 1.0f);
					float r = ch.color1.r * ( 1.0f - t ) + ch.color2.r * t;
					float g = ch.color1.g * ( 1.0f - t ) + ch.color2.g * t;
					float b = ch.color1.b * ( 1.0f - t ) + ch.color2.b * t;
					surface.setPixel( ivec2( x, y ), Color8u( (uint8_t)( r * 255.0f ), (uint8_t)( g * 255.0f ), (uint8_t)( b * 255.0f ) ) );
				}
			}
			ch.path = "[Clouds]";
			break;
		}

		case ProceduralType::FileTexture:
			// Don't regenerate file textures
			return;
	}

	ch.texture = gl::Texture::create( surface );
	ch.textureType = TextureType::Texture2D;
	ch.resolution = vec3( size, size, 1.0f );
	ch.timeDependent = false;	// Procedural textures are static
	ch.time = 0.0f;				// Reset to 0
}

void ChannelManager::drawChannelsPane( bool *pOpen, App *app )
{
	// Position on right side below Playback pane
	ImGui::SetNextWindowPos( ImVec2( (float)app->getWindowWidth() - 600, 1060 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 580, 400 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "Channels", pOpen );

	for( int i = 0; i < 4; ++i ) {
		ImGui::PushID( i );

		std::string label = "iChannel" + std::to_string( i );
		if( ImGui::CollapsingHeader( label.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) ) {
			// Texture type selector
			const char* typeNames[] = { "Checkerboard", "Linear Gradient", "Radial Gradient", "Noise", "Clouds", "File Texture" };
			int currentType = (int)mChannels[i].proceduralType;
			if( ImGui::Combo( "Type", &currentType, typeNames, IM_ARRAYSIZE( typeNames ) ) ) {
				mChannels[i].proceduralType = (ProceduralType)currentType;
				if( currentType != (int)ProceduralType::FileTexture ) {
					generateProceduralTexture( i );
				}
			}

			// Type-specific controls
			switch( mChannels[i].proceduralType ) {
				case ProceduralType::Checkerboard: {
					float col1[3] = { mChannels[i].color1.r, mChannels[i].color1.g, mChannels[i].color1.b };
					if( ImGui::ColorEdit3( "Color 1", col1 ) ) {
						mChannels[i].color1 = Color( col1[0], col1[1], col1[2] );
						generateProceduralTexture( i );
					}
					float col2[3] = { mChannels[i].color2.r, mChannels[i].color2.g, mChannels[i].color2.b };
					if( ImGui::ColorEdit3( "Color 2", col2 ) ) {
						mChannels[i].color2 = Color( col2[0], col2[1], col2[2] );
						generateProceduralTexture( i );
					}
					if( ImGui::SliderInt( "Grid Size", &mChannels[i].gridSize, 2, 64 ) ) {
						generateProceduralTexture( i );
					}
					break;
				}

				case ProceduralType::LinearGradient:
				case ProceduralType::RadialGradient: {
					float col1[3] = { mChannels[i].color1.r, mChannels[i].color1.g, mChannels[i].color1.b };
					if( ImGui::ColorEdit3( "Start Color", col1 ) ) {
						mChannels[i].color1 = Color( col1[0], col1[1], col1[2] );
						generateProceduralTexture( i );
					}
					float col2[3] = { mChannels[i].color2.r, mChannels[i].color2.g, mChannels[i].color2.b };
					if( ImGui::ColorEdit3( "End Color", col2 ) ) {
						mChannels[i].color2 = Color( col2[0], col2[1], col2[2] );
						generateProceduralTexture( i );
					}
					if( mChannels[i].proceduralType == ProceduralType::LinearGradient ) {
						if( ImGui::SliderFloat( "Angle", &mChannels[i].angle, 0.0f, 360.0f ) ) {
							generateProceduralTexture( i );
						}
					}
					break;
				}

				case ProceduralType::Noise: {
					float col1[3] = { mChannels[i].color1.r, mChannels[i].color1.g, mChannels[i].color1.b };
					if( ImGui::ColorEdit3( "Min Color", col1 ) ) {
						mChannels[i].color1 = Color( col1[0], col1[1], col1[2] );
						generateProceduralTexture( i );
					}
					float col2[3] = { mChannels[i].color2.r, mChannels[i].color2.g, mChannels[i].color2.b };
					if( ImGui::ColorEdit3( "Max Color", col2 ) ) {
						mChannels[i].color2 = Color( col2[0], col2[1], col2[2] );
						generateProceduralTexture( i );
					}
					if( ImGui::Button( "Regenerate Noise" ) ) {
						generateProceduralTexture( i );
					}
					break;
				}

				case ProceduralType::Clouds: {
					float col1[3] = { mChannels[i].color1.r, mChannels[i].color1.g, mChannels[i].color1.b };
					if( ImGui::ColorEdit3( "Color 0", col1 ) ) {
						mChannels[i].color1 = Color( col1[0], col1[1], col1[2] );
						generateProceduralTexture( i );
					}
					float col2[3] = { mChannels[i].color2.r, mChannels[i].color2.g, mChannels[i].color2.b };
					if( ImGui::ColorEdit3( "Color 1", col2 ) ) {
						mChannels[i].color2 = Color( col2[0], col2[1], col2[2] );
						generateProceduralTexture( i );
					}
					if( ImGui::SliderFloat( "Frequency", &mChannels[i].frequency, 1.0f, 16.0f ) ) {
						generateProceduralTexture( i );
					}
					if( ImGui::SliderInt( "Octaves", &mChannels[i].octaves, 1, 8 ) ) {
						generateProceduralTexture( i );
					}
					break;
				}

				case ProceduralType::FileTexture: {
					// Display current texture type prominently
					const char* textureTypeStr = "Unknown";
					ImVec4 typeColor = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
					switch( mChannels[i].textureType ) {
						case TextureType::Texture2D:
							textureTypeStr = "2D Texture";
							typeColor = ImVec4( 0.4f, 0.8f, 1.0f, 1.0f );  // Light blue
							break;
						case TextureType::TextureCube:
							textureTypeStr = "Cubemap";
							typeColor = ImVec4( 1.0f, 0.7f, 0.3f, 1.0f );  // Orange
							break;
						case TextureType::Texture3D:
							textureTypeStr = "3D Texture";
							typeColor = ImVec4( 0.7f, 1.0f, 0.4f, 1.0f );  // Green
							break;
					}
					ImGui::TextColored( typeColor, "Type: %s", textureTypeStr );

					// Load buttons
					if( ImGui::Button( "Load 2D Texture" ) ) {
						auto path = app->getOpenFilePath( "", ImageIo::getLoadExtensions() );
						if( !path.empty() ) {
							loadChannelTexture( i, path );
						}
					}
					ImGui::SameLine();
					if( ImGui::Button( "Load Cubemap" ) ) {
						auto path = app->getOpenFilePath( "", ImageIo::getLoadExtensions() );
						if( !path.empty() ) {
							loadChannelCubemap( i, path );
						}
					}

					break;
				}
			}

			// Resolution control for procedural textures
			if( mChannels[i].proceduralType != ProceduralType::FileTexture ) {
				int texSizes[] = { 64, 128, 256, 512, 1024, 2048 };
				const char* texSizeNames[] = { "64x64", "128x128", "256x256", "512x512", "1024x1024", "2048x2048" };
				int currentSizeIdx = 2; // default 256
				for( int j = 0; j < 6; ++j ) {
					if( mChannels[i].textureSize == texSizes[j] ) {
						currentSizeIdx = j;
						break;
					}
				}
				if( ImGui::Combo( "Resolution", &currentSizeIdx, texSizeNames, 6 ) ) {
					mChannels[i].textureSize = texSizes[currentSizeIdx];
					generateProceduralTexture( i );
				}
			}

			// Display path/name
			if( !mChannels[i].path.empty() ) {
				ImGui::TextWrapped( "Source: %s", mChannels[i].path.string().c_str() );
			}

			// Filter and wrap modes
			ImGui::Checkbox( "Linear Filter", &mChannels[i].filterLinear );
			ImGui::SameLine();
			ImGui::Checkbox( "Repeat Wrap", &mChannels[i].wrapRepeat );

			// Resolution info
			ImGui::Text( "Actual Resolution: %.0f x %.0f", mChannels[i].resolution.x, mChannels[i].resolution.y );
		}

		ImGui::PopID();
	}

	ImGui::End();
}
