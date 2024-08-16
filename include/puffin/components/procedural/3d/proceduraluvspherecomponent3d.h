#pragma once

#include "nlohmann/json.hpp"
#include "puffin/types/vector.h"

namespace puffin::procedural
{
	struct UVSphereComponent3D
	{
		UVSphereComponent3D() = default;

		double radius = 0.5;
		Vector2i segments = { 10 };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UVSphereComponent3D, radius, segments)
	};
}
