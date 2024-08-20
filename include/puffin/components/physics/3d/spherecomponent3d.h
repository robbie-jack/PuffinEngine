#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/3d/shapecomponent3d.h"

namespace puffin::physics
{
	struct SphereComponent3D : ShapeComponent3D
	{
		SphereComponent3D() = default;

		explicit SphereComponent3D(const float& radius) : radius(radius) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SphereComponent3D, centreOfMass, radius)
	};
}