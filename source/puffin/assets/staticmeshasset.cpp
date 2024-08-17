#include "puffin/assets/staticmeshasset.h"

#include "lz4.h"
#include "lz4hc.h"
#include "puffin/assets/assetregistry.h"

using json = nlohmann::json;

namespace puffin::assets
{
	////////////////////////////////
	// StaticMeshAsset
	////////////////////////////////

	// Public

	StaticMeshAsset::StaticMeshAsset() : Asset(fs::path())
	{
	}

	StaticMeshAsset::StaticMeshAsset(const fs::path& path): Asset(path)
	{
	}

	StaticMeshAsset::StaticMeshAsset(const UUID id, const fs::path& path): Asset(id, path)
	{
	}

	const std::string& StaticMeshAsset::GetType() const
	{
		return gStaticMeshTypeString;
	}

	const uint32_t& StaticMeshAsset::GetVersion() const
	{
		return gStaticMeshVersion;
	}

	bool StaticMeshAsset::Save()
	{
		if (mIsLoaded)
		{
			return Save(mMeshAssetInfo, mVertices.data(), mIndices.data());
		}

		return false;
	}

	bool StaticMeshAsset::Save(MeshAssetInfo& meshAssetInfo, const void* vertexData, const void* indexData)
	{
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();

		// Create AssetData Struct
		AssetData assetData;
		assetData.id = GetID();
		assetData.type = AssetType::StaticMesh;
		assetData.version = gStaticMeshVersion;

		// Copy Vertices/Indices to binary blob
		const size_t byteSizeTotal = meshAssetInfo.vertexByteSizeTotal + meshAssetInfo.indexByteSizeTotal;

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(byteSizeTotal);

		// Copy Vertex Buffer
		memcpy(mergedBuffer.data(), vertexData, meshAssetInfo.vertexByteSizeTotal);

		// Copy Index Buffer
		memcpy(mergedBuffer.data() + meshAssetInfo.vertexByteSizeTotal, indexData, meshAssetInfo.indexByteSizeTotal);

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

			meshAssetInfo.compressionMode = CompressionMode::Uncompressed;
		}
		else
		{
			assetData.binaryBlob.resize(compressedSize);
		}

		// Write json data
		std::vector<json> meshInfoJson;
		meshInfoJson.resize(meshAssetInfo.subMeshInfos.size());

		for (int i = 0; i < meshAssetInfo.subMeshInfos.size(); i++)
		{
			auto& infoJson = meshInfoJson[i];
			auto& info = meshAssetInfo.subMeshInfos[i];

			infoJson["vertexOffset"] = info.vertexOffset;
			infoJson["indexOffset"] = info.indexOffset;
			infoJson["vertexCount"] = info.vertexCount;
			infoJson["indexCount"] = info.indexCount;
			infoJson["vertexByteSize"] = info.vertexByteSizeTotal;
			infoJson["indexByteSize"] = info.indexByteSizeTotal;
			infoJson["subMeshIdx"] = info.subMeshIdx;
		}

		assetData.jsonData["originalFile"] = meshAssetInfo.originalFile;
		assetData.jsonData["compressionMode"] = meshAssetInfo.compressionMode;
		assetData.jsonData["vertexFormat"] = meshAssetInfo.vertexFormat;
		assetData.jsonData["vertexCountTotal"] = meshAssetInfo.vertexCountTotal;
		assetData.jsonData["indexCountTotal"] = meshAssetInfo.indexCountTotal;
		assetData.jsonData["vertexByteSizeTotal"] = meshAssetInfo.vertexByteSizeTotal;
		assetData.jsonData["indexByteSizeTotal"] = meshAssetInfo.indexByteSizeTotal;
		assetData.jsonData["subMeshInfo"] = meshInfoJson;

		// Save Asset Data out to Binary File
		return SaveBinaryFile(fullPath, assetData);
	}

	bool StaticMeshAsset::Load(bool loadHeaderOnly)
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
		if (!LoadBinaryFile(fullPath, data, loadHeaderOnly))
		{
			return false;
		}

		// Parse Metadata from Json
		ParseMeshInfo(data, mMeshAssetInfo);

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

	void StaticMeshAsset::Unload()
	{
		mVertices.clear();
		mVertices.shrink_to_fit();

		mIndices.clear();
		mIndices.shrink_to_fit();

		mIsLoaded = false;
	}

	std::vector<char>& StaticMeshAsset::Vertices()
	{
		return mVertices;
	}

	std::vector<uint32_t>& StaticMeshAsset::Indices()
	{
		return mIndices;
	}

	const std::vector<SubMeshInfo>& StaticMeshAsset::SubMeshInfo() const
	{
		return mMeshAssetInfo.subMeshInfos;
	}

	rendering::VertexFormat StaticMeshAsset::VertexFormat() const
	{
		return mMeshAssetInfo.vertexFormat;
	}

	uint64_t StaticMeshAsset::VertexByteSize() const
	{
		return parseVertexSizeFromFormat(mMeshAssetInfo.vertexFormat);
	}

	uint64_t StaticMeshAsset::IndexByteSize() const
	{
		return sizeof(uint32_t);
	}

	// Private

	void StaticMeshAsset::ParseMeshInfo(const AssetData& data, MeshAssetInfo& meshAssetInfo)
	{
		// Fill Asset Info & Mesh Info structs with metadata

		meshAssetInfo.originalFile = data.jsonData["originalFile"];
		meshAssetInfo.compressionMode = data.jsonData["compressionMode"];
		meshAssetInfo.vertexFormat = data.jsonData["vertexFormat"];
		meshAssetInfo.vertexCountTotal = data.jsonData["vertexCountTotal"];
		meshAssetInfo.indexCountTotal = data.jsonData["indexCountTotal"];
		meshAssetInfo.vertexByteSizeTotal = data.jsonData["vertexByteSizeTotal"];
		meshAssetInfo.indexByteSizeTotal = data.jsonData["indexByteSizeTotal"];

		meshAssetInfo.subMeshInfos.resize(data.jsonData["subMeshInfo"].size());

		for (int i = 0; i < data.jsonData["subMeshInfo"].size(); ++i)
		{
			auto& sub_mesh_json = data.jsonData["subMeshInfo"][i];
			auto& sub_mesh_info = meshAssetInfo.subMeshInfos[i];

			sub_mesh_info.vertexOffset = sub_mesh_json["vertexOffset"];
			sub_mesh_info.indexOffset	= sub_mesh_json["indexOffset"];
			sub_mesh_info.vertexCount	= sub_mesh_json["vertexCount"];
			sub_mesh_info.indexCount = sub_mesh_json["indexCount"];
			sub_mesh_info.vertexByteSizeTotal = sub_mesh_json["vertexByteSize"];
			sub_mesh_info.indexByteSizeTotal = sub_mesh_json["indexByteSize"];
			sub_mesh_info.subMeshIdx = sub_mesh_json["subMeshIdx"];
		}
	}
}
