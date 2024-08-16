#pragma once

#include "puffin/types/vector.h"
#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	/*
	 * Component containing variables used by directional lights
	 */
	struct DirectionaLightComponent3D
	{
		Vector3f direction = { 1.0f, 0.0f, 0.0f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(DirectionaLightComponent3D, direction)
	};
}
