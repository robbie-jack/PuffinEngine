#pragma once

#include "puffin/types/vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct VelocityComponent2D
	{
#ifdef PFN_USE_DOUBLE_PRECISION
		Vector2d linear = Vector2d(0.0);
#else
		Vector2f linear = Vector2f(0.0f);
#endif

		float angular = 0.0f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(VelocityComponent2D, linear, angular)
	};
}