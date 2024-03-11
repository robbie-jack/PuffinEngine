#pragma once

#include "Types/UUID.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin::assets
{
	// Enum for asset type. Be careful about changing values as they are stored within the asset files
	enum class AssetType : uint32_t
	{
		None = 0,
		StaticMesh = 10,
		SkeletalMesh = 20,
		Texture = 30,
		Shader = 40,
		Material = 50,
		MaterialInstance = 55,
		Sound = 60,
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(AssetType,
	{
		{AssetType::None, "None"},
		{AssetType::StaticMesh, "StaticMesh"},
		{AssetType::SkeletalMesh, "SkeletalMesh"},
		{AssetType::Texture, "Texture"},
		{AssetType::Shader, "Shader"},
		{AssetType::Material, "Material"},
		{AssetType::MaterialInstance, "MaterialInstance"},
		{AssetType::Sound, "Sound"}
	});

	struct AssetData
	{
		AssetData()
		{
			type = AssetType::None;
			version = 0;
			id = gInvalidID;
			binaryBlob.clear();
		}

		~AssetData()
		{
			type = AssetType::None;
			version = 0;
			id = gInvalidID;
			json.clear();
			binaryBlob.clear();
		}

		AssetType type;
		uint32_t version;
		PuffinID id;
		json json;
		std::vector<char> binaryBlob;
	};

	enum class CompressionMode : uint8_t
	{
		None = 0,
		LZ4
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(CompressionMode,
	{
		{ CompressionMode::None, "None" },
		{ CompressionMode::LZ4, "LZ4" }
	});

	struct AssetInfo
	{
		CompressionMode compressionMode;
		std::string originalFile;
	};

	const static std::unordered_map<CompressionMode, const char*> gCompressionModeToString =
	{
		{ CompressionMode::None, "None" },
		{ CompressionMode::LZ4, "LZ4" }
	};

	static CompressionMode parseCompressionMode(const char* f)
	{
		if (strcmp(f, "LZ4") == 0)
		{
			return CompressionMode::LZ4;
		}

		return CompressionMode::None;
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
		const std::string jsonString = assetData.json.dump();
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

		assetData.json = json::parse(jsonString);

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
		data["data"] = assetData.json;

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
		assetData.json = data["data"];

		is.close();

		return true;
	}

	class Asset
	{
	public:

		Asset(const fs::path& path) : mId(generateID()), mPath(path) {}
		Asset(const PuffinID uuid, const fs::path& path) : mId(uuid), mPath(path) {}

		virtual ~Asset() = default;

		PuffinID id() const
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

		PuffinID mId = gInvalidID; // UUID of Asset
		fs::path mPath; // Relative Asset Path

	};
}