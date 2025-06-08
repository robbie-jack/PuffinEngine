#pragma once

#include "types/vector2.h"

namespace puffin::physics
{
	struct ShapeComponent2D
	{
		Vector2f centreOfMass = Vector2f(0.0f);
	};
}
