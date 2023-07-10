#include "Assets/ShaderAsset.h"

#include "Assets/AssetRegistry.h"

namespace puffin::assets
{
	bool ShaderAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = gShaderAssetType;
		data.version = gShaderAssetVersion;

		// Fill Metadata from Info struct
		json metadata;

		metadata["shader_type"] = parseShaderStringFromType(mShaderType);
		metadata["shader_path"] = mShaderPath;
		metadata["binary_path"] = mBinaryPath;

		data.json = metadata.dump();

		const size_t binarySize = mCode.size() * sizeof(uint32_t);

		// Copy code to binary blob
		data.binaryBlob.resize(binarySize);

		std::copy_n(mCode.data(), mCode.size(), data.binaryBlob.data());

		return saveBinaryFile(fullPath, data);
	}

	bool ShaderAsset::load()
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
		if (!loadBinaryFile(fullPath, data))
		{
			return false;
		}

		json metadata = json::parse(data.json);

		const std::string shaderType = metadata["shader_type"];
		mShaderType = parseShaderTypeFromString(shaderType.c_str());

		const std::string& shaderPath = metadata["shader_path"];
		mShaderPath = fs::path(shaderPath);

		const std::string& binaryPath = metadata["binary_path"];
		mBinaryPath = fs::path(binaryPath);

		const size_t codeSize = data.binaryBlob.size() / sizeof(uint32_t);

		mCode.resize(codeSize);

		std::copy_n(data.binaryBlob.data(), data.binaryBlob.size(), mCode.data());

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
