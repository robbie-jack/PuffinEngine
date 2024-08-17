#pragma once

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	/*
	 * Component containing variables used by directional lights
	 */
	struct DirectionalLightComponent3D : LightComponent3D
	{
		Vector3f direction = { 1.0f, 0.0f, 0.0f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(DirectionalLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent)
	};
}
