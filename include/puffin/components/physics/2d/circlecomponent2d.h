#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"

namespace puffin::physics
{
	struct CircleComponent2D : ShapeComponent2D
	{
		CircleComponent2D() = default;

		explicit CircleComponent2D(const float& radius) : radius(radius) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircleComponent2D, centreOfMass, radius)
	};
}