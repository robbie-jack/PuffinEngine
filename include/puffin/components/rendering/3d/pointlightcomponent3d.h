#pragma once

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	/*
	 * Component containing variables used by point lights
	 */
	struct PointLightComponent3D : LightComponent3D
	{
		float constantAttenuation = 1.f;
		float linearAttenuation = .09f;
		float quadraticAttenuation = .032f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent,
			constantAttenuation, linearAttenuation, quadraticAttenuation)
	};
}