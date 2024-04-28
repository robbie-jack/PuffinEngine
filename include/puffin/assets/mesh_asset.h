#pragma once

#include <vector>

#include "asset.h"
#include "puffin/types/Vertex.h"

namespace puffin::assets
{
	static const std::string gStaticMeshTypeString = "StaticMesh";
	static constexpr uint32_t gStaticMeshVersion = 1; // Latest version of Static Mesh Asset Format

	struct MeshAssetInfo : AssetInfo
	{
		rendering::VertexFormat vertexFormat = rendering::VertexFormat::Unknown;
		uint64_t vertexCountTotal = 0;
		uint64_t indexCountTotal = 0;
		uint64_t vertexByteSizeTotal = 0;
		uint64_t indexByteSizeTotal = 0;
		size_t subMeshCount = 0;
	};

	// Struct defining info about a single mesh or sub-mesh
	struct SubMeshInfo
	{
		uint64_t vertexOffset = 0; // Offset into vertex vector where this (sub)mesh vertices begin
		uint64_t indexOffset = 0; // Offset into vertex vector where this (sub)mesh indices begin
		uint64_t vertexCount = 0; // Number of vertices in this (sub)mesh
		uint64_t indexCount = 0; // Number of indices in this (sub)mesh
		uint64_t vertexByteSize = 0; // Total amount of memory, in bytes, used by all vertices in this (sub)mesh, i.e. vertexCount * sizeof(VertexType)
		uint64_t indexByteSize = 0; // Total amount of memory, in bytes, used by all indices in this (sub)mesh, i.e. vertexCount * sizeof(IndexType)
		size_t subMeshIdx = 0; // index of this (sub)mesh. Always 0 for single meshes
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

		bool save(MeshAssetInfo& meshAssetInfo, std::vector<SubMeshInfo>& subMeshInfo, const void* vertexData, const void* indexData);

		bool load(bool loadHeaderOnly = false) override;

		void unload() override;

		[[nodiscard]] const std::vector<char>& vertices() const { return mVertices; }

		[[nodiscard]] const std::vector<uint32_t>& indices() const { return mIndices; }

		[[nodiscard]] const std::vector<SubMeshInfo>& subMeshInfo() const { return mSubMeshInfo; }

		[[nodiscard]] rendering::VertexFormat vertexFormat() const { return mMeshAssetInfo.vertexFormat; }

		[[nodiscard]] uint64_t vertexSize() const { return rendering::parseVertexSizeFromFormat(mMeshAssetInfo.vertexFormat); } // Size of an individual vertex, in bytes
		[[nodiscard]] uint64_t indexSize() const { return sizeof(uint32_t); } // Size of an individual index, in bytes

		[[nodiscard]] uint64_t vertexCountTotal() const { return mMeshAssetInfo.vertexCountTotal; } // Total number of vertices in this mesh
		[[nodiscard]] uint64_t indexCountTotal() const { return mMeshAssetInfo.indexCountTotal; } // Total number of indices in this mesh

		[[nodiscard]] uint64_t vertexByteSizeTotal() const { return mMeshAssetInfo.vertexByteSizeTotal; } // Total amount of memory, in bytes, used by all vertices in this mesh
		[[nodiscard]] uint64_t indexByteSizeTotal() const { return mMeshAssetInfo.indexByteSizeTotal; } // Total amount of memory, in bytes, used by all indices in this mesh

	private:

		std::vector<char> mVertices;
		std::vector<uint32_t> mIndices;

		MeshAssetInfo mMeshAssetInfo;
		std::vector<SubMeshInfo> mSubMeshInfo;

		[[nodiscard]] static void parseMeshInfo(const AssetData& data, MeshAssetInfo& outMeshAssetInfo, std::vector<SubMeshInfo>& outSubeshInfo);
	};
}
