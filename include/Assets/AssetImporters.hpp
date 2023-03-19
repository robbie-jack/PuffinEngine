#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace tinygltf
{
	class Model;
	class BufferView;
}

namespace Puffin::IO
{
	//////////////////////
	// Model Importers
	//////////////////////

	bool LoadAndImportModel(const fs::path& modelPath);

	bool LoadGLTFModel(tinygltf::Model& model, const fs::path& modelPath);

	bool ImportGLTFModel(const tinygltf::Model& model, const fs::path& modelPath);

	bool LoadAndImportGLTFModel(const fs::path& modelPath);

	bool LoadBinaryData(const fs::path& binaryPath, const int& byteOffset, const int& byteLength, std::vector<char>& binaryData);

	//////////////////////
	// Texture Importers
	//////////////////////

	bool LoadAndImportTexture(fs::path texturePath);
}
