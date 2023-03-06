#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin::IO::GLTF
{
	inline void ImportModel(fs::path modelPath)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.string());

		model.
	}

	inline void ImportScene()
	{
		
	}
}