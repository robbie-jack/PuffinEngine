#pragma once

#include "puffin/types/vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct ShapeComponent2D
	{
		Vector2f centreOfMass = Vector2f(0.0f);
	};
}
