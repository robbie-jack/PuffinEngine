#pragma once

#include <vector>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "puffin/types/vector.h"

#include "nlohmann/json.hpp"


namespace puffin::rendering
{
	enum class LightType
	{
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	const std::vector<std::string> gLightTypeLabels = { "Point", "Spot", "Directional" };

	NLOHMANN_JSON_SERIALIZE_ENUM(LightType, {
	                             { LightType::Point, "Point"},
	                             { LightType::Spot, "Spot"},
	                             { LightType::Directional, "Directional"}
	                             })

	/*
	 * Component containing variables used by all light types
	 */
	struct LightComponent3D
	{
		Vector3f color = {1.f, 1.f, 1.f};

		float ambientIntensity = .05f; // Intensity multiplier applied to ambient/indirect color
		float specularIntensity = 1.f; // Intensity multiplier applied to specular highlights
		int specularExponent = 64; // Exponent specular value is raised to

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightComponent3D, color, ambientIntensity,specularIntensity, specularExponent)
	};
}
