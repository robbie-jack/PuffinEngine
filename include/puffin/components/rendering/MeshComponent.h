#pragma once

#include "puffin/types/uuid.h"
#include "puffin/types/vector.h"
#include "puffin/types/vertex.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() = default;
		
		MeshComponent(const PuffinID mesh_id, const PuffinID mat_id, const uint8_t idx = 0) :
			mesh_asset_id(mesh_id), mat_asset_id(mat_id), sub_mesh_idx(idx)
		{
		}

		PuffinID mesh_asset_id = gInvalidID;
		PuffinID mat_asset_id = gInvalidID;
		uint8_t sub_mesh_idx = 0; // Index of sub mesh to render for the set model, always 0 for models with no sub-mesh

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, mesh_asset_id, mat_asset_id, sub_mesh_idx)
	};

	struct ProceduralMeshComponent
	{
		ProceduralMeshComponent() {}

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		explicit ProceduralMeshComponent(const PuffinID texture_id) : texture_asset_id(texture_id) {}

		PuffinID texture_asset_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent, texture_asset_id)
	};
}