#include "puffin/assets/asset.h"

namespace puffin::assets
{
	CompressionMode ParseCompressionModeFromString(const char* string)
	{
		if (strcmp(string, "LZ4") == 0)
		{
			return CompressionMode::LZ4;
		}

		return CompressionMode::Uncompressed;
	}

	const char* ParseCompressionStringFromMode(CompressionMode mode)
	{
		return gCompressionModeToString.at(mode);
	}

	bool SaveBinaryFile(const fs::path& path, const AssetData& assetData)
	{
		if (!fs::exists(path.parent_path()))
		{
			fs::create_directories(path.parent_path());
		}

		// Open File for Writing
		std::ofstream outFile(path.c_str(), std::ios::binary | std::ios::out);

		if (!outFile.is_open())
			return false;

		// Write asset id
		outFile.write(reinterpret_cast<const char*>(&assetData.id), sizeof(uint_least64_t));

		// Write Asset Type
		outFile.write(reinterpret_cast<const char*>(&assetData.type), sizeof(uint32_t));

		// Write Asset Version
		outFile.write(reinterpret_cast<const char*>(&assetData.version), sizeof(uint32_t));

		// Write Json Length
		const std::string jsonString = assetData.jsonData.dump();
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

	bool LoadBinaryFile(const fs::path& path, AssetData& assetData, const bool& loadHeaderOnly)
	{
		// Open File for Loading
		std::ifstream inFile;
		inFile.open(path.c_str(), std::ios::binary);

		// Return false if file does not exist
		if (!inFile.is_open())
			return false;

		// Start reading from beginning
		inFile.seekg(0);

		// Read asset id
		inFile.read(reinterpret_cast<char*>(&assetData.id), sizeof(uint_least64_t));

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

		assetData.jsonData = json::parse(jsonString);

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

	bool SaveJsonFile(const fs::path& path, const AssetData& assetData)
	{
		std::ofstream os(path.string());

		json data;

		data["id"] = assetData.id;
		data["type"] = assetData.type;
		data["version"] = assetData.version;
		data["data"] = assetData.jsonData;

		os << std::setw(4) << data << std::endl;

		os.close();

		return true;
	}

	bool LoadJsonFile(const fs::path& path, AssetData& assetData)
	{
		if (!exists(path))
			return false;

		std::ifstream is(path.string());

		json data;
		is >> data;

		assetData.id = data["id"];
		assetData.type = data["type"];
		assetData.version = data["version"];
		assetData.jsonData = data["data"];

		is.close();

		return true;
	}

	Asset::Asset(const fs::path& path): mID(puffin::generate_id()), mPath(path)
	{
	}

	Asset::Asset(const puffin::PuffinID uuid, const fs::path& path): mID(uuid), mPath(path)
	{
	}

	PuffinID Asset::GetID() const
	{
		return mID;
	}

	fs::path Asset::GetRelativePath()
	{
		return mPath;
	}

	void Asset::SetRelativePath(const fs::path& path)
	{
		mPath = path;
	}

	bool Asset::IsLoaded()
	{
		return mIsLoaded;
	}
}
