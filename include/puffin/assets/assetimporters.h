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

	bool load_and_import_obj_model(const fs::path& modelPath, fs::path assetSubdirectory);

	// GLTF

	bool load_gltf_model(const fs::path& modelPath, tinygltf::Model& model);

	bool import_gltf_model(const fs::path& modelPath, const tinygltf::Model& model, fs::path assetSubdirectory);

	bool load_and_import_gltf_model(const fs::path& modelPath, fs::path assetSubdirectory);

	//////////////////////
	// Texture Importers
	//////////////////////
	
	bool LoadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat = true);
}
