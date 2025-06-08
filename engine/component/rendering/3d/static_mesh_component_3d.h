#pragma once

#include "nlohmann/json.hpp"

#include "types/uuid.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

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
	inline void reflection::RegisterType<rendering::StaticMeshComponent3D>()
	{
		using namespace rendering;

		entt::meta<StaticMeshComponent3D>()
			.type(entt::hs("StaticMeshComponent3D"))
			.data<&StaticMeshComponent3D::meshID>(entt::hs("meshID"))
			.data<&StaticMeshComponent3D::materialID>(entt::hs("materialID"))
			.data<&StaticMeshComponent3D::subMeshIdx>(entt::hs("subMeshIdx"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::StaticMeshComponent3D>(const rendering::StaticMeshComponent3D& data)
		{
			nlohmann::json json;

			json["meshID"] = data.meshID;
			json["materialID"] = data.materialID;
			json["subMeshIdx"] = data.subMeshIdx;

			return json;
		}

		template<>
		inline rendering::StaticMeshComponent3D Deserialize<rendering::StaticMeshComponent3D>(const nlohmann::json& json)
		{
			rendering::StaticMeshComponent3D data;

			data.meshID = json["meshID"];
			data.materialID = json["materialID"];
			data.subMeshIdx = json["subMeshIdx"];
			
			return data;
		}
	}
}