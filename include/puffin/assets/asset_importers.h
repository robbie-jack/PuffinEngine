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

	bool loadAndImportModel(const fs::path& modelPath, fs::path assetSubdirectory);

	// OBJ

	bool loadAndImportOBJModel(const fs::path& modelPath, fs::path assetSubdirectory);

	// GLTF

	bool loadGLTFModel(const fs::path& modelPath, tinygltf::Model& model);

	bool importGLTFModel(const fs::path& modelPath, const tinygltf::Model& model, fs::path assetSubdirectory);

	bool loadAndImportGLTFModel(const fs::path& modelPath, fs::path assetSubdirectory);

	//////////////////////
	// Texture Importers
	//////////////////////
	
	bool loadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat = true);
}
