#pragma once

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace tinygltf
{
	class Model;
	class BufferView;
}

namespace puffin::io
{
	//////////////////////
	// Model Importers
	//////////////////////

	bool loadAndImportModel(const fs::path& modelPath);

	bool loadGltfModel(tinygltf::Model& model, const fs::path& modelPath);

	bool importGltfModel(const tinygltf::Model& model, const fs::path& modelPath);

	bool loadAndImportGltfModel(const fs::path& modelPath);

	bool loadBinaryData(const fs::path& binaryPath, const int& byteOffset, const int& byteLength, std::vector<char>& binaryData);

	//////////////////////
	// Texture Importers
	//////////////////////
	
	bool loadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory = "textures", bool useBCFormat = true);
}
