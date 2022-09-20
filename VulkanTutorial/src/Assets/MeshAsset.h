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

		const std::string& Type() const
		{
			return G_STATIC_MESH_TYPE;
		}

		const uint32_t& Version() const
		{
			return G_STATIC_MESH_VERSION;
		}

		bool Save(const MeshInfo& info, const void* verticesData, const void* indicesData);

		bool Load();

		void Unload();

		const std::vector<Rendering::Vertex_PNCTV_32>& GetVertices() const
		{
			return m_vertices;
		}

		const std::vector<uint32_t>& GetIndices() const
		{
			return m_indices;
		}

	private:

		std::vector<Rendering::Vertex_PNCTV_32> m_vertices;
		std::vector<uint32_t> m_indices;

		MeshInfo ParseMeshInfo(const AssetData& data);
	};
}