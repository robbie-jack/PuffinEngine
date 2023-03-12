#include "Assets/AssetImporters.hpp"

#define TINYGLTF_IMPLEMENTATION

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#include "tiny_gltf.h"

#include "Assets/TextureAsset.h"

#include <iostream>
#include <string>

namespace Puffin::IO
{
	//////////////////////
	// Model Importers
	//////////////////////
	
	bool LoadAndImportModel(fs::path modelPath)
	{
		if (modelPath.extension() == ".gltf")
			return LoadAndImportGLTFModel(modelPath);

		return false;
	}

	bool LoadGLTFModel(tinygltf::Model& model, fs::path modelPath)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::cout << "Loading glTF: " << modelPath << std::endl;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.string());

		if (!warn.empty())
		{
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty())
		{
			std::cout << "ERR: " << err << std::endl;
		}

		if (!ret)
			std::cout << "Failed to load glTF: " << modelPath << std::endl;
		else
			std::cout << "Loaded glTF: " << modelPath << std::endl;

		return ret;
	}

	bool ImportGLTFModel(const tinygltf::Model& model)
	{
		for (const auto& mesh : model.meshes)
		{
			//mesh.
		}

		return false;
	}

	bool LoadAndImportGLTFModel(fs::path modelPath)
	{
		tinygltf::Model model;

		if (!LoadGLTFModel(model, modelPath))
			return false;

		return ImportGLTFModel(model);
	}

	//////////////////////
	// Texture Importers
	//////////////////////

	bool LoadAndImportTexture(fs::path texturePath)
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
		bool ret = asset->Save(info, pixelPtr);

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		return ret;
	}
}
