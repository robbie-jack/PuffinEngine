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
			info.originalFile = mOriginalFile;
			info.compressionMode = mCompressionMode;
			info.vertexFormat = mVertexFormat;
			info.numVertices = mNumVertices;
			info.numIndices = mNumIndices;
			info.verticesSize = mNumVertices * vertexSize();
			info.indicesSize = mNumIndices * indexSize();

			return save(info, mVertices.data(), mIndices.data());
		}

		return false;
	}

	bool StaticMeshAsset::save(MeshInfo& info, const void* vertexData, const void* indexData)
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::StaticMesh;
		data.version = gStaticMeshVersion;

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
		//int compressedSize = LZ4_compress_default(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging));

		// Compress using HC LZ4 mode (higher compression ratio, takes longer to compress, doesn't effect decompression time)
		int compressedSize = LZ4_compress_HC(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging), LZ4HC_CLEVEL_DEFAULT);

		// If compression rate is more than 80% of original, it's not worth compressing the image
		if (const double compressionRate = static_cast<float>(compressedSize) / static_cast<float>(fullSize); compressionRate > 0.8 || compressedSize == 0)
		{
			compressedSize = fullSize;
			data.binaryBlob.resize(compressedSize);

			memcpy(data.binaryBlob.data(), mergedBuffer.data(), compressedSize);

			info.compressionMode = CompressionMode::None;
		}
		else
		{
			data.binaryBlob.resize(compressedSize);
		}

		// Fill Metadata from Info struct
		json metadata;

		metadata["vertex_format"] = parseVertexStringFromFormat(info.vertexFormat);
		metadata["compression"] = parseCompressionStringFromMode(info.compressionMode);
		metadata["num_vertices"] = info.numVertices;
		metadata["num_indices"] = info.numIndices;
		metadata["vertex_buffer_size"] = info.verticesSize;
		metadata["index_buffer_size"] = info.indicesSize;
		metadata["original_file"] = info.originalFile;

		// Pass metadata to asset data struct
		data.json = metadata.dump();

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

		mOriginalFile = info.originalFile;
		mCompressionMode = info.compressionMode;
		mVertexFormat = info.vertexFormat;
		mNumVertices = info.numVertices;
		mNumIndices = info.numIndices;

		// Decompress Binary Data
		uint64_t totalSize = info.verticesSize + info.indicesSize;

		std::vector<char> decompressedBuffer;
		decompressedBuffer.resize(totalSize);

		if (info.compressionMode == CompressionMode::LZ4)
		{
			LZ4_decompress_safe(data.binaryBlob.data(), decompressedBuffer.data(),
				static_cast<int>(data.binaryBlob.size()), static_cast<int>(decompressedBuffer.size()));
		}
		else
		{
			std::copy_n(data.binaryBlob.data(), static_cast<int>(totalSize), decompressedBuffer.data());
		}

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