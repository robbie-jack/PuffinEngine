#pragma once

#include <vector>

#include "puffin/types/uuid.h"
#include "puffin/types/vertex.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

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
		inline void Serialize<rendering::ProceduralMeshComponent3D>(const rendering::ProceduralMeshComponent3D& data, Archive& archive)
		{
			archive.Set("materialID", data.materialID);
		}

		template<>
		inline void Deserialize<rendering::ProceduralMeshComponent3D>(const Archive& archive, rendering::ProceduralMeshComponent3D& data)
		{
			archive.Get("materialID", data.materialID);
		}
	}
}
