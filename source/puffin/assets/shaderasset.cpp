#include "puffin/assets/shaderasset.h"

#include "puffin/assets/assetregistry.h"

namespace puffin::assets
{
	ShaderType ParseShaderTypeFromString(const char* typeString)
	{
		if (strcmp(typeString, "Vertex") == 0)
		{
			return ShaderType::Vertex;
		}

		if (strcmp(typeString, "Fragment") == 0)
		{
			return ShaderType::Fragment;
		}

		return ShaderType::Unknown;
	}

	const char* ParseShaderStringFromType(const ShaderType type)
	{
		switch(type)
		{
		case ShaderType::Unknown: return "Unknown";
		case ShaderType::Vertex: return "Vertex";
		case ShaderType::Fragment: return "Fragment";
		}

		return "Unknown";
	}

	ShaderAsset::ShaderAsset(): Asset(fs::path())
	{
	}

	ShaderAsset::ShaderAsset(const fs::path& path): Asset(path)
	{
	}

	ShaderAsset::ShaderAsset(const UUID id, const fs::path& path): Asset(id, path)
	{
	}

	const std::string& ShaderAsset::GetType() const
	{
		return gShaderAssetType;
	}

	const uint32_t& ShaderAsset::GetVersion() const
	{
		return gShaderAssetVersion;
	}

	bool ShaderAsset::Save()
	{
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.id = GetID();
		data.type = AssetType::Shader;
		data.version = gShaderAssetVersion;

		// Fill Metadata from Info struct
		data.jsonData["shader_type"] = ParseShaderStringFromType(mShaderType);
		data.jsonData["shader_path"] = mShaderPath;
		data.jsonData["binary_path"] = mBinaryPath;

		return SaveJsonFile(fullPath, data);
	}

	bool ShaderAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadJsonFile(fullPath, data))
		{
			return false;
		}

		const std::string shaderType = data.jsonData["shader_type"];
		mShaderType = ParseShaderTypeFromString(shaderType.c_str());

		const std::string shaderPath = data.jsonData["shader_path"];
		mShaderPath = shaderPath;

		const std::string binaryPath = data.jsonData["binary_path"];
		mBinaryPath = binaryPath;

		if (loadHeaderOnly)
			return true;

		LoadCodeFromBinary();

		mIsLoaded = true;
		return true;
	}

	void ShaderAsset::Unload()
	{
		mCode.clear();
		mCode.shrink_to_fit();

		mIsLoaded = false;
	}

	void ShaderAsset::LoadCodeFromBinary()
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

	void ShaderAsset::SetType(const ShaderType shaderType)
	{
		mShaderType = shaderType;
	}

	void ShaderAsset::SetShaderPath(const fs::path& shaderPath)
	{
		mShaderPath = shaderPath;
	}

	const fs::path& ShaderAsset::GetBinaryPath()
	{
		return mBinaryPath;
	}

	void ShaderAsset::SetBinaryPath(const fs::path& binaryPath)
	{
		mBinaryPath = binaryPath;
	}
}
