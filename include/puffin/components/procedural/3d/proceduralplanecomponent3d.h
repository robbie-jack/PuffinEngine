#pragma once

#include "proceduralcubecomponent3d.h"
#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace procedural
	{
		struct ProceduralPlaneComponent3D
		{
			ProceduralPlaneComponent3D() = default;

			Vector2f halfSize = { 10.f }; // Half size of plane
			Vector2i quadCount = { 10 }; // Number of quads that make up planes surface along a single axis

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralPlaneComponent3D, halfSize, quadCount)
		};
	}

	template<>
	inline void reflection::RegisterType<procedural::ProceduralPlaneComponent3D>()
	{
		using namespace procedural;

		entt::meta<ProceduralPlaneComponent3D>()
			.type(entt::hs("ProceduralPlaneComponent3D"))
			.data<&ProceduralPlaneComponent3D::halfSize>(entt::hs("halfSize"))
			.data<&ProceduralPlaneComponent3D::quadCount>(entt::hs("quadCount"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<procedural::ProceduralPlaneComponent3D>(const procedural::ProceduralPlaneComponent3D& data, Archive& archive)
		{
			archive.Set("halfSize", data.halfSize);
			archive.Set("quadCount", data.quadCount);
		}

		template<>
		inline void Deserialize<procedural::ProceduralPlaneComponent3D>(const Archive& archive, procedural::ProceduralPlaneComponent3D& data)
		{
			archive.Get("halfSize", data.halfSize);
			archive.Get("quadCount", data.quadCount);
		}
	}
}
