#pragma once

#include "puffin/types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct ShapeComponent2D
	{
		Vector2f centre_of_mass = Vector2f(0.0f);
	};

	struct CircleComponent2D : ShapeComponent2D
	{
		CircleComponent2D() = default;

		CircleComponent2D(const float& radius_) : radius(radius_) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircleComponent2D, centre_of_mass, radius)
	};

	struct BoxComponent2D : ShapeComponent2D
	{
		BoxComponent2D() = default;

		BoxComponent2D(const Vector2f& halfExtent_) : half_extent(halfExtent_) {}

		Vector2f half_extent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centre_of_mass, half_extent)
	};
}
