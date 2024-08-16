#pragma once

#include "puffin/types/vector.h"

namespace puffin::physics
{
	struct ShapeComponent3D
	{
		Vector3f centreOfMass = Vector3f(0.0f);
	};
}