#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "puffin/types/Vector.h"

#include <vector>
#include <string>

#include "nlohmann/json.hpp"

namespace puffin
{
	namespace rendering
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

		struct LightComponent
		{
			Vector3f color = {1.f, 1.f, 1.f};
			Vector3f direction = { 0.5f, -0.5f, 0.0f };

			float ambientIntensity = .05f; // Intensity multiplier applied to ambient/indirect color
			float specularIntensity = 1.f; // Intensity multiplier applied to specular highlights
			int specularExponent = 64; // Exponent specular value is raised to

			float constantAttenuation = 1.f;
			float linearAttenuation = .09f;
			float quadraticAttenuation = .032f;

			float innerCutoffAngle = 30.0f;
			float outerCutoffAngle = 45.0f;

			LightType type = LightType::Point;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightComponent, 
				color,
				direction,
				ambientIntensity,
				specularIntensity, 
				specularExponent, 
				constantAttenuation, 
				linearAttenuation, 
				quadraticAttenuation,
				innerCutoffAngle,
				outerCutoffAngle,
				type)
		};

		const std::vector<uint16_t> gShadowResolutionValues = { 512, 1024, 2048, 4096, 8192, 16384 };
		const std::vector<std::string> gShadowResolutionLabels = { "512", "1024", "2048", "4096", "8192", "16384" };

		// Component for lights that cast shadows
		struct ShadowCasterComponent
		{
			ShadowCasterComponent() = default;

			uint16_t width = 2048;
			uint16_t height = 2048;
			glm::mat4 lightSpaceView;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShadowCasterComponent, width, height)
		};
	}
}