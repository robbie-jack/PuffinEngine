#include "Assets/MeshAsset.h"
#include "nlohmann/json.hpp"
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
			MeshInfo info;
			info.vertexFormat = mVertexFormat;
			info.numVertices = mNumVertices;
			info.numIndices = mNumIndices;
			info.verticesSize = mNumVertices * vertexSize();
			info.indicesSize = mNumIndices * indexSize();
			info.originalFile = mOriginalFile;

			return save(info, mVertices.data(), mIndices.data());
		}

		return false;
	}

	bool StaticMeshAsset::save(const MeshInfo& info, const void* vertexData, const void* indexData)
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::StaticMesh;
		data.version = gStaticMeshVersion;

		// Fill Metadata from Info struct
		json metadata;

		metadata["vertex_format"] = parseVertexStringFromFormat(info.vertexFormat);
		metadata["num_vertices"] = info.numVertices;
		metadata["num_indices"] = info.numIndices;
		metadata["vertex_buffer_size"] = info.verticesSize;
		metadata["index_buffer_size"] = info.indicesSize;
		metadata["compression"] = "LZ4";
		metadata["original_file"] = info.originalFile;

		// Pass metadata to asset data struct
		data.json = metadata.dump();

		// Copy Vertices/Indices to binary blob
		size_t fullSize = info.verticesSize + info.indicesSize;

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(fullSize);

		// Copy Vertex Buffer
		memcpy(mergedBuffer.data(), vertexData, info.verticesSize);

		// Copy Index Buffer
		memcpy(mergedBuffer.data() + info.verticesSize, indexData, info.indicesSize);

		// Compress Data and store in binary blob
		const size_t compressStaging = LZ4_compressBound(static_cast<int>(fullSize));

		data.binaryBlob.resize(compressStaging);

		// Compress using default LZ4 mode
		//const int compressedSize = LZ4_compress_default(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging));

		// Compress using HC LZ4 mode (higher compression ratio, takes longer to compress, doesn't effect decompression time)
		const int compressedSize = LZ4_compress_HC(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging), LZ4HC_CLEVEL_DEFAULT);

		data.binaryBlob.resize(compressedSize);

		// Save Asset Data out to Binary File
		return saveBinaryFile(fullPath, data);
	}

	bool StaticMeshAsset::load()
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
		if (!loadBinaryFile(fullPath, data))
		{
			return false;
		}

		// Parse Metadata from Json
		MeshInfo info = parseMeshInfo(data);

		mVertexFormat = info.vertexFormat;
		mNumVertices = info.numVertices;
		mNumIndices = info.numIndices;
		mOriginalFile = info.originalFile;

		// Decompress Binary Data
		std::vector<char> decompressedBuffer;
		decompressedBuffer.resize(info.verticesSize + info.indicesSize);

		LZ4_decompress_safe(data.binaryBlob.data(), decompressedBuffer.data(), 
		static_cast<int>(data.binaryBlob.size()), static_cast<int>(decompressedBuffer.size()));

		// Copy Vertex Buffer
		mVertices.resize(info.verticesSize);
		memcpy(mVertices.data(), decompressedBuffer.data(), info.verticesSize);

		// Copy Index Buffer
		mIndices.resize(info.indicesSize);
		memcpy(mIndices.data(), decompressedBuffer.data() + info.verticesSize, info.indicesSize);

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

	MeshInfo StaticMeshAsset::parseMeshInfo(const AssetData& data) const
	{
		// Parse metadata into json
		json metadata = json::parse(data.json);

		// Fill Mesh Info struct with metadata
		MeshInfo info;
		info.numVertices = metadata["num_vertices"];
		info.numIndices = metadata["num_indices"];
		info.verticesSize = metadata["vertex_buffer_size"];
		info.indicesSize = metadata["index_buffer_size"];
		info.originalFile = metadata["original_file"];

		const std::string vertexFormat = metadata["vertex_format"];
		info.vertexFormat = rendering::parseVertexFormatFromString(vertexFormat.c_str());

		const std::string compressionMode = metadata["compression"];
		info.compressionMode = parseCompressionMode(compressionMode.c_str());

		return info;
	}
}