#pragma once

#include "BGFXVertex.hpp"

#include "ECS/EntityID.h"
#include "Types/UUID.h"

#include <set>
#include <vector>

namespace puffin::rendering
{
	struct AssetDataBGFX
	{
		UUID assetId; // ID of asset
	};

	struct MeshDataBGFX : AssetDataBGFX
	{
		bgfx::VertexBufferHandle vertexBufferHandle;
		bgfx::IndexBufferHandle indexBufferHandle;

		uint32_t numVertices; // Number of Vertices in Mesh
		uint32_t numIndices; // Number of Indices in Mesh
	};

	struct TextureDataBGFX : AssetDataBGFX
	{
		bgfx::TextureHandle handle;
	};

	struct MaterialDataBGFX : AssetDataBGFX
	{
		bgfx::ProgramHandle programHandle;
		std::vector<UUID> texIDs;
	};

	struct MeshDrawBatch
	{
		MeshDataBGFX meshData;
		MaterialDataBGFX matData;

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
