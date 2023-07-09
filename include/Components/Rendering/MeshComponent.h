#pragma once

#include <Types/UUID.h>
#include "Types/Vector.h"
#include "Types\Vertex.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() = default;
		
		MeshComponent(const PuffinID meshId, const PuffinID matId) :
			meshAssetId(meshId), matAssetID(matId)
		{
		}

		PuffinID meshAssetId;
		PuffinID matAssetID;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetId, matAssetID)
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