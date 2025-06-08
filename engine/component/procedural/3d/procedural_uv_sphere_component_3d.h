#pragma once

#include "nlohmann/json.hpp"

#include "types/vector2.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

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
		inline nlohmann::json Serialize<procedural::ProceduralUVSphereComponent3D>(const procedural::ProceduralUVSphereComponent3D& data)
		{
			nlohmann::json json;

			json["radius"] = data.radius;
			json["segments"] = Serialize(data.segments);

			return json;
		}

		template<>
		inline procedural::ProceduralUVSphereComponent3D Deserialize<procedural::ProceduralUVSphereComponent3D>(const nlohmann::json& json)
		{
			procedural::ProceduralUVSphereComponent3D data;

			data.radius = json["radius"];
			data.segments = Deserialize<Vector2i>(json["segments"]);
			
			return data;
		}
	}
}
