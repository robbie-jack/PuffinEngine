#pragma once

#include "puffin/types/vector3.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct VelocityComponent3D
	{
#ifdef PFN_USE_DOUBLE_PRECISION
		Vector3d linear = Vector3d(0.0);
#else
		Vector3f linear = Vector3f(0.0f);
#endif

		Vector3f angular = Vector3(0.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(VelocityComponent3D, linear, angular)
	};
}