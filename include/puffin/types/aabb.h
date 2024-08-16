#pragma once

#include "vector.h"

namespace puffin
{
	struct AABB2D
	{
		Vector2f min;
		Vector2f max;
	};

	struct AABB3D
	{
		Vector3f min;
		Vector3f max;
	};
}
