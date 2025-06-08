#pragma once

#include "nlohmann/json.hpp"

#include "component/procedural/3d/procedural_plane_component_3d.h"


namespace puffin
{
	namespace procedural
	{
		struct ProceduralTerrainComponent3D : ProceduralPlaneComponent3D
		{
			ProceduralTerrainComponent3D() = default;

			int64_t seed = 983758376;
			double heightMult = 10.0;
			double frequencyMult = 2.0;
			double frequency = 10.0;
			int octaves = 4;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralTerrainComponent3D, halfSize, quadCount, seed, heightMult, frequencyMult, frequency, octaves)
		};
	}

	template<>
	inline void reflection::RegisterType<procedural::ProceduralTerrainComponent3D>()
	{
		using namespace procedural;

		entt::meta<ProceduralTerrainComponent3D>()
			.type(entt::hs("ProceduralTerrainComponent3D"))
			.data<&ProceduralTerrainComponent3D::halfSize>(entt::hs("halfSize"))
			.data<&ProceduralTerrainComponent3D::quadCount>(entt::hs("quadCount"))
			.data<&ProceduralTerrainComponent3D::seed>(entt::hs("seed"))
			.data<&ProceduralTerrainComponent3D::heightMult>(entt::hs("heightMult"))
			.data<&ProceduralTerrainComponent3D::frequencyMult>(entt::hs("frequencyMult"))
			.data<&ProceduralTerrainComponent3D::frequency>(entt::hs("frequency"))
			.data<&ProceduralTerrainComponent3D::octaves>(entt::hs("octaves"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<procedural::ProceduralTerrainComponent3D>(const procedural::ProceduralTerrainComponent3D& data)
		{
			nlohmann::json json;

			json["halfSize"] = Serialize(data.halfSize);
			json["quadCount"] = Serialize(data.quadCount);
			json["seed"] = data.seed;
			json["heightMult"] = data.heightMult;
			json["frequencyMult"] = data.frequencyMult;
			json["frequency"] = data.frequency;
			json["octaves"] = data.octaves;

			return json;
		}

		template<>
		inline procedural::ProceduralTerrainComponent3D Deserialize<procedural::ProceduralTerrainComponent3D>(const nlohmann::json& json)
		{
			procedural::ProceduralTerrainComponent3D data;

			data.halfSize = Deserialize<Vector2f>(json["halfSize"]);
			data.quadCount = Deserialize<Vector2i>(json["quadCount"]);
			data.seed = json["seed"];
			data.heightMult = json["heightMult"];
			data.frequencyMult = json["frequencyMult"];
			data.frequency = json["frequency"];
			data.octaves = json["octaves"];
			
			return data;
		}
	}
}