#pragma once

#include "BGFXVertex.hpp"

#include "ECS/EntityID.h"
#include "Types/UUID.h"

#include <set>
#include <vector>

namespace puffin::rendering
{
	struct AssetData
	{
		UUID assetId; // ID of asset
	};

	struct MeshData : AssetData
	{
		bgfx::VertexBufferHandle vertexBufferHandle;
		bgfx::IndexBufferHandle indexBufferHandle;

		uint32_t numVertices; // Number of Vertices in Mesh
		uint32_t numIndices; // Number of Indices in Mesh
	};

	struct TextureData : AssetData
	{
		bgfx::TextureHandle handle;
	};

	struct MaterialData : AssetData
	{
		bgfx::ProgramHandle programHandle;
		std::vector<UUID> texIDs;
	};

	struct MeshDrawBatch
	{
		MeshData meshData;
		MaterialData matData;

		std::set<ECS::EntityID> entities; // Set of Entities using this mesh/shader combo
	};

	struct LightUniformHandles
	{
		bgfx::UniformHandle position;
		bgfx::UniformHandle direction;
		bgfx::UniformHandle color;
		bgfx::UniformHandle ambientSpecular;
		bgfx::UniformHandle attenuation;

		bgfx::UniformHandle index; // Index into light array for each light type
	};
}
