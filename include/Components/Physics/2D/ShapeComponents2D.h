#pragma once

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct ShapeComponent2D
	{
		Vector2f centreOfMass = Vector2f(0.0f);
	};

	struct CircleComponent2D : ShapeComponent2D
	{
		CircleComponent2D() = default;

		CircleComponent2D(const float& radius_) : radius(radius_) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircleComponent2D, centreOfMass, radius)
	};

	struct BoxComponent2D : ShapeComponent2D
	{
		BoxComponent2D() = default;

		BoxComponent2D(const Vector2f& halfExtent_) : halfExtent(halfExtent_) {}

		Vector2f halfExtent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centreOfMass, halfExtent)
	};
}
