#pragma once

#include "Asset.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace puffin::assets
{
	static const std::string gShaderAssetType = "Shader";
	static constexpr uint32_t gShaderAssetVersion = 1; // Latest version of Shader Asset Format

	enum class ShaderType : uint8_t
	{
		Unknown = 0,
		Vertex,
		Fragment
	};

	static ShaderType parseShaderTypeFromString(const char* typeString)
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

	static const char* parseShaderStringFromType(const ShaderType type)
	{
		switch(type)
		{
		case ShaderType::Unknown: return "Unknown";
		case ShaderType::Vertex: return "Vertex";
		case ShaderType::Fragment: return "Fragment";
		}

		return "Unknown";
	}

	class ShaderAsset : public Asset
	{
	public:

		ShaderAsset() : Asset(fs::path()) {}

		explicit ShaderAsset(const fs::path& path) : Asset(path) {}

		ShaderAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

		~ShaderAsset() override = default;

		[[nodiscard]] const std::string& type() const override
		{
			return gShaderAssetType;
		}

		[[nodiscard]] const uint32_t& version() const override
		{
			return gShaderAssetVersion;
		}

		bool save();

		bool load();

		void unload() override;

		void loadCodeFromBinary();

		void setType(const ShaderType shaderType) { mShaderType = shaderType; }

		void setShaderPath(const fs::path& shaderPath) { mShaderPath = shaderPath; }

		const fs::path& binaryPath() { return mBinaryPath; }

		void setBinaryPath(const fs::path& binaryPath) { mBinaryPath = binaryPath; }

		[[nodiscard]] const std::vector<uint32_t>& code() const { return mCode; }

	private:

		ShaderType mShaderType = ShaderType::Unknown; // Shader Type
		fs::path mShaderPath; // Path to original HLSL/GLSL shader
		fs::path mBinaryPath; // Path that compiled shader binary was exported from
		std::vector<uint32_t> mCode; // Compile shader code

	};

}