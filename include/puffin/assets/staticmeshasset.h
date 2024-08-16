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
		uint64_t vertexOffset = 0; // Offset into vertex vector where this (sub)mesh vertices begin
		uint64_t indexOffset = 0; // Offset into vertex vector where this (sub)mesh indices begin
		uint64_t vertexCount = 0; // Number of vertices in this (sub)mesh
		uint64_t indexCount = 0; // Number of indices in this (sub)mesh
		uint64_t vertexByteSizeTotal = 0; // Total amount of memory, in bytes, used by all vertices in this (sub)mesh, i.e. vertexCount * sizeof(VertexType)
		uint64_t indexByteSizeTotal = 0; // Total amount of memory, in bytes, used by all indices in this (sub)mesh, i.e. vertexCount * sizeof(IndexType)
		size_t subMeshIdx = 0; // index of this (sub)mesh. Always 0 for single meshes
	};

	struct MeshAssetInfo : AssetInfo
	{
		rendering::VertexFormat vertexFormat = rendering::VertexFormat::Unknown;
		uint64_t vertexCountTotal = 0;
		uint64_t indexCountTotal = 0;
		uint64_t vertexByteSizeTotal = 0;
		uint64_t indexByteSizeTotal = 0;

		std::vector<SubMeshInfo> subMeshInfos;
	};

	class StaticMeshAsset final : public Asset
	{
	public:

		StaticMeshAsset();
		explicit StaticMeshAsset(const fs::path& path);
		StaticMeshAsset(const UUID id, const fs::path& path);

		~StaticMeshAsset() override = default;

		[[nodiscard]] const std::string& GetType() const override;
		[[nodiscard]] const uint32_t& GetVersion() const override;

		bool Save() override;
		bool Save(MeshAssetInfo& meshAssetInfo, const void* vertexData, const void* indexData);

		bool Load(bool loadHeaderOnly = false) override;

		void Unload() override;

		[[nodiscard]] const std::vector<char>& Vertices() const;
		[[nodiscard]] const std::vector<uint32_t>& Indices() const;
		[[nodiscard]] const std::vector<SubMeshInfo>& SubMeshInfo() const;
		[[nodiscard]] rendering::VertexFormat VertexFormat() const;

		[[nodiscard]] uint64_t VertexByteSize() const;
		// Size of an individual vertex, in bytes
		[[nodiscard]] uint64_t IndexByteSize() const;
		// Size of an individual index, in bytes

		[[nodiscard]] uint64_t VertexCountTotal() const { return mMeshAssetInfo.vertexCountTotal; } // Total number of vertices in this mesh
		[[nodiscard]] uint64_t IndexCountTotal() const { return mMeshAssetInfo.indexCountTotal; } // Total number of indices in this mesh

		[[nodiscard]] uint64_t VertexByteSizeTotal() const { return mMeshAssetInfo.vertexByteSizeTotal; } // Total amount of memory, in bytes, used by all vertices in this mesh
		[[nodiscard]] uint64_t IndexByteSizeTotal() const { return mMeshAssetInfo.indexByteSizeTotal; } // Total amount of memory, in bytes, used by all indices in this mesh

	private:

		std::vector<char> mVertices;
		std::vector<uint32_t> mIndices;

		MeshAssetInfo mMeshAssetInfo = {};

		static void ParseMeshInfo(const AssetData& data, MeshAssetInfo& meshAssetInfo);
	};
}
