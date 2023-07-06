#pragma once

#include <Types/UUID.h>
#include "Types/Vector.h"
#include "Types/Vertex.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() = default;
		
		MeshComponent(const PuffinID meshId, const PuffinID textureId) :
			meshAssetId(meshId), textureAssetId(textureId)
		{
		}

		// Mesh Data
		PuffinID meshAssetId;

		// Texture
		PuffinID textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetId, textureAssetId)
	};

	struct ProceduralMeshComponent
	{
		ProceduralMeshComponent() {}

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		explicit ProceduralMeshComponent(PuffinID textureId) : textureAssetId(textureId) {}

		PuffinID textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent, textureAssetId)
	};
}