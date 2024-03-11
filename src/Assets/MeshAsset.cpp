#include "Assets/MeshAsset.h"
#include "Types/Vertex.h"
#include "lz4.h"
#include "lz4hc.h"

using json = nlohmann::json;

namespace puffin::assets
{
	////////////////////////////////
	// StaticMeshAsset
	////////////////////////////////

	// Public

	bool StaticMeshAsset::save()
	{
		if (mIsLoaded)
		{
			return save(mMeshAssetInfo, mSubMeshInfo, mVertices.data(), mIndices.data());
		}

		return false;
	}

	bool StaticMeshAsset::save(MeshAssetInfo& meshAssetInfo, std::vector<SubMeshInfo>& subMeshInfo, const void* vertexData, const void* indexData)
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData assetData;
		assetData.type = AssetType::StaticMesh;
		assetData.version = gStaticMeshVersion;

		size_t vertexByteSizeTotal = 0;
		size_t indexByteSizeTotal = 0;
		for (auto& info : subMeshInfo)
		{
			vertexByteSizeTotal += info.vertexByteSize;
			indexByteSizeTotal += info.indexByteSize;
		}

		// Copy Vertices/Indices to binary blob
		const size_t byteSizeTotal = vertexByteSizeTotal + indexByteSizeTotal;

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(byteSizeTotal);

		// Copy Vertex Buffer
		memcpy(mergedBuffer.data(), vertexData, vertexByteSizeTotal);

		// Copy Index Buffer
		memcpy(mergedBuffer.data() + vertexByteSizeTotal, indexData, indexByteSizeTotal);

		// Compress Data and store in binary blob
		const size_t compressStaging = LZ4_compressBound(static_cast<int>(byteSizeTotal));

		assetData.binaryBlob.resize(compressStaging);

		// Compress using default LZ4 mode
		//int compressedSize = LZ4_compress_default(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging));

