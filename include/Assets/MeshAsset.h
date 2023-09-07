#pragma once

#include "AssetRegistry.h"
#include <Components/Rendering/MeshComponent.h>
#include "Types/Vertex.h"

#include <vector>

namespace puffin::assets
{
	static const std::string gStaticMeshTypeString = "StaticMesh";
	static constexpr uint32_t gStaticMeshVersion = 1; // Latest version of Static Mesh Asset Format

	struct MeshInfo : AssetInfo
	{
		rendering::VertexFormat vertexFormat;
		uint64_t numVertices;
		uint64_t numIndices;
		uint64_t verticesSize;
		uint64_t indicesSize;
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

		bool save(MeshInfo& info, const void* vertexData, const void* indexData);

		bool load() override;

		void unload() override;

		[[nodiscard]] const std::vector<char>& vertices() const { return mVertices; }

		[[nodiscard]] const std::vector<uint32_t>& indices() const { return mIndices; }

		[[nodiscard]] rendering::VertexFormat vertexFormat() const { return mVertexFormat; }

		[[nodiscard]] uint32_t numVertices() const { return mNumVertices; }
		[[nodiscard]] uint32_t numIndices() const { return mNumIndices; }

		[[nodiscard]] uint32_t vertexSize() const { return rendering::parseVertexSizeFromFormat(mVertexFormat); }
		[[nodiscard]] uint32_t indexSize() const { return sizeof(uint32_t); }

	private:

		std::vector<char> mVertices;
		std::vector<uint32_t> mIndices;

		rendering::VertexFormat mVertexFormat;
		uint32_t mNumVertices, mNumIndices;

		std::string mOriginalFile;
		CompressionMode mCompressionMode;

		[[nodiscard]] static MeshInfo parseMeshInfo(const AssetData& data);
	};
}