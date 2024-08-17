#pragma once

#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/types/vector.h"
#include "nlohmann/json.hpp"

namespace puffin::rendering
{
	/*
	 * Component containing variables used by spot lights
	 */
	struct SpotLightComponent3D : LightComponent3D
	{
		Vector3f direction = { 1.0f, 0.0f, 0.0f };

		float constantAttenuation = 1.f;
		float linearAttenuation = .09f;
		float quadraticAttenuation = .032f;

		float innerCutoffAngle = 30.0f;
		float outerCutoffAngle = 45.0f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpotLightComponent3D, color, ambientIntensity, specularIntensity, specularExponent,
			direction, constantAttenuation, linearAttenuation, quadraticAttenuation,
			innerCutoffAngle, outerCutoffAngle)
	};
}
