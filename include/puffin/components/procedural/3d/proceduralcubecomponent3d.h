#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace procedural
	{
		struct ProceduralCubeComponent3D
		{
			ProceduralCubeComponent3D() = default;

			Vector3f halfSize = { 10.f }; // Half size of cube
			Vector3i quadCount = { 10 }; // Number of quads that make up cubes surface

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralCubeComponent3D, halfSize, quadCount)
		};
	}

	template<>
	inline void reflection::RegisterType<procedural::ProceduralCubeComponent3D>()
	{
		using namespace procedural;

		entt::meta<ProceduralCubeComponent3D>()
			.type(entt::hs("ProceduralCubeComponent3D"))
			.data<&ProceduralCubeComponent3D::halfSize>(entt::hs("halfSize"))
			.data<&ProceduralCubeComponent3D::quadCount>(entt::hs("quadCount"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<procedural::ProceduralCubeComponent3D>(const procedural::ProceduralCubeComponent3D& data, Archive& archive)
		{
			archive.Set("halfSize", data.halfSize);
			archive.Set("quadCount", data.quadCount);
		}

		template<>
		inline void Deserialize<procedural::ProceduralCubeComponent3D>(const Archive& archive, procedural::ProceduralCubeComponent3D& data)
		{
			archive.Get("halfSize", data.halfSize);
			archive.Get("quadCount", data.quadCount);
		}

		template<>
		inline nlohmann::json Serialize<procedural::ProceduralCubeComponent3D>(const procedural::ProceduralCubeComponent3D& data)
		{
			nlohmann::json json;

			json["halfSize"] = Serialize(data.halfSize);
			json["quadCount"] = Serialize(data.quadCount);

			return json;
		}

		template<>
		inline procedural::ProceduralCubeComponent3D Deserialize<procedural::ProceduralCubeComponent3D>(const nlohmann::json& json)
		{
			procedural::ProceduralCubeComponent3D data;

			data.halfSize = Deserialize<Vector3f>(json["halfSize"]);
			data.quadCount = Deserialize<Vector3i>(json["quadCount"]);
			
			return data;
		}
	}
}
