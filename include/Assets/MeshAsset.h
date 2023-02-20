#pragma once

#include "AssetRegistry.h"
#include <Components/Rendering/MeshComponent.h>
#include "Types/Vertex.hpp"

#include <vector>

namespace Puffin::Assets
{
	static const std::string G_STATIC_MESH_TYPE = "StaticMesh";
	static constexpr uint32_t G_STATIC_MESH_VERSION = 1; // Latest version of Static Mesh Asset Format

	struct MeshInfo
	{
		CompressionMode compressionMode;
		std::string originalFile;
		Rendering::VertexFormat vertexFormat;
		uint64_t numVertices;
		uint64_t numIndices;
		uint64_t verticesSize;
		uint64_t indicesSize;
	};

	class StaticMeshAsset : public Asset
	{
	public:

		StaticMeshAsset() : Asset(fs::path()) {}

		StaticMeshAsset(const fs::path& path) : Asset(path) {}

		StaticMeshAsset(const UUID id, const fs::path& path) : Asset(id, path) {}

		~StaticMeshAsset() override = default;

		const std::string& Type() const
		{
			return G_STATIC_MESH_TYPE;
		}

		const uint32_t& Version() const
		{
			return G_STATIC_MESH_VERSION;
		}

		bool Save();

		bool Save(const MeshInfo& info, const void* vertexData, const void* indexData);

		bool Load();

		void Unload() override;

		const std::vector<char>& GetVertices() const { return m_vertices; }

		const std::vector<uint32_t>& GetIndices() const { return m_indices; }

		Rendering::VertexFormat GetFormat() const { return m_vertexFormat; }

		uint32_t GetNumVertices() const { return m_numVertices; }
		uint32_t GetNumIndices() const { return m_numIndices; }

		uint32_t GetVertexSize() const { return Rendering::GetVertexSizeFromFormat(m_vertexFormat); }
		uint32_t GetIndexSize() const { return sizeof(uint32_t); }

	private:

		std::vector<char> m_vertices;
		std::vector<uint32_t> m_indices;

		Rendering::VertexFormat m_vertexFormat;
		uint32_t m_numVertices, m_numIndices;

		std::string m_originalFile;

		MeshInfo ParseMeshInfo(const AssetData& data);
	};
}