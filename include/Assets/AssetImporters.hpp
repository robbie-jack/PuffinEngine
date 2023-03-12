#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace tinygltf
{
	class Model;
}

namespace Puffin::IO
{
	//////////////////////
	// Model Importers
	//////////////////////

	bool LoadAndImportModel(fs::path modelPath);

	bool LoadGLTFModel(tinygltf::Model& model, fs::path modelPath);

	bool ImportGLTFModel(const tinygltf::Model& model);

	bool LoadAndImportGLTFModel(fs::path modelPath);

	//////////////////////
	// Texture Importers
	//////////////////////

	bool LoadAndImportTexture(fs::path texturePath);
}
