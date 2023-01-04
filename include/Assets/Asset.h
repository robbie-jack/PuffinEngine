#pragma once

#include "Types/UUID.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace Puffin::Assets
{
	struct AssetData
	{
		AssetData()
		{
			type = "";
			version = 0;
			json = "";
			binaryBlob.clear();
		}

		~AssetData() {}

		std::string_view type;
		uint32_t version;
		std::string json;
		std::vector<char> binaryBlob;
	};

	enum class CompressionMode : uint8_t
	{
		None,
		LZ4
	};

	static CompressionMode ParseCompressionMode(const char* f)
	{
		if (strcmp(f, "LZ4") == 0)
		{
			return CompressionMode::LZ4;
		}
		else
		{
			return CompressionMode::None;
		}
	}

	static bool SaveBinaryFile(const fs::path& path, const AssetData& data)
	{
		// Open File for Writing
		std::ofstream outFile(path.c_str(), std::ios::binary | std::ios::out);

		// Write Asset Type & Length
		const uint32_t typeLength = data.type.size();
		outFile.write(reinterpret_cast<const char*>(&typeLength), sizeof(uint32_t));
		outFile.write(reinterpret_cast<const char*>(&data.type), typeLength);

		// Write Asset Version
		outFile.write(reinterpret_cast<const char*>(&data.version), sizeof(uint32_t));

		// Write Json Length
		const uint32_t jsonLength = data.json.size();
		outFile.write(reinterpret_cast<const char*>(&jsonLength), sizeof(uint32_t));

		// Write Binary Blob Length
		const uint32_t blobLength = data.binaryBlob.size();
		outFile.write(reinterpret_cast<const char*>(&blobLength), sizeof(uint32_t));

		// Write Json
		outFile.write(data.json.data(), jsonLength);

		// Write Binary Blob
		outFile.write(data.binaryBlob.data(), blobLength);

		// Close File
		outFile.close();

		return true;
	}

	static bool LoadBinaryFile(const fs::path& path, AssetData& data)
	{
		// Open File for Loading
		std::ifstream inFile;
		inFile.open(path.c_str(), std::ios::binary);

		// Return false if file does not exist
		if (!inFile.is_open())
			return false;

		// Start reading from beginning
		inFile.seekg(0);

		// Read Asset Type & Length
		uint32_t typeLength;
		inFile.read(reinterpret_cast<char*>(&typeLength), sizeof(uint32_t));
		inFile.read(reinterpret_cast<char*>(&data.type), typeLength);

		// Read Asset Version
		inFile.read(reinterpret_cast<char*>(&data.version), sizeof(uint32_t));

		// Read Json Length
		uint32_t jsonLength;
		inFile.read(reinterpret_cast<char*>(&jsonLength), sizeof(uint32_t));

		// Read Binary Blob Length
		uint32_t blobLength;
		inFile.read(reinterpret_cast<char*>(&blobLength), sizeof(uint32_t));

		// Read Json
		data.json.resize(jsonLength);
		inFile.read(data.json.data(), jsonLength);

		// Read Binary Blob
		data.binaryBlob.resize(blobLength);
		inFile.read(data.binaryBlob.data(), blobLength);

		// Close File
		inFile.close();

		return true;
	}

	class Asset
	{
	public:

		Asset(const fs::path& path) : m_path(path) {}
		Asset(const UUID uuid, const fs::path& path) : m_id(uuid), m_path(path) {}

		UUID ID()
		{
			return m_id;
		}

		fs::path RelativePath()
		{
			return m_path;
		}

		void RelativePath(fs::path path)
		{
			m_path = path;
		}

		virtual const std::string& Type() const = 0;
		virtual const uint32_t& Version() const = 0;

	protected:

		bool m_isLoaded = false; // Is Asset Currently Loaded

	private:

		UUID m_id; // UUID of Asset
		fs::path m_path; // Relative Asset Path

	};
}