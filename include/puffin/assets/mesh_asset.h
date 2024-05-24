#pragma once

#include <vector>

#include "asset.h"
#include "puffin/types/vertex.h"

namespace puffin::assets
{
	static const std::string gStaticMeshTypeString = "StaticMesh";
	static constexpr uint32_t gStaticMeshVersion = 1; // Latest version of Static Mesh Asset Format

	// Struct defining info about a single mesh or sub-mesh
	struct SubMeshInfo
	{
		uint64_t vertex_offset = 0; // Offset into vertex vector where this (sub)mesh vertices begin
		uint64_t index_offset = 0; // Offset into vertex vector where this (sub)mesh indices begin
		uint64_t vertex_count = 0; // Number of vertices in this (sub)mesh
		uint64_t index_count = 0; // Number of indices in this (sub)mesh
		uint64_t vertex_byte_size_total = 0; // Total amount of memory, in bytes, used by all vertices in this (sub)mesh, i.e. vertexCount * sizeof(VertexType)
		uint64_t index_byte_size_total = 0; // Total amount of memory, in bytes, used by all indices in this (sub)mesh, i.e. vertexCount * sizeof(IndexType)
		size_t sub_mesh_idx = 0; // index of this (sub)mesh. Always 0 for single meshes
	};

	struct MeshAssetInfo : AssetInfo
	{
		rendering::VertexFormat vertex_format = rendering::VertexFormat::Unknown;
		uint64_t vertex_count_total = 0;
		uint64_t index_count_total = 0;
		uint64_t vertex_byte_size_total = 0;
		uint64_t index_byte_size_total = 0;

		std::vector<SubMeshInfo> sub_mesh_infos;
	};

	class StaticMeshAsset final : public Asset
	{
	public:

		StaticMeshAsset() : Asset(fs::path()) {}

		StaticMeshAsset(const fs::path& path) : Asset(path) {}

		StaticMeshAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

		~StaticMeshAsset() override = default;

		[[nodiscard]] const std::string& type() const override
		{
			return gStaticMeshTypeString;
		}

		[[nodiscard]] const uint32_t& version() const override
		{
			return gStaticMeshVersion;
		}

		bool save() override;

		bool save(MeshAssetInfo& mesh_asset_info, const void* vertex_data, const void* index_data);

		bool load(bool loadHeaderOnly = false) override;

		void unload() override;

		[[nodiscard]] const std::vector<char>& vertices() const { return m_vertices; }

		[[nodiscard]] const std::vector<uint32_t>& indices() const { return m_indices; }

		[[nodiscard]] const std::vector<SubMeshInfo>& sub_mesh_info() const { return m_mesh_asset_info.sub_mesh_infos; }

		[[nodiscard]] rendering::VertexFormat vertex_format() const { return m_mesh_asset_info.vertex_format; }

		[[nodiscard]] uint64_t vertex_byte_size() const { return rendering::parseVertexSizeFromFormat(m_mesh_asset_info.vertex_format); } // Size of an individual vertex, in bytes
		[[nodiscard]] uint64_t index_byte_size() const { return sizeof(uint32_t); } // Size of an individual index, in bytes

		[[nodiscard]] uint64_t vertex_count_total() const { return m_mesh_asset_info.vertex_count_total; } // Total number of vertices in this mesh
		[[nodiscard]] uint64_t index_count_total() const { return m_mesh_asset_info.index_count_total; } // Total number of indices in this mesh

		[[nodiscard]] uint64_t vertex_byte_size_total() const { return m_mesh_asset_info.vertex_byte_size_total; } // Total amount of memory, in bytes, used by all vertices in this mesh
		[[nodiscard]] uint64_t index_byte_size_total() const { return m_mesh_asset_info.index_byte_size_total; } // Total amount of memory, in bytes, used by all indices in this mesh

	private:

		std::vector<char> m_vertices;
		std::vector<uint32_t> m_indices;

		MeshAssetInfo m_mesh_asset_info;

		static void parse_mesh_info(const AssetData& data, MeshAssetInfo& out_mesh_asset_info);
	};
}
