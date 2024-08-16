#pragma once

#include "proceduralplanecomponent3d.h"
#include "nlohmann/json.hpp"

namespace puffin::procedural
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