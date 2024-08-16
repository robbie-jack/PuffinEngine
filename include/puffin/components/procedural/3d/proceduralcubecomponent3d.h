#pragma once

#include "nlohmann/json.hpp"
#include "puffin/types/vector.h"

namespace puffin::procedural
{
	struct ProceduralCubeComponent3D
	{
		ProceduralCubeComponent3D() = default;

		Vector3f halfSize = { 10.f }; // Half size of plane
		Vector3i quadCount = { 10 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralCubeComponent3D, halfSize, quadCount)
	};
}
