#pragma once

#include "puffin/types/uuid.h"

#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() = default;
		
		MeshComponent(const PuffinID meshID, const PuffinID materialID, const uint8_t subMeshIdx = 0) :
			meshID(meshID), materialID(materialID), subMeshIdx(subMeshIdx)
		{
		}

		PuffinID meshID = gInvalidID;
		PuffinID materialID = gInvalidID;
		uint8_t subMeshIdx = 0; // Index of sub mesh to render for the set model, always 0 for models with no sub-mesh

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshID, materialID, subMeshIdx)
	};
}