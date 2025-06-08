#pragma once

#include "puffin/assets/asset.h"

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

	ShaderType ParseShaderTypeFromString(const char* typeString);
	const char* ParseShaderStringFromType(const ShaderType type);

	class ShaderAsset : public Asset
	{
	public:

		ShaderAsset();
		explicit ShaderAsset(const fs::path& path);
		ShaderAsset(const UUID id, const fs::path& path);

		~ShaderAsset() override = default;

		[[nodiscard]] const std::string& GetType() const override;
		[[nodiscard]] const uint32_t& GetVersion() const override;

		bool Save() override;

		bool Load(bool loadHeaderOnly = false) override;

		void Unload() override;

		void LoadCodeFromBinary();

		void SetType(const ShaderType shaderType);
		void SetShaderPath(const fs::path& shaderPath);

		const fs::path& GetBinaryPath();

		void SetBinaryPath(const fs::path& binaryPath);

		[[nodiscard]] const std::vector<uint32_t>& GetCode() const { return mCode; }

	private:

		ShaderType mShaderType = ShaderType::Unknown; // Shader Type
		fs::path mShaderPath; // Path to original HLSL/GLSL shader
		fs::path mBinaryPath; // Path that compiled shader binary was exported from
		std::vector<uint32_t> mCode; // Compile shader code

	};

}