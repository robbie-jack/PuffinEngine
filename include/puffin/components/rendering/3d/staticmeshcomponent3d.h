#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/uuid.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		struct StaticMeshComponent3D
		{
			StaticMeshComponent3D() = default;

			StaticMeshComponent3D(const UUID meshID, const UUID materialID, const uint8_t subMeshIdx = 0) :
				meshID(meshID), materialID(materialID), subMeshIdx(subMeshIdx)
			{
			}

			UUID meshID = gInvalidID;
			UUID materialID = gInvalidID;
			uint8_t subMeshIdx = 0; // Index of sub mesh to render for the set model, always 0 for models with no sub-mesh

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(StaticMeshComponent3D, meshID, materialID, subMeshIdx)
		};
	}

	template<>
	inline void RegisterType<rendering::StaticMeshComponent3D>()
	{
		using namespace rendering;

		entt::meta<StaticMeshComponent3D>()
			.type(entt::hs("StaticMeshComponent3D"))
			.data<&StaticMeshComponent3D::meshID>(entt::hs("meshID"))
			.data<&StaticMeshComponent3D::materialID>(entt::hs("materialID"))
			.data<&StaticMeshComponent3D::subMeshIdx>(entt::hs("subMeshIdx"));
	}
}