#pragma once

#include <Types/UUID.h>
#include "Types/Vector.h"
#include "Types/Vertex.hpp"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() {}
		
		MeshComponent(UUID meshId, UUID textureId) :
			meshAssetId(meshId), textureAssetId(textureId)
		{
		}

		// Mesh Data
		UUID meshAssetId;

		// Texture
		UUID textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetId, textureAssetId)
	};

	struct ProceduralMeshComponent
	{
		ProceduralMeshComponent() {}

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		explicit ProceduralMeshComponent(UUID textureId) : textureAssetId(textureId) {}

		UUID textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent, textureAssetId)
	};
}