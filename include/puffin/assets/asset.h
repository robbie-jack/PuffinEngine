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
			type = AssetType::Invalid;
			version = 0;
			id = puffin::gInvalidID;
			binaryBlob.clear();
		}

		~AssetData()
		{
			type = AssetType::Invalid;
			version = 0;
			id = puffin::gInvalidID;
			json_data.clear();
			binaryBlob.clear();
		}

		AssetType type;
		uint32_t version;
		puffin::PuffinID id;
		json json_data;
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

	static CompressionMode parseCompressionMode(const char* f)
	{
		if (strcmp(f, "LZ4") == 0)
		{
			return CompressionMode::LZ4;
		}

		return CompressionMode::Uncompressed;
	}

	static const char* parseCompressionStringFromMode(CompressionMode mode)
	{
		return gCompressionModeToString.at(mode);
	}

	static bool saveBinaryFile(const fs::path& path, const AssetData& assetData)
	{
		if (!fs::exists(path.parent_path()))
		{
			fs::create_directories(path.parent_path());
		}

		// Open File for Writing
		std::ofstream outFile(path.c_str(), std::ios::binary | std::ios::out);

		if (!outFile.is_open())
			return false;

		// Write Asset Type
		outFile.write(reinterpret_cast<const char*>(&assetData.type), sizeof(uint32_t));

		// Write Asset Version
		outFile.write(reinterpret_cast<const char*>(&assetData.version), sizeof(uint32_t));

		// Write Json Length
		const std::string jsonString = assetData.json_data.dump();
		const uint32_t jsonLength = jsonString.size();
		outFile.write(reinterpret_cast<const char*>(&jsonLength), sizeof(uint32_t));

		// Write Binary Blob Length
		const uint32_t blobLength = assetData.binaryBlob.size();
		outFile.write(reinterpret_cast<const char*>(&blobLength), sizeof(uint32_t));

		// Write Json
		outFile.write(jsonString.data(), jsonLength);

		// Write Binary Blob
		outFile.write(assetData.binaryBlob.data(), blobLength);

		// Close File
		outFile.close();

		return true;
	}

	static bool loadBinaryFile(const fs::path& path, AssetData& assetData, const bool& loadHeaderOnly = false)
	{
		// Open File for Loading
		std::ifstream inFile;
		inFile.open(path.c_str(), std::ios::binary);

		// Return false if file does not exist
		if (!inFile.is_open())
			return false;

		// Start reading from beginning
		inFile.seekg(0);

		// Read Asset Type
		inFile.read(reinterpret_cast<char*>(&assetData.type), sizeof(uint32_t));

		// Read Asset Version
		inFile.read(reinterpret_cast<char*>(&assetData.version), sizeof(uint32_t));

		// Read Json Length
		uint32_t jsonLength;
		inFile.read(reinterpret_cast<char*>(&jsonLength), sizeof(uint32_t));

		// Read Binary Blob Length
		uint32_t blobLength;
		inFile.read(reinterpret_cast<char*>(&blobLength), sizeof(uint32_t));

		// Read Json
		std::string jsonString;
		jsonString.resize(jsonLength);
		inFile.read(jsonString.data(), jsonLength);

		assetData.json_data = json::parse(jsonString);

		// Load only header data, skip binary blob
		if (loadHeaderOnly)
		{
			inFile.close();

			return true;
		}

		// Read Binary Blob
		assetData.binaryBlob.resize(blobLength);
		inFile.read(assetData.binaryBlob.data(), blobLength);

		// Close File
		inFile.close();

		return true;
	}

	inline bool saveJsonFile(const fs::path& path, const AssetData& assetData)
	{
		std::ofstream os(path.string());

		json data;

		data["type"] = assetData.type;
		data["version"] = assetData.version;
		data["id"] = assetData.id;
		data["data"] = assetData.json_data;

		os << std::setw(4) << data << std::endl;

		os.close();

		return true;
	}

	inline bool loadJsonFile(const fs::path& path, AssetData& assetData)
	{
		if (!exists(path))
			return false;

		std::ifstream is(path.string());

		json data;
		is >> data;

		assetData.type = data["type"];
		assetData.version = data["version"];
		assetData.id = data["id"];
		assetData.json_data = data["data"];

		is.close();

		return true;
	}

	class Asset
	{
	public:

		Asset(const fs::path& path) : mId(puffin::generate_id()), mPath(path) {}
		Asset(const puffin::PuffinID uuid, const fs::path& path) : mId(uuid), mPath(path) {}

		virtual ~Asset() = default;

		puffin::PuffinID id() const
		{
			return mId;
		}

		fs::path relativePath()
		{
			return mPath;
		}

		void setRelativePath(const fs::path& path)
		{
			mPath = path;
		}

		virtual const std::string& type() const = 0;
		virtual const uint32_t& version() const = 0;
		virtual bool save() = 0;
		virtual bool load(bool loadHeaderOnly = false) = 0;
		virtual void unload() = 0;

		virtual bool isLoaded() { return mIsLoaded; }

	protected:

		bool mIsLoaded = false; // Is Asset Currently Loaded

	private:

		puffin::PuffinID mId = puffin::gInvalidID; // UUID of Asset
		fs::path mPath; // Relative Asset Path

	};
}
