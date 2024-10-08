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
		struct ProceduralUVSphereComponent3D
		{
			ProceduralUVSphereComponent3D() = default;

			double radius = 0.5;
			Vector2i segments = { 10 };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralUVSphereComponent3D, radius, segments)
		};
	}

	template<>
	inline void reflection::RegisterType<procedural::ProceduralUVSphereComponent3D>()
	{
		using namespace procedural;

		entt::meta<ProceduralUVSphereComponent3D>()
			.type(entt::hs("ProceduralUVSphereComponent3D"))
			.data<&ProceduralUVSphereComponent3D::radius>(entt::hs("radius"))
			.data<&ProceduralUVSphereComponent3D::segments>(entt::hs("segments"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<procedural::ProceduralUVSphereComponent3D>(const procedural::ProceduralUVSphereComponent3D& data, Archive& archive)
		{
			archive.Set("radius", data.radius);
			archive.Set("segments", data.segments);
		}

		template<>
		inline void Deserialize<procedural::ProceduralUVSphereComponent3D>(const Archive& archive, procedural::ProceduralUVSphereComponent3D& data)
		{
			archive.Get("radius", data.radius);
			archive.Get("segments", data.segments);
		}
	}
}