		// Compress using HC LZ4 mode (higher compression ratio, takes longer to compress, doesn't effect decompression time)
		int compressedSize = LZ4_compress_HC(mergedBuffer.data(), assetData.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging), LZ4HC_CLEVEL_DEFAULT);

		// If compression rate is more than 80% of original, it's not worth compressing the image
		if (const double compressionRate = static_cast<float>(compressedSize) / static_cast<float>(byteSizeTotal); compressionRate > 0.8 || compressedSize == 0)
		{
			compressedSize = byteSizeTotal;
			assetData.binaryBlob.resize(compressedSize);

			memcpy(assetData.binaryBlob.data(), mergedBuffer.data(), compressedSize);

			meshAssetInfo.compressionMode = CompressionMode::None;
		}
		else
		{
			assetData.binaryBlob.resize(compressedSize);
		}

		// Write json data
		std::vector<json> meshInfoJson;
		meshInfoJson.resize(subMeshInfo.size());

		for (int i = 0; i < subMeshInfo.size(); i++)
		{
			auto& infoJson = meshInfoJson[i];
			auto& info = subMeshInfo[i];

			infoJson["vertexOffset"] = info.vertexOffset;
			infoJson["indexOffset"] = info.indexOffset;
			infoJson["vertexCount"] = info.vertexCount;
			infoJson["indexCount"] = info.indexCount;
			infoJson["vertexByteSize"] = info.vertexByteSize;
			infoJson["indexByteSize"] = info.indexByteSize;
			infoJson["subMeshIdx"] = info.subMeshIdx;
		}

		assetData.json["originalFile"] = meshAssetInfo.originalFile;
		assetData.json["compressionMode"] = meshAssetInfo.compressionMode;
		assetData.json["vertexFormat"] = meshAssetInfo.vertexFormat;
		assetData.json["vertexCountTotal"] = meshAssetInfo.vertexCountTotal;
		assetData.json["indexCountTotal"] = meshAssetInfo.indexCountTotal;
		assetData.json["vertexByteSizeTotal"] = meshAssetInfo.vertexByteSizeTotal;
		assetData.json["indexByteSizeTotal"] = meshAssetInfo.indexByteSizeTotal;
		assetData.json["subMeshCount"] = meshAssetInfo.subMeshCount;
		assetData.json["subMeshInfo"] = meshInfoJson;

		// Save Asset Data out to Binary File
		return saveBinaryFile(fullPath, assetData);
	}

	bool StaticMeshAsset::load(bool loadHeaderOnly)
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
		if (!loadBinaryFile(fullPath, data, loadHeaderOnly))
		{
			return false;
		}

		// Parse Metadata from Json
		parseMeshInfo(data, mMeshAssetInfo, mSubMeshInfo);

		if (loadHeaderOnly)
			return true;

		size_t vertexByteSizeTotal = mMeshAssetInfo.vertexByteSizeTotal;
		size_t indexByteSizeTotal = mMeshAssetInfo.indexByteSizeTotal;

		// Decompress Binary Data
		const uint64_t totalSize = vertexByteSizeTotal + indexByteSizeTotal;

		std::vector<char> decompressedBuffer;
		decompressedBuffer.resize(totalSize);

		if (mMeshAssetInfo.compressionMode == CompressionMode::LZ4)
		{
			LZ4_decompress_safe(data.binaryBlob.data(), decompressedBuffer.data(),
				static_cast<int>(data.binaryBlob.size()), static_cast<int>(decompressedBuffer.size()));
		}
		else
		{
			std::copy_n(data.binaryBlob.data(), static_cast<int>(totalSize), decompressedBuffer.data());
		}

		// Copy Vertex Buffer
		mVertices.resize(vertexByteSizeTotal);
		memcpy(mVertices.data(), decompressedBuffer.data(), vertexByteSizeTotal);

		// Copy Index Buffer
		mIndices.resize(indexByteSizeTotal);
		memcpy(mIndices.data(), decompressedBuffer.data() + vertexByteSizeTotal, indexByteSizeTotal);

		mIsLoaded = true;
		return true;
	}

	void StaticMeshAsset::unload()
	{
		mVertices.clear();
		mVertices.shrink_to_fit();

		mIndices.clear();
		mIndices.shrink_to_fit();

		mIsLoaded = false;
	}

	// Private

	void StaticMeshAsset::parseMeshInfo(const AssetData& data, MeshAssetInfo& outMeshAssetInfo, std::vector<SubMeshInfo>& outSubMeshInfo)
	{
		// Fill Asset Info & Mesh Info structs with metadata

		outMeshAssetInfo.originalFile = data.json["originalFile"];
		outMeshAssetInfo.compressionMode = data.json["compressionMode"];
		outMeshAssetInfo.vertexFormat = data.json["vertexFormat"];
		outMeshAssetInfo.vertexCountTotal = data.json["vertexCountTotal"];
		outMeshAssetInfo.indexCountTotal = data.json["indexCountTotal"];
		outMeshAssetInfo.vertexByteSizeTotal = data.json["vertexByteSizeTotal"];
		outMeshAssetInfo.indexByteSizeTotal = data.json["indexByteSizeTotal"];
		outMeshAssetInfo.subMeshCount = data.json["subMeshCount"];

		for (auto& subMeshJson : data.json["subMeshInfo"])
		{
			SubMeshInfo subMeshInfo;

			subMeshInfo.vertexOffset = subMeshJson["vertexOffset"];
			subMeshInfo.indexOffset	= subMeshJson["indexOffset"];
			subMeshInfo.vertexCount	= subMeshJson["vertexCount"];
			subMeshInfo.indexCount = subMeshJson["indexCount"];
			subMeshInfo.vertexByteSize = subMeshJson["vertexByteSize"];
			subMeshInfo.indexByteSize = subMeshJson["indexByteSize"];
			subMeshInfo.subMeshIdx = subMeshJson["subMeshIdx"];

			outSubMeshInfo.push_back(subMeshInfo);
		}
	}
}