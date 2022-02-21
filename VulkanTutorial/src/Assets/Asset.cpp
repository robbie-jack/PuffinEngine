#include "Asset.h"

#include <fstream>

namespace Puffin::Assets
{
	bool SaveBinaryFile(const char* path, const AssetData& data)
	{
		// Open File for Writing
		std::ofstream outFile(path, std::ios::binary | std::ios::out);

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

	bool LoadBinaryFile(const char* path, AssetData& data)
	{
		// Open File for Loading
		std::ifstream inFile;
		inFile.open(path, std::ios::binary);

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

}