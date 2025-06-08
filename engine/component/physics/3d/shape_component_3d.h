#pragma once

#include "types/vector3.h"

namespace puffin::physics
{
	struct ShapeComponent3D
	{
		Vector3f centreOfMass = Vector3f(0.0f);
	};
}