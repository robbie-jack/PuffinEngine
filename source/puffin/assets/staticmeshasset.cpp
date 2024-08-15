#include "puffin/assets/mesh_asset.h"

#include "lz4.h"
#include "lz4hc.h"
#include "puffin/assets/asset_registry.h"

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
			return save(m_mesh_asset_info, m_vertices.data(), m_indices.data());
		}

		return false;
	}

	bool StaticMeshAsset::save(MeshAssetInfo& mesh_asset_info, const void* vertex_data, const void* index_data)
	{
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();

		// Create AssetData Struct
		AssetData assetData;
		assetData.id = id();
		assetData.type = AssetType::StaticMesh;
		assetData.version = gStaticMeshVersion;

		// Copy Vertices/Indices to binary blob
		const size_t byteSizeTotal = mesh_asset_info.vertex_byte_size_total + mesh_asset_info.index_byte_size_total;

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(byteSizeTotal);

		// Copy Vertex Buffer
		memcpy(mergedBuffer.data(), vertex_data, mesh_asset_info.vertex_byte_size_total);

		// Copy Index Buffer
		memcpy(mergedBuffer.data() + mesh_asset_info.vertex_byte_size_total, index_data, mesh_asset_info.index_byte_size_total);

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

			mesh_asset_info.compressionMode = CompressionMode::Uncompressed;
		}
		else
		{
			assetData.binaryBlob.resize(compressedSize);
		}

		// Write json data
		std::vector<json> meshInfoJson;
		meshInfoJson.resize(mesh_asset_info.sub_mesh_infos.size());

		for (int i = 0; i < mesh_asset_info.sub_mesh_infos.size(); i++)
		{
			auto& infoJson = meshInfoJson[i];
			auto& info = mesh_asset_info.sub_mesh_infos[i];

			infoJson["vertexOffset"] = info.vertex_offset;
			infoJson["indexOffset"] = info.index_offset;
			infoJson["vertexCount"] = info.vertex_count;
			infoJson["indexCount"] = info.index_count;
			infoJson["vertexByteSize"] = info.vertex_byte_size_total;
			infoJson["indexByteSize"] = info.index_byte_size_total;
			infoJson["subMeshIdx"] = info.sub_mesh_idx;
		}

		assetData.json_data["originalFile"] = mesh_asset_info.originalFile;
		assetData.json_data["compressionMode"] = mesh_asset_info.compressionMode;
		assetData.json_data["vertexFormat"] = mesh_asset_info.vertex_format;
		assetData.json_data["vertexCountTotal"] = mesh_asset_info.vertex_count_total;
		assetData.json_data["indexCountTotal"] = mesh_asset_info.index_count_total;
		assetData.json_data["vertexByteSizeTotal"] = mesh_asset_info.vertex_byte_size_total;
		assetData.json_data["indexByteSizeTotal"] = mesh_asset_info.index_byte_size_total;
		assetData.json_data["subMeshInfo"] = meshInfoJson;

		// Save Asset Data out to Binary File
		return saveBinaryFile(fullPath, assetData);
	}

	bool StaticMeshAsset::load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!loadBinaryFile(fullPath, data, loadHeaderOnly))
		{
			return false;
		}

		// Parse Metadata from Json
		parse_mesh_info(data, m_mesh_asset_info);

		if (loadHeaderOnly)
			return true;

		size_t vertexByteSizeTotal = m_mesh_asset_info.vertex_byte_size_total;
		size_t indexByteSizeTotal = m_mesh_asset_info.index_byte_size_total;

		// Decompress Binary Data
		const uint64_t totalSize = vertexByteSizeTotal + indexByteSizeTotal;

		std::vector<char> decompressedBuffer;
		decompressedBuffer.resize(totalSize);

		if (m_mesh_asset_info.compressionMode == CompressionMode::LZ4)
		{
			LZ4_decompress_safe(data.binaryBlob.data(), decompressedBuffer.data(),
				static_cast<int>(data.binaryBlob.size()), static_cast<int>(decompressedBuffer.size()));
		}
		else
		{
			std::copy_n(data.binaryBlob.data(), static_cast<int>(totalSize), decompressedBuffer.data());
		}

		// Copy Vertex Buffer
		m_vertices.resize(vertexByteSizeTotal);
		memcpy(m_vertices.data(), decompressedBuffer.data(), vertexByteSizeTotal);

		// Copy Index Buffer
		m_indices.resize(indexByteSizeTotal);
		memcpy(m_indices.data(), decompressedBuffer.data() + vertexByteSizeTotal, indexByteSizeTotal);

		mIsLoaded = true;
		return true;
	}

	void StaticMeshAsset::unload()
	{
		m_vertices.clear();
		m_vertices.shrink_to_fit();

		m_indices.clear();
		m_indices.shrink_to_fit();

		mIsLoaded = false;
	}

	// Private

	void StaticMeshAsset::parse_mesh_info(const AssetData& data, MeshAssetInfo& out_mesh_asset_info)
	{
		// Fill Asset Info & Mesh Info structs with metadata

		out_mesh_asset_info.originalFile = data.json_data["originalFile"];
		out_mesh_asset_info.compressionMode = data.json_data["compressionMode"];
		out_mesh_asset_info.vertex_format = data.json_data["vertexFormat"];
		out_mesh_asset_info.vertex_count_total = data.json_data["vertexCountTotal"];
		out_mesh_asset_info.index_count_total = data.json_data["indexCountTotal"];
		out_mesh_asset_info.vertex_byte_size_total = data.json_data["vertexByteSizeTotal"];
		out_mesh_asset_info.index_byte_size_total = data.json_data["indexByteSizeTotal"];

		out_mesh_asset_info.sub_mesh_infos.resize(data.json_data["subMeshInfo"].size());

		for (int i = 0; i < data.json_data["subMeshInfo"].size(); ++i)
		{
			auto& sub_mesh_json = data.json_data["subMeshInfo"][i];
			auto& sub_mesh_info = out_mesh_asset_info.sub_mesh_infos[i];

			sub_mesh_info.vertex_offset = sub_mesh_json["vertexOffset"];
			sub_mesh_info.index_offset	= sub_mesh_json["indexOffset"];
			sub_mesh_info.vertex_count	= sub_mesh_json["vertexCount"];
			sub_mesh_info.index_count = sub_mesh_json["indexCount"];
			sub_mesh_info.vertex_byte_size_total = sub_mesh_json["vertexByteSize"];
			sub_mesh_info.index_byte_size_total = sub_mesh_json["indexByteSize"];
			sub_mesh_info.sub_mesh_idx = sub_mesh_json["subMeshIdx"];
		}
	}
}
