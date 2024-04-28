#pragma once

#include "puffin/types/uuid.h"
#include "puffin/types/Vector.h"
#include "puffin/types/Vertex.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() = default;
		
		MeshComponent(const PuffinID meshId, const PuffinID matId, const uint8_t Idx = 0) :
			meshAssetID(meshId), matAssetID(matId), subMeshIdx(Idx)
		{
		}

		PuffinID meshAssetID = gInvalidID;
		PuffinID matAssetID = gInvalidID;
		uint8_t subMeshIdx = 0; // Index of sub mesh to render for the set model, always 0 for models with no sub-mesh

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetID, matAssetID, subMeshIdx)
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