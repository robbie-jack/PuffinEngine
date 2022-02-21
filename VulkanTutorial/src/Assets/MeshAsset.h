#pragma once

#include "AssetRegistry.h"
#include <Components/Rendering/MeshComponent.h>

#include <vector>

namespace Puffin::Assets
{
	static const std::string G_STATIC_MESH_TYPE = "StaticMesh";
	static constexpr uint32_t G_STATIC_MESH_VERSION = 1; // Latest version of Static Mesh Asset Format

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

		bool Save(const std::vector<Rendering::Vertex>& vertices, const std::vector<uint32_t>& indices);

		bool Load();

		void Unload();

		void AddVertex(Rendering::Vertex vertex)
		{
			m_vertices.emplace_back(vertex);
		}

		void AddIndex(uint32_t index)
		{
			m_indices.emplace_back(index);
		}

		const std::vector<Rendering::Vertex>& GetVertices() const
		{
			return m_vertices;
		}

		const std::vector<uint32_t>& GetIndices() const
		{
			return m_indices;
		}

	private:

		std::vector<Rendering::Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

	};
}