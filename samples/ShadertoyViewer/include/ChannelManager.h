#pragma once

#include "cinder/gl/Texture.h"
#include "cinder/Color.h"
#include "cinder/app/App.h"
#include <filesystem>

// Procedural texture types
enum class ProceduralType {
	Checkerboard,
	LinearGradient,
	RadialGradient,
	Noise,
	Clouds,
	FileTexture
};

// Channel configuration for texture inputs
struct ChannelConfig {
	ci::gl::TextureRef	texture;
	std::filesystem::path path;
	bool			filterLinear = true;	// false = nearest
	bool			wrapRepeat = true;		// false = clamp
	ci::vec3		resolution;				// width, height, 1.0
	float			time = 0.0f;			// channel playback time

	// Cache for texture state to avoid redundant GL calls
	bool			lastFilterLinear = true;
	bool			lastWrapRepeat = true;

	// Procedural texture params
	ProceduralType	proceduralType = ProceduralType::Checkerboard;
	ci::Color		color1 = ci::Color( 0.0f, 0.0f, 0.0f );	// Primary color
	ci::Color		color2 = ci::Color( 1.0f, 1.0f, 1.0f );	// Secondary color
	int				gridSize = 8;						// Checkerboard grid size
	float			angle = 0.0f;						// Gradient angle (degrees)
	float			frequency = 4.0f;					// Clouds frequency
	int				octaves = 4;						// Clouds octaves
	int				textureSize = 256;					// Resolution for procedural textures
};

class ChannelManager {
public:
	ChannelManager();

	// Channel management
	void createDefaultTexture( int channelIndex );
	void loadChannelTexture( int channelIndex, const std::filesystem::path &path );
	void generateProceduralTexture( int channelIndex );

	// Access
	ChannelConfig& getChannel( int index ) { return mChannels[index]; }
	const ChannelConfig& getChannel( int index ) const { return mChannels[index]; }

	// ImGui interface
	void drawChannelsPane( bool *pOpen, ci::app::App *app );

private:
	ChannelConfig mChannels[4];
};
