#include "puffin/assets/shader_asset.h"

#include "puffin/assets/asset_registry.h"

namespace puffin::assets
{
	bool ShaderAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::Shader;
		data.version = gShaderAssetVersion;

		// Fill Metadata from Info struct
		data.json_data["shader_type"] = parseShaderStringFromType(mShaderType);
		data.json_data["shader_path"] = mShaderPath;
		data.json_data["binary_path"] = mBinaryPath;

		return saveJsonFile(fullPath, data);
	}

	bool ShaderAsset::load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!loadJsonFile(fullPath, data))
		{
			return false;
		}

		const std::string shaderType = data.json_data["shader_type"];
		mShaderType = parseShaderTypeFromString(shaderType.c_str());

		const std::string shaderPath = data.json_data["shader_path"];
		mShaderPath = shaderPath;

		const std::string binaryPath = data.json_data["binary_path"];
		mBinaryPath = binaryPath;

		if (loadHeaderOnly)
			return true;

		loadCodeFromBinary();

		mIsLoaded = true;
		return true;
	}

	void ShaderAsset::unload()
	{
		mCode.clear();
		mCode.shrink_to_fit();

		mIsLoaded = false;
	}

	void ShaderAsset::loadCodeFromBinary()
	{
		std::ifstream file(mBinaryPath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			return;
		}

		// find what the size of the file is by looking up the location of the cursor
		// because the cursor is at the end, it gives the size directly in bytes
		const size_t fileSize = file.tellg();

		mCode.resize(fileSize / sizeof(uint32_t));

		// put file cursor at beginning
		file.seekg(0);

		// load entire file into buffer
		file.read(reinterpret_cast<char*>(mCode.data()), fileSize);

		file.close();
	}
}
