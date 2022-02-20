#pragma once

#ifndef MESH_ASSET_H
#define MESH_ASSET_H

#include <AssetRegistry.h>
#include <Components/Rendering/MeshComponent.h>

#include <vector>

namespace Puffin
{
	namespace IO
	{
		class StaticMeshAsset : public Asset
		{
		public:

			StaticMeshAsset() : Asset(fs::path()) {}
			StaticMeshAsset(fs::path path) : Asset(path) {}
			StaticMeshAsset(UUID id, fs::path path) : Asset(id, path) {}

			std::string Type() override;

			bool Save() override;

			bool Load() override;

			void Unload() override;

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

			template<class Archive>
			void serialize(Archive& archive) const
			{
				archive(cereal::base_class<Asset>(this));
			}

		private:

			std::vector<Rendering::Vertex> m_vertices;
			std::vector<uint32_t> m_indices;

		};
	}
}



#endif // !MESH_ASSET_H