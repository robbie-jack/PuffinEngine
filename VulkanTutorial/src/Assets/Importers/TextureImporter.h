#pragma once

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <stb-master\stb_image.h>

#include "Assets/TextureAsset.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin::IO
{
	static bool ImportTexture(fs::path texturePath)
	{
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(texturePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		void* pixelPtr = pixels;

		// Instantiate new Texture Asset to store loaded Pixel data
		fs::path importPath = fs::path() / "textures" / texturePath.stem();
		importPath += ".ptexture";

		Assets::TextureInfo info;
		info.compressionMode = Assets::CompressionMode::LZ4;
		info.originalFile = texturePath.string();
		info.textureFormat = Assets::TextureFormat::RGBA8;
		info.textureHeight = (uint32_t)texHeight;
		info.textureWidth = (uint32_t)texWidth;
		info.originalSize = info.textureHeight * info.textureWidth * 4;

		auto asset = Assets::AssetRegistry::Get()->AddAsset<Assets::TextureAsset>(importPath);
		bool savedSuccessfully = asset->Save(info, pixelPtr);

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		return savedSuccessfully;
	}
}