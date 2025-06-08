#pragma once

#include <filesystem>

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

	bool LoadAndImportModel(const fs::path& modelPath, fs::path assetSubdirectory);

	// OBJ

	bool LoadAndImportObjModel(const fs::path& modelPath, fs::path assetSubdirectory);

	// GLTF

	bool LoadGltfModel(const fs::path& modelPath, tinygltf::Model& model);

	bool ImportGltfModel(const fs::path& modelPath, const tinygltf::Model& model, fs::path assetSubdirectory);

	bool LoadAndImportGltfModel(const fs::path& modelPath, fs::path assetSubdirectory);

	//////////////////////
	// Texture Importers
	//////////////////////
	
	bool LoadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat = true);
}
