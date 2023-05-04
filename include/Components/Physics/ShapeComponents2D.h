#pragma once

#include "Physics/Onager2D/Shapes/CircleShape2D.h"
#include "Physics/Onager2D/Shapes/BoxShape2D.h"

#include <Types/Vector.h>

#include "nlohmann/json.hpp"

#include <memory>

namespace puffin
{
	namespace physics
	{
		struct ShapeComponent2D
		{
			Vector2f centreOfMass = Vector2f(0.0f);
		};

		struct CircleComponent2D : public ShapeComponent2D
		{
			float radius = 0.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircleComponent2D, centreOfMass, radius)
		};

		struct BoxComponent2D : public ShapeComponent2D
		{
			Vector2f halfExtent = { 0.5f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centreOfMass, halfExtent)
		};
	}
}