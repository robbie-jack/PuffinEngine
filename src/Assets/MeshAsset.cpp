#include "Assets/MeshAsset.h"
#include "nlohmann/json.hpp"
#include "lz4.h"

using json = nlohmann::json;

namespace Puffin::Assets
{
	////////////////////////////////
	// StaticMeshAsset
	////////////////////////////////

	// Public

	bool StaticMeshAsset::Save()
	{
		if (m_isLoaded)
		{
			MeshInfo info;
			info.vertexFormat = m_vertexFormat;
			info.numVertices = m_numVertices;
			info.numIndices = m_numIndices;
			info.verticesSize = m_numVertices * GetVertexSize();
			info.indicesSize = m_numIndices * GetIndexSize();
			info.originalFile = m_originalFile;

			return Save(info, m_vertices.data(), m_indices.data());
		}

		return false;
	}

	bool StaticMeshAsset::Save(const MeshInfo& info, const void* vertexData, const void* indexData)
	{
		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = G_STATIC_MESH_TYPE;
		data.version = G_STATIC_MESH_VERSION;

		// Fill Metadata from Info struct
		json metadata;

		metadata["vertex_format"] = Rendering::ParseVertexStringFromFormat(info.vertexFormat);
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
		size_t compressStaging = LZ4_compressBound(static_cast<int>(fullSize));

		data.binaryBlob.resize(compressStaging);

		int compressedSize = LZ4_compress_default(mergedBuffer.data(), data.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(compressStaging));
		data.binaryBlob.resize(compressedSize);

		// Save Asset Data out to Binary File
		return SaveBinaryFile(fullPath, data);
	}

	bool StaticMeshAsset::Load()
	{
		// Check if file is already loaded
		if (m_isLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadBinaryFile(fullPath, data))
		{
			return false;
		}

		// Parse Metadata from Json
		MeshInfo info = ParseMeshInfo(data);

		m_vertexFormat = info.vertexFormat;
		m_numVertices = info.numVertices;
		m_numIndices = info.numIndices;
		m_originalFile = info.originalFile;

		// Decompress Binary Data
		std::vector<char> decompressedBuffer;
		decompressedBuffer.resize(info.verticesSize + info.indicesSize);

		LZ4_decompress_safe(data.binaryBlob.data(), decompressedBuffer.data(), 
		static_cast<int>(data.binaryBlob.size()), static_cast<int>(decompressedBuffer.size()));

		// Copy Vertex Buffer
		m_vertices.resize(info.verticesSize);
		memcpy(m_vertices.data(), decompressedBuffer.data(), info.verticesSize);

		// Copy Index Buffer
		m_indices.resize(info.indicesSize);
		memcpy(m_indices.data(), decompressedBuffer.data() + info.verticesSize, info.indicesSize);

		m_isLoaded = true;
		return true;
	}

	void StaticMeshAsset::Unload()
	{
		m_vertices.clear();
		m_vertices.shrink_to_fit();

		m_indices.clear();
		m_indices.shrink_to_fit();

		m_isLoaded = false;
	}

	// Private

	MeshInfo StaticMeshAsset::ParseMeshInfo(const AssetData& data)
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

		std::string vertexFormat = metadata["vertex_format"];
		info.vertexFormat = Rendering::ParseVertexFormatFromString(vertexFormat.c_str());
		
		std::string compressionMode = metadata["compression"];
		info.compressionMode = ParseCompressionMode(compressionMode.c_str());

		return info;
	}
}