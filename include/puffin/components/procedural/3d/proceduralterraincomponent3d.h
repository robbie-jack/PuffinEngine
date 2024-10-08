#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/procedural/3d/proceduralplanecomponent3d.h"


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
		inline void Serialize<procedural::ProceduralTerrainComponent3D>(const procedural::ProceduralTerrainComponent3D& data, Archive& archive)
		{
			archive.Set("halfSize", data.halfSize);
			archive.Set("quadCount", data.quadCount);
			archive.Set("seed", data.seed);
			archive.Set("heightMult", data.heightMult);
			archive.Set("frequencyMult", data.frequencyMult);
			archive.Set("frequency", data.frequency);
			archive.Set("octaves", data.octaves);
		}

		template<>
		inline void Deserialize<procedural::ProceduralTerrainComponent3D>(const Archive& archive, procedural::ProceduralTerrainComponent3D& data)
		{
			archive.Get("halfSize", data.halfSize);
			archive.Get("quadCount", data.quadCount);
			archive.Get("seed", data.seed);
			archive.Get("heightMult", data.heightMult);
			archive.Get("frequencyMult", data.frequencyMult);
			archive.Get("frequency", data.frequency);
			archive.Get("octaves", data.octaves);
		}
	}
}