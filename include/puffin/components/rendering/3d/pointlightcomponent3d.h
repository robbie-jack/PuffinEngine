#pragma once

#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	/*
	 * Component containing variables used by point lights
	 */
	struct PointLightComponent3D
	{
		float constantAttenuation = 1.f;
		float linearAttenuation = .09f;
		float quadraticAttenuation = .032f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointLightComponent3D, constantAttenuation, linearAttenuation, quadraticAttenuation)
	};
}