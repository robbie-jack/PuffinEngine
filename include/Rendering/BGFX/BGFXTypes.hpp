#pragma once

#include "BGFXVertex.hpp"

#include "ECS/EntityID.h"
#include "Types/UUID.h"


#include <set>
#include <bx/math.h>

namespace Puffin::Rendering::BGFX
{
	struct MeshData
	{
		bgfx::VertexBufferHandle vertexBufferHandle;
		bgfx::IndexBufferHandle indexBufferHandle;

		UUID assetID; // ID of Mesh Asset
		uint32_t numVertices; // Number of Vertices in Mesh
		uint32_t numIndices; // Number of Indices in Mesh

		std::set<ECS::EntityID> entities; // Set of Entities using this mesh
	};

	struct MeshDrawBatch
	{
		bgfx::VertexBufferHandle vertexBufferHandle;
		bgfx::IndexBufferHandle indexBufferHandle;
		bgfx::ProgramHandle programHandle;

		std::set<ECS::EntityID> entities; // Set of Entities using this mesh/shader combo
	};

	struct TextureData
	{
		bgfx::TextureHandle handle;

		std::set<ECS::EntityID> entities; // Set of Entities using this texture
	};
}
