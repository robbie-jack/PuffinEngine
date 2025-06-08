#pragma once

#include "nlohmann/json.hpp"

#include "types/vector2.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

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
		inline nlohmann::json Serialize<procedural::ProceduralPlaneComponent3D>(const procedural::ProceduralPlaneComponent3D& data)
		{
			nlohmann::json json;

			json["halfSize"] = Serialize(data.halfSize);
			json["quadCount"] = Serialize(data.quadCount);

			return json;
		}

		template<>
		inline procedural::ProceduralPlaneComponent3D Deserialize<procedural::ProceduralPlaneComponent3D>(const nlohmann::json& json)
		{
			procedural::ProceduralPlaneComponent3D data;

			data.halfSize = Deserialize<Vector2f>(json["halfSize"]);
			data.quadCount = Deserialize<Vector2i>(json["quadCount"]);
			
			return data;
		}
	}
}
