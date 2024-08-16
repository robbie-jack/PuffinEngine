#pragma once

#include "puffin/types/uuid.h"

#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	struct StaticMeshComponent3D
	{
		StaticMeshComponent3D() = default;
		
		StaticMeshComponent3D(const UUID meshID, const UUID materialID, const uint8_t subMeshIdx = 0) :
			meshID(meshID), materialID(materialID), subMeshIdx(subMeshIdx)
		{
		}

		UUID meshID = gInvalidId;
		UUID materialID = gInvalidId;
		uint8_t subMeshIdx = 0; // Index of sub mesh to render for the set model, always 0 for models with no sub-mesh

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(StaticMeshComponent3D, meshID, materialID, subMeshIdx)
	};
}