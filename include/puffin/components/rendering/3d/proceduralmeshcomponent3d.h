#pragma once

#include <vector>

#include "puffin/types/uuid.h"
#include "puffin/types/vertex.h"
#include "puffin/utility/reflection.h"

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
	inline void RegisterType<rendering::ProceduralMeshComponent3D>()
	{
		using namespace rendering;

		entt::meta<ProceduralMeshComponent3D>()
			.type(entt::hs("ProceduralMeshComponent3D"))
			.data<&ProceduralMeshComponent3D::materialID>(entt::hs("materialID"));
	}
}
