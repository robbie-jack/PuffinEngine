#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/types/vector2.h"

namespace puffin::physics
{
	struct BoxComponent2D : ShapeComponent2D
	{
		BoxComponent2D() = default;

		explicit BoxComponent2D(const Vector2f& halfExtent) : halfExtent(halfExtent) {}

		Vector2f halfExtent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centreOfMass, halfExtent)
	};
}
