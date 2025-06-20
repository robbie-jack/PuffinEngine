#pragma once

#include <vector>

#include "types/uuid.h"
#include "types/vertex.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct ProceduralMeshComponent3D
		{
			ProceduralMeshComponent3D() = default;

			std::vector<rendering::VertexPNTV32> vertices;
			std::vector<uint32_t> indices;

			explicit ProceduralMeshComponent3D(const UUID textureID) : materialID(textureID) {}

			UUID materialID;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent3D, materialID)
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::ProceduralMeshComponent3D>()
	{
		using namespace rendering;

		entt::meta<ProceduralMeshComponent3D>()
			.type(entt::hs("ProceduralMeshComponent3D"))
			.data<&ProceduralMeshComponent3D::materialID>(entt::hs("materialID"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::ProceduralMeshComponent3D>(const rendering::ProceduralMeshComponent3D& data)
		{
			nlohmann::json json;
			
			json["materialID"] = data.materialID;

			return json;
		}

		template<>
		inline rendering::ProceduralMeshComponent3D Deserialize<rendering::ProceduralMeshComponent3D>(const nlohmann::json& json)
		{
			rendering::ProceduralMeshComponent3D data;
			
			data.materialID = json["materialID"];
			
			return data;
		}
	}
}
