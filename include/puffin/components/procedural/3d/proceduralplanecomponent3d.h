#pragma once

#include "nlohmann/json.hpp"
#include "puffin/types/vector2.h"

namespace puffin::procedural
{
	struct ProceduralPlaneComponent3D
	{
		ProceduralPlaneComponent3D() = default;

		Vector2f halfSize = { 10.f }; // Half size of plane
		Vector2i quadCount = { 10 }; // Number of quads that make up planes surface along a single axis

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralPlaneComponent3D, halfSize, quadCount)
	};

}
