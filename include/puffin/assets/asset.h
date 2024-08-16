#pragma once

#include "puffin/types/uuid.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin::assets
{
	// Enum for asset type. Be careful about changing values as they are stored within the asset files
    enum class AssetType : std::uint8_t
	{
        Invalid = 0,
        StaticMesh = 10,
        SkeletalMesh = 20,
        Texture = 30,
        Shader = 40,
        Material = 50,
        MaterialInstance = 55,
        Sound = 60,
	};

	static const std::unordered_map<AssetType, std::string> gAssetTypeToString =
	{
		{ AssetType::Invalid, "Invalid" },
		{ AssetType::StaticMesh, "StaticMesh" },
		{ AssetType::SkeletalMesh, "SkeletalMesh" },
		{ AssetType::Texture, "Texture" },
		{ AssetType::Shader, "Shader" },
		{ AssetType::Material, "Material" },
		{ AssetType::MaterialInstance, "MaterialInstance" },
		{ AssetType::Sound, "Sound" },
	};

    NLOHMANN_JSON_SERIALIZE_ENUM(puffin::assets::AssetType,
	{
        {puffin::assets::AssetType::Invalid, "None"},
        {puffin::assets::AssetType::StaticMesh, "StaticMesh"},
        {puffin::assets::AssetType::SkeletalMesh, "SkeletalMesh"},
        {puffin::assets::AssetType::Texture, "Texture"},
        {puffin::assets::AssetType::Shader, "Shader"},
        {puffin::assets::AssetType::Material, "Material"},
        {puffin::assets::AssetType::MaterialInstance, "MaterialInstance"},
        {puffin::assets::AssetType::Sound, "Sound"}
	});

	struct AssetData
	{
		AssetData()
		{
			id = puffin::gInvalidId;
			type = AssetType::Invalid;
			version = 0;
			binaryBlob.clear();
		}

		~AssetData()
		{
			id = puffin::gInvalidId;
			type = AssetType::Invalid;
			version = 0;
			jsonData.clear();
			binaryBlob.clear();
		}

		puffin::UUID id;
		AssetType type;
		uint32_t version;
		json jsonData;
		std::vector<char> binaryBlob;
	};

	enum class CompressionMode : uint8_t
	{
		Uncompressed = 0,
		LZ4
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(CompressionMode,
	{
		{ CompressionMode::Uncompressed, "Uncompressed" },
		{ CompressionMode::LZ4, "LZ4" }
	});

	struct AssetInfo
	{
		CompressionMode compressionMode;
		std::string originalFile;
	};

	const static std::unordered_map<CompressionMode, const char*> gCompressionModeToString =
	{
		{ CompressionMode::Uncompressed, "Uncompressed" },
		{ CompressionMode::LZ4, "LZ4" }
	};

	static CompressionMode ParseCompressionModeFromString(const char* string);

	static const char* ParseCompressionStringFromMode(CompressionMode mode);

	bool SaveBinaryFile(const fs::path& path, const AssetData& assetData);
	bool LoadBinaryFile(const fs::path& path, AssetData& assetData, const bool& loadHeaderOnly = false);

	bool SaveJsonFile(const fs::path& path, const AssetData& assetData);
	bool LoadJsonFile(const fs::path& path, AssetData& assetData);

	class Asset
	{
	public:

		Asset(const fs::path& path);
		Asset(const puffin::UUID uuid, const fs::path& path);

		virtual ~Asset() = default;

		[[nodiscard]] UUID GetID() const;
		[[nodiscard]] fs::path GetRelativePath();

		void SetRelativePath(const fs::path& path);

		[[nodiscard]] virtual const std::string& GetType() const = 0;
		[[nodiscard]] virtual const uint32_t& GetVersion() const = 0;
		virtual bool Save() = 0;
		virtual bool Load(bool loadHeaderOnly = false) = 0;
		virtual void Unload() = 0;

		virtual bool IsLoaded();

	protected:

		bool mIsLoaded = false; // Is Asset Currently Loaded

	private:

		puffin::UUID mID = puffin::gInvalidId; // UUID of Asset
		fs::path mPath; // Relative Asset Path

	};
}
